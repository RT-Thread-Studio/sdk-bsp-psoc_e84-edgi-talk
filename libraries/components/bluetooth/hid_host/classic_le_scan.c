/* SPDX-License-Identifier: Apache-2.0 */
#include "classic_le_scan.h"
#include "ad_parser.h"
#include "bluetooth_data_types.h"
#include "bluetooth_gatt.h"
#include "btstack_event.h"

bool classic_le_advertisement(const uint8_t *packet, uint16_t size,
                              struct classic_hid_candidate *out)
{
    /* BTstack normalizes each legacy report to a 12-byte header plus AD. */
    if (!packet || !out || size < 12 || hci_event_packet_get_type(packet) != GAP_EVENT_ADVERTISING_REPORT ||
        packet[1] + 2u != size)
        return false;
    uint8_t length = gap_event_advertising_report_get_data_length(packet);
    uint8_t type = gap_event_advertising_report_get_address_type(packet);
    if (length > LE_ADVERTISING_DATA_SIZE || 12u + length != size || type > BD_ADDR_TYPE_LE_RANDOM_IDENTITY)
        return false;

    const uint8_t *ad = gap_event_advertising_report_get_data(packet);
    *out = (struct classic_hid_candidate){
        .ble = true, .address_type = type, .rssi = (int8_t)gap_event_advertising_report_get_rssi(packet),
        .hid_service = ad_data_contains_uuid16(length, ad, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE)
    };
    gap_event_advertising_report_get_address(packet, out->address);
    ad_context_t it;
    for (ad_iterator_init(&it, length, ad); ad_iterator_has_more(&it); ad_iterator_next(&it))
    {
        uint8_t field = ad_iterator_get_data_type(&it);
        const uint8_t *data = ad_iterator_get_data(&it);
        size_t len = ad_iterator_get_data_len(&it);
        if (field == BLUETOOTH_DATA_TYPE_APPEARANCE && len == 2)
            out->appearance = little_endian_read_16(data, 0);
        bool complete = field == BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME;
        if (len && (complete || (field == BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME && !out->name_complete)))
        {
            if (len >= sizeof(out->name)) len = sizeof(out->name) - 1;
            for (size_t i = 0; i < len; ++i)
                out->name[i] = data[i] < 32 || data[i] == 127 ? '?' : (char)data[i];
            out->name[len] = '\0';
            out->name_complete = complete;
        }
    }
    return true;
}
