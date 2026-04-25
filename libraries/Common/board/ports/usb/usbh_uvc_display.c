/*
 * CherryUSB UVC Host Display Layer
 *
 * Renders UVC YUYV or MJPEG frames to the 512x800 RGB565 LCD.
 * YUYV pixels are converted to RGB565 on the fly.
 * MJPEG frames are decoded with TJpgDec into a temporary RGB565 surface,
 * then scaled to the LCD using the same nearest-neighbour path.
 */

#include <rtthread.h>
#include <string.h>
#include <board.h>
#include "cy_graphics.h"
#include "usbh_uvc_stream.h"
#include "tjpgd.h"
#include "usbh_uvc_display_hook.h"

#undef  USB_DBG_TAG
#define USB_DBG_TAG "uvc_disp"
#include "usb_log.h"

/* LCD dimensions (portrait mode as configured in drv_lcd.c) */
#define LCD_W   512
#define LCD_H   800
#define LCD_STRIDE          512U
#define LCD_BITS_PER_PIXEL  16U
#define LCD_BUF_SIZE        (LCD_STRIDE * LCD_H * LCD_BITS_PER_PIXEL / 8U)

#define UVC_ITCM_SECTION __attribute__((section(".cy_itcm")))
#define UVC_DTCM_SECTION __attribute__((section(".cy_dtcm")))
#define UVC_SHARED_SECTION __attribute__((section(".m33_m55_shared_hyperram")))
#define UVC_RGB565_STAGE_SECTION __attribute__((section(".cy_uvc_rgb565_data")))

#define UVC_MJPEG_WORKBUF_SIZE   4096U
#define UVC_MJPEG_MAX_FRAME_SIZE (128U * 1024U)
#define UVC_MJPEG_MAX_WIDTH      640U
#define UVC_MJPEG_MAX_HEIGHT     480U
#define UVC_MJPEG_MAX_PIXELS     (UVC_MJPEG_MAX_WIDTH * UVC_MJPEG_MAX_HEIGHT)
#define UVC_YUYV_STAGE_W         320U
#define UVC_YUYV_STAGE_H         240U
#define UVC_YUYV_STAGE_PIXELS    (UVC_YUYV_STAGE_W * UVC_YUYV_STAGE_H)
#define UVC_YUYV_STAGE_BYTES     (UVC_YUYV_STAGE_PIXELS * sizeof(uint16_t))

static uint16_t   *lcd_fb;          /* pointer into LCD framebuffer */
static uint32_t    lcd_fb_size;     /* bytes */
static rt_bool_t   g_uvc_lcd_hw_ready;

static GFXSS_Type *g_uvc_gfxbase = (GFXSS_Type*)GFXSS;
static cy_stc_gfx_context_t g_uvc_gfx_context;
/* Reuse LCD driver's framebuffer instead of allocating an additional 800x512 RGB565 surface. */
extern uint8_t graphics_buffer[LCD_BUF_SIZE];
static uvc_display_overlay_cb_t g_uvc_overlay_cb;
static void *g_uvc_overlay_cb_ctx;

struct uvc_update_rect {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
};

struct uvc_mjpeg_decoder {
    const uint8_t *src;
    uint32_t src_size;
    uint32_t src_offset;
    uint16_t width;
    uint16_t height;
    uint16_t *dst_rgb565;
};

struct uvc_mjpeg_header_info {
    uint32_t soi_offset;
    uint32_t sos_offset;
    uint32_t eoi_offset;
    rt_bool_t has_sof0;
    rt_bool_t has_dqt;
    rt_bool_t has_dht;
    rt_bool_t has_sos;
    rt_bool_t has_eoi;
};

struct uvc_mjpeg_validate_info {
    uint16_t width;
    uint16_t height;
    uint8_t ncomp;
    uint8_t qt_mask;
    uint8_t dc_mask;
    uint8_t ac_mask;
    uint8_t sos_mask;
    uint16_t marker_count;
};

struct uvc_display_context {
    uint16_t src_w;
    uint16_t src_h;
    uint16_t dst_w;
    uint16_t dst_h;
    uint16_t y_offset;
    uint32_t src_stride;
    rt_bool_t prepared;
    rt_bool_t background_synced;
    struct uvc_update_rect update_rect;
    uint32_t row_offsets[LCD_H];
    uint16_t x_offsets[LCD_W];
    uint16_t pair_offsets[LCD_W];
    uint8_t y_offsets[LCD_W];
};

static UVC_DTCM_SECTION struct uvc_display_context g_uvc_display_ctx;
static UVC_DTCM_SECTION uint8_t g_uvc_mjpeg_workbuf[UVC_MJPEG_WORKBUF_SIZE];
static UVC_DTCM_SECTION rt_bool_t g_uvc_yuv_lut_ready;
static UVC_DTCM_SECTION int32_t g_uvc_y_lut[256];
static UVC_DTCM_SECTION int32_t g_uvc_u_to_b_lut[256];
static UVC_DTCM_SECTION int32_t g_uvc_u_to_g_lut[256];
static UVC_DTCM_SECTION int32_t g_uvc_v_to_r_lut[256];
static UVC_DTCM_SECTION int32_t g_uvc_v_to_g_lut[256];
static UVC_DTCM_SECTION uint8_t g_uvc_mjpeg_diag_budget = 8U;
static UVC_SHARED_SECTION uint16_t g_uvc_mjpeg_rgb565[UVC_MJPEG_MAX_PIXELS];
static UVC_SHARED_SECTION uint8_t g_uvc_mjpeg_frame[UVC_MJPEG_MAX_FRAME_SIZE];
/* Keep a 320x240 RGB565 draw-stage buffer in shared SoCMEM to reduce YUV conversion load. */
static UVC_RGB565_STAGE_SECTION uint16_t g_uvc_yuyv_stage_rgb565[UVC_YUYV_STAGE_PIXELS];

#define UVC_MJPEG_STD_DHT_SIZE 420U

static const uint8_t g_uvc_dht_bits_dc_luma[16] = {
    0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t g_uvc_dht_val_dc_luma[12] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b
};

static const uint8_t g_uvc_dht_bits_ac_luma[16] = {
    0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
    0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d
};

static const uint8_t g_uvc_dht_val_ac_luma[162] = {
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
    0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
    0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
    0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
    0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
    0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
    0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
    0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
};

static const uint8_t g_uvc_dht_bits_dc_chroma[16] = {
    0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t g_uvc_dht_val_dc_chroma[12] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b
};

static const uint8_t g_uvc_dht_bits_ac_chroma[16] = {
    0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04,
    0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77
};

