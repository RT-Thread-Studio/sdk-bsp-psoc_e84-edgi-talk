/*
 * HCI H4 UART transport for the Edgi-Talk CYW55512 Bluetooth controller.
 *
 * The controller is connected to SCB4, exposed as RT-Thread device "uart4".
 * Hardware CTS/RTS flow control is enabled by the generated PDL config.
 * Sending and receiving is split into small pieces that are advanced from
 * btstack_uart_block_rtt_poll(), which runs on the BTstack run loop thread.
 */

#include <rtthread.h>
#include <rtdevice.h>

#include "drv_gpio.h"
#include "btstack_uart_block.h"
#include "btstack_uart_block_rtt.h"
#include "btstack_run_loop_rtt.h"

static struct rt_device *bt_uart_dev;
static int bt_uart_opened;
static uint32_t bt_uart_baudrate;

static uint8_t *bt_rx_buffer;
static uint16_t bt_rx_length;
static uint16_t bt_rx_position;

static const uint8_t *bt_tx_buffer;
static uint16_t bt_tx_length;
static uint16_t bt_tx_position;

static void (*bt_block_received)(void);
static void (*bt_block_sent)(void);
static void (*bt_wakeup_handler)(void);

static rt_err_t bt_uart_rx_indicate(rt_device_t dev, rt_size_t size)
{
    RT_UNUSED(dev);
    RT_UNUSED(size);

    /* Wake the BTstack run loop so it drains the UART promptly. */
    btstack_run_loop_rtt_wakeup();
    if (bt_wakeup_handler != RT_NULL)
    {
        bt_wakeup_handler();
    }
    return RT_EOK;
}

static int bt_uart_set_baudrate(uint32_t baudrate)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    if (bt_uart_dev == RT_NULL)
    {
        return -1;
    }

    config.baud_rate = baudrate;
    config.data_bits = DATA_BITS_8;
    config.stop_bits = STOP_BITS_1;
    config.parity      = PARITY_NONE;
    config.flowcontrol = RT_SERIAL_FLOWCONTROL_CTSRTS;

    if (rt_device_control(bt_uart_dev, RT_DEVICE_CTRL_CONFIG, &config) != RT_EOK)
    {
        return -1;
    }

    bt_uart_baudrate = baudrate;
    rt_kprintf("[bt] uart baud -> %u\n", (unsigned)baudrate);
    return 0;
}

uint32_t btstack_uart_block_rtt_get_baudrate(void)
{
    return bt_uart_baudrate;
}

