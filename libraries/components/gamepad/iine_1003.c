/* SPDX-License-Identifier: Apache-2.0 */
#include "gamepad.h"
#include "iine_layout.h"
#include <string.h>

/* The usage IDs are one-based. The labels were recorded by pressing each
 * physical control in isolation on the Classic HID endpoint. */
static const struct gamepad_control_name controls[] = {
    {9, 1, 1, "B"},
    {9, 2, 1, "A"},
    {9, 4, 1, "Y"},
    {9, 5, 1, "X"},
    {9, 7, 1, "L"},
    {9, 8, 1, "R"},
    {9, 9, 1, "ZL"},
    {9, 10, 1, "ZR"},
    {9, 11, 1, "-"},
    {9, 12, 1, "+"},
    {9, 13, 1, "Home"}
};

static const struct gamepad_mapping mapping = {
    .controls = controls,
    .control_count = sizeof(controls) / sizeof(controls[0]),
    .hat = iine_dpad,
    .hat_count = sizeof(iine_dpad) / sizeof(iine_dpad[0]),
    .hat_report_id = 1
};

static bool match(const struct gamepad_peer *peer)
{
    return peer->transport == HID_TRANSPORT_BR_EDR && !strcmp(peer->name, "Wireless Controller");
}

static bool identify(const struct gamepad_peer *peer, const uint8_t *descriptor, size_t length)
{
    return peer->transport == HID_TRANSPORT_BR_EDR && iine_layout_matches(descriptor, length);
}

const struct gamepad_model gamepad_iine_1003 = {
    .name = "IINE-1003", .priority = 100,
    .match = match, .identify = identify, .mapping = &mapping
};