static const uint8_t g_uvc_dht_val_ac_chroma[162] = {
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
    0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
    0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
    0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
    0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
    0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
    0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
    0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
    0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
    0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
    0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
    0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
};

static const char *uvc_mjpeg_result_string(JRESULT result)
{
    switch (result) {
    case JDR_OK:
        return "ok";
    case JDR_INTR:
        return "interrupted";
    case JDR_INP:
        return "input";
    case JDR_MEM1:
        return "mem1";
    case JDR_MEM2:
        return "mem2";
    case JDR_PAR:
        return "param";
    case JDR_FMT1:
        return "fmt1";
    case JDR_FMT2:
        return "fmt2";
    case JDR_FMT3:
        return "fmt3";
    default:
        return "unknown";
    }
}

static const char *uvc_mjpeg_validate_jpeg(const uint8_t *jpeg, uint32_t jpeg_size,
                                           struct uvc_mjpeg_validate_info *info)
{
    uint32_t offset;

    if ((jpeg == RT_NULL) || (jpeg_size < 4U) || (info == RT_NULL)) {
        return "param";
    }

    memset(info, 0, sizeof(*info));

    if ((jpeg[0] != 0xffU) || (jpeg[1] != 0xd8U)) {
        return "soi";
    }

    offset = 2U;
    while ((offset + 3U) < jpeg_size) {
        uint16_t marker;
        uint16_t seg_len;
        const uint8_t *seg;
        uint32_t ndata;

        if (jpeg[offset] != 0xffU) {
            return "marker-prefix";
        }

        while (((offset + 1U) < jpeg_size) && (jpeg[offset + 1U] == 0xffU)) {
            offset++;
        }

        if ((offset + 3U) >= jpeg_size) {
            return "marker-trunc";
        }

        marker = (uint16_t)(((uint16_t)jpeg[offset] << 8) | jpeg[offset + 1U]);
        seg_len = (uint16_t)(((uint16_t)jpeg[offset + 2U] << 8) | jpeg[offset + 3U]);
        if ((seg_len <= 2U) || ((marker >> 8) != 0xffU)) {
            return "marker-len";
        }
        if ((offset + 2U + seg_len) > jpeg_size) {
            return "segment-overrun";
        }

        info->marker_count++;
        seg = jpeg + offset + 4U;
        ndata = (uint32_t)seg_len - 2U;

        switch (marker & 0xffU) {
        case 0xc0: {
            uint8_t i;

            if (ndata < 6U) {
                return "sof0-short";
            }

            info->height = (uint16_t)(((uint16_t)seg[1] << 8) | seg[2]);
            info->width = (uint16_t)(((uint16_t)seg[3] << 8) | seg[4]);
            info->ncomp = seg[5];
            if ((info->ncomp != 1U) && (info->ncomp != 3U)) {
                return "sof0-ncomp";
            }
            if ((info->width == 0U) || (info->height == 0U)) {
                return "sof0-size";
            }
            if (ndata < (uint32_t)(6U + 3U * info->ncomp)) {
                return "sof0-trunc";
            }

            for (i = 0U; i < info->ncomp; i++) {
                uint8_t samp = seg[7U + 3U * i];
                uint8_t qid = seg[8U + 3U * i];

                if (qid > 3U) {
                    return "sof0-qid";
                }
                if (i == 0U) {
                    if ((samp != 0x11U) && (samp != 0x22U) && (samp != 0x21U)) {
                        return "sof0-y-samp";
                    }
                } else if (samp != 0x11U) {
                    return "sof0-c-samp";
                }

                info->qt_mask |= (uint8_t)(1U << qid);
            }
        } break;

        case 0xdb: {
            const uint8_t *d = seg;
            uint32_t left = ndata;

            while (left > 0U) {
                uint8_t prop;
                uint8_t precision;
                uint8_t qid;
                uint32_t table_size;

                if (left < 65U) {
                    return "dqt-short";
                }

                prop = *d++;
                left--;
                precision = (uint8_t)(prop >> 4);
                qid = (uint8_t)(prop & 0x0fU);
                if (precision != 0U) {
                    return "dqt-precision";
                }
                if (qid > 3U) {
                    return "dqt-qid";
                }

                table_size = 64U;
                if (left < table_size) {
                    return "dqt-data";
                }
                d += table_size;
                left -= table_size;
                info->qt_mask |= (uint8_t)(1U << qid);
            }
        } break;

        case 0xc4: {
            const uint8_t *d = seg;
            uint32_t left = ndata;

            while (left > 0U) {
                uint8_t prop;
                uint8_t cls;
                uint8_t num;
                uint32_t np = 0U;
                uint8_t i;

                if (left < 17U) {
                    return "dht-short";
                }

                prop = *d++;
                left--;
                if ((prop & 0xeeU) != 0U) {
                    return "dht-id";
                }
                cls = (uint8_t)(prop >> 4);
                num = (uint8_t)(prop & 0x0fU);

                for (i = 0U; i < 16U; i++) {
                    np += *d++;
                }
                left -= 16U;

                if (left < np) {
                    return "dht-data";
                }

                for (i = 0U; i < np; i++) {
                    uint8_t value = d[i];
                    if ((cls == 0U) && (value > 11U)) {
                        return "dht-dc";
                    }
                }

                if (cls == 0U) {
                    info->dc_mask |= (uint8_t)(1U << num);
                } else {
                    info->ac_mask |= (uint8_t)(1U << num);
                }

                d += np;
                left -= np;
            }
        } break;

        case 0xda: {
            uint8_t i;

            if (ndata < 1U) {
                return "sos-short";
            }
            if ((info->width == 0U) || (info->height == 0U)) {
                return "sos-before-sof0";
            }
            if (seg[0] != info->ncomp) {
                return "sos-ncomp";
            }
            if (ndata < (uint32_t)(1U + 2U * info->ncomp + 3U)) {
                return "sos-trunc";
            }

            for (i = 0U; i < info->ncomp; i++) {
                uint8_t sel = seg[2U + 2U * i];
                uint8_t qid = (uint8_t)((info->qt_mask >> i) & 0x01U);
                (void)qid;

                if ((sel != 0x00U) && (sel != 0x11U)) {
                    return "sos-hsel";
                }

                info->sos_mask |= (uint8_t)(1U << i);
            }

            if ((info->dc_mask & 0x01U) == 0U || (info->ac_mask & 0x01U) == 0U) {
                return "sos-y-huff-missing";
            }
            if ((info->ncomp > 1U) && (((info->dc_mask & 0x02U) == 0U) || ((info->ac_mask & 0x02U) == 0U))) {
                return "sos-c-huff-missing";
            }
            return RT_NULL;
        }

        default:
            break;
        }

        offset += 2U + seg_len;
    }

