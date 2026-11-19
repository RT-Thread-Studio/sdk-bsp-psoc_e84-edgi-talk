/* SPDX-License-Identifier: Apache-2.0 */
#include "gamepad.h"

static bool match(const struct gamepad_peer *peer)
{
    if (peer->transport == HID_TRANSPORT_BLE)
        return peer->hid_service || peer->appearance == 0x03c3 || peer->appearance == 0x03c4;
    unsigned minor = (peer->class_of_device >> 2) & 15;
    return (peer->class_of_device & 0x1f00) == 0x0500 && (minor == 1 || minor == 2);
}

static bool identify(const struct gamepad_peer *peer, const uint8_t *map, size_t length)
{
    /* The host calls identify only for established HID endpoints. Some GATT
     * devices expose input reports without returning a Report Map payload. */
    (void)peer;
    (void)map;
    (void)length;
    return true;
}

const struct gamepad_model gamepad_generic_hid = {
    .name = "Generic HID", .priority = 0, .match = match, .identify = identify
};
