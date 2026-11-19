/* SPDX-License-Identifier: Apache-2.0 */
#ifndef CLASSIC_LE_SCAN_H
#define CLASSIC_LE_SCAN_H

#include "classic_hid_host.h"

/* Decode one BTstack GAP advertising or scan response event. The caller
 * merges the available fields by BLE address and address type. */
bool classic_le_advertisement(const uint8_t *packet, uint16_t size,
                              struct classic_hid_candidate *out);

#endif