    return "no-sos";
}

static rt_bool_t uvc_mjpeg_parse_header(const uint8_t *src, uint32_t src_size,
                                        struct uvc_mjpeg_header_info *info)
{
    uint32_t offset;

    if ((src == RT_NULL) || (src_size < 4U) || (info == RT_NULL)) {
        return RT_FALSE;
    }

    memset(info, 0, sizeof(*info));
    info->soi_offset = src_size;
    info->sos_offset = src_size;
    info->eoi_offset = src_size;

    for (offset = 0U; (offset + 1U) < src_size; offset++) {
        if ((src[offset] == 0xffU) && (src[offset + 1U] == 0xd8U)) {
            info->soi_offset = offset;
            break;
        }
    }

    if (info->soi_offset >= src_size) {
        return RT_FALSE;
    }

    offset = info->soi_offset + 2U;

    while ((offset + 3U) < src_size) {
        uint16_t marker;
        uint16_t seg_len;

        if (src[offset] != 0xffU) {
            offset++;
            continue;
        }

        while (((offset + 1U) < src_size) && (src[offset + 1U] == 0xffU)) {
            offset++;
        }

        if ((offset + 1U) >= src_size) {
            break;
        }

        marker = (uint16_t)(((uint16_t)src[offset] << 8) | src[offset + 1U]);

        if (marker == 0xffc0U) {
            info->has_sof0 = RT_TRUE;
        } else if (marker == 0xffdbU) {
            info->has_dqt = RT_TRUE;
        } else if (marker == 0xffc4U) {
            info->has_dht = RT_TRUE;
        }

        if (marker == 0xffdaU) {
            info->has_sos = RT_TRUE;
            info->sos_offset = offset;
            break;
        }

        if (marker == 0xffd9U) {
            info->has_eoi = RT_TRUE;
            info->eoi_offset = offset;
            break;
        }

        if ((marker >= 0xffd0U) && (marker <= 0xffd7U)) {
            offset += 2U;
            continue;
        }

        if ((marker == 0xffd8U) || (marker == 0xff01U)) {
            offset += 2U;
            continue;
        }

        seg_len = (uint16_t)(((uint16_t)src[offset + 2U] << 8) | src[offset + 3U]);
        if ((seg_len < 2U) || ((offset + 2U + seg_len) > src_size)) {
            return RT_FALSE;
        }

        offset += 2U + seg_len;
    }

    if (info->has_sos) {
        for (offset = src_size; offset > (info->sos_offset + 1U); offset--) {
            if ((src[offset - 2U] == 0xffU) && (src[offset - 1U] == 0xd9U)) {
                info->has_eoi = RT_TRUE;
                info->eoi_offset = offset - 2U;
                break;
            }
        }
    }

    return RT_TRUE;
}

static rt_err_t uvc_mjpeg_append_bytes(const uint8_t *src, uint32_t copy_size,
                                       uint32_t *out_offset)
{
    if ((src == RT_NULL) || (out_offset == RT_NULL)) {
        return -RT_ERROR;
    }

    if ((*out_offset + copy_size) > sizeof(g_uvc_mjpeg_frame)) {
        return -RT_ERROR;
    }

    memcpy(g_uvc_mjpeg_frame + *out_offset, src, copy_size);
    *out_offset += copy_size;
    return RT_EOK;
}

static rt_err_t uvc_mjpeg_append_standard_dht(uint32_t *out_offset)
{
    uint8_t segment[UVC_MJPEG_STD_DHT_SIZE];
    uint32_t write_offset = 4U;
    unsigned int i;
    const struct {
        uint8_t table_id;
        const uint8_t *bits;
        uint32_t bits_len;
        const uint8_t *values;
        uint32_t values_len;
    } tables[] = {
        { 0x00U, g_uvc_dht_bits_dc_luma, (uint32_t)sizeof(g_uvc_dht_bits_dc_luma), g_uvc_dht_val_dc_luma, (uint32_t)sizeof(g_uvc_dht_val_dc_luma) },
        { 0x10U, g_uvc_dht_bits_ac_luma, (uint32_t)sizeof(g_uvc_dht_bits_ac_luma), g_uvc_dht_val_ac_luma, (uint32_t)sizeof(g_uvc_dht_val_ac_luma) },
        { 0x01U, g_uvc_dht_bits_dc_chroma, (uint32_t)sizeof(g_uvc_dht_bits_dc_chroma), g_uvc_dht_val_dc_chroma, (uint32_t)sizeof(g_uvc_dht_val_dc_chroma) },
        { 0x11U, g_uvc_dht_bits_ac_chroma, (uint32_t)sizeof(g_uvc_dht_bits_ac_chroma), g_uvc_dht_val_ac_chroma, (uint32_t)sizeof(g_uvc_dht_val_ac_chroma) },
    };

    if (out_offset == RT_NULL) {
        return -RT_ERROR;
    }

    segment[0] = 0xffU;
    segment[1] = 0xc4U;

    for (i = 0U; i < (sizeof(tables) / sizeof(tables[0])); i++) {
        uint32_t copy_size = 1U + tables[i].bits_len + tables[i].values_len;

        if ((write_offset + copy_size) > sizeof(segment)) {
            return -RT_ERROR;
        }

        segment[write_offset++] = tables[i].table_id;
        memcpy(&segment[write_offset], tables[i].bits, tables[i].bits_len);
        write_offset += tables[i].bits_len;
        memcpy(&segment[write_offset], tables[i].values, tables[i].values_len);
        write_offset += tables[i].values_len;
    }

    segment[2] = (uint8_t)(((write_offset - 2U) >> 8) & 0xffU);
    segment[3] = (uint8_t)((write_offset - 2U) & 0xffU);
    return uvc_mjpeg_append_bytes(segment, write_offset, out_offset);
}

static rt_err_t uvc_mjpeg_collect_sof0_qt_mask(const uint8_t *segment,
                                               uint16_t seg_len,
                                               uint8_t *qt_mask)
{
    uint32_t payload_size;
    uint8_t ncomp;
    uint8_t i;

    if ((segment == RT_NULL) || (qt_mask == RT_NULL) || (seg_len < 8U)) {
        return -RT_ERROR;
    }

    payload_size = (uint32_t)seg_len - 2U;
    ncomp = segment[9];
    if ((ncomp == 0U) || (payload_size < (uint32_t)(6U + 3U * ncomp))) {
        return -RT_ERROR;
    }

    for (i = 0U; i < ncomp; i++) {
        uint8_t qid = segment[12U + 3U * i];
        if (qid > 3U) {
            return -RT_ERROR;
        }
        *qt_mask |= (uint8_t)(1U << qid);
    }

    return RT_EOK;
}

