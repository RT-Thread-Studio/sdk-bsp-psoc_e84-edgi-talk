/*
 * USB Display application - adapted from CherryUSB demo/display/usbdisplay_template.c
 *
 * Uses the usbd_display class driver with mempool-based double buffering.
 * Receives frames from the Windows xfz1986_usb_graphic driver and renders
 * them to the on-board LCD via RT-Thread graphic device.
 */
#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <drivers/touch.h>

#include "usb_display.h"
#include "usb_config.h"
#include "usbd_core.h"
#include "usbd_display.h"
#include "usbd_hid.h"
#include "drv_touch.h"

#define USB_DISPLAY_BUSID 0

#ifndef USBHS_BASE
#define USBHS_BASE 0x44900000UL
#endif

#define DISPLAY_IN_EP  0x81
#define DISPLAY_OUT_EP 0x02
#define HID_TOUCH_IN_EP 0x83

#define HID_TOUCH_EP_MPS      8
#define HID_TOUCH_EP_INTERVAL 1

#ifdef CONFIG_USB_HS
#define DISPLAY_EP_MPS 512
#else
#define DISPLAY_EP_MPS 64
#endif

#define USBD_VID           0x303A
#define USBD_PID           0x2986
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#define USB_CONFIG_SIZE (9 + 9 + 7 + 7 + 9 + 9 + 7)

#ifndef USB_DISPLAY_LOG_ENABLE
#define USB_DISPLAY_LOG_ENABLE 1
#endif

#if USB_DISPLAY_LOG_ENABLE
#define USB_DISPLAY_LOG(...) rt_kprintf(__VA_ARGS__)
#else
#define USB_DISPLAY_LOG(...)
#endif

/* ---------- Display parameters ---------- */
#define USB_DISPLAY_MAX_WIDTH  480U
#define USB_DISPLAY_MAX_HEIGHT 800U
#define USB_DISPLAY_FRAME_BYTES (USB_DISPLAY_MAX_WIDTH * USB_DISPLAY_MAX_HEIGHT * 2U)
#define USB_TOUCH_LOGICAL_MAX_X (USB_DISPLAY_MAX_WIDTH - 1U)
#define USB_TOUCH_LOGICAL_MAX_Y (USB_DISPLAY_MAX_HEIGHT - 1U)
#define HID_TOUCH_REPORT_DESC_SIZE 74

#define USB_TOUCH_DEV_NAME "ST7102"
#define USB_TOUCH_POLL_MS  10
#define USB_TOUCH_DEBOUNCE_DOWN 2
#define USB_TOUCH_DEBOUNCE_UP   2
#define USB_TOUCH_JITTER_PIXELS 3

/*
 * Product string encodes the resolution and format for the Windows driver.
 * Format: cherryusb_R<W>x<H>_E<fmt>_Fps<fps>_Bl<blocksize>
 *   fmt: rgb16 = RGB565, jpg9 = JPEG
 */
#define USB_DISPLAY_PRODUCT_STRING "cherryusb_R480x800_Ergb16_Fps30_Bl128"

/* ---------- LCD related ---------- */
static uint8_t *framebuffer;
static uint32_t framebuffer_size;
static struct rt_device *lcd_dev;
static struct rt_device_graphic_info lcd_info;
static rt_thread_t display_thread;
/** Framebuffer stride in bytes (pitch). */
static uint32_t fb_stride_bytes;
static uint16_t usb_frame_width;
static uint16_t usb_frame_height;

static uint32_t gui_frame_count;
static uint32_t gui_frame_count_last;
static rt_tick_t gui_fps_last_tick;

/* ---------- USB descriptor ---------- */
static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0101, 0x01)
};

