/* SPDX-License-Identifier: Apache-2.0 */
#include "gamepad.h"
#include "iine_layout.h"
#include <string.h>

/* Each key was pressed and released separately in C / Gamepad mode. */
static const struct gamepad_control_name controls[] = {
    {9, 1, 1, "A"},
    {9, 2, 1, "B"}
};

static const struct gamepad_mapping mapping = {
    .controls = controls, .control_count = sizeof(controls) / sizeof(controls[0]),
    .hat = iine_dpad, .hat_count = sizeof(iine_dpad) / sizeof(iine_dpad[0]),
    .hat_report_id = 1
};

static bool match(const struct gamepad_peer *peer)
{
    return peer->transport == HID_TRANSPORT_BLE && !strcmp(peer->name, "Wireless Controller");
}

static bool identify(const struct gamepad_peer *peer, const uint8_t *map, size_t length)
{
    /* USB-assigned VID/PID reported by the C-mode BLE Device Information
     * Service. The Classic 1003 shares this layout but reverses A and B. */
    return match(peer) && peer->pnp.source == 2 && peer->pnp.vendor == 0x4353 &&
           peer->pnp.product == 0x9b09 && iine_layout_matches(map, length);
}

const struct gamepad_model gamepad_iine_1001 = {
    .name = "IINE-1001", .priority = 100,
    .match = match, .identify = identify, .mapping = &mapping
};
