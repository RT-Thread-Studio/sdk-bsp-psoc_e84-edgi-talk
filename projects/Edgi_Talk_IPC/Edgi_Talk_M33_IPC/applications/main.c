#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <string.h>

#include "drv_ipc.h"

#define LED_PIN_B                 GET_PIN(16, 5)
#define MSG_INTERVAL_MS           (1000U)

static void ipc_test_run(void)
{
    rt_device_t ipc_tx_dev;
    rt_device_t ipc_rx_dev;
    edge_rc_frame_t tx_frame;
    edge_rc_frame_t rx_frame;
    rt_uint32_t seq = 0;
    rt_tick_t last_send_tick = 0;
    rt_uint32_t tx_count = 0;
    rt_uint32_t rx_count = 0;

    if (edge_ipc_device_register() != RT_EOK) {
        rt_kprintf("[M33] IPC: Device register failed\r\n");
        return;
    }

    /* M33 sends on ipc1 and receives replies on ipc0. */
    ipc_tx_dev = edge_ipc_device_find(EDGE_IPC0_DEVICE_NAME);
    ipc_rx_dev = edge_ipc_device_find(EDGE_IPC1_DEVICE_NAME);
    if (ipc_tx_dev == RT_NULL || ipc_rx_dev == RT_NULL) {
        rt_kprintf("[M33] IPC: TX/RX device not found\r\n");
        return;
    }

    if (rt_device_open(ipc_tx_dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK) {
        rt_kprintf("[M33] IPC: Open TX device failed\r\n");
        return;
    }

    if (rt_device_open(ipc_rx_dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK) {
        rt_kprintf("[M33] IPC: Open RX device failed\r\n");
        rt_device_close(ipc_tx_dev);
        return;
    }

    rt_kprintf("\r\n");
    rt_kprintf("========================================\r\n");
    rt_kprintf("[M33] IPC Demo Started\r\n");
    rt_kprintf("----------------------------------------\r\n");
    rt_kprintf("Mode:     Sender\r\n");
    rt_kprintf("Target:   Cortex-M55\r\n");
    rt_kprintf("Message:  Hello M33\r\n");
    rt_kprintf("Interval: %d ms\r\n", MSG_INTERVAL_MS);
    rt_kprintf("========================================\r\n");
    rt_kprintf("\r\n");

    last_send_tick = rt_tick_get();

    while (1) {
        /* 定时发送消息 */
        if ((rt_tick_get() - last_send_tick) >= rt_tick_from_millisecond(MSG_INTERVAL_MS)) {
            /* 准备发送消息 - "Hello M33" */
            memset(&tx_frame, 0, sizeof(tx_frame));
            tx_frame.client_id = CM55_IPC_PIPE_CLIENT_ID;
            tx_frame.role = RC_ROLE_M33;
            tx_frame.magic = RC_MAGIC_WORD;
            tx_frame.seq = ++seq;
            /* Encode "Hello M33" into channels (2 chars per channel) */
            tx_frame.channel[0] = 0x4865UL;  /* 'He' in ASCII */
            tx_frame.channel[1] = 0x6C6CUL;  /* 'll' in ASCII */
            tx_frame.channel[2] = 0x6F20UL;  /* 'o ' in ASCII */
            tx_frame.channel[3] = 0x4D33UL;  /* 'M3' in ASCII */
            tx_frame.channel[4] = 0x0033UL;  /* '3'  in ASCII */
            tx_frame.checksum = edge_rc_checksum(&tx_frame);

            /* 发送消息 */
            if (rt_device_write(ipc_tx_dev, 0, &tx_frame, 1) == 1) {
                rt_kprintf("[M33] TX -> [M55]: \"Hello M33\" | Seq: %5lu | Time: %8lu ms\r\n",
                           seq, rt_tick_get() * 1000 / RT_TICK_PER_SECOND);
                tx_count++;
            } else {
                rt_kprintf("[M33] TX Failed\r\n");
            }

            last_send_tick = rt_tick_get();
        }

        /* 接收 M55 的回复 */
        if (rt_device_read(ipc_rx_dev, 0, &rx_frame, 1) == 1) {
            if (rx_frame.magic == RC_MAGIC_WORD &&
                rx_frame.role == RC_ROLE_M55_ECHO &&
                edge_rc_checksum(&rx_frame) == rx_frame.checksum) {

                /* 解析消息 - Check for "Hello M55" */
                if (rx_frame.channel[0] == 0x4865UL &&  /* 'He' */
                    rx_frame.channel[1] == 0x6C6CUL &&  /* 'll' */
                    rx_frame.channel[2] == 0x6F20UL &&  /* 'o ' */
                    rx_frame.channel[3] == 0x4D35UL &&  /* 'M5' */
                    rx_frame.channel[4] == 0x0035UL) {  /* '5' */

                    rt_kprintf("[M33] RX <- [M55]: \"Hello M55\" | Seq: %5lu | Time: %8lu ms\r\n",
                               rx_frame.seq, rt_tick_get() * 1000 / RT_TICK_PER_SECOND);
                    rx_count++;

                    /* 每 10 次交互打印统计信息 */
                    if (rx_count % 10 == 0) {
                        rt_kprintf("----------------------------------------\r\n");
                        rt_kprintf("[M33] Statistics: TX=%lu, RX=%lu\r\n", tx_count, rx_count);
                        rt_kprintf("----------------------------------------\r\n");
                    }
                }
            } else {
                rt_kprintf("[M33] Invalid message received\r\n");
            }
        }

        rt_thread_mdelay(1);
    }
}
MSH_CMD_EXPORT(ipc_test_run, Start M33 IPC test);

int main(void)
{
    rt_kprintf("\r\n");
    rt_kprintf("========================================\r\n");
    rt_kprintf("   RT-Thread on Cortex-M33 Core         \r\n");
    rt_kprintf("========================================\r\n");
    rt_kprintf("Command: ipc_test_run\r\n");
    rt_kprintf("Status:  Running\r\n");
    rt_kprintf("========================================\r\n");
    rt_kprintf("\r\n");

    rt_pin_mode(LED_PIN_B, PIN_MODE_OUTPUT);

    while (1)
    {
        rt_pin_write(LED_PIN_B, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED_PIN_B, PIN_LOW);
        rt_thread_mdelay(500);
    }

    return 0;
}
