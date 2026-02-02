#include <string.h>
#include <rtthread.h>
#include <rtdevice.h>

#include "usb_display.h"
#include "usb_config.h"
#include "usbd_core.h"
#include "usbd_graphic.h"
#include "usbd_hid.h"
#include "usb_def.h"

#define USB_DISPLAY_BUSID 0

#ifndef USBHS_BASE
#define USBHS_BASE 0x44900000UL
#endif

#define VENDOR_IN_EP  0x81
#define VENDOR_OUT_EP 0x01

#ifdef CONFIG_USB_HS
#define VENDOR_MAX_MPS 512
#else
#define VENDOR_MAX_MPS 64
#endif

#define USBD_VID           0x303A
#define USBD_PID           0x1986
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#define USB_DISPLAY_CONFIG_DESC_SIZ   (9 + 9 + 7 + 7)

#define USB_DISPLAY_CMD_FRAME_START 0x01
#define USB_DISPLAY_CMD_FRAME_END   0x04
#define USB_DISPLAY_CMD_FRAME_CLOSE 0x08

#define USB_DISPLAY_FORMAT_RGB565 0x00

#ifndef USB_DISPLAY_LOG_ENABLE
#define USB_DISPLAY_LOG_ENABLE 1
#endif

#if USB_DISPLAY_LOG_ENABLE
#define USB_DISPLAY_LOG(...) rt_kprintf(__VA_ARGS__)
#else
#define USB_DISPLAY_LOG(...)
#endif

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[VENDOR_MAX_MPS];

#define USB_DISPLAY_MAX_WIDTH  480U
#define USB_DISPLAY_MAX_HEIGHT 800U
#define USB_DISPLAY_FRAME_BYTES (USB_DISPLAY_MAX_WIDTH * USB_DISPLAY_MAX_HEIGHT * 2U)

/** LCD frame buffer base address. */
static uint8_t *framebuffer;
static uint32_t framebuffer_size;
static struct rt_device *lcd_dev;
static struct rt_device_graphic_info lcd_info;
static struct rt_semaphore frame_sem;
static rt_thread_t frame_thread;
/** Framebuffer stride in bytes (pitch). */
static uint32_t fb_stride_bytes;
static uint16_t usb_frame_width;
static uint16_t usb_frame_height;
static uint32_t usb_frame_capacity;
/** Optional staging buffer when direct framebuffer write is not available. */
static uint8_t *usb_framebuffer;
static uint32_t usb_frame_bytes;
/**
 * Direct write mode:
 * RT_TRUE  - write USB payload directly into LCD framebuffer.
 * RT_FALSE - copy into usb_framebuffer first, then blit to LCD framebuffer.
 */
static rt_bool_t use_direct_fb;
static uint32_t gui_frame_count;
static uint32_t gui_frame_count_last;
static rt_tick_t gui_fps_last_tick;

/** Static staging buffer placed in SOC memory region. */
CY_SECTION(".cy_socmem_data") static uint8_t usb_framebuffer_static[USB_DISPLAY_FRAME_BYTES];

rt_inline uint32_t usb_display_min_u32(uint32_t a, uint32_t b)
{
    return (a < b) ? a : b;
}

/*
 * Copy USB RGB565 payload into LCD framebuffer with stride handling.
 * - offset/len are relative to a tightly packed frame (line_bytes = width * 2).
 * - Handles unaligned writes and uses line-based copies when possible.
 */
static void usb_display_copy_to_framebuffer(uint32_t offset, const uint8_t *src, uint32_t len)
{
    uint32_t line_bytes = usb_frame_width * 2U;
    uint32_t height = lcd_info.height;
    if ((line_bytes == 0U) || (framebuffer == RT_NULL))
    {
        return;
    }
    while (len > 0U)
    {
        uint32_t row = offset / line_bytes;
        if (row >= height)
        {
            return;
        }
        uint32_t col = offset - (row * line_bytes);

        if (col == 0U)
        {
            uint32_t lines = len / line_bytes;
            uint32_t max_lines = height - row;
            if (lines > max_lines)
            {
                lines = max_lines;
            }
            if (lines > 0U)
            {
                uint8_t *dst = framebuffer + (row * fb_stride_bytes);
                const uint8_t *src_line = src;
                for (uint32_t i = 0; i < lines; ++i)
                {
                    memcpy(dst, src_line, line_bytes);
                    dst += fb_stride_bytes;
                    src_line += line_bytes;
                }

                uint32_t consumed = lines * line_bytes;
                src += consumed;
                offset += consumed;
                len -= consumed;
                continue;
            }
        }

        uint32_t copy_len = line_bytes - col;
        if (copy_len > len)
        {
            copy_len = len;
        }
        uint32_t start = (row * fb_stride_bytes) + col;
        uint8_t *dst = framebuffer + start;
        memcpy(dst, src, copy_len);
        src += copy_len;
        offset += copy_len;
        len -= copy_len;
    }
}