static int bt_uart_open_device(void)
{
    if (bt_uart_dev == RT_NULL)
    {
        return -1;
    }

    if (bt_uart_opened)
    {
        return 0;
    }

    if (rt_device_open(bt_uart_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK)
    {
        rt_kprintf("[bt] open uart failed\n");
        return -1;
    }

    rt_device_set_rx_indicate(bt_uart_dev, bt_uart_rx_indicate);
    bt_uart_opened = 1;
    return 0;
}

static int bt_uart_init(const btstack_uart_config_t *uart_config)
{
    bt_uart_dev = rt_device_find(uart_config->device_name);
    if (bt_uart_dev == RT_NULL)
    {
        rt_kprintf("[bt] cannot find %s\n", uart_config->device_name);
        return -1;
    }

    if (bt_uart_open_device() != 0)
    {
        return -1;
    }

    /* Set the initial baud rate after the device has been opened. */
    if (bt_uart_set_baudrate(uart_config->baudrate) != 0)
    {
        return -1;
    }

    /* BT UART CTS is P10.2; the SCB drives RTS on P10.3. */
    rt_kprintf("[bt] %s open, CTS=%d\n", uart_config->device_name,
               rt_pin_read(GET_PIN(10, 2)));
    return 0;
}

static int bt_uart_open(void)
{
    return bt_uart_open_device();
}

static int bt_uart_close(void)
{
    if (bt_uart_dev != RT_NULL && bt_uart_opened)
    {
        rt_device_close(bt_uart_dev);
        bt_uart_opened = 0;
    }
    return 0;
}

static void bt_uart_set_block_received(void (*block_handler)(void))
{
    bt_block_received = block_handler;
}

static void bt_uart_set_block_sent(void (*block_handler)(void))
{
    bt_block_sent = block_handler;
}

static int bt_uart_set_parity(int parity)
{
    RT_UNUSED(parity);
    return 0;
}

static int bt_uart_set_flowcontrol(int flowcontrol)
{
    RT_UNUSED(flowcontrol);
    /* Flow control is fixed by the generated SCB4 configuration. */
    return 0;
}

static void bt_uart_receive_block(uint8_t *buffer, uint16_t length)
{
    bt_rx_buffer   = buffer;
    bt_rx_length   = length;
    bt_rx_position = 0;
}

static void bt_uart_send_block(const uint8_t *buffer, uint16_t length)
{
    bt_tx_buffer   = buffer;
    bt_tx_length   = length;
    bt_tx_position = 0;
}

static int bt_uart_get_supported_sleep_modes(void)
{
    return 0;
}

static void bt_uart_set_sleep(btstack_uart_sleep_mode_t sleep_mode)
{
    RT_UNUSED(sleep_mode);
}

static void bt_uart_set_wakeup_handler(void (*wakeup_handler)(void))
{
    bt_wakeup_handler = wakeup_handler;
}

static const btstack_uart_block_t btstack_uart_block_rtt = {
    &bt_uart_init,
    &bt_uart_open,
    &bt_uart_close,
    &bt_uart_set_block_received,
    &bt_uart_set_block_sent,
    &bt_uart_set_baudrate,
    &bt_uart_set_parity,
    &bt_uart_set_flowcontrol,
    &bt_uart_receive_block,
    &bt_uart_send_block,
    &bt_uart_get_supported_sleep_modes,
    &bt_uart_set_sleep,
    &bt_uart_set_wakeup_handler,
};

const btstack_uart_block_t *btstack_uart_block_rtt_instance(void)
{
    return &btstack_uart_block_rtt;
}

void btstack_uart_block_rtt_flush(void)
{
    uint8_t discard[32];

    if (bt_uart_dev == RT_NULL || !bt_uart_opened)
    {
        return;
    }

    while (rt_device_read(bt_uart_dev, -1, discard, sizeof(discard)) > 0)
    {
    }
}

void btstack_uart_block_rtt_poll(void)
{
    if (bt_uart_dev == RT_NULL || !bt_uart_opened)
    {
        return;
    }

    if (bt_tx_position < bt_tx_length)
    {
        rt_size_t written = rt_device_write(bt_uart_dev, 0,
                                            bt_tx_buffer + bt_tx_position,
                                            bt_tx_length - bt_tx_position);
        if (written > 0)
        {
            bt_tx_position += (uint16_t)written;
        }

        if (bt_tx_position >= bt_tx_length)
        {
            void (*handler)(void) = bt_block_sent;

            bt_tx_buffer   = RT_NULL;
            bt_tx_length   = 0;
            bt_tx_position = 0;
            if (handler != RT_NULL)
            {
                handler();
            }
        }
    }

    if (bt_rx_position < bt_rx_length)
    {
        rt_size_t received = rt_device_read(bt_uart_dev, -1,
                                            bt_rx_buffer + bt_rx_position,
                                            bt_rx_length - bt_rx_position);
        if (received > 0)
        {
            bt_rx_position += (uint16_t)received;
        }

        if (bt_rx_position >= bt_rx_length)
        {
            void (*handler)(void) = bt_block_received;

            bt_rx_buffer   = RT_NULL;
            bt_rx_length   = 0;
            bt_rx_position = 0;
            if (handler != RT_NULL)
            {
                handler();
            }
        }
    }
}