static const uint8_t hid_touch_report_desc[HID_TOUCH_REPORT_DESC_SIZE] = {
    0x05, 0x0D, /* USAGE_PAGE (Digitizers) */
    0x09, 0x04, /* USAGE (Touch Screen) */
    0xA1, 0x01, /* COLLECTION (Application) */
    0x09, 0x22, /*   USAGE (Finger) */
    0xA1, 0x02, /*   COLLECTION (Logical) */
    0x09, 0x42, /*     USAGE (Tip Switch) */
    0x09, 0x32, /*     USAGE (In Range) */
    0x15, 0x00, /*     LOGICAL_MINIMUM (0) */
    0x25, 0x01, /*     LOGICAL_MAXIMUM (1) */
    0x75, 0x01, /*     REPORT_SIZE (1) */
    0x95, 0x02, /*     REPORT_COUNT (2) */
    0x81, 0x02, /*     INPUT (Data,Var,Abs) */
    0x75, 0x06, /*     REPORT_SIZE (6) */
    0x95, 0x01, /*     REPORT_COUNT (1) */
    0x81, 0x03, /*     INPUT (Cnst,Var,Abs) */
    0x05, 0x01, /*     USAGE_PAGE (Generic Desktop) */
    0x09, 0x30, /*     USAGE (X) */
    0x15, 0x00, /*     LOGICAL_MINIMUM (0) */
    0x26, WBVAL(USB_TOUCH_LOGICAL_MAX_X), /* LOGICAL_MAXIMUM (X) */
    0x75, 0x10, /*     REPORT_SIZE (16) */
    0x95, 0x01, /*     REPORT_COUNT (1) */
    0x81, 0x02, /*     INPUT (Data,Var,Abs) */
    0x09, 0x31, /*     USAGE (Y) */
    0x15, 0x00, /*     LOGICAL_MINIMUM (0) */
    0x26, WBVAL(USB_TOUCH_LOGICAL_MAX_Y), /* LOGICAL_MAXIMUM (Y) */
    0x75, 0x10, /*     REPORT_SIZE (16) */
    0x95, 0x01, /*     REPORT_COUNT (1) */
    0x81, 0x02, /*     INPUT (Data,Var,Abs) */
    0xC0,       /*   END_COLLECTION */
    0x05, 0x0D, /*   USAGE_PAGE (Digitizers) */
    0x09, 0x54, /*   USAGE (Contact Count) */
    0x15, 0x00, /*   LOGICAL_MINIMUM (0) */
    0x25, 0x01, /*   LOGICAL_MAXIMUM (1) */
    0x75, 0x08, /*   REPORT_SIZE (8) */
    0x95, 0x01, /*   REPORT_COUNT (1) */
    0x81, 0x02, /*   INPUT (Data,Var,Abs) */
    0xC0        /* END_COLLECTION */
};

static const uint8_t config_descriptor[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    USB_INTERFACE_DESCRIPTOR_INIT(0x00, 0x00, 0x02, 0xff, 0x00, 0x00, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(DISPLAY_IN_EP, 0x02, DISPLAY_EP_MPS, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(DISPLAY_OUT_EP, 0x02, DISPLAY_EP_MPS, 0x00),
    0x09,
    USB_DESCRIPTOR_TYPE_INTERFACE,
    0x01,
    0x00,
    0x01,
    0x03,
    0x00,
    0x00,
    0x00,
    0x09,
    HID_DESCRIPTOR_TYPE_HID,
    0x11,
    0x01,
    0x00,
    0x01,
    HID_DESCRIPTOR_TYPE_HID_REPORT,
    WBVAL(HID_TOUCH_REPORT_DESC_SIZE),
    0x07,
    USB_DESCRIPTOR_TYPE_ENDPOINT,
    HID_TOUCH_IN_EP,
    0x03,
    WBVAL(HID_TOUCH_EP_MPS),
    HID_TOUCH_EP_INTERVAL,
};

static const uint8_t device_quality_descriptor[] = {
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x00,
    0x00,
};

static const char *string_descriptors[] = {
    (const char[]){ 0x09, 0x04 },            /* Langid */
    "Edgi-talk",                             /* Manufacturer */
    USB_DISPLAY_PRODUCT_STRING,              /* Product - encodes resolution/format */
    "20260210",                              /* Serial Number */
};

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return config_descriptor;
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
    (void)speed;
#ifdef CONFIG_USB_HS
    return device_quality_descriptor;
#else
    return RT_NULL;
#endif
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;
    if (index >= (sizeof(string_descriptors) / sizeof(char *)))
    {
        return RT_NULL;
    }
    return string_descriptors[index];
}

static const struct usb_descriptor display_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback
};