static rt_err_t uvc_mjpeg_append_dqt_segment(const uint8_t *segment,
                                             uint16_t seg_len,
                                             uint8_t qt_tables[4][64],
                                             uint8_t *defined_qt_mask,
                                             uint32_t *out_offset,
                                             rt_bool_t *normalized_16bit,
                                             rt_bool_t *trimmed_tail)
{
    uint8_t temp[4U + 4U * 65U];
    const uint8_t *data;
    uint32_t ndata;
    uint32_t write_offset = 4U;
    uint8_t local_mask = 0U;

    if ((segment == RT_NULL) || (defined_qt_mask == RT_NULL) ||
        (out_offset == RT_NULL) || (normalized_16bit == RT_NULL) ||
        (trimmed_tail == RT_NULL) || (seg_len < 3U)) {
        return -RT_ERROR;
    }

    data = segment + 4U;
    ndata = (uint32_t)seg_len - 2U;
    temp[0] = 0xffU;
    temp[1] = 0xdbU;

    while (ndata > 0U) {
        uint8_t prop;
        uint8_t precision;
        uint8_t qid;
        uint8_t i;

        prop = *data++;
        ndata--;
        precision = (uint8_t)(prop >> 4);
        qid = (uint8_t)(prop & 0x0fU);

        if (qid > 3U) {
            return -RT_ERROR;
        }

        if (precision == 0U) {
            if (ndata < 64U) {
                if (local_mask != 0U) {
                    *trimmed_tail = RT_TRUE;
                    break;
                }
                return -RT_ERROR;
            }

            temp[write_offset++] = qid;
            memcpy(&temp[write_offset], data, 64U);
            memcpy(qt_tables[qid], data, 64U);
            write_offset += 64U;
            data += 64U;
            ndata -= 64U;
        } else if (precision == 1U) {
            if (ndata < 128U) {
                if (local_mask != 0U) {
                    *trimmed_tail = RT_TRUE;
                    break;
                }
                return -RT_ERROR;
            }

            temp[write_offset++] = qid;
            for (i = 0U; i < 64U; i++) {
                uint16_t value = (uint16_t)(((uint16_t)data[2U * i] << 8) |
                                            data[2U * i + 1U]);
                uint8_t qvalue = (value == 0U) ? 1U : (uint8_t)((value > 255U) ? 255U : value);

                temp[write_offset + i] = qvalue;
                qt_tables[qid][i] = qvalue;
            }
            write_offset += 64U;
            data += 128U;
            ndata -= 128U;
            *normalized_16bit = RT_TRUE;
        } else {
            return -RT_ERROR;
        }

        local_mask |= (uint8_t)(1U << qid);
    }

    if (write_offset <= 4U) {
        return -RT_ERROR;
    }

    temp[2] = (uint8_t)(((write_offset - 2U) >> 8) & 0xffU);
    temp[3] = (uint8_t)((write_offset - 2U) & 0xffU);
    *defined_qt_mask |= local_mask;
    return uvc_mjpeg_append_bytes(temp, write_offset, out_offset);
}

static rt_err_t uvc_mjpeg_append_missing_qtables(uint8_t used_qt_mask,
                                                 uint8_t *defined_qt_mask,
                                                 uint8_t qt_tables[4][64],
                                                 uint32_t *out_offset,
                                                 uint8_t *duplicated_tables)
{
    uint8_t temp[4U + 4U * 65U];
    uint32_t write_offset = 4U;
    uint8_t fallback_qid = 0xffU;
    uint8_t qid;

    if ((defined_qt_mask == RT_NULL) || (out_offset == RT_NULL) ||
        (duplicated_tables == RT_NULL)) {
        return -RT_ERROR;
    }

    for (qid = 0U; qid < 4U; qid++) {
        if ((*defined_qt_mask & (1U << qid)) != 0U) {
            fallback_qid = qid;
            break;
        }
    }

    if (fallback_qid == 0xffU) {
        return -RT_ERROR;
    }

    temp[0] = 0xffU;
    temp[1] = 0xdbU;

    for (qid = 0U; qid < 4U; qid++) {
        if (((used_qt_mask & (1U << qid)) != 0U) && ((*defined_qt_mask & (1U << qid)) == 0U)) {
            temp[write_offset++] = qid;
            memcpy(&temp[write_offset], qt_tables[fallback_qid], 64U);
            memcpy(qt_tables[qid], qt_tables[fallback_qid], 64U);
            write_offset += 64U;
            *defined_qt_mask |= (uint8_t)(1U << qid);
            (*duplicated_tables)++;
        }
    }

    if (write_offset <= 4U) {
        return RT_EOK;
    }

    temp[2] = (uint8_t)(((write_offset - 2U) >> 8) & 0xffU);
    temp[3] = (uint8_t)((write_offset - 2U) & 0xffU);
    return uvc_mjpeg_append_bytes(temp, write_offset, out_offset);
}

