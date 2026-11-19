/* SPDX-License-Identifier: Apache-2.0 */
#ifndef RTTHREAD_BTSTACK_H
#define RTTHREAD_BTSTACK_H

#include "bluetooth_hci_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Copies the IO operations. Runs init_profile once on the host thread
 * after HCI initialization and before powering on the Bluetooth stack. */
int rtthread_btstack_start(const struct bluetooth_hci_uart *io, void (*init_profile)(void));

#ifdef __cplusplus
}
#endif
#endif