/* ---------- USB touch HID ---------- */
struct hid_touch_report {
    uint8_t tip_inrange;
    uint16_t x;
    uint16_t y;
    uint8_t contact_count;
} __PACKED;

#define HID_STATE_IDLE 0
#define HID_STATE_BUSY 1

static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX struct hid_touch_report touch_report;
static volatile uint8_t hid_touch_state = HID_STATE_IDLE;

static rt_device_t touch_dev;
static struct rt_touch_info touch_info;
static rt_thread_t touch_thread;

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event)
    {
    case USBD_EVENT_RESET:
        break;
    case USBD_EVENT_CONNECTED:
        break;
    case USBD_EVENT_DISCONNECTED:
        break;
    case USBD_EVENT_RESUME:
        break;
    case USBD_EVENT_SUSPEND:
        break;
    case USBD_EVENT_CONFIGURED:
        hid_touch_state = HID_STATE_IDLE;
        break;
    case USBD_EVENT_SET_REMOTE_WAKEUP:
        break;
    case USBD_EVENT_CLR_REMOTE_WAKEUP:
        break;
    default:
        break;
    }
}

static void usbd_hid_touch_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;
    (void)nbytes;
    hid_touch_state = HID_STATE_IDLE;
}

static struct usbd_endpoint hid_in_ep = {
    .ep_cb = usbd_hid_touch_in_callback,
    .ep_addr = HID_TOUCH_IN_EP
};

static void usb_touch_get_src_max(uint16_t *src_max_x, uint16_t *src_max_y)
{
    uint32_t lcd_w = (lcd_info.width > 0) ? lcd_info.width : USB_DISPLAY_MAX_WIDTH;
    uint32_t lcd_h = (lcd_info.height > 0) ? lcd_info.height : USB_DISPLAY_MAX_HEIGHT;
    uint32_t range_x = (uint32_t)touch_info.range_x;
    uint32_t range_y = (uint32_t)touch_info.range_y;
    rt_bool_t use_touch_range = RT_FALSE;

    if ((range_x > 0U) && (range_y > 0U))
    {
        if ((range_x <= (lcd_w * 2U)) && (range_x >= (lcd_w / 2U)) &&
            (range_y <= (lcd_h * 2U)) && (range_y >= (lcd_h / 2U)))
        {
            use_touch_range = RT_TRUE;
        }
    }

    if (use_touch_range)
    {
        *src_max_x = (range_x > 0U) ? (uint16_t)(range_x - 1U) : (uint16_t)(lcd_w - 1U);
        *src_max_y = (range_y > 0U) ? (uint16_t)(range_y - 1U) : (uint16_t)(lcd_h - 1U);
    }
    else
    {
        *src_max_x = (lcd_w > 0U) ? (uint16_t)(lcd_w - 1U) : (uint16_t)(USB_DISPLAY_MAX_WIDTH - 1U);
        *src_max_y = (lcd_h > 0U) ? (uint16_t)(lcd_h - 1U) : (uint16_t)(USB_DISPLAY_MAX_HEIGHT - 1U);
    }
}

static uint16_t usb_touch_scale_coord(rt_int32_t coord, uint16_t src_max, uint16_t logical_max)
{
    if (coord < 0)
    {
        coord = 0;
    }

    if (src_max == 0U)
    {
        return 0;
    }

    if ((uint32_t)coord > src_max)
    {
        coord = (rt_int32_t)src_max;
    }

    return (uint16_t)(((uint32_t)coord * (uint32_t)logical_max) / (uint32_t)src_max);
}

