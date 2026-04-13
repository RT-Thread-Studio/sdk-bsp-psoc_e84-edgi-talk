#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rtthread.h>

#include "board.h"
#include "usbd_core.h"
#include "usbd_cdc_acm.h"

#include "deepcraft_usbd.h"

#define DEEPCRAFT_USB_BUSID 0
#define DEEPCRAFT_USB_BASE  USBHS_BASE

#define CDC_IN_EP  0x81
#define CDC_OUT_EP 0x02
#define CDC_INT_EP 0x83

#define DEEPCRAFT_VID       0x058B
#define DEEPCRAFT_PID       0x027D
#define DEEPCRAFT_MAX_POWER 100

#ifdef CONFIG_USB_HS
#define CDC_MAX_MPS 512
#else
#define CDC_MAX_MPS 64
#endif

#define USB_CONFIG_SIZE (9 + CDC_ACM_DESCRIPTOR_LEN)

#define CDC_RX_TRANSFER_SIZE CDC_MAX_MPS
#define CDC_TX_CHUNK_SIZE    2048
#define CDC_RX_RING_SIZE     8192

static deepcraft_usbd_t *g_active_usb = RT_NULL;

static volatile rt_bool_t g_usb_configured = RT_FALSE;
static volatile rt_bool_t g_tx_busy = RT_FALSE;
static volatile rt_bool_t g_rx_overflow = RT_FALSE;

static char g_serial_string[37] = "00000000-0000-0000-0000-000000000000";

static uint8_t g_rx_ring[CDC_RX_RING_SIZE];
static volatile uint32_t g_rx_head = 0;
static volatile uint32_t g_rx_tail = 0;

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX static uint8_t g_out_transfer_buf[CDC_RX_TRANSFER_SIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX static uint8_t g_tx_transfer_buf[CDC_TX_CHUNK_SIZE];

static struct usbd_interface g_intf0;
static struct usbd_interface g_intf1;

static void deepcraft_cdc_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes);
static void deepcraft_cdc_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes);
static void deepcraft_start_out_read(uint8_t busid);

static struct usbd_endpoint g_cdc_out_ep = {
    .ep_addr = CDC_OUT_EP,
    .ep_cb = deepcraft_cdc_bulk_out,
};

static struct usbd_endpoint g_cdc_in_ep = {
    .ep_addr = CDC_IN_EP,
    .ep_cb = deepcraft_cdc_bulk_in,
};

static const uint8_t g_device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, DEEPCRAFT_VID, DEEPCRAFT_PID, 0x0100, 0x01)
};

static const uint8_t g_config_descriptor[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, DEEPCRAFT_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, CDC_MAX_MPS, 0x02)
};

static const uint8_t g_device_quality_descriptor[] = {
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

static const char *g_string_descriptors[4] = {
    (const char[]){ 0x09, 0x04 },
    "Infineon Technologies",
    "Imagimob Streaming Device",
    g_serial_string,
};

static const uint8_t *deepcraft_device_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return g_device_descriptor;
}

static const uint8_t *deepcraft_config_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return g_config_descriptor;
}

static const uint8_t *deepcraft_device_quality_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return g_device_quality_descriptor;
}

static const char *deepcraft_string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;
    if (index >= 4)
    {
        return RT_NULL;
    }
    return g_string_descriptors[index];
}

static const struct usb_descriptor g_cdc_descriptor = {
    .device_descriptor_callback = deepcraft_device_descriptor_callback,
    .config_descriptor_callback = deepcraft_config_descriptor_callback,
    .device_quality_descriptor_callback = deepcraft_device_quality_descriptor_callback,
    .string_descriptor_callback = deepcraft_string_descriptor_callback,
};

static uint32_t deepcraft_ring_count_locked(void)
{
    if (g_rx_head >= g_rx_tail)
    {
        return g_rx_head - g_rx_tail;
    }
    return CDC_RX_RING_SIZE - (g_rx_tail - g_rx_head);
}

static uint32_t deepcraft_ring_free_locked(void)
{
    return (CDC_RX_RING_SIZE - 1U) - deepcraft_ring_count_locked();
}

static void deepcraft_start_out_read(uint8_t busid)
{
    uint32_t rx_chunk = usbd_get_ep_mps(busid, CDC_OUT_EP);
    if ((rx_chunk == 0U) || (rx_chunk > sizeof(g_out_transfer_buf)))
    {
        rx_chunk = sizeof(g_out_transfer_buf);
    }
    usbd_ep_start_read(busid, CDC_OUT_EP, g_out_transfer_buf, rx_chunk);
}

static void deepcraft_ring_push_bytes(const uint8_t *data, uint32_t len)
{
    rt_base_t level = rt_hw_interrupt_disable();

    uint32_t free_len = deepcraft_ring_free_locked();
    if (len > free_len)
    {
        g_rx_overflow = RT_TRUE;
        g_rx_head = 0;
        g_rx_tail = 0;
        rt_hw_interrupt_enable(level);
        return;
    }

    for (uint32_t i = 0; i < len; i++)
    {
        g_rx_ring[g_rx_head] = data[i];
        g_rx_head++;
        if (g_rx_head >= CDC_RX_RING_SIZE)
        {
            g_rx_head = 0;
        }
    }

    rt_hw_interrupt_enable(level);
}

static rt_bool_t deepcraft_ring_pop_byte(uint8_t *out)
{
    rt_bool_t ok = RT_FALSE;
    rt_base_t level = rt_hw_interrupt_disable();

    if (g_rx_head != g_rx_tail)
    {
        *out = g_rx_ring[g_rx_tail];
        g_rx_tail++;
        if (g_rx_tail >= CDC_RX_RING_SIZE)
        {
            g_rx_tail = 0;
        }
        ok = RT_TRUE;
    }

    rt_hw_interrupt_enable(level);
    return ok;
}

