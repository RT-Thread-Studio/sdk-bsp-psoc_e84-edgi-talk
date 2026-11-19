/* SPDX-License-Identifier: Apache-2.0 */
#include "dual_ble_hid.h"

#include <rtthread.h>
#include <string.h>
#include "btstack_config.h"
#include "bluetooth_gatt.h"
#include "btstack_event.h"
#include "btstack_hid.h"
#include "btstack_run_loop.h"
#include "gap.h"
#include "hci.h"
#include "ble/gatt_client.h"
#include "ble/le_device_db.h"
#include "ble/sm.h"

#define MAX_INPUT_REPORTS 16
#define MAX_REPORT_MAP    512 /* Maximum ATT attribute value length. */

enum phase { IDLE, CONNECTING, SECURITY, DEVICE_INFO, PNP_ID, PEER_NAME, SERVICE, CHARACTERISTICS, PROTOCOL,
             REPORT_MAP, DESCRIPTORS, REFERENCE, SUBSCRIBE, EXIT_SUSPEND, READY, CLOSING };
struct input_report {
    gatt_client_characteristic_t characteristic;
    uint16_t reference, ccc;
    uint8_t id;
    bool input, subscribed;
};

static struct {
    enum phase phase;
    struct classic_hid_candidate peer;
    hci_con_handle_t handle;
    gatt_client_service_t service;
    struct input_report reports[MAX_INPUT_REPORTS];
    unsigned count, current, subscribed;
    gatt_client_characteristic_t control, protocol;
    uint16_t map_handle, map_length;
    uint8_t map[MAX_REPORT_MAP];
    int failure;
    bool listening, retried_pairing, fresh_pair_pending, forget_on_close;
    bd_addr_type_t forget_type;
    bd_addr_t forget_id;
} session = { .handle = HCI_CON_HANDLE_INVALID };
static struct classic_le_hid_callbacks app;
static btstack_packet_callback_registration_t hci_events, sm_events;
static gatt_client_notification_t listener;
static btstack_timer_source_t timer, fresh_pair_timer;
/* A resolved private address can differ from the address used at discovery.
 * Retain the bond identity of the current BLE preference for explicit forget. */
static struct classic_hid_candidate remembered;
static bd_addr_t remembered_id;
static bd_addr_type_t remembered_type;

static void gatt_event(uint8_t kind, uint16_t channel, uint8_t *packet, uint16_t size);
static void query_done(uint8_t att_status);

static void status(const char *stage, int detail)
{
    if (app.status) app.status(stage, detail);
}

void dual_ble_hid_reset(void)
{
    btstack_run_loop_remove_timer(&timer);
    btstack_run_loop_remove_timer(&fresh_pair_timer);
    if (session.listening)
        gatt_client_stop_listening_for_characteristic_value_updates(&listener);
    if (session.forget_on_close)
        gap_delete_bonding(session.forget_type, session.forget_id);
    memset(&session, 0, sizeof(session));
    session.handle = HCI_CON_HANDLE_INVALID;
}

static void closed(int reason)
{
    if (session.failure) reason = session.failure;
    dual_ble_hid_reset();
    app.closed(reason);
}

int dual_ble_hid_disconnect(void)
{
    if (session.phase == IDLE) return -RT_ENOENT;
    if (session.phase == CLOSING) return 0;
    enum phase previous = session.phase;
    session.phase = CLOSING;
    uint8_t rc = session.handle == HCI_CON_HANDLE_INVALID ? gap_connect_cancel() : gap_disconnect(session.handle);
    if (rc && rc != ERROR_CODE_COMMAND_DISALLOWED)
    {
        session.phase = previous;
        status("BLE disconnect failed", rc);
        return -RT_ERROR;
    }
    return 0;
}

static void fail(const char *stage, int rc)
{
    if (!rc || session.phase == CLOSING || session.phase == IDLE) return;
    if (!session.failure) session.failure = rc;
    status(stage, rc);
    dual_ble_hid_disconnect();
}

static void timeout(btstack_timer_source_t *unused)
{
    (void)unused;
    fail("BLE setup timed out", ERROR_CODE_CONNECTION_TIMEOUT);
}