static rt_err_t uvc_mjpeg_prepare_frame(const uint8_t *src, uint32_t src_size,
                                        const uint8_t **jpeg, uint32_t *jpeg_size)
{
    struct uvc_mjpeg_header_info info;
    uint32_t offset;
    uint32_t out_offset = 0U;
    uint32_t dropped_bytes = 0U;
    uint32_t entropy_offset = 0U;
    uint32_t entropy_size;
    uint8_t used_qt_mask = 0U;
    uint8_t defined_qt_mask = 0U;
    uint8_t duplicated_qtables = 0U;
    uint8_t qt_tables[4][64];
    rt_bool_t insert_dht;
    rt_bool_t append_eoi;
    rt_bool_t normalized_dqt16 = RT_FALSE;
    rt_bool_t trimmed_dqt_tail = RT_FALSE;

    if ((src == RT_NULL) || (src_size < 4U) || (jpeg == RT_NULL) || (jpeg_size == RT_NULL)) {
        return -RT_ERROR;
    }

    memset(qt_tables, 0, sizeof(qt_tables));

    if (!uvc_mjpeg_parse_header(src, src_size, &info)) {
        if (g_uvc_mjpeg_diag_budget > 0U) {
            USB_LOG_WRN("MJPEG header parse failed size=%lu\r\n", (unsigned long)src_size);
            g_uvc_mjpeg_diag_budget--;
        }
        return -RT_ERROR;
    }

    if (!info.has_sos || !info.has_sof0 || !info.has_dqt) {
        if (g_uvc_mjpeg_diag_budget > 0U) {
            USB_LOG_WRN("MJPEG header incomplete: sof0=%d dqt=%d dht=%d sos=%d eoi=%d size=%lu\r\n",
                        info.has_sof0, info.has_dqt, info.has_dht,
                        info.has_sos, info.has_eoi, (unsigned long)src_size);
            g_uvc_mjpeg_diag_budget--;
        }
        return -RT_ERROR;
    }

    insert_dht = info.has_dht ? RT_FALSE : RT_TRUE;
    append_eoi = info.has_eoi ? RT_FALSE : RT_TRUE;

    g_uvc_mjpeg_frame[out_offset++] = 0xffU;
    g_uvc_mjpeg_frame[out_offset++] = 0xd8U;

    offset = info.soi_offset + 2U;
    while ((offset + 1U) < src_size) {
        uint32_t marker_start = offset;
        uint16_t marker;
        uint16_t seg_len;
        uint32_t segment_size;

        while ((offset < src_size) && (src[offset] != 0xffU)) {
            offset++;
        }
        dropped_bytes += offset - marker_start;

        while (((offset + 1U) < src_size) && (src[offset + 1U] == 0xffU)) {
            offset++;
            dropped_bytes++;
        }

        if ((offset + 1U) >= src_size) {
            break;
        }

        marker = (uint16_t)(((uint16_t)src[offset] << 8) | src[offset + 1U]);

        if (marker == 0xffd9U) {
            break;
        }

        if ((marker >= 0xffd0U) && (marker <= 0xffd7U)) {
            dropped_bytes += 2U;
            offset += 2U;
            continue;
        }

        if ((marker == 0xffd8U) || (marker == 0xff01U)) {
            dropped_bytes += 2U;
            offset += 2U;
            continue;
        }

        if ((offset + 3U) >= src_size) {
            return -RT_ERROR;
        }

        seg_len = (uint16_t)(((uint16_t)src[offset + 2U] << 8) | src[offset + 3U]);
        if ((seg_len < 2U) || ((offset + 2U + seg_len) > src_size)) {
            return -RT_ERROR;
        }

        segment_size = 2U + seg_len;

        if (marker == 0xffdaU) {
            if ((used_qt_mask & (uint8_t)(~defined_qt_mask)) != 0U) {
                if (uvc_mjpeg_append_missing_qtables(used_qt_mask, &defined_qt_mask,
                                                     qt_tables, &out_offset,
                                                     &duplicated_qtables) != RT_EOK) {
                    return -RT_ERROR;
                }
            }

            if (insert_dht) {
                if (uvc_mjpeg_append_standard_dht(&out_offset) != RT_EOK) {
                    goto frame_overflow;
                }
            }

            if (uvc_mjpeg_append_bytes(src + offset, segment_size, &out_offset) != RT_EOK) {
                goto frame_overflow;
            }

            entropy_offset = offset + segment_size;
            break;
        }

        if (marker == 0xffc0U) {
            if (uvc_mjpeg_collect_sof0_qt_mask(src + offset, seg_len, &used_qt_mask) != RT_EOK) {
                return -RT_ERROR;
            }
            if (uvc_mjpeg_append_bytes(src + offset, segment_size, &out_offset) != RT_EOK) {
                goto frame_overflow;
            }
        } else if (marker == 0xffdbU) {
            if (uvc_mjpeg_append_dqt_segment(src + offset, seg_len, qt_tables,
                                             &defined_qt_mask, &out_offset,
                                             &normalized_dqt16,
                                             &trimmed_dqt_tail) != RT_EOK) {
                return -RT_ERROR;
            }
        } else if ((marker == 0xffc4U) || (marker == 0xffddU)) {
            if (uvc_mjpeg_append_bytes(src + offset, segment_size, &out_offset) != RT_EOK) {
                goto frame_overflow;
            }
        } else {
            dropped_bytes += segment_size;
        }

        offset += segment_size;
    }

    if (entropy_offset == 0U) {
        return -RT_ERROR;
    }

    if (info.has_eoi) {
        if (info.eoi_offset < entropy_offset) {
            return -RT_ERROR;
        }
        entropy_size = info.eoi_offset - entropy_offset;
    } else {
        entropy_size = src_size - entropy_offset;
    }

    if (uvc_mjpeg_append_bytes(src + entropy_offset, entropy_size, &out_offset) != RT_EOK) {
        goto frame_overflow;
    }

    if ((out_offset + 2U) > sizeof(g_uvc_mjpeg_frame)) {
        goto frame_overflow;
    }
    g_uvc_mjpeg_frame[out_offset++] = 0xffU;
    g_uvc_mjpeg_frame[out_offset++] = 0xd9U;

    if ((g_uvc_mjpeg_diag_budget > 0U) &&
        (insert_dht || append_eoi || (info.soi_offset != 0U) || (dropped_bytes != 0U) ||
         normalized_dqt16 || trimmed_dqt_tail || (duplicated_qtables != 0U))) {
        USB_LOG_WRN("MJPEG patched: dht=%d eoi=%d drop=%lu dqt16=%d dqtdup=%u dqtail=%d size=%lu\r\n",
                    insert_dht, append_eoi,
                    (unsigned long)dropped_bytes, normalized_dqt16,
                    duplicated_qtables, trimmed_dqt_tail,
                    (unsigned long)out_offset);
        g_uvc_mjpeg_diag_budget--;
    }

    *jpeg = g_uvc_mjpeg_frame;
    *jpeg_size = out_offset;
    return RT_EOK;

frame_overflow:
    if (g_uvc_mjpeg_diag_budget > 0U) {
        USB_LOG_ERR("MJPEG frame overflow after patch size=%lu\r\n", (unsigned long)src_size);
        g_uvc_mjpeg_diag_budget--;
    }
    return -RT_ERROR;
}

/* ---- YUYV → RGB565 inline helpers ---- */
static inline int clamp8(int v)
{
    if (v < 0)   return 0;
    if (v > 255) return 255;
    return v;
}

