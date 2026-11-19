/* SPDX-License-Identifier: Apache-2.0 */
#ifndef HID_PNP_ID_H
#define HID_PNP_ID_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Device Information Service, PnP ID (0x2a50). A zero source is unknown. */
struct hid_pnp_id
{
    uint8_t source;
    uint16_t vendor, product, version;
};

static inline bool hid_pnp_id_decode(struct hid_pnp_id *id, const uint8_t *data, size_t length)
{
    if (!id || !data || length != 7 || (data[0] != 1 && data[0] != 2)) return false;
    id->source = data[0];
    id->vendor = (uint16_t)(data[1] | (data[2] << 8));
    id->product = (uint16_t)(data[3] | (data[4] << 8));
    id->version = (uint16_t)(data[5] | (data[6] << 8));
    return true;
}

#endif
