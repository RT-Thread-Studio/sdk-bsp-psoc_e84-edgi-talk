/* SPDX-License-Identifier: Apache-2.0 */
/* Included by the pinned BTstack HID host after its descriptor-storage helpers. */
#include "btstack_hid_descriptor_cache.h"

bool hid_host_set_cached_report_descriptor(uint16_t cid, const uint8_t *map, uint16_t size)
{
    hid_host_connection_t *c = hid_host_get_connection_for_hid_cid(cid);
    if (!c || !c->incoming || c->state != HID_HOST_CONNECTION_ESTABLISHED ||
        c->hid_descriptor_len || !map || !size)
        return false;
    hid_descriptor_storage_init(c);
    if (size > c->hid_descriptor_max_len)
        return false;
    memcpy(hid_host_descriptor_storage + c->hid_descriptor_offset, map, size);
    c->hid_descriptor_len = size;
    c->hid_descriptor_status = ERROR_CODE_SUCCESS;
    return true;
}
