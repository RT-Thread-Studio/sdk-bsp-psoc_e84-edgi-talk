/* SPDX-License-Identifier: Apache-2.0 */
#ifndef DUAL_BLE_HID_H
#define DUAL_BLE_HID_H

#include "classic_hid_host.h"
#include "hid_le_bond.h"

/* All calls and callbacks run on the BTstack thread. A successful connect
 * owns one typed BLE peer until closed, including while cancellation is pending. */
struct classic_le_hid_callbacks
{
    void (*link)(void);
    bool (*descriptor)(const struct classic_hid_candidate *peer, const uint8_t *map, size_t length);
    void (*input)(uint8_t report_id, const uint8_t *data, size_t length);
    void (*closed)(int reason);
    void (*status)(const char *stage, int detail);
    void (*ready)(void);
};

void dual_ble_hid_init(const struct classic_le_hid_callbacks *callbacks);
int dual_ble_hid_connect(const struct classic_hid_candidate *peer, unsigned timeout_ms);
int dual_ble_hid_pair(void);
int dual_ble_hid_disconnect(void);
void dual_ble_hid_forget(const struct classic_hid_candidate *peer);
void dual_ble_hid_reset(void); /* Controller shutdown. No closed callback. */
/* Export only the current encrypted HID-ready peer. Import on the BTstack
 * thread after sm_init and verification of the local controller address. */
int dual_ble_hid_export_bond(struct hid_le_bond *out);
int dual_ble_hid_restore_bond(const struct hid_le_bond *bond);

#endif
