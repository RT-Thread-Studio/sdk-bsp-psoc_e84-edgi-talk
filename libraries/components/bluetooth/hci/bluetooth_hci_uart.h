/* SPDX-License-Identifier: Apache-2.0 */
#ifndef BLUETOOTH_HCI_UART_H
#define BLUETOOTH_HCI_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Board-supplied byte stream for an H4 controller. open prepares the controller
 * and UART at the indicated speed. A blocking read returns 1..len bytes. A
 * successful write returns the full length. Errors are negative. */
struct bluetooth_hci_uart
{
    uint32_t baudrate;
    bool flow_control;
    int (*open)(void);
    int (*read)(uint8_t *data, size_t len);
    int (*write)(const uint8_t *data, size_t len);
};

#endif
