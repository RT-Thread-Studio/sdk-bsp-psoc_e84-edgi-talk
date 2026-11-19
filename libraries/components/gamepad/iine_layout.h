/* SPDX-License-Identifier: Apache-2.0 */
#ifndef IINE_HID_LAYOUT_H
#define IINE_HID_LAYOUT_H

#include "gamepad_report.h"

bool iine_layout_matches(const uint8_t *map, size_t length);
extern const struct gamepad_hat_position iine_dpad[9];

#endif
