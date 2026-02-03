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

#include "usb_display.h"
#include "usb_config.h"
#include "usbd_core.h"
#include "usbd_display.h"

#define USB_DISPLAY_BUSID 0

#ifndef USBHS_BASE
#define USBHS_BASE 0x44900000UL
#endif

#define DISPLAY_IN_EP  0x81
#define DISPLAY_OUT_EP 0x02

#ifdef CONFIG_USB_HS
#define DISPLAY_EP_MPS 512
#else
#define DISPLAY_EP_MPS 64
#endif

#define USBD_VID           0x303A
#define USBD_PID           0x2987
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#define USB_CONFIG_SIZE (9 + 9 + 7 + 7)

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

static const uint8_t config_descriptor[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x01, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    USB_INTERFACE_DESCRIPTOR_INIT(0x00, 0x00, 0x02, 0xff, 0x00, 0x00, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(DISPLAY_IN_EP, 0x02, DISPLAY_EP_MPS, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(DISPLAY_OUT_EP, 0x02, DISPLAY_EP_MPS, 0x00),
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
        break;
    case USBD_EVENT_SET_REMOTE_WAKEUP:
        break;
    case USBD_EVENT_CLR_REMOTE_WAKEUP:
        break;
    default:
        break;
    }
}

/* ---------- Frame pool (double buffering) ---------- */
static struct usbd_interface intf0;
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

    /* Create display processing thread */
    display_thread = rt_thread_create("usbdisp", usb_display_thread_entry, RT_NULL, 4096, 12, 10);
    if (display_thread == RT_NULL)
    {
        USB_DISPLAY_LOG("usb_display: create thread failed\n");
        return -RT_ERROR;
    }
    rt_thread_startup(display_thread);

    /* Init frame pool (double buffering) */
    frame_pool[0].frame_buf = usb_display_buffer[0];
    frame_pool[0].frame_bufsize = USB_DISPLAY_FRAME_BYTES;
    frame_pool[1].frame_buf = usb_display_buffer[1];
    frame_pool[1].frame_bufsize = USB_DISPLAY_FRAME_BYTES;

    /* Register USB display device */
    usbd_desc_register(USB_DISPLAY_BUSID, &display_descriptor);
    usbd_add_interface(USB_DISPLAY_BUSID, usbd_display_init_intf(&intf0, DISPLAY_OUT_EP, DISPLAY_IN_EP, frame_pool, 2));
    usbd_initialize(USB_DISPLAY_BUSID, reg_base, usbd_event_handler);

    USB_DISPLAY_LOG("usb_display: initialized with usbd_display driver\r\n");
    return RT_EOK;
}
