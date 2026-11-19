/* SPDX-License-Identifier: Apache-2.0 */
#ifndef EDGI_BTSTACK_HID_DESCRIPTOR_CACHE_H
#define EDGI_BTSTACK_HID_DESCRIPTOR_CACHE_H

#include <stdbool.h>
#include <stdint.h>

/* Seed an incoming BTstack connection in its successful opened callback.
 * The caller owns identity, bond and security checks; BTstack copies the map. */
bool hid_host_set_cached_report_descriptor(uint16_t cid, const uint8_t *map, uint16_t size);

#endif
