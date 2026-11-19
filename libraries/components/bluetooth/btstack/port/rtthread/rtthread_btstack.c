/* SPDX-License-Identifier: Apache-2.0 */
#include <rtthread.h>
#include <rthw.h>

#include "rtthread_btstack.h"
#include "btstack_memory.h"
#include "btstack_run_loop.h"
#include "hci.h"
#include "hci_transport_h4.h"

enum {
    WAKE_CALLBACK = 1u,
    WAKE_RX = 2u,
    WAKE_TX = 4u,
    WAKE_EXIT = 8u,
};

static struct rt_event wake;
static struct rt_semaphore rx_request;
static uint8_t *rx_buffer;
static uint16_t rx_length;
static void (*block_received)(void);
static void (*block_sent)(void);
static int started;
static struct bluetooth_hci_uart uart;
static void (*setup_profile)(void);

static uint32_t now_ms(void)
{
    return rt_tick_get_millisecond();
}

static void set_timer(btstack_timer_source_t *timer, uint32_t timeout_ms)
{
    timer->timeout = now_ms() + timeout_ms + 1;
}

static void wake_run_loop(void)
{
    rt_event_send(&wake, WAKE_CALLBACK);
}

static void queue_callback(btstack_context_callback_registration_t *callback)
{
    rt_base_t level = rt_hw_interrupt_disable();
    btstack_run_loop_base_add_callback(callback);
    rt_hw_interrupt_enable(level);
    wake_run_loop();
}

static void dispatch_callbacks(void)
{
    for (;;)
    {
        rt_base_t level = rt_hw_interrupt_disable();
        btstack_context_callback_registration_t *cb = (btstack_context_callback_registration_t *)
            btstack_linked_list_pop(&btstack_run_loop_base_callbacks);
        rt_hw_interrupt_enable(level);
        if (!cb)
            return;
        cb->callback(cb->context);
    }
}

static void exit_run_loop(void)
{
    rt_event_send(&wake, WAKE_EXIT);
}

static void run(void)
{
    rt_uint32_t flags = 0;
    for (;;)
    {
        if (flags & WAKE_EXIT)
            return;
        if (flags & WAKE_TX)
            block_sent();
        if (flags & WAKE_RX)
            block_received();
        btstack_run_loop_base_poll_data_sources();
        dispatch_callbacks();
        btstack_run_loop_base_process_timers(now_ms());

        int32_t ms = btstack_run_loop_base_get_time_until_timeout(now_ms());
        rt_int32_t timeout = ms < 0 ? RT_WAITING_FOREVER : (rt_int32_t)rt_tick_from_millisecond(ms);
        flags = 0;
        rt_event_recv(&wake, WAKE_CALLBACK | WAKE_RX | WAKE_TX | WAKE_EXIT,
                      RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, timeout, &flags);
    }
}

static const btstack_run_loop_t rt_run_loop = {
    .init = btstack_run_loop_base_init,
    .add_data_source = btstack_run_loop_base_add_data_source,
    .remove_data_source = btstack_run_loop_base_remove_data_source,
    .enable_data_source_callbacks = btstack_run_loop_base_enable_data_source_callbacks,
    .disable_data_source_callbacks = btstack_run_loop_base_disable_data_source_callbacks,
    .set_timer = set_timer,
    .add_timer = btstack_run_loop_base_add_timer,
    .remove_timer = btstack_run_loop_base_remove_timer,
    .execute = run,
    .dump_timer = btstack_run_loop_base_dump_timer,
    .get_time_ms = now_ms,
    .poll_data_sources_from_irq = wake_run_loop,
    .execute_on_main_thread = queue_callback,
    .trigger_exit = exit_run_loop,
};

static int uart_init(const btstack_uart_config_t *config)
{
    return (config->baudrate == uart.baudrate && (config->flowcontrol != 0) == uart.flow_control) ? 0 : -RT_EINVAL;
}