static rt_err_t usb_touch_open_device(void)
{
    touch_dev = rt_device_find(USB_TOUCH_DEV_NAME);
    if (touch_dev == RT_NULL)
    {
        if (rt_hw_ST7102_port() == RT_EOK)
        {
            touch_dev = rt_device_find(USB_TOUCH_DEV_NAME);
        }
    }

    if (touch_dev == RT_NULL)
    {
        USB_DISPLAY_LOG("usb_touch: device %s not found\r\n", USB_TOUCH_DEV_NAME);
        return -RT_ERROR;
    }

    if (rt_device_open(touch_dev, RT_DEVICE_FLAG_INT_RX) != RT_EOK)
    {
        USB_DISPLAY_LOG("usb_touch: open device failed\r\n");
        return -RT_ERROR;
    }

    if (rt_device_control(touch_dev, RT_TOUCH_CTRL_GET_INFO, &touch_info) != RT_EOK)
    {
        rt_memset(&touch_info, 0, sizeof(touch_info));
    }

    return RT_EOK;
}

static void usb_touch_thread_entry(void *parameter)
{
    (void)parameter;
    struct rt_touch_data touch_data[1];
    rt_int16_t last_raw_x = 0;
    rt_int16_t last_raw_y = 0;
    rt_bool_t last_raw_pressed = RT_FALSE;
    rt_int16_t filtered_x = 0;
    rt_int16_t filtered_y = 0;
    rt_bool_t filtered_pressed = RT_FALSE;
    rt_uint8_t down_count = 0;
    rt_uint8_t up_count = 0;
    uint16_t src_max_x = 0;
    uint16_t src_max_y = 0;

    while (1)
    {
        if (touch_dev == RT_NULL)
        {
            rt_thread_mdelay(USB_TOUCH_POLL_MS);
            continue;
        }

        if (!usb_device_is_configured(USB_DISPLAY_BUSID))
        {
            rt_thread_mdelay(USB_TOUCH_POLL_MS);
            continue;
        }

        rt_memset(touch_data, 0, sizeof(touch_data));
        rt_size_t num = rt_device_read(touch_dev, 0, touch_data, 1);
        rt_bool_t raw_pressed = last_raw_pressed;
        rt_int16_t raw_x = last_raw_x;
        rt_int16_t raw_y = last_raw_y;

        if (num > 0)
        {
            if ((touch_data[0].event == RT_TOUCH_EVENT_DOWN) || (touch_data[0].event == RT_TOUCH_EVENT_MOVE))
            {
                raw_pressed = RT_TRUE;
                raw_x = touch_data[0].x_coordinate;
                raw_y = touch_data[0].y_coordinate;
            }
            else if (touch_data[0].event == RT_TOUCH_EVENT_UP)
            {
                raw_pressed = RT_FALSE;
                raw_x = touch_data[0].x_coordinate;
                raw_y = touch_data[0].y_coordinate;
            }
        }
        else if (last_raw_pressed)
        {
            raw_pressed = RT_FALSE;
        }

        if (raw_pressed)
        {
            if (down_count < 0xFF)
            {
                down_count++;
            }
            up_count = 0;
        }
        else
        {
            if (up_count < 0xFF)
            {
                up_count++;
            }
            down_count = 0;
        }

        if (!filtered_pressed)
        {
            if (raw_pressed && (down_count >= USB_TOUCH_DEBOUNCE_DOWN) && (hid_touch_state == HID_STATE_IDLE))
            {
                filtered_pressed = RT_TRUE;
                filtered_x = raw_x;
                filtered_y = raw_y;

                rt_int32_t map_x = filtered_x;
                rt_int32_t map_y = filtered_y;

                usb_touch_get_src_max(&src_max_x, &src_max_y);

                touch_report.tip_inrange = 0x03;
                touch_report.x = usb_touch_scale_coord(map_x, src_max_x, USB_TOUCH_LOGICAL_MAX_X);
                touch_report.y = usb_touch_scale_coord(map_y, src_max_y, USB_TOUCH_LOGICAL_MAX_Y);
                touch_report.contact_count = 1;

                hid_touch_state = HID_STATE_BUSY;
                int ret = usbd_ep_start_write(USB_DISPLAY_BUSID,
                                              HID_TOUCH_IN_EP,
                                              (uint8_t *)&touch_report,
                                              sizeof(touch_report));
                if (ret < 0)
                {
                    hid_touch_state = HID_STATE_IDLE;
                    filtered_pressed = RT_FALSE;
                }
            }
        }
        else
        {
            if (!raw_pressed && (up_count >= USB_TOUCH_DEBOUNCE_UP) && (hid_touch_state == HID_STATE_IDLE))
            {
                filtered_pressed = RT_FALSE;

                rt_int32_t map_x = filtered_x;
                rt_int32_t map_y = filtered_y;

                usb_touch_get_src_max(&src_max_x, &src_max_y);

                touch_report.tip_inrange = 0x00;
                touch_report.x = usb_touch_scale_coord(map_x, src_max_x, USB_TOUCH_LOGICAL_MAX_X);
                touch_report.y = usb_touch_scale_coord(map_y, src_max_y, USB_TOUCH_LOGICAL_MAX_Y);
                touch_report.contact_count = 0;

                hid_touch_state = HID_STATE_BUSY;
                int ret = usbd_ep_start_write(USB_DISPLAY_BUSID,
                                              HID_TOUCH_IN_EP,
                                              (uint8_t *)&touch_report,
                                              sizeof(touch_report));
                if (ret < 0)
                {
                    hid_touch_state = HID_STATE_IDLE;
                }
            }
            else if (raw_pressed && (hid_touch_state == HID_STATE_IDLE))
            {
                rt_int32_t dx = raw_x - filtered_x;
                rt_int32_t dy = raw_y - filtered_y;
                if ((dx < 0) || (dy < 0))
                {
                    dx = (dx < 0) ? -dx : dx;
                    dy = (dy < 0) ? -dy : dy;
                }
                if ((dx >= USB_TOUCH_JITTER_PIXELS) || (dy >= USB_TOUCH_JITTER_PIXELS))
                {
                    filtered_x = raw_x;
                    filtered_y = raw_y;

                    rt_int32_t map_x = filtered_x;
                    rt_int32_t map_y = filtered_y;

                    usb_touch_get_src_max(&src_max_x, &src_max_y);

                    touch_report.tip_inrange = 0x03;
                    touch_report.x = usb_touch_scale_coord(map_x, src_max_x, USB_TOUCH_LOGICAL_MAX_X);
                    touch_report.y = usb_touch_scale_coord(map_y, src_max_y, USB_TOUCH_LOGICAL_MAX_Y);
                    touch_report.contact_count = 1;

                    hid_touch_state = HID_STATE_BUSY;
                    int ret = usbd_ep_start_write(USB_DISPLAY_BUSID,
                                                  HID_TOUCH_IN_EP,
                                                  (uint8_t *)&touch_report,
                                                  sizeof(touch_report));
                    if (ret < 0)
                    {
                        hid_touch_state = HID_STATE_IDLE;
                    }
                }
            }
        }

        last_raw_pressed = raw_pressed;
        last_raw_x = raw_x;
        last_raw_y = raw_y;

        rt_thread_mdelay(USB_TOUCH_POLL_MS);
    }
}

