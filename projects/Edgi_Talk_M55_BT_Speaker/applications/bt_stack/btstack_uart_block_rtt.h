#ifndef __BTSTACK_UART_BLOCK_RTT_H
#define __BTSTACK_UART_BLOCK_RTT_H

#include "btstack_uart_block.h"

#ifdef __cplusplus
extern "C" {
#endif

const btstack_uart_block_t *btstack_uart_block_rtt_instance(void);
void btstack_uart_block_rtt_poll(void);
void btstack_uart_block_rtt_flush(void);
uint32_t btstack_uart_block_rtt_get_baudrate(void);

#ifdef __cplusplus
}
#endif

#endif /* __BTSTACK_UART_BLOCK_RTT_H */