static void arm_timeout(unsigned ms)
{
    btstack_run_loop_remove_timer(&timer);
    btstack_run_loop_set_timer(&timer, ms);
    btstack_run_loop_add_timer(&timer);
}

int dual_ble_hid_connect(const struct classic_hid_candidate *peer, unsigned timeout_ms)
{
    if (!peer || !peer->ble || peer->address_type > BD_ADDR_TYPE_LE_RANDOM_IDENTITY) return -RT_EINVAL;
    if (session.phase != IDLE) return -RT_EBUSY;
    session.peer = *peer;
    session.peer.pnp = (struct hid_pnp_id){0};
    session.phase = CONNECTING;
    arm_timeout(timeout_ms);
    uint8_t rc = gap_connect(peer->address, (bd_addr_type_t)peer->address_type);
    if (rc)
    {
        dual_ble_hid_reset();
        status("BLE connect failed", rc);
        return -RT_ERROR;
    }
    return 0;
}

int dual_ble_hid_pair(void)
{
    if (session.phase == CLOSING || session.handle == HCI_CON_HANDLE_INVALID) return -RT_ENOENT;
    if (!gap_encryption_key_size(session.handle))
        sm_request_pairing(session.handle);
    return 0;
}

static bool same_peer(const struct classic_hid_candidate *a, const struct classic_hid_candidate *b)
{
    return a->ble == b->ble && a->address_type == b->address_type && !memcmp(a->address, b->address, 6);
}

void dual_ble_hid_forget(const struct classic_hid_candidate *peer)
{
    if (!peer || !peer->ble) return;
    bd_addr_t address;
    bd_addr_type_t type = (bd_addr_type_t)(peer->address_type & 1);
    memcpy(address, peer->address, sizeof(address));
    if (remembered.ble && same_peer(peer, &remembered))
    {
        memcpy(address, remembered_id, sizeof(address));
        type = remembered_type;
        remembered.ble = false;
    }
    if (session.phase != IDLE && same_peer(peer, &session.peer))
    {
        /* Pairing may finish between the forget request and the ACL teardown. */
        session.forget_on_close = true;
        session.forget_type = type;
        memcpy(session.forget_id, address, sizeof(address));
    }
    gap_delete_bonding(type, address);
}

static void remember_identity(void)
{
    remembered = session.peer;
    remembered_type = (bd_addr_type_t)(session.peer.address_type & 1);
    memcpy(remembered_id, session.peer.address, sizeof(remembered_id));
    int index = sm_le_device_index(session.handle);
    if (index >= 0)
    {
        int type;
        le_device_db_info(index, &type, remembered_id, NULL);
        remembered_type = (bd_addr_type_t)type;
    }
}