static void usb_display_blit_to_lcd(void)
{
    if (use_direct_fb)
    {
        return;
    }
    if ((usb_framebuffer == RT_NULL) || (framebuffer == RT_NULL))
    {
        return;
    }
    uint32_t line_bytes = usb_frame_width * 2U;
    uint32_t pad_bytes = 0U;
    if (fb_stride_bytes > line_bytes)
    {
        pad_bytes = fb_stride_bytes - line_bytes;
    }
    uint32_t lines = usb_display_min_u32(usb_frame_height, lcd_info.height);
    for (uint32_t y = 0; y < lines; ++y)
    {
        uint8_t *dst = framebuffer + (y * fb_stride_bytes);
        uint8_t *src = usb_framebuffer + (y * line_bytes);
        memcpy(dst, src, line_bytes);
        if (pad_bytes > 0U)
        {
            memset(dst + line_bytes, 0x00, pad_bytes);
        }
    }
}

static void usb_display_clear_stride_padding(void)
{
    uint32_t line_bytes = usb_frame_width * 2U;
    if (fb_stride_bytes <= line_bytes)
    {
        return;
    }
    uint32_t pad_bytes = fb_stride_bytes - line_bytes;
    uint32_t lines = usb_display_min_u32(usb_frame_height, lcd_info.height);
    for (uint32_t y = 0; y < lines; ++y)
    {
        uint8_t *dst = framebuffer + (y * fb_stride_bytes) + line_bytes;
        memset(dst, 0x00, pad_bytes);
    }
}

static const uint8_t device_descriptor[] =
{
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0000, 0x00)
};

static const uint8_t config_descriptor[] =
{
    USB_CONFIG_DESCRIPTOR_INIT(USB_DISPLAY_CONFIG_DESC_SIZ, 0x01, 0x01, 0xc0, USBD_MAX_POWER),
    USB_INTERFACE_DESCRIPTOR_INIT(0x00, 0x00, 0x02, 0xff, 0x00, 0x00, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(VENDOR_IN_EP, 2, VENDOR_MAX_MPS, 0),
    USB_ENDPOINT_DESCRIPTOR_INIT(VENDOR_OUT_EP, 2, VENDOR_MAX_MPS, 0),
};

static const uint8_t device_quality_descriptor[] =
{
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
};

static const char *string_descriptors[] =
{
    (const char[]){ 0x09, 0x04 },
    "Edgi-talk",
    "USB_Graphic",
    "20260202",
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
    if (index > 3)
    {
        return RT_NULL;
    }
    return string_descriptors[index];
}

static const struct usb_descriptor vendor_descriptor =
{
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback
};

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event)
    {
    case USBD_EVENT_CONFIGURED:
        usbd_ep_start_read(busid, VENDOR_OUT_EP, read_buffer, VENDOR_MAX_MPS);
        break;
    default:
        break;
    }
}

static void usb_display_submit_frame(void)
{
    if (frame_sem.value < 1)
    {
        rt_sem_release(&frame_sem);
    }
}

static void usb_display_frame_worker(void *parameter)
{
    (void)parameter;
    while (1)
    {
        rt_sem_take(&frame_sem, RT_WAITING_FOREVER);
        if ((lcd_dev != RT_NULL) && (framebuffer != RT_NULL))
        {
            usb_display_blit_to_lcd();
            lcd_dev->control(lcd_dev, RTGRAPHIC_CTRL_RECT_UPDATE, RT_NULL);

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
        }
    }
}

