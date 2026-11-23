/*
 * Minimal BTstack run loop for RT-Thread.
 *
 * The upstream RT-Thread BTstack package uses the POSIX run loop, which needs
 * pthreads/select/termios.  This example instead runs the loop in a dedicated
 * RT-Thread thread and polls the HCI UART block directly.
 */

#include <rtthread.h>

#include "btstack_run_loop.h"
#include "btstack_run_loop_base.h"
#include "btstack_linked_list.h"

#include "btstack_run_loop_rtt.h"
#include "btstack_uart_block_rtt.h"

static rt_sem_t btstack_loop_sem;
static volatile int btstack_loop_exit;
static void (*btstack_poll_callback)(void);

static uint32_t rtt_get_time_ms(void)
{
    return (uint32_t)(((uint64_t)rt_tick_get() * 1000u) / RT_TICK_PER_SECOND);
}

static void rtt_init(void)
{
    btstack_run_loop_base_init();

    if (btstack_loop_sem == RT_NULL)
    {
        btstack_loop_sem = rt_sem_create("bt_loop", 0, RT_IPC_FLAG_FIFO);
    }
    btstack_loop_exit = 0;
}

static void rtt_add_data_source(btstack_data_source_t *data_source)
{
    btstack_run_loop_base_add_data_source(data_source);
}

static bool rtt_remove_data_source(btstack_data_source_t *data_source)
{
    return btstack_run_loop_base_remove_data_source(data_source);
}

static void rtt_enable_data_source_callbacks(btstack_data_source_t *data_source, uint16_t callbacks)
{
    btstack_run_loop_base_enable_data_source_callbacks(data_source, callbacks);
}

static void rtt_disable_data_source_callbacks(btstack_data_source_t *data_source, uint16_t callbacks)
{
    btstack_run_loop_base_disable_data_source_callbacks(data_source, callbacks);
}

static void rtt_set_timer(btstack_timer_source_t *timer, uint32_t timeout_in_ms)
{
    timer->timeout = rtt_get_time_ms() + timeout_in_ms;
}

static void rtt_add_timer(btstack_timer_source_t *timer)
{
    btstack_run_loop_base_add_timer(timer);
    btstack_run_loop_rtt_wakeup();
}

static bool rtt_remove_timer(btstack_timer_source_t *timer)
{
    return btstack_run_loop_base_remove_timer(timer);
}

static void rtt_dump_timer(void)
{
}

static void rtt_process_data_sources(void)
{
    btstack_linked_list_iterator_t it;

    btstack_linked_list_iterator_init(&it, &btstack_run_loop_base_data_sources);
    while (btstack_linked_list_iterator_has_next(&it))
    {
        btstack_data_source_t *ds =
            (btstack_data_source_t *)btstack_linked_list_iterator_next(&it);

        if (ds->process == NULL)
        {
            continue;
        }

        if (ds->flags & DATA_SOURCE_CALLBACK_READ)
        {
            ds->process(ds, DATA_SOURCE_CALLBACK_READ);
        }
        if (ds->flags & DATA_SOURCE_CALLBACK_WRITE)
        {
            ds->process(ds, DATA_SOURCE_CALLBACK_WRITE);
        }
        if (ds->flags & DATA_SOURCE_CALLBACK_POLL)
        {
            ds->process(ds, DATA_SOURCE_CALLBACK_POLL);
        }
    }
}

static void rtt_execute(void)
{
    while (!btstack_loop_exit)
    {
        uint32_t now = rtt_get_time_ms();
        int32_t wait_ms;

        btstack_run_loop_base_process_timers(now);
        rtt_process_data_sources();
        btstack_uart_block_rtt_poll();

        /* Run application commands that must execute on the BT thread. */
        if (btstack_poll_callback != RT_NULL)
        {
            btstack_poll_callback();
        }

        wait_ms = btstack_run_loop_base_get_time_until_timeout(rtt_get_time_ms());
        if (wait_ms < 1)
        {
            wait_ms = 1;
        }
        if (wait_ms > 10)
        {
            /*
             * A short timeout keeps the UART poll responsive.  The RX
             * interrupt also wakes this semaphore immediately.
             */
            wait_ms = 1;
        }

        if (btstack_loop_sem != RT_NULL)
        {
            rt_sem_take(btstack_loop_sem, rt_tick_from_millisecond(wait_ms));
        }
    }
}

static const btstack_run_loop_t btstack_run_loop_rtt = {
    &rtt_init,
    &rtt_add_data_source,
    &rtt_remove_data_source,
    &rtt_enable_data_source_callbacks,
    &rtt_disable_data_source_callbacks,
    &rtt_set_timer,
    &rtt_add_timer,
    &rtt_remove_timer,
    &rtt_execute,
    &rtt_dump_timer,
    &rtt_get_time_ms,
};

const btstack_run_loop_t *btstack_run_loop_rtt_instance(void)
{
    return &btstack_run_loop_rtt;
}

void btstack_run_loop_rtt_wakeup(void)
{
    if (btstack_loop_sem != RT_NULL)
    {
        rt_sem_release(btstack_loop_sem);
    }
}

void btstack_run_loop_rtt_request_exit(void)
{
    btstack_loop_exit = 1;
    btstack_run_loop_rtt_wakeup();
}

void btstack_run_loop_rtt_set_poll_callback(void (*callback)(void))
{
    btstack_poll_callback = callback;
}