static inline uint16_t yuv_to_rgb565_fast(uint8_t y,
                                          int32_t u_to_b,
                                          int32_t u_to_g,
                                          int32_t v_to_r,
                                          int32_t v_to_g)
{
    int32_t y_term = g_uvc_y_lut[y];
    int r = clamp8((int)((y_term + v_to_r + 128) >> 8));
    int g = clamp8((int)((y_term + u_to_g + v_to_g + 128) >> 8));
    int b = clamp8((int)((y_term + u_to_b + 128) >> 8));

    return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

static inline uint16_t rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((uint16_t)(r & 0xf8U) << 8) |
                      ((uint16_t)(g & 0xfcU) << 3) |
                      ((uint16_t)b >> 3));
}

static UVC_ITCM_SECTION void uvc_display_prepare_yuv_lut(void)
{
    if (g_uvc_yuv_lut_ready) {
        return;
    }

    for (uint32_t i = 0; i < 256U; i++) {
        int32_t y = (int32_t)i - 16;
        int32_t uv = (int32_t)i - 128;

        if (y < 0) {
            y = 0;
        }

        g_uvc_y_lut[i] = 298 * y;
        g_uvc_u_to_b_lut[i] = 516 * uv;
        g_uvc_u_to_g_lut[i] = -100 * uv;
        g_uvc_v_to_r_lut[i] = 409 * uv;
        g_uvc_v_to_g_lut[i] = -208 * uv;
    }

    g_uvc_yuv_lut_ready = RT_TRUE;
}

static UVC_ITCM_SECTION rt_err_t uvc_display_flush(const struct uvc_update_rect *rect)
{
    cy_en_gfx_status_t status;

    if (!g_uvc_lcd_hw_ready) {
        return -RT_ERROR;
    }

    if ((rect != RT_NULL) && (rect->width > 0U) && (rect->height > 0U) &&
        (rect->x == 0U) &&
        ((g_uvc_gfx_context.dc_context.display_type == GFX_DISP_TYPE_DBI_A) ||
         (g_uvc_gfx_context.dc_context.display_type == GFX_DISP_TYPE_DBI_B) ||
         (g_uvc_gfx_context.dc_context.display_type == GFX_DISP_TYPE_DBI_C) ||
         (g_uvc_gfx_context.dc_context.display_type == GFX_DISP_TYPE_DSI_DBI))) {
        uint32_t start_line = rect->y;
        uint32_t end_line = (uint32_t)rect->y + rect->height;

        if (end_line > LCD_H) {
            end_line = LCD_H;
        }

        status = Cy_GFXSS_TransferPartialFrame(g_uvc_gfxbase, start_line, end_line, &g_uvc_gfx_context);
    } else {
        status = Cy_GFXSS_Set_FrameBuffer(g_uvc_gfxbase, (uint32_t *)lcd_fb, &g_uvc_gfx_context);
    }

    return (status == CY_GFX_SUCCESS) ? RT_EOK : -RT_ERROR;
}

static UVC_ITCM_SECTION rt_err_t uvc_display_hw_init(void)
{
    if (g_uvc_lcd_hw_ready) {
        return RT_EOK;
    }

    /* LCD driver is responsible for GFXSS/panel initialization. */
    memset(&g_uvc_gfx_context, 0, sizeof(g_uvc_gfx_context));

    lcd_fb = (uint16_t *)graphics_buffer;
    lcd_fb_size = LCD_BUF_SIZE;
    memset(lcd_fb, 0, lcd_fb_size);

    g_uvc_lcd_hw_ready = RT_TRUE;
    return uvc_display_flush(RT_NULL);
}

static UVC_ITCM_SECTION rt_err_t uvc_display_refresh_fb_info(void)
{
    if (!g_uvc_lcd_hw_ready) {
        return -RT_ERROR;
    }

    lcd_fb = (uint16_t *)graphics_buffer;
    lcd_fb_size = LCD_BUF_SIZE;
    return RT_EOK;
}

static UVC_ITCM_SECTION rt_err_t uvc_display_prepare_geometry(uint16_t src_w, uint16_t src_h)
{
    struct uvc_display_context *ctx = &g_uvc_display_ctx;

    if ((src_w == 0U) || (src_h == 0U)) {
        return -RT_ERROR;
    }

    if (ctx->prepared && (ctx->src_w == src_w) && (ctx->src_h == src_h)) {
        return RT_EOK;
    }

    ctx->src_w = src_w;
    ctx->src_h = src_h;
    ctx->src_stride = (uint32_t)src_w * 2U;
    ctx->dst_w = LCD_W;
    ctx->dst_h = (uint16_t)(((uint32_t)src_h * LCD_W) / src_w);
    if (ctx->dst_h == 0U) {
        ctx->dst_h = 1U;
    }
    if (ctx->dst_h > LCD_H) {
        ctx->dst_h = LCD_H;
    }
    ctx->y_offset = (uint16_t)((LCD_H - ctx->dst_h) / 2U);

    ctx->update_rect.x = 0U;
    ctx->update_rect.y = ctx->y_offset;
    ctx->update_rect.width = LCD_W;
    ctx->update_rect.height = ctx->dst_h;

    for (uint16_t dy = 0; dy < ctx->dst_h; dy++) {
        uint32_t sy = ((uint32_t)dy * src_h) / ctx->dst_h;
        if (sy >= src_h) {
            sy = src_h - 1U;
        }
        ctx->row_offsets[dy] = sy * ctx->src_stride;
    }

    for (uint16_t dx = 0; dx < ctx->dst_w; dx++) {
        uint32_t sx = ((uint32_t)dx * src_w) / ctx->dst_w;
        if (sx >= src_w) {
            sx = src_w - 1U;
        }
        ctx->x_offsets[dx] = (uint16_t)sx;
        ctx->pair_offsets[dx] = (uint16_t)((sx & ~1U) * 2U);
        ctx->y_offsets[dx] = (uint8_t)((sx & 1U) ? 2U : 0U);
    }

    ctx->prepared = RT_TRUE;
    ctx->background_synced = RT_FALSE;
    return RT_EOK;
}

static UVC_ITCM_SECTION void uvc_display_sync_background(void)
{
    if (!g_uvc_display_ctx.background_synced) {
        if (uvc_display_refresh_fb_info() != RT_EOK) {
            return;
        }

        memset(lcd_fb, 0, lcd_fb_size);
        (void)uvc_display_flush(RT_NULL);

        g_uvc_display_ctx.background_synced = RT_TRUE;
    }
}