/* ---------- Frame pool (double buffering) ---------- */
static struct usbd_interface intf0;
static struct usbd_interface intf1;
static struct usbd_display_frame frame_pool[2];

/*
 * Frame buffers placed in GPU memory region for DMA/no-cache access.
 * Each buffer is USB_DISPLAY_FRAME_BYTES and must be a multiple of 16384.
 * Using gfx_mem (3MB) instead of cy_socmem_data to avoid m55_data_secondary overflow.
 */
CY_SECTION(".cy_usb_display_buf") USB_MEM_ALIGNX
static uint8_t usb_display_buffer[2][USB_DISPLAY_FRAME_BYTES];

/* ---------- Frame rendering helpers ---------- */

struct usbd_disp_frame_header {
    uint16_t crc16;
    uint8_t type;
    uint8_t cmd;
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    uint32_t frame_id : 10;
    uint32_t payload_total : 22;
} __PACKED;

rt_inline uint32_t usb_display_min_u32(uint32_t a, uint32_t b)
{
    return (a < b) ? a : b;
}

/**
 * The usbd_display driver delivers a raw frame in frame_buf with a
 * usbd_disp_frame_header at offset 0. The actual pixel payload starts
 * after the header (16 bytes). We need to parse the header to get
 * offset info and copy the pixels into the LCD framebuffer.
 *
 * For simplicity we copy the entire payload (after header) linearly
 * into the LCD framebuffer with stride handling.
 */
