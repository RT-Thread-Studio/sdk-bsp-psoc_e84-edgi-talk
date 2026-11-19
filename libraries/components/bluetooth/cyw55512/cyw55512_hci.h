/* SPDX-License-Identifier: Apache-2.0 */
#ifndef CYW55512_HCI_H
#define CYW55512_HCI_H

#include "bluetooth_hci_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const struct bluetooth_hci_uart cyw55512_hci_uart;

/* Called once before starting the H4 host. */
int cyw55512_hci_open(void);
/* Single RX consumer. Blocks until at least one byte is available. */
int cyw55512_hci_read(uint8_t *buf, size_t len);
/* Single serialized TX producer. Returns the full byte count or a negative error. */
int cyw55512_hci_write(const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif
#endif