static UVC_ITCM_SECTION void uvc_display_rgb565(const uint16_t *src, uint32_t src_size,
                                                uint16_t src_w, uint16_t src_h)
{
    struct uvc_display_context *ctx = &g_uvc_display_ctx;

    if (!lcd_fb || !src || src_size < sizeof(uint16_t)) {
        return;
    }

    if (uvc_display_prepare_geometry(src_w, src_h) != RT_EOK) {
        return;
    }
    uvc_display_sync_background();

    for (uint16_t dy = 0; dy < ctx->dst_h; dy++) {
        const uint16_t *srow;
        uint16_t *drow;

        if ((ctx->row_offsets[dy] + ctx->src_stride) > src_size) {
            break;
        }

        srow = (const uint16_t *)((const uint8_t *)src + ctx->row_offsets[dy]);
        drow = lcd_fb + (ctx->y_offset + dy) * LCD_W;

        for (uint16_t dx = 0; dx < ctx->dst_w; dx++) {
            drow[dx] = srow[ctx->x_offsets[dx]];
        }
    }
}

static size_t uvc_mjpeg_input_func(JDEC *jd, uint8_t *buf, size_t nbyte)
{
    struct uvc_mjpeg_decoder *decoder = (struct uvc_mjpeg_decoder *)jd->device;
    size_t remain;

    if ((decoder == RT_NULL) || (decoder->src == RT_NULL) ||
        (decoder->src_offset >= decoder->src_size)) {
        return 0U;
    }

    remain = (size_t)(decoder->src_size - decoder->src_offset);
    if (nbyte > remain) {
        nbyte = remain;
    }

    if ((buf != RT_NULL) && (nbyte > 0U)) {
        memcpy(buf, decoder->src + decoder->src_offset, nbyte);
    }

    decoder->src_offset += (uint32_t)nbyte;
    return nbyte;
}

static int uvc_mjpeg_output_func(JDEC *jd, void *bitmap, JRECT *rect)
{
    struct uvc_mjpeg_decoder *decoder = (struct uvc_mjpeg_decoder *)jd->device;
    uint16_t block_w;
    uint16_t block_h;

    if ((decoder == RT_NULL) || (bitmap == RT_NULL) || (rect == RT_NULL) ||
        (decoder->dst_rgb565 == RT_NULL)) {
        return 0;
    }

    if ((rect->left >= decoder->width) || (rect->top >= decoder->height)) {
        return 1;
    }

    block_w = (uint16_t)(rect->right - rect->left + 1U);
    block_h = (uint16_t)(rect->bottom - rect->top + 1U);

#if JD_FORMAT == 1
    {
        const uint16_t *src = (const uint16_t *)bitmap;

        for (uint16_t y = 0; y < block_h; y++) {
            uint16_t *dst = decoder->dst_rgb565 +
                            (uint32_t)(rect->top + y) * decoder->width + rect->left;

            memcpy(dst, src + (uint32_t)y * block_w, (size_t)block_w * sizeof(uint16_t));
        }
    }
#else
    {
        const uint8_t *src = (const uint8_t *)bitmap;

        for (uint16_t y = 0; y < block_h; y++) {
            uint16_t *dst = decoder->dst_rgb565 +
                            (uint32_t)(rect->top + y) * decoder->width + rect->left;

            for (uint16_t x = 0; x < block_w; x++) {
                uint8_t b = *src++;
                uint8_t g = *src++;
                uint8_t r = *src++;

                dst[x] = rgb888_to_rgb565(r, g, b);
            }
        }
    }
#endif

    return 1;
}

static rt_err_t uvc_display_mjpeg_decode(const uint8_t *src, uint32_t src_size,
                                         uint16_t *width, uint16_t *height)
{
    struct uvc_mjpeg_decoder decoder;
    struct uvc_mjpeg_validate_info validate_info;
    const uint8_t *jpeg;
    const char *validate_error;
    uint32_t jpeg_size;
    JDEC jd;
    JRESULT result;

    if ((src == RT_NULL) || (src_size < 4U) || (width == RT_NULL) || (height == RT_NULL)) {
        return -RT_ERROR;
    }

    memset(&decoder, 0, sizeof(decoder));
    decoder.dst_rgb565 = g_uvc_mjpeg_rgb565;

    if (uvc_mjpeg_prepare_frame(src, src_size, &jpeg, &jpeg_size) != RT_EOK) {
        return -RT_ERROR;
    }

    decoder.src = jpeg;
    decoder.src_size = jpeg_size;

    validate_error = uvc_mjpeg_validate_jpeg(jpeg, jpeg_size, &validate_info);
    if (validate_error != RT_NULL) {
        if (g_uvc_mjpeg_diag_budget > 0U) {
            USB_LOG_WRN("MJPEG validate failed: %s m=%u wh=%ux%u comp=%u qt=%02x dc=%02x ac=%02x size=%lu\r\n",
                        validate_error, validate_info.marker_count,
                        validate_info.width, validate_info.height,
                        validate_info.ncomp, validate_info.qt_mask,
                        validate_info.dc_mask, validate_info.ac_mask,
                        (unsigned long)jpeg_size);
            g_uvc_mjpeg_diag_budget--;
        }
        return -RT_ERROR;
    }

    result = jd_prepare(&jd, uvc_mjpeg_input_func,
                        g_uvc_mjpeg_workbuf, sizeof(g_uvc_mjpeg_workbuf),
                        &decoder);
    if (result != JDR_OK) {
        USB_LOG_WRN("MJPEG prepare failed: %s src=%lu jpeg=%lu\r\n",
                    uvc_mjpeg_result_string(result),
                    (unsigned long)src_size, (unsigned long)jpeg_size);
        return -RT_ERROR;
    }

    if (((uint32_t)jd.width * (uint32_t)jd.height) > UVC_MJPEG_MAX_PIXELS) {
        USB_LOG_ERR("MJPEG frame too large: %ux%u\r\n", jd.width, jd.height);
        return -RT_ERROR;
    }

    decoder.width = jd.width;
    decoder.height = jd.height;

    result = jd_decomp(&jd, uvc_mjpeg_output_func, 0);
    if (result != JDR_OK) {
        USB_LOG_WRN("MJPEG decode failed: %s src=%lu jpeg=%lu\r\n",
                    uvc_mjpeg_result_string(result),
                    (unsigned long)src_size, (unsigned long)jpeg_size);
        return -RT_ERROR;
    }

    *width = decoder.width;
    *height = decoder.height;
    return RT_EOK;
}

/*
 * Render a YUYV frame to the LCD framebuffer using nearest-neighbour scaling.
 *
 * Camera image (src_w × src_h) is scaled to fill LCD width (LCD_W) while
 * maintaining aspect ratio. Image is centred vertically; remaining areas
 * are left black.
 *
 * YUYV format: [Y0 U0 Y1 V0] per 2 pixels, 4 bytes per 2 pixels.
 */
