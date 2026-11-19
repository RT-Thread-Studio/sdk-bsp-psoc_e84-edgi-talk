/* SPDX-License-Identifier: Apache-2.0 */
#ifndef BLE_CONTROLLER_H
#define BLE_CONTROLLER_H

/* Compatibility with the original Edgi-Talk BLE controller interface. */
#include "cyw55512_hci.h"
#define ble_controller_open  cyw55512_hci_open
#define ble_controller_read  cyw55512_hci_read
#define ble_controller_write cyw55512_hci_write

#endif