static int uart_open(void)
{
    /* The board has already opened the controller byte stream. */
    return 0;
}

static int uart_close(void)
{
    exit_run_loop();
    return 0;
}

static int uart_baudrate(uint32_t baudrate)
{
    return baudrate == uart.baudrate ? 0 : -RT_EINVAL;
}

static void on_rx(void (*handler)(void))
{
    block_received = handler;
}

static void on_tx(void (*handler)(void))
{
    block_sent = handler;
}

static void receive_block(uint8_t *dest, uint16_t length)
{
    /* H4 requests exactly one block and waits for block_received before
     * requesting the next header or payload. */
    rx_buffer = dest;
    rx_length = length;
    rt_sem_release(&rx_request);
}

static void send_block(const uint8_t *data, uint16_t length)
{
    int sent = uart.write(data, length);
    if (sent != length)
    {
        rt_kprintf("H4 UART write failed (%d)\n", sent);
        exit_run_loop();
        return;
    }
    rt_event_send(&wake, WAKE_TX);
}

static const btstack_uart_t h4_uart = {
    .init = uart_init,
    .open = uart_open,
    .close = uart_close,
    .set_block_received = on_rx,
    .set_block_sent = on_tx,
    .set_baudrate = uart_baudrate,
    .receive_block = receive_block,
    .send_block = send_block,
};

static void rx_thread(void *arg)
{
    for (;;)
    {
        rt_sem_take(&rx_request, RT_WAITING_FOREVER);
        uint8_t *dest = rx_buffer;
        unsigned remaining = rx_length;
        while (remaining)
        {
            int n = uart.read(dest, remaining);
            if (n <= 0 || (unsigned)n > remaining)
            {
                rt_kprintf("H4 UART read failed (%d)\n", n);
                exit_run_loop();
                return;
            }
            dest += n;
            remaining -= (unsigned)n;
        }
        rt_event_send(&wake, WAKE_RX);
    }
}

static void host_thread(void *arg)
{
    static hci_transport_config_uart_t config = {
        .type = HCI_TRANSPORT_CONFIG_UART,
    };
    config.baudrate_init = uart.baudrate;
    config.flowcontrol = uart.flow_control;
    btstack_memory_init();
    btstack_run_loop_init(&rt_run_loop);
    hci_init(hci_transport_h4_instance_for_uart(&h4_uart), &config);
    setup_profile();
    int rc = hci_power_control(HCI_POWER_ON);
    if (rc == 0)
        btstack_run_loop_execute();
    rt_kprintf("BTstack stopped (%d); reset board to restart\n", rc);
}

int rtthread_btstack_start(const struct bluetooth_hci_uart *io, void (*init_profile)(void))
{
    if (!io || !io->open || !io->read || !io->write || !io->baudrate || !init_profile)
        return -RT_EINVAL;
    rt_base_t level = rt_hw_interrupt_disable();
    if (started)
    {
        rt_hw_interrupt_enable(level);
        return -RT_EBUSY;
    }
    started = 1;
    uart = *io;
    setup_profile = init_profile;
    rt_hw_interrupt_enable(level);

    rt_thread_t rx = rt_thread_create("bt_hci", rx_thread, RT_NULL, 2048, 16, 10);
    rt_thread_t host = rt_thread_create("bt_host", host_thread, RT_NULL, 6144, 18, 10);
    int rc = rx && host ? uart.open() : -RT_ENOMEM;
    if (rc)
    {
        if (rx) rt_thread_delete(rx);
        if (host) rt_thread_delete(host);
        level = rt_hw_interrupt_disable();
        started = 0;
        rt_hw_interrupt_enable(level);
        return rc;
    }

    rt_event_init(&wake, "bt_loop", RT_IPC_FLAG_FIFO);
    rt_sem_init(&rx_request, "bt_req", 0, RT_IPC_FLAG_FIFO);
    rt_thread_startup(rx);
    rt_thread_startup(host);
    return 0;
}