static UVC_ITCM_SECTION void uvc_display_yuyv(const uint8_t *src, uint32_t src_size,
                                              uint16_t src_w, uint16_t src_h)
{
    struct uvc_display_context *ctx = &g_uvc_display_ctx;

    if (!lcd_fb || !src || src_size < 4)
        return;

    uvc_display_prepare_yuv_lut();
    if ((src_w == UVC_YUYV_STAGE_W) && (src_h == UVC_YUYV_STAGE_H)) {
        uint16_t *dst = g_uvc_yuyv_stage_rgb565;

        if (src_size < (uint32_t)UVC_YUYV_STAGE_W * UVC_YUYV_STAGE_H * 2U) {
            return;
        }

        for (uint16_t y = 0; y < UVC_YUYV_STAGE_H; y++) {
            const uint8_t *srow = src + (uint32_t)y * UVC_YUYV_STAGE_W * 2U;
            uint16_t *drow = dst + (uint32_t)y * UVC_YUYV_STAGE_W;

            for (uint16_t pair = 0; pair < (UVC_YUYV_STAGE_W / 2U); pair++) {
                uint8_t y0 = srow[0];
                uint8_t u = srow[1];
                uint8_t y1 = srow[2];
                uint8_t v = srow[3];
                int32_t u_to_b = g_uvc_u_to_b_lut[u];
                int32_t u_to_g = g_uvc_u_to_g_lut[u];
                int32_t v_to_r = g_uvc_v_to_r_lut[v];
                int32_t v_to_g = g_uvc_v_to_g_lut[v];

                drow[0] = yuv_to_rgb565_fast(y0, u_to_b, u_to_g, v_to_r, v_to_g);
                drow[1] = yuv_to_rgb565_fast(y1, u_to_b, u_to_g, v_to_r, v_to_g);

                srow += 4;
                drow += 2;
            }
        }

        uvc_display_rgb565(g_uvc_yuyv_stage_rgb565,
                           UVC_YUYV_STAGE_BYTES,
                           UVC_YUYV_STAGE_W, UVC_YUYV_STAGE_H);
        return;
    }

    if (uvc_display_prepare_geometry(src_w, src_h) != RT_EOK) {
        return;
    }
    uvc_display_sync_background();

    for (uint16_t dy = 0; dy < ctx->dst_h; dy++) {
        const uint8_t *srow;
        uint16_t *drow;
        uint16_t cached_pair = 0xffffU;
        int32_t u_to_b = 0;
        int32_t u_to_g = 0;
        int32_t v_to_r = 0;
        int32_t v_to_g = 0;

        if ((ctx->row_offsets[dy] + ctx->src_stride) > src_size) {
            break;
        }

        srow = src + ctx->row_offsets[dy];
        drow = lcd_fb + (ctx->y_offset + dy) * LCD_W;

        for (uint16_t dx = 0; dx < ctx->dst_w; dx++) {
            uint16_t pair = ctx->pair_offsets[dx];
            uint8_t y_val;

            if (pair != cached_pair) {
                uint8_t u_val = srow[pair + 1U];
                uint8_t v_val = srow[pair + 3U];

                u_to_b = g_uvc_u_to_b_lut[u_val];
                u_to_g = g_uvc_u_to_g_lut[u_val];
                v_to_r = g_uvc_v_to_r_lut[v_val];
                v_to_g = g_uvc_v_to_g_lut[v_val];
                cached_pair = pair;
            }

            y_val = srow[pair + ctx->y_offsets[dx]];
            drow[dx] = yuv_to_rgb565_fast(y_val, u_to_b, u_to_g, v_to_r, v_to_g);
        }
    }
}

/* ---- public API ---- */

int uvc_display_init(void)
{
    if (uvc_display_hw_init() != RT_EOK) {
        USB_LOG_ERR("LCD hardware init failed\r\n");
        return -1;
    }

    if (uvc_display_refresh_fb_info() != RT_EOK) {
        USB_LOG_ERR("LCD framebuffer unavailable\r\n");
        return -1;
    }

    USB_LOG_INFO("LCD: %ux%u %dbpp fb=%p size=%u\r\n",
                 LCD_W, LCD_H, 16,
                 lcd_fb, lcd_fb_size);
    return 0;
}

void uvc_display_set_overlay_callback(uvc_display_overlay_cb_t cb, void *user_ctx)
{
    g_uvc_overlay_cb = cb;
    g_uvc_overlay_cb_ctx = user_ctx;
}

static void uvc_display_run_overlay_callback(void)
{
    uvc_display_overlay_info_t info;

    if ((g_uvc_overlay_cb == RT_NULL) || (lcd_fb == RT_NULL)) {
        return;
    }

    info.framebuffer = lcd_fb;
    info.lcd_width = LCD_W;
    info.lcd_height = LCD_H;
    info.src_width = g_uvc_display_ctx.src_w;
    info.src_height = g_uvc_display_ctx.src_h;
    info.dst_width = g_uvc_display_ctx.dst_w;
    info.dst_height = g_uvc_display_ctx.dst_h;
    info.dst_y_offset = g_uvc_display_ctx.y_offset;

    g_uvc_overlay_cb(&info, g_uvc_overlay_cb_ctx);
}

UVC_ITCM_SECTION void uvc_display_frame(struct usbh_videoframe *frame,
                                        uint16_t src_w, uint16_t src_h)
{
    uint16_t jpeg_w;
    uint16_t jpeg_h;

    if (!frame)
        return;

    if (uvc_display_refresh_fb_info() != RT_EOK || !lcd_fb)
        return;

    if (frame->frame_format == USBH_VIDEO_FORMAT_MJPEG) {
        if (uvc_display_mjpeg_decode(frame->frame_buf, frame->frame_size,
                                     &jpeg_w, &jpeg_h) != RT_EOK) {
            return;
        }

        uvc_display_rgb565(g_uvc_mjpeg_rgb565,
                           (uint32_t)jpeg_w * jpeg_h * sizeof(uint16_t),
                           jpeg_w, jpeg_h);
        (void)uvc_display_flush(&g_uvc_display_ctx.update_rect);
        return;
    }

    /* YUYV rendering */
    uvc_display_yuyv(frame->frame_buf, frame->frame_size, src_w, src_h);
    uvc_display_run_overlay_callback();

    /* Flush only the active image band to reduce LCD transfer time. */
    (void)uvc_display_flush(&g_uvc_display_ctx.update_rect);
}