static void usb_display_render_frame(struct usbd_display_frame *frame)
{
    if ((framebuffer == RT_NULL) || (frame == RT_NULL))
    {
        return;
    }

    /*
     * Frame buffer layout from usbd_display:
     *   [0..15]   : usbd_disp_frame_header (16 bytes)
     *   [16..end] : pixel payload
     *
     * frame->frame_size = header->payload_total (total pixel bytes)
     * frame->frame_format = header->type (RGB565, etc.)
     */
    const struct usbd_disp_frame_header *header = (const struct usbd_disp_frame_header *)frame->frame_buf;
    uint32_t header_size = 16U; /* sizeof(usbd_disp_frame_header) */
    uint8_t *payload = frame->frame_buf + header_size;
    uint32_t payload_len = frame->frame_size;

    if (payload_len == 0U)
    {
        return;
    }

    uint16_t src_x = header->x;
    uint16_t src_y = header->y;
    uint16_t src_w = header->width;
    uint16_t src_h = header->height;

    if ((src_w == 0U) || (src_h == 0U))
    {
        src_w = usb_frame_width;
        src_h = usb_frame_height;
        src_x = 0U;
        src_y = 0U;
    }

    if ((src_x >= lcd_info.width) || (src_y >= lcd_info.height))
    {
        return;
    }

    uint32_t max_w = lcd_info.width - src_x;
    uint32_t max_h = lcd_info.height - src_y;
    uint32_t copy_w = usb_display_min_u32(src_w, max_w);
    uint32_t copy_h = usb_display_min_u32(src_h, max_h);

    uint32_t line_bytes = ((uint32_t)src_w) * 2U;
    uint32_t copy_line_bytes = ((uint32_t)copy_w) * 2U;
    if (line_bytes == 0U)
    {
        return;
    }

    uint32_t available_lines = payload_len / line_bytes;
    if (available_lines < copy_h)
    {
        copy_h = available_lines;
    }

    for (uint32_t y = 0; y < copy_h; y++)
    {
        uint8_t *dst = framebuffer + ((src_y + y) * fb_stride_bytes) + ((uint32_t)src_x * 2U);
        uint8_t *src = payload + (y * line_bytes);
        memcpy(dst, src, copy_line_bytes);
    }
}

/* ---------- Display thread ---------- */
static void usb_display_thread_entry(void *parameter)
{
    (void)parameter;
    struct usbd_display_frame *frame;
    int ret;

    while (1)
    {
        ret = usbd_display_dequeue(&frame, 0xffffffff);
        if (ret < 0)
        {
            continue;
        }

        // USB_DISPLAY_LOG("frame type: %u, frame size %u\r\n", frame->frame_format, frame->frame_size);

        /* Render to LCD framebuffer */
        usb_display_render_frame(frame);

        /* Notify LCD to refresh */
        if (lcd_dev != RT_NULL)
        {
            const struct usbd_disp_frame_header *header = (const struct usbd_disp_frame_header *)frame->frame_buf;
            struct rt_device_rect_info rect;
            rect.x = header->x;
            rect.y = header->y;
            rect.width = header->width;
            rect.height = header->height;
            if ((rect.width == 0U) || (rect.height == 0U))
            {
                rect.x = 0U;
                rect.y = 0U;
                rect.width = usb_frame_width;
                rect.height = usb_frame_height;
            }
            lcd_dev->control(lcd_dev, RTGRAPHIC_CTRL_RECT_UPDATE, &rect);
        }

        /* FPS tracking */
        gui_frame_count++;
        if (gui_fps_last_tick == 0)
        {
            gui_fps_last_tick = rt_tick_get();
            gui_frame_count_last = gui_frame_count;
        }
        else
        {
            rt_tick_t now = rt_tick_get();
            rt_tick_t delta = now - gui_fps_last_tick;
            if (delta >= (RT_TICK_PER_SECOND * 5U))
            {
                uint32_t frames = gui_frame_count - gui_frame_count_last;
                uint32_t fps_x100 = (frames * 100U * RT_TICK_PER_SECOND) / delta;
                USB_DISPLAY_LOG("lcd: fps=%u.%02u (%u frames/%u ticks)\r\n",
                                fps_x100 / 100U,
                                fps_x100 % 100U,
                                frames,
                                delta);
                gui_fps_last_tick = now;
                gui_frame_count_last = gui_frame_count;
            }
        }

        /* Return frame buffer to the pool */
        usbd_display_enqueue(frame);
    }
}