static bool reusable_bond(const struct hid_le_bond *b)
{
    static const uint8_t zero[16];
    static const uint8_t invalid_address[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    if (b->identity_type != BD_ADDR_TYPE_LE_PUBLIC &&
        !(b->identity_type == BD_ADDR_TYPE_LE_RANDOM && (b->identity[0] & 0xc0) == 0xc0)) return false;
    if (b->flags & ~(HID_LE_BOND_AUTHENTICATED | HID_LE_BOND_AUTHORIZED | HID_LE_BOND_SC)) return false;
    if ((b->flags & HID_LE_BOND_SC) && (b->ediv || memcmp(b->rand, zero, sizeof(b->rand)))) return false;
    return memcmp(b->identity, zero, sizeof(b->identity)) &&
           memcmp(b->identity, invalid_address, sizeof(b->identity)) && memcmp(b->ltk, zero, sizeof(b->ltk)) &&
           b->key_size >= 7 && b->key_size <= 16;
}

int dual_ble_hid_export_bond(struct hid_le_bond *out)
{
    if (!out) return -RT_EINVAL;
    memset(out, 0, sizeof(*out));
    if (session.phase != READY || session.forget_on_close || !gap_encryption_key_size(session.handle))
        return -RT_ENOENT;
    int index = sm_le_device_index(session.handle);
    if (index < 0 || index >= le_device_db_max_count()) return -RT_ENOENT;
    int type, key_size, authenticated, authorized, sc;
    le_device_db_info(index, &type, out->identity, out->irk);
    le_device_db_encryption_get(index, &out->ediv, out->rand, out->ltk, &key_size, &authenticated, &authorized, &sc);
    out->identity_type = (uint8_t)type;
    out->key_size = (uint8_t)key_size;
    out->flags = (authenticated ? HID_LE_BOND_AUTHENTICATED : 0) |
                 (authorized ? HID_LE_BOND_AUTHORIZED : 0) | (sc ? HID_LE_BOND_SC : 0);
    if (reusable_bond(out)) return 0;
    memset(out, 0, sizeof(*out));
    return -RT_ENOENT;
}

int dual_ble_hid_restore_bond(const struct hid_le_bond *bond)
{
    if (!bond || !reusable_bond(bond)) return -RT_EINVAL;
    if (session.phase != IDLE) return -RT_EBUSY;
    /* The pinned DB uses mutable array parameters even for input. Keep the
     * caller's (possibly read-only) storage record independent of the DB. */
    struct hid_le_bond b = *bond;
    for (int i = 0; i < le_device_db_max_count(); ++i)
    {
        int type;
        bd_addr_t address;
        le_device_db_info(i, &type, address, NULL);
        if (type != b.identity_type || memcmp(address, b.identity, sizeof(address))) continue;
        /* A live pairing may have replaced this identity's keys. Preserve
         * it when a caller retries an import after a controller restart. */
        sm_key_t current;
        static const uint8_t zero[16];
        int size;
        le_device_db_encryption_get(i, NULL, NULL, current, &size, NULL, NULL, NULL);
        if (size >= 7 && size <= 16 && memcmp(current, zero, sizeof(current))) return 0;
        le_device_db_remove(i); /* Same identity, no usable encryption entry. */
        break;
    }
    int index = le_device_db_add(b.identity_type, b.identity, b.irk);
    if (index < 0) return -RT_EFULL;
    le_device_db_encryption_set(index, b.ediv, b.rand, b.ltk, b.key_size,
            !!(b.flags & HID_LE_BOND_AUTHENTICATED), !!(b.flags & HID_LE_BOND_AUTHORIZED), !!(b.flags & HID_LE_BOND_SC));
    return 0;
}

static void notify(uint8_t kind, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel;
    if (kind != HCI_EVENT_PACKET || (session.phase != READY && session.phase != EXIT_SUSPEND) || size < 12 ||
        hci_event_packet_get_type(packet) != GATT_EVENT_NOTIFICATION ||
        gatt_event_notification_get_handle(packet) != session.handle) return;
    uint16_t handle = gatt_event_notification_get_value_handle(packet);
    uint16_t length = gatt_event_notification_get_value_length(packet);
    if (length > size - 12u) return;
    for (unsigned i = 0; i < session.count; ++i)
    {
        struct input_report *r = &session.reports[i];
        if (r->subscribed && r->characteristic.value_handle == handle)
        {
            app.input(r->id, gatt_event_notification_get_value(packet), length);
            break;
        }
    }
}

static const gatt_client_characteristic_t *control_characteristic(void)
{
    return session.phase == PROTOCOL ? &session.protocol : &session.control;
}

static uint8_t control_value = 1; /* Report protocol / exit suspend. */

static void write_control_command(uint8_t kind, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel;
    if (kind != HCI_EVENT_PACKET || size < 4 ||
        hci_event_packet_get_type(packet) != GATT_EVENT_CAN_WRITE_WITHOUT_RESPONSE ||
        little_endian_read_16(packet, 2) != session.handle ||
        (session.phase != PROTOCOL && session.phase != EXIT_SUSPEND))
        return;
    const gatt_client_characteristic_t *c = control_characteristic();
    uint8_t rc = gatt_client_write_value_of_characteristic_without_response(session.handle, c->value_handle,
                                                                            1, &control_value);
    query_done(rc);
}

static void write_control(enum phase phase)
{
    session.phase = phase;
    const gatt_client_characteristic_t *c = control_characteristic();
    uint8_t rc = 0;
    if (c->properties & ATT_PROPERTY_WRITE)
        rc = gatt_client_write_value_of_characteristic(gatt_event, session.handle, c->value_handle, 1, &control_value);
    else if (c->properties & ATT_PROPERTY_WRITE_WITHOUT_RESPONSE)
        rc = gatt_client_request_can_write_without_response_event(write_control_command, session.handle);
    else
    {
        query_done(0);
        return;
    }
    if (rc) query_done(rc);
}

static void next_input(void)
{
    while (session.current < session.count)
    {
        gatt_client_characteristic_t *c = &session.reports[session.current].characteristic;
        if (c->value_handle < c->end_handle)
        {
            session.phase = DESCRIPTORS;
            fail("BLE report descriptors", gatt_client_discover_characteristic_descriptors(gatt_event, session.handle, c));
            return;
        }
        ++session.current;
    }
    if (!session.subscribed)
    {
        fail("BLE no subscribed HID inputs", ERROR_CODE_UNSUPPORTED_FEATURE_OR_PARAMETER_VALUE);
        return;
    }
    session.peer.hid_service = true;
    if (app.descriptor && !app.descriptor(&session.peer, session.map, session.map_length))
    {
        fail("BLE unsupported HID layout", ERROR_CODE_UNSUPPORTED_FEATURE_OR_PARAMETER_VALUE);
        return;
    }
    gatt_client_listen_for_characteristic_value_updates(&listener, notify, session.handle, NULL);
    session.listening = true;
    write_control(EXIT_SUSPEND);
}

static void characteristic(const uint8_t *packet)
{
    gatt_client_characteristic_t c;
    gatt_event_characteristic_query_result_get_characteristic(packet, &c);
    switch (c.uuid16)
    {
    case ORG_BLUETOOTH_CHARACTERISTIC_REPORT_MAP:
        session.map_handle = c.value_handle;
        break;
    case ORG_BLUETOOTH_CHARACTERISTIC_HID_CONTROL_POINT:
        session.control = c;
        break;
    case ORG_BLUETOOTH_CHARACTERISTIC_PROTOCOL_MODE:
        session.protocol = c;
        break;
    case ORG_BLUETOOTH_CHARACTERISTIC_REPORT:
        if (!(c.properties & ATT_PROPERTY_NOTIFY)) break;
        if (session.count == MAX_INPUT_REPORTS)
            fail("BLE too many HID inputs", ERROR_CODE_MEMORY_CAPACITY_EXCEEDED);
        else
            session.reports[session.count++].characteristic = c;
        break;
    }
}

static void discover_hid(void)
{
    memset(&session.service, 0, sizeof(session.service));
    session.phase = SERVICE;
    fail("BLE HID discovery", gatt_client_discover_primary_services_by_uuid16(gatt_event, session.handle,
                                                    ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE));
}

#ifdef HID_HOST_USING_PNP_ID
static void read_name_or_discover(void)
{
    if (!session.peer.name[0])
    {
        session.phase = PEER_NAME;
        if (!gatt_client_read_value_of_characteristics_by_uuid16(gatt_event, session.handle, 1, 0xffff,
                                                                ORG_BLUETOOTH_CHARACTERISTIC_GAP_DEVICE_NAME)) return;
    }
    discover_hid();
}

static void read_identity_value(const uint8_t *packet, uint16_t size)
{
    if (size < 12) return;
    uint16_t n = gatt_event_characteristic_value_query_result_get_value_length(packet);
    if (n > size - 12u) return;
    const uint8_t *value = gatt_event_characteristic_value_query_result_get_value(packet);
    if (session.phase == PNP_ID)
        hid_pnp_id_decode(&session.peer.pnp, value, n);
    else if (!session.peer.name[0])
    {
        if (n >= sizeof(session.peer.name)) n = sizeof(session.peer.name) - 1;
        for (unsigned i = 0; i < n; ++i)
            session.peer.name[i] = value[i] < 32 || value[i] == 127 ? '?' : (char)value[i];
        session.peer.name[n] = '\0';
    }
}
#endif

static void query_done(uint8_t att_status)
{
    static uint8_t enable[2] = {1, 0}; /* BTstack retains write data until completion. */
    struct input_report *r;
    uint8_t rc;
    switch (session.phase)
    {
#ifdef HID_HOST_USING_PNP_ID
    case DEVICE_INFO:
        if (!att_status && session.service.start_group_handle)
        {
            session.phase = PNP_ID;
            rc = gatt_client_read_value_of_characteristics_by_uuid16(gatt_event, session.handle,
                    session.service.start_group_handle, session.service.end_group_handle, ORG_BLUETOOTH_CHARACTERISTIC_PNP_ID);
            if (!rc) return;
        }
        read_name_or_discover();
        return;
    case PNP_ID:
        read_name_or_discover();
        return;
    case PEER_NAME:
        discover_hid();
        return;
#endif
    case SERVICE:
        if (att_status || !session.service.start_group_handle)
        {
            fail("BLE HID service", att_status ? att_status : ATT_ERROR_ATTRIBUTE_NOT_FOUND);
            return;
        }
        session.phase = CHARACTERISTICS;
        fail("BLE HID characteristics", gatt_client_discover_characteristics_for_service(gatt_event, session.handle, &session.service));
        return;
    case CHARACTERISTICS:
        if (att_status) { fail("BLE HID characteristics", att_status); return; }
        write_control(PROTOCOL);
        return;
    case PROTOCOL:
        if (att_status) status("BLE report protocol write failed", att_status);
        if (session.map_handle)
        {
            session.phase = REPORT_MAP;
            rc = gatt_client_read_long_value_of_characteristic_using_value_handle(gatt_event, session.handle, session.map_handle);
            if (!rc) return;
            status("BLE report map", rc);
        }
        break;
    case REPORT_MAP:
        if (att_status && !(att_status == ATT_ERROR_INVALID_OFFSET && session.map_length))
        {
            status("BLE report map", att_status);
            session.map_length = 0; /* Generic HID can receive raw reports. */
        }
        break;
    case DESCRIPTORS:
        r = &session.reports[session.current];
        if (!att_status && r->reference && r->ccc)
        {
            session.phase = REFERENCE;
            /* Both GATT APIs issue ATT Read. In BTstack 1.6.2 the descriptor
             * helper underreports its event size by four bytes. The regular
             * handle read returns the complete Report Reference value. */
            rc = gatt_client_read_value_of_characteristic_using_value_handle(gatt_event, session.handle, r->reference);
            if (!rc) return;
            status("BLE read Report Reference", rc);
        }
        else if (att_status) status("BLE report descriptors", att_status);
        ++session.current;
        break;
    case REFERENCE:
        r = &session.reports[session.current];
        if (att_status) status("BLE Report Reference", att_status);
        if (!att_status && r->input)
        {
            session.phase = SUBSCRIBE;
            rc = gatt_client_write_characteristic_descriptor_using_descriptor_handle(gatt_event, session.handle, r->ccc, sizeof(enable), enable);
            if (!rc) return;
            status("BLE subscribe input", rc);
        }
        ++session.current;
        break;
    case SUBSCRIBE:
        if (!att_status)
        {
            session.reports[session.current].subscribed = true;
            ++session.subscribed;
        }
        else status("BLE subscribe input", att_status);
        ++session.current;
        break;
    case EXIT_SUSPEND:
        if (att_status) status("BLE exit suspend write failed", att_status);
        btstack_run_loop_remove_timer(&timer);
        session.phase = READY;
        remember_identity();
        status("BLE HID ready, subscribed inputs", session.subscribed);
        app.ready();
        return;
    default:
        return;
    }
    next_input();
}

static void gatt_event(uint8_t kind, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel;
    if (kind != HCI_EVENT_PACKET || size < 4 || session.phase <= SECURITY || session.phase >= READY ||
        little_endian_read_16(packet, 2) != session.handle) return;
    switch (hci_event_packet_get_type(packet))
    {
    case GATT_EVENT_SERVICE_QUERY_RESULT:
        if (session.phase == SERVICE || session.phase == DEVICE_INFO)
        {
            gatt_client_service_t service;
            gatt_event_service_query_result_get_service(packet, &service);
            if (!session.service.start_group_handle) session.service = service;
        }
        break;
    case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
        if (session.phase == CHARACTERISTICS) characteristic(packet);
        break;
    case GATT_EVENT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT:
        if (session.phase == REPORT_MAP && size >= 14 &&
            gatt_event_long_characteristic_value_query_result_get_value_handle(packet) == session.map_handle)
        {
            uint16_t n = gatt_event_long_characteristic_value_query_result_get_value_length(packet);
            uint16_t offset = gatt_event_long_characteristic_value_query_result_get_value_offset(packet);
            if (offset != session.map_length || n > sizeof(session.map) - session.map_length ||
                n > size - 14u)
                fail("BLE invalid HID map", ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH);
            else
            {
                memcpy(session.map + offset, gatt_event_long_characteristic_value_query_result_get_value(packet), n);
                session.map_length += n;
            }
        }
        break;
    case GATT_EVENT_ALL_CHARACTERISTIC_DESCRIPTORS_QUERY_RESULT:
        if (session.phase == DESCRIPTORS)
        {
            gatt_client_characteristic_descriptor_t desc;
            gatt_event_all_characteristic_descriptors_query_result_get_characteristic_descriptor(packet, &desc);
            struct input_report *r = &session.reports[session.current];
            if (desc.uuid16 == ORG_BLUETOOTH_DESCRIPTOR_REPORT_REFERENCE) r->reference = desc.handle;
            if (desc.uuid16 == ORG_BLUETOOTH_DESCRIPTOR_GATT_CLIENT_CHARACTERISTIC_CONFIGURATION) r->ccc = desc.handle;
        }
        break;
    case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT:
#ifdef HID_HOST_USING_PNP_ID
        if (session.phase == PNP_ID || session.phase == PEER_NAME)
        {
            read_identity_value(packet, size);
            break;
        }
#endif
        if (session.phase == REFERENCE && size >= 14 &&
            gatt_event_characteristic_value_query_result_get_value_handle(packet) == session.reports[session.current].reference &&
            gatt_event_characteristic_value_query_result_get_value_length(packet) == 2)
        {
            const uint8_t *ref = gatt_event_characteristic_value_query_result_get_value(packet);
            session.reports[session.current].id = ref[0];
            session.reports[session.current].input = ref[1] == HID_REPORT_TYPE_INPUT;
        }
        break;
    case GATT_EVENT_QUERY_COMPLETE:
        query_done(gatt_event_query_complete_get_att_status(packet));
        break;
    }
}

static void retry_pairing(btstack_timer_source_t *unused)
{
    (void)unused;
    if (session.phase != SECURITY || !session.fresh_pair_pending) return;
    session.fresh_pair_pending = false;
    sm_request_pairing(session.handle);
}

static void security_event(uint8_t kind, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel;
    (void)size;
    if (kind != HCI_EVENT_PACKET || session.phase != SECURITY) return;
    hci_con_handle_t handle;
    uint8_t rc;
    switch (hci_event_packet_get_type(packet))
    {
    case SM_EVENT_JUST_WORKS_REQUEST:
        handle = sm_event_just_works_request_get_handle(packet);
        if (handle == session.handle) sm_just_works_confirm(handle);
        return;
    case SM_EVENT_PAIRING_COMPLETE:
        if (session.fresh_pair_pending) return;
        handle = sm_event_pairing_complete_get_handle(packet);
        rc = sm_event_pairing_complete_get_status(packet);
        break;
    case SM_EVENT_REENCRYPTION_COMPLETE:
        handle = sm_event_reencryption_complete_get_handle(packet);
        rc = sm_event_reencryption_complete_get_status(packet);
        if (handle == session.handle && !session.retried_pairing &&
            (rc == ERROR_CODE_PIN_OR_KEY_MISSING || rc == ERROR_CODE_AUTHENTICATION_FAILURE))
        {
            bd_addr_t address;
            sm_event_reencryption_complete_get_address(packet, address);
            gap_delete_bonding((bd_addr_type_t)(sm_event_reencryption_complete_get_addr_type(packet) & 1), address);
            session.retried_pairing = true;
            /* The current BTstack SM dispatch can also emit the old pairing
             * completion before releasing its setup context. Retry afterwards. */
            session.fresh_pair_pending = true;
            status("BLE stored key rejected, pairing again", rc);
            btstack_run_loop_set_timer(&fresh_pair_timer, 1);
            btstack_run_loop_add_timer(&fresh_pair_timer);
            return;
        }
        break;
    default:
        return;
    }
    if (handle != session.handle) return;
    uint8_t key_size = gap_encryption_key_size(handle);
    if (rc || !key_size)
    {
        fail("BLE encryption", rc ? rc : ERROR_CODE_AUTHENTICATION_FAILURE);
        return;
    }
    status(hci_event_packet_get_type(packet) == SM_EVENT_PAIRING_COMPLETE ?
           "BLE paired and encrypted, key bytes" : "BLE bond re-encrypted, key bytes", key_size);
    arm_timeout(60000);
#ifdef HID_HOST_USING_PNP_ID
    session.phase = DEVICE_INFO;
    if (!gatt_client_discover_primary_services_by_uuid16(gatt_event, handle, ORG_BLUETOOTH_SERVICE_DEVICE_INFORMATION))
        return;
    read_name_or_discover();
#else
    discover_hid();
#endif
}

static bool connected_to_target(const uint8_t *packet)
{
    bd_addr_t address;
    uint8_t type = gap_subevent_le_connection_complete_get_peer_address_type(packet);
    gap_subevent_le_connection_complete_get_peer_address(packet, address);
    if (type == session.peer.address_type && !memcmp(address, session.peer.address, sizeof(address)))
        return true;
    if (type < BD_ADDR_TYPE_LE_PUBLIC_IDENTITY || session.peer.address_type != BD_ADDR_TYPE_LE_RANDOM)
        return false;
    gap_subevent_le_connection_complete_get_peer_resolvable_private_address(packet, address);
    return !memcmp(address, session.peer.address, sizeof(address));
}

static void hci_event(uint8_t kind, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel;
    (void)size;
    if (kind != HCI_EVENT_PACKET || session.phase == IDLE) return;
    switch (hci_event_packet_get_type(packet))
    {
    case HCI_EVENT_META_GAP:
        if (hci_event_gap_meta_get_subevent_code(packet) != GAP_SUBEVENT_LE_CONNECTION_COMPLETE ||
            session.handle != HCI_CON_HANDLE_INVALID) return;
        /* BTstack allows one pending outgoing LE request. On error the
         * controller may omit both address and handle, including on cancel. */
        if (gap_subevent_le_connection_complete_get_status(packet))
        {
            closed(gap_subevent_le_connection_complete_get_status(packet));
            return;
        }
        if (gap_subevent_le_connection_complete_get_role(packet) != HCI_ROLE_MASTER) return;
        session.handle = gap_subevent_le_connection_complete_get_connection_handle(packet);
        if (session.phase == CLOSING || !connected_to_target(packet))
        {
            session.phase = SECURITY; /* Cancel raced with connection complete. */
            fail("BLE cancelled or unexpected peer", ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION);
            return;
        }
        session.phase = SECURITY;
        arm_timeout(40000);
        app.link();
        if (session.phase == SECURITY) sm_request_pairing(session.handle);
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        if (session.handle != HCI_CON_HANDLE_INVALID &&
            hci_event_disconnection_complete_get_connection_handle(packet) == session.handle)
            closed(hci_event_disconnection_complete_get_reason(packet));
        break;
    case HCI_EVENT_ENCRYPTION_CHANGE:
        if (session.handle != HCI_CON_HANDLE_INVALID && session.phase > SECURITY &&
            hci_event_encryption_change_get_connection_handle(packet) == session.handle &&
            !hci_event_encryption_change_get_encryption_enabled(packet))
            fail("BLE encryption lost", ERROR_CODE_AUTHENTICATION_FAILURE);
        break;
    }
}

void dual_ble_hid_init(const struct classic_le_hid_callbacks *callbacks)
{
    app = *callbacks;
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_BONDING | SM_AUTHREQ_SECURE_CONNECTION);
    gatt_client_init();
    hci_events.callback = hci_event;
    sm_events.callback = security_event;
    hci_add_event_handler(&hci_events);
    sm_add_event_handler(&sm_events);
    btstack_run_loop_set_timer_handler(&timer, timeout);
    btstack_run_loop_set_timer_handler(&fresh_pair_timer, retry_pairing);
}