static void usbd_graphic_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    uint32_t cmd;
    uint32_t payload_len;
    uint32_t copy_len;

    if (nbytes < 4)
    {
        usbd_ep_start_read(busid, ep, read_buffer, VENDOR_MAX_MPS);
        return;
    }

    cmd = *(uint32_t *)read_buffer;
    if (cmd & USB_DISPLAY_CMD_FRAME_START)
    {
        usb_frame_bytes = 0;
    }

    payload_len = nbytes - 4;
    if (usb_frame_bytes < usb_frame_capacity)
    {
        uint32_t remaining = usb_frame_capacity - usb_frame_bytes;
        copy_len = (payload_len < remaining) ? payload_len : remaining;
        if (copy_len > 0U)
        {
            if (use_direct_fb && (framebuffer != RT_NULL))
            {
                usb_display_copy_to_framebuffer(usb_frame_bytes, &read_buffer[4], copy_len);
                usb_frame_bytes += copy_len;
            }
            else if (usb_framebuffer != RT_NULL)
            {
                memcpy(usb_framebuffer + usb_frame_bytes, &read_buffer[4], copy_len);
                usb_frame_bytes += copy_len;
            }
        }
    }

    usbd_ep_start_read(busid, ep, read_buffer, VENDOR_MAX_MPS);

    if (cmd & USB_DISPLAY_CMD_FRAME_END)
    {
        if (usb_frame_bytes == usb_frame_capacity)
        {
            if (use_direct_fb)
            {
                usb_display_clear_stride_padding();
            }
            usb_display_submit_frame();
        }
        else
        {
            USB_DISPLAY_LOG("usb_display: frame size mismatch bytes=%u expect=%u\n",
                            usb_frame_bytes,
                            usb_frame_capacity);
        }
    }

}

static void usbd_graphic_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;
    (void)nbytes;
}

static struct usbd_endpoint graphic_out_ep =
{
    .ep_addr = VENDOR_OUT_EP,
    .ep_cb = usbd_graphic_bulk_out
};

static struct usbd_endpoint graphic_in_ep =
{
    .ep_addr = VENDOR_IN_EP,
    .ep_cb = usbd_graphic_bulk_in
};

static struct usbd_interface graphic_intf;

int usb_display_init(void)
{
    uintptr_t reg_base = (uintptr_t)USBHS_BASE;

    if (rt_sem_init(&frame_sem, "usbdisp", 0, RT_IPC_FLAG_FIFO) != RT_EOK)
    {
        return -RT_ERROR;
    }

    frame_thread = rt_thread_create("usbdisp", usb_display_frame_worker, RT_NULL, 2048, 12, 10);
    if (frame_thread == RT_NULL)
    {
        return -RT_ERROR;
    }
    rt_thread_startup(frame_thread);

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
            usb_frame_capacity = (uint32_t)usb_frame_width * (uint32_t)usb_frame_height * 2U;
            if ((framebuffer != RT_NULL) && (fb_stride_bytes == (usb_frame_width * 2U)))
            {
                use_direct_fb = RT_TRUE;
            }
            else if (usb_frame_capacity <= sizeof(usb_framebuffer_static))
            {
                usb_framebuffer = usb_framebuffer_static;
                memset(usb_framebuffer, 0x00, usb_frame_capacity);
            }
            else
            {
                usb_framebuffer = RT_NULL;
            }
            usbd_graphic_set_info(usb_frame_width, usb_frame_height, 60, USB_DISPLAY_FORMAT_RGB565);
            rt_kprintf("usb_display: lcd %ux%u bpp=%u stride=%u fb=%p size=%u, usb %ux%u\r\n",
                       lcd_info.width,
                       lcd_info.height,
                       lcd_info.bits_per_pixel,
                       fb_stride_bytes,
                       framebuffer,
                       framebuffer_size,
                       usb_frame_width,
                       usb_frame_height);
            if (usb_framebuffer == RT_NULL)
            {
                USB_DISPLAY_LOG("usb_display: alloc usb frame buffer failed\n");
            }
        }
    }

    if (framebuffer == RT_NULL)
    {
        USB_DISPLAY_LOG("usb_display: lcd not ready\n");
    }

    usbd_desc_register(USB_DISPLAY_BUSID, &vendor_descriptor);
    usbd_add_interface(USB_DISPLAY_BUSID, usbd_graphic_init_intf(&graphic_intf));
    usbd_add_endpoint(USB_DISPLAY_BUSID, &graphic_out_ep);
    usbd_add_endpoint(USB_DISPLAY_BUSID, &graphic_in_ep);

    return usbd_initialize(USB_DISPLAY_BUSID, reg_base, usbd_event_handler);
}