/* ---------- Init ---------- */
int usb_display_init(void)
{
    uintptr_t reg_base = (uintptr_t)USBHS_BASE;

    /* Init LCD */
    lcd_dev = rt_device_find("lcd");
    if (lcd_dev != RT_NULL)
    {
        rt_device_open(lcd_dev, RT_DEVICE_OFLAG_RDWR);
        if (lcd_dev->control(lcd_dev, RTGRAPHIC_CTRL_GET_INFO, &lcd_info) == RT_EOK)
        {
            framebuffer = (uint8_t *)lcd_info.framebuffer;
            usb_frame_width = USB_DISPLAY_MAX_WIDTH;
            usb_frame_height = USB_DISPLAY_MAX_HEIGHT;
            if (lcd_info.pitch > 0)
            {
                fb_stride_bytes = lcd_info.pitch;
            }
            else
            {
                fb_stride_bytes = (lcd_info.width * (lcd_info.bits_per_pixel / 8));
            }
            framebuffer_size = fb_stride_bytes * lcd_info.height;
            rt_kprintf("usb_display: lcd %ux%u bpp=%u stride=%u fb=%p size=%u, usb %ux%u\r\n",
                       lcd_info.width,
                       lcd_info.height,
                       lcd_info.bits_per_pixel,
                       fb_stride_bytes,
                       framebuffer,
                       framebuffer_size,
                       usb_frame_width,
                       usb_frame_height);
        }
    }

    if (framebuffer == RT_NULL)
    {
        USB_DISPLAY_LOG("usb_display: lcd not ready, will still init USB\n");
    }

    /* Init frame pool (double buffering) */
    frame_pool[0].frame_buf = usb_display_buffer[0];
    frame_pool[0].frame_bufsize = USB_DISPLAY_FRAME_BYTES;
    frame_pool[1].frame_buf = usb_display_buffer[1];
    frame_pool[1].frame_bufsize = USB_DISPLAY_FRAME_BYTES;

    /* Register USB display device */
    usbd_desc_register(USB_DISPLAY_BUSID, &display_descriptor);
    usbd_add_interface(USB_DISPLAY_BUSID, usbd_display_init_intf(&intf0, DISPLAY_OUT_EP, DISPLAY_IN_EP, frame_pool, 2));
    usbd_add_interface(USB_DISPLAY_BUSID, usbd_hid_init_intf(USB_DISPLAY_BUSID, &intf1, hid_touch_report_desc, HID_TOUCH_REPORT_DESC_SIZE));
    usbd_add_endpoint(USB_DISPLAY_BUSID, &hid_in_ep);
    usbd_initialize(USB_DISPLAY_BUSID, reg_base, usbd_event_handler);

    /* Create display processing thread after mempool is ready */
    display_thread = rt_thread_create("usbdisp", usb_display_thread_entry, RT_NULL, 4096, 12, 10);
    if (display_thread == RT_NULL)
    {
        USB_DISPLAY_LOG("usb_display: create thread failed\n");
        return -RT_ERROR;
    }
    rt_thread_startup(display_thread);

    if (usb_touch_open_device() == RT_EOK)
    {
        touch_thread = rt_thread_create("usbtch", usb_touch_thread_entry, RT_NULL, 2048, 13, 10);
        if (touch_thread == RT_NULL)
        {
            USB_DISPLAY_LOG("usb_touch: create thread failed\r\n");
        }
        else
        {
            rt_thread_startup(touch_thread);
        }
    }

    USB_DISPLAY_LOG("usb_display: initialized with usbd_display driver\r\n");
    return RT_EOK;
}
