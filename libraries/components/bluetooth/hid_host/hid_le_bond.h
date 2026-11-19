/* SPDX-License-Identifier: Apache-2.0 */
#ifndef HID_LE_BOND_H
#define HID_LE_BOND_H

#include <stdint.h>

enum {
    HID_LE_BOND_AUTHENTICATED = 1,
    HID_LE_BOND_AUTHORIZED = 2,
    HID_LE_BOND_SC = 4
};

/* One peer's LE security database entry. Encode named fields for storage.
 * key_size == 0 represents a preference without reusable encryption keys. */
struct hid_le_bond
{
    uint8_t identity_type, identity[6], key_size, flags;
    uint8_t irk[16], ltk[16], rand[8];
    uint16_t ediv;
};

#endif