static void deepcraft_usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event)
    {
    case USBD_EVENT_CONFIGURED:
        g_usb_configured = RT_TRUE;
        g_rx_overflow = RT_FALSE;
        g_rx_head = 0;
        g_rx_tail = 0;
        deepcraft_start_out_read(busid);
        break;
    case USBD_EVENT_DISCONNECTED:
    case USBD_EVENT_RESET:
        g_usb_configured = RT_FALSE;
        g_tx_busy = RT_FALSE;
        g_rx_overflow = RT_FALSE;
        g_rx_head = 0;
        g_rx_tail = 0;
        break;
    default:
        break;
    }
}

static void deepcraft_cdc_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)ep;
    if (nbytes > 0)
    {
        deepcraft_ring_push_bytes(g_out_transfer_buf, nbytes);
    }
    deepcraft_start_out_read(busid);
}

static void deepcraft_cdc_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    if ((nbytes % usbd_get_ep_mps(busid, ep)) == 0U && nbytes > 0U)
    {
        usbd_ep_start_write(busid, CDC_IN_EP, RT_NULL, 0);
        return;
    }

    g_tx_busy = RT_FALSE;
}

static void deepcraft_cdc_init(uint8_t busid, uintptr_t reg_base)
{
    usbd_desc_register(busid, &g_cdc_descriptor);

    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &g_intf0));
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &g_intf1));
    usbd_add_endpoint(busid, &g_cdc_out_ep);
    usbd_add_endpoint(busid, &g_cdc_in_ep);

    usbd_initialize(busid, reg_base, deepcraft_usbd_event_handler);
}

static bool deepcraft_pb_read(pb_istream_t *stream, pb_byte_t *buf, size_t count)
{
    deepcraft_usbd_t *usb = (deepcraft_usbd_t *)stream->state;
    uint8_t discard = 0;

    for (size_t i = 0; i < count; i++)
    {
        uint8_t *dst = (buf != RT_NULL) ? (uint8_t *)&buf[i] : &discard;

        while (!deepcraft_ring_pop_byte(dst))
        {
            if (g_rx_overflow)
            {
                g_rx_overflow = RT_FALSE;
                return false;
            }
            protocol_call_device_poll(usb->protocol, &usb->ostream);
            rt_thread_mdelay(1);
        }
    }

    return true;
}

static bool deepcraft_pb_write(pb_ostream_t *stream, const pb_byte_t *buf, size_t count)
{
    size_t offset = 0;

    (void)stream;

    if (!g_usb_configured)
    {
        return false;
    }

    while (offset < count)
    {
        size_t chunk = count - offset;
        if (chunk > CDC_TX_CHUNK_SIZE)
        {
            chunk = CDC_TX_CHUNK_SIZE;
        }

        while (g_tx_busy)
        {
            rt_thread_mdelay(1);
        }

        rt_memcpy(g_tx_transfer_buf, &buf[offset], chunk);
        g_tx_busy = RT_TRUE;

        if (usbd_ep_start_write(DEEPCRAFT_USB_BUSID, CDC_IN_EP, g_tx_transfer_buf, (uint32_t)chunk) < 0)
        {
            g_tx_busy = RT_FALSE;
            return false;
        }

        for (int retry = 0; retry < 2000; retry++)
        {
            if (!g_tx_busy)
            {
                break;
            }
            rt_thread_mdelay(1);
        }

        if (g_tx_busy)
        {
            g_tx_busy = RT_FALSE;
            return false;
        }

        offset += chunk;
    }

    return true;
}

deepcraft_usbd_t *deepcraft_usbd_create(protocol_t *protocol)
{
    if (protocol == RT_NULL)
    {
        return RT_NULL;
    }

    deepcraft_usbd_t *usb = (deepcraft_usbd_t *)rt_calloc(1, sizeof(deepcraft_usbd_t));
    if (usb == RT_NULL)
    {
        return RT_NULL;
    }

    usb->protocol = protocol;
    g_active_usb = usb;

    const pb_byte_t *serial = protocol->board.serial.uuid;
    rt_snprintf(g_serial_string, sizeof(g_serial_string),
                "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                serial[0], serial[1], serial[2], serial[3],
                serial[4], serial[5],
                serial[6], serial[7],
                serial[8], serial[9],
                serial[10], serial[11], serial[12], serial[13], serial[14], serial[15]);

    g_usb_configured = RT_FALSE;
    g_tx_busy = RT_FALSE;
    g_rx_overflow = RT_FALSE;
    g_rx_head = 0;
    g_rx_tail = 0;

    deepcraft_cdc_init(DEEPCRAFT_USB_BUSID, DEEPCRAFT_USB_BASE);

    while (!g_usb_configured)
    {
        rt_thread_mdelay(20);
    }

    usb->istream = (pb_istream_t){ deepcraft_pb_read, (void *)usb, SIZE_MAX, 0 };
    usb->ostream = (pb_ostream_t){ deepcraft_pb_write, (void *)usb, SIZE_MAX, 0, RT_NULL };

    return usb;
}

void deepcraft_usbd_destroy(deepcraft_usbd_t *usb)
{
    if (usb == RT_NULL)
    {
        return;
    }

    if (g_active_usb == usb)
    {
        g_active_usb = RT_NULL;
    }

    rt_free(usb);
}

void deepcraft_usbd_resync_rx(void)
{
    rt_base_t level = rt_hw_interrupt_disable();
    g_rx_overflow = RT_FALSE;
    g_rx_head = 0;
    g_rx_tail = 0;
    rt_hw_interrupt_enable(level);
}
