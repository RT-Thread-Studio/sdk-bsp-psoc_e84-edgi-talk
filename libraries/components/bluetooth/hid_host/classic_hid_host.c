/* SPDX-License-Identifier: Apache-2.0 */
#include <rtthread.h>
#include <rthw.h>
#include <stdbool.h>
#include <string.h>
#include "classic_hid_host.h"
#include "classic_le_scan.h"
#include "dual_ble_hid.h"
#include "rtthread_btstack.h"
#include "classic/btstack_link_key_db_memory.h"
#include "btstack_hid_descriptor_cache.h"
#include "btstack_event.h"
#include "bluetooth_sdp.h"
#include "btstack_hid_parser.h"
#include "btstack_run_loop.h"
#include "classic/hid_host.h"
#include "classic/sdp_client.h"
#include "classic/sdp_server.h"
#include "classic/sdp_util.h"
#include "gap.h"
#include "ble/le_device_db.h"
#include "hci.h"
#include "l2cap.h"

#define CLASSIC_RECONNECT_WAIT_MS 5000u

static struct classic_hid_callbacks app;
static struct classic_hid_candidate peers[CLASSIC_HID_MAX_CANDIDATES];
static unsigned peer_count;
static btstack_packet_callback_registration_t hci_handler;
static btstack_context_callback_registration_t command;
static btstack_context_callback_registration_t start_sdp;
static btstack_timer_source_t inquiry_retry;
static btstack_timer_source_t le_scan_timer;
enum le_scan_phase { LE_SCAN_OFF, LE_SCAN_STARTING, LE_SCAN_ACTIVE,
                     LE_SCAN_CANCEL_START, LE_SCAN_STOPPING };
static enum le_scan_phase le_scan;
static uint8_t descriptor_storage[CLASSIC_HID_MAX_DESCRIPTOR];
static struct {
    bd_addr_t address;
    uint16_t length;
    uint8_t map[sizeof(descriptor_storage)];
} cached;
/* Interrupt data may precede the first SDP result on an incoming connection. */
static struct {
    uint16_t length;
    uint8_t data[128];
} early[4];
static unsigned early_count, early_dropped;
static bool layout_ready, reconnect_grace;
static bool sdp_queued, attempted_on_acl, incoming_acl, waiting_control;
static btstack_timer_source_t control_wait;
static struct {
    bool busy, live;
    hci_con_handle_t acl;
    bd_addr_t address;
    uint16_t record, filled, map_offset, map_length;
    uint8_t attribute[sizeof(descriptor_storage) + 32];
} prefetch;
static bd_addr_t selected;
static uint16_t hid_cid;
static hci_con_handle_t selected_acl = HCI_CON_HANDLE_INVALID;
static bool started, working, inquiring, manual, idle, selected_valid, pending_connect;
static bool inquiry_active, report_discoveries;
static bool discard_connection, numbered_reports, command_pending, command_cancelled;
enum user_action { MANUAL_SCAN, CONNECT_SELECTED, RECONNECT_SAVED, FORGET_SAVED,
                   DISCONNECT_PEER, PAIR_PEER, FORGET_BOND };
static enum user_action requested_action;
static struct classic_hid_candidate requested_peer, le_target, le_identity, session_preferred;
static bool le_busy, le_link, le_layout, le_input_ready;

static struct classic_hid_store storage;
static struct classic_hid_bonds bonds, staged_bonds;
static struct classic_hid_status public_status;
static bd_addr_t local_address, active_peer;
static bool link_open, retry_armed, listen_before_reconnect;
static unsigned reconnect_cursor, page_delay_ms;

static bool valid_address(const uint8_t a[6]);
static bool has_saved_key(const struct classic_hid_bond_entry *entry);

static bool has_preference(void)
{
    return bonds.count || session_preferred.ble;
}

static void bond_candidate(const struct classic_hid_bond_entry *entry, struct classic_hid_candidate *peer)
{
    *peer = (struct classic_hid_candidate){
        .ble = entry->ble, .address_type = entry->address_type, .hid_service = true, .rssi = 127
    };
    memcpy(peer->address, entry->address, sizeof(peer->address));
    memcpy(peer->name, entry->name, sizeof(peer->name));
}

static struct classic_hid_candidate preferred_peer(void)
{
    struct classic_hid_candidate peer = session_preferred;
    if (bonds.count) bond_candidate(&bonds.entries[0], &peer);
    return peer;
}

bool classic_hid_bonded_at(unsigned index, struct classic_hid_candidate *out, bool *has_key)
{
    if (!index || !out) return false;
    rt_base_t level = rt_hw_interrupt_disable();
    bool present = index <= bonds.count;
    if (present)
    {
        const struct classic_hid_bond_entry *entry = &bonds.entries[index - 1];
        bond_candidate(entry, out);
        if (has_key) *has_key = has_saved_key(entry);
    }
    rt_hw_interrupt_enable(level);
    return present;
}

static void publish_status(void)
{
    struct classic_hid_status s = {0};
    s.local_valid = working && valid_address(local_address);
    if (s.local_valid) memcpy(s.local_address, local_address, sizeof(local_address));
    if (has_preference())
    {
        struct classic_hid_candidate peer = preferred_peer();
        s.bound = true;
        s.bound_ble = peer.ble;
        s.bound_type = peer.address_type;
        s.has_link_key = bonds.count && has_saved_key(&bonds.entries[0]);
        memcpy(s.bound_address, peer.address, sizeof(s.bound_address));
    }
    s.connected = link_open || le_input_ready;
    s.peer_ble = le_input_ready;
    s.peer_type = le_input_ready ? le_target.address_type : 0;
    if (s.connected) memcpy(s.peer_address, le_input_ready ? le_target.address : active_peer, sizeof(s.peer_address));
    s.manual = manual;
    s.connecting = !s.connected && (pending_connect || hid_cid || incoming_acl || le_busy);
    s.target_valid = selected_valid;
    s.target_ble = selected_valid && le_target.ble;
    s.target_type = s.target_ble ? le_target.address_type : 0;
    if (selected_valid) memcpy(s.target_address, selected, sizeof(selected));
    rt_base_t level = rt_hw_interrupt_disable();
    public_status = s;
    rt_hw_interrupt_enable(level);
}

void classic_hid_get_status(struct classic_hid_status *out)
{
    if (!out) return;
    rt_base_t level = rt_hw_interrupt_disable();
    *out = public_status;
    rt_hw_interrupt_enable(level);
}

static void status(const char *stage, int detail)
{
    publish_status();
    if (app.status)
        app.status(stage, detail);
}

static bool valid_address(const uint8_t a[6])
{
    static const uint8_t zero[6] = {0}, ones[6] = {255,255,255,255,255,255};
    return memcmp(a, zero, 6) && memcmp(a, ones, 6);
}

static bool stable_le_address(uint8_t type, const uint8_t address[6])
{
    /* A random static address has its two most significant bits set. An
     * old private address alone is insufficient to select the same device. */
    return valid_address(address) && (type == 0 || (type == 1 && (address[0] & 0xc0) == 0xc0));
}

static bool same_peer(const struct classic_hid_candidate *a, const struct classic_hid_candidate *b)
{
    return a->ble == b->ble && (a->address_type & 1) == (b->address_type & 1) &&
           !memcmp(a->address, b->address, sizeof(a->address));
}

static bool reusable_key_type(link_key_type_t type)
{
    return type <= AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P256 &&
           type != DEBUG_COMBINATION_KEY && type != CHANGED_COMBINATION_KEY;
}

static bool has_saved_key(const struct classic_hid_bond_entry *entry)
{
    return entry->ble ? entry->keys.le.key_size != 0 :
           reusable_key_type((link_key_type_t)entry->keys.classic.type) &&
           !btstack_is_null(entry->keys.classic.key, sizeof(entry->keys.classic.key));
}

static int find_bond(const struct classic_hid_candidate *peer)
{
    for (unsigned i = 0; i < bonds.count; ++i)
    {
        const struct classic_hid_bond_entry *entry = &bonds.entries[i];
        if (entry->ble != peer->ble) continue;
        uint8_t type = peer->address_type & 1;
        if ((entry->address_type == type && !memcmp(entry->address, peer->address, 6)) ||
            (entry->ble && entry->keys.le.key_size && entry->keys.le.identity_type == type &&
             !memcmp(entry->keys.le.identity, peer->address, 6)))
            return (int)i;
    }
    return -1;
}

static struct classic_hid_candidate current_peer(void)
{
    if (le_target.ble) return le_identity.ble ? le_identity : le_target;
    struct classic_hid_candidate peer = le_target;
    memcpy(peer.address, selected, sizeof(peer.address));
    return peer;
}

static void select_target(const struct classic_hid_candidate *peer)
{
    le_target = *peer;
    memcpy(selected, le_target.address, sizeof(selected));
    selected_valid = true;
}

static void select_saved(void)
{
    struct classic_hid_candidate peer = preferred_peer();
    select_target(&peer);
    manual = false;
    reconnect_cursor = bonds.count ? 1 : 0;
    page_delay_ms = 1000;
}

static void restore_classic_key(const uint8_t address[6])
{
    struct classic_hid_candidate peer = {0};
    memcpy(peer.address, address, sizeof(peer.address));
    int index = find_bond(&peer);
    if (index >= 0 && has_saved_key(&bonds.entries[index]))
    {
        struct classic_hid_bond_entry *entry = &bonds.entries[index];
        gap_store_link_key_for_bd_addr(entry->address, entry->keys.classic.key,
                                       (link_key_type_t)entry->keys.classic.type);
    }
}

static void restore_bindings(void)
{
    manual = true;
    gap_delete_all_link_keys();
    le_device_db_init();
    int rc = storage.load(&staged_bonds);
    if (!rc && (staged_bonds.count > CLASSIC_HID_MAX_BONDS || !valid_address(local_address) ||
                memcmp(staged_bonds.local, local_address, sizeof(local_address))))
        rc = -RT_EINVAL;
    if (rc)
    {
        memset(&staged_bonds, 0, sizeof(staged_bonds));
        if (rc != -RT_EEMPTY) status("saved peers unavailable", rc);
        return;
    }
    for (unsigned i = 0; i < staged_bonds.count; ++i)
    {
        struct classic_hid_bond_entry *entry = &staged_bonds.entries[i];
        if (!valid_address(entry->address) || !memcmp(entry->address, local_address, sizeof(local_address)) ||
            (entry->ble ? !stable_le_address(entry->address_type, entry->address) : entry->address_type != 0))
            continue;
        entry->name[sizeof(entry->name) - 1] = '\0';
        if (entry->ble && entry->keys.le.key_size)
        {
            rc = dual_ble_hid_restore_bond(&entry->keys.le);
            if (rc)
            {
                status("saved BLE keys unavailable", rc);
                memset(&entry->keys.le, 0, sizeof(entry->keys.le));
            }
        }
        else if (!entry->ble && has_saved_key(entry))
            gap_store_link_key_for_bd_addr(entry->address, entry->keys.classic.key,
                                           (link_key_type_t)entry->keys.classic.type);
        rt_base_t level = rt_hw_interrupt_disable();
        bonds.entries[bonds.count++] = *entry;
        rt_hw_interrupt_enable(level);
    }
    memcpy(bonds.local, local_address, sizeof(bonds.local));
    memset(&staged_bonds, 0, sizeof(staged_bonds));
    if (bonds.count)
    {
        select_saved();
        reconnect_cursor = 0; /* On boot, try the most recent bond first. */
        listen_before_reconnect = true;
        status("saved peers restored", bonds.count);
    }
}

static int commit_bonds(void)
{
    memcpy(staged_bonds.local, local_address, sizeof(staged_bonds.local));
    int rc = storage.save(&staged_bonds);
    if (!rc)
    {
        rt_base_t level = rt_hw_interrupt_disable();
        bonds = staged_bonds;
        rt_hw_interrupt_enable(level);
    }
    memset(&staged_bonds, 0, sizeof(staged_bonds));
    return rc;
}

static void remember_bond(struct classic_hid_bond_entry *entry)
{
    if (!valid_address(local_address) || !memcmp(local_address, entry->address, sizeof(local_address)))
        return;
    struct classic_hid_candidate peer;
    bond_candidate(entry, &peer);
    int index = find_bond(&peer);
    if (index < 0 && entry->ble && entry->keys.le.key_size)
    {
        peer.address_type = entry->keys.le.identity_type;
        memcpy(peer.address, entry->keys.le.identity, sizeof(peer.address));
        index = find_bond(&peer);
    }
    if (index >= 0 && !entry->name[0])
        memcpy(entry->name, bonds.entries[index].name, sizeof(entry->name));
    if (index != 0 || memcmp(entry, &bonds.entries[0], sizeof(*entry)))
    {
        if (index < 0 && bonds.count == CLASSIC_HID_MAX_BONDS)
        {
            status("saved peer list full, use gamepad forget <index>", -RT_EFULL);
            return;
        }
        staged_bonds = bonds;
        if (index < 0) index = (int)staged_bonds.count++;
        memmove(&staged_bonds.entries[1], &staged_bonds.entries[0], (unsigned)index * sizeof(*entry));
        staged_bonds.entries[0] = *entry;
        int rc = commit_bonds();
        if (rc)
        {
            status("save peer failed", rc);
            return;
        }
        status(has_saved_key(entry) ? "bonded peer saved" : "peer saved, fresh pairing required", 0);
    }
    session_preferred.ble = false;
    manual = false;
    reconnect_cursor = 1; /* The peer that just connected is now first in the list. */
    listen_before_reconnect = true;
    page_delay_ms = 1000;
    publish_status();
}

static void remember_ready_peer(void)
{
    if (!storage.save || !link_open || discard_connection || !selected_valid || le_target.ble || !cached.length ||
        memcmp(active_peer, selected, sizeof(selected)) || memcmp(cached.address, selected, sizeof(selected)) ||
        gap_security_level(selected_acl) < LEVEL_2)
        return;

    struct classic_hid_bond_entry entry = {0};
    memcpy(entry.address, selected, sizeof(entry.address));
    memcpy(entry.name, le_target.name, sizeof(entry.name));
    link_key_type_t type;
    if (gap_get_link_key_for_bd_addr(selected, entry.keys.classic.key, &type) && reusable_key_type(type))
        entry.keys.classic.type = (uint8_t)type;
    if (!has_saved_key(&entry))
    {
        memset(entry.keys.classic.key, 0, sizeof(entry.keys.classic.key));
        entry.keys.classic.type = CLASSIC_HID_NO_LINK_KEY;
    }
    remember_bond(&entry);
}

static bool is_gamepad(uint32_t cod)
{
    unsigned minor = (cod >> 2) & 15;
    return (cod & 0x1f00) == 0x0500 && (minor == 1 || minor == 2);
}

static void start_inquiry(void);
static void connect_selected(void);
static void after_inquiry(int rc);

static bool saved_classic_peer(const struct classic_hid_candidate *peer)
{
    if (peer->ble) return false;
    int index = find_bond(peer);
    return index >= 0 && has_saved_key(&bonds.entries[index]);
}

static bool has_saved_classic_peer(void)
{
    for (unsigned i = 0; i < bonds.count; ++i)
        if (!bonds.entries[i].ble && has_saved_key(&bonds.entries[i])) return true;
    return false;
}

static unsigned next_bonded_peer(unsigned index)
{
    for (; index < bonds.count; ++index)
        if (has_saved_key(&bonds.entries[index]) &&
            (!bonds.entries[index].ble || hci_le_supported())) break;
    return index;
}

static void cancel_retry(void)
{
    btstack_run_loop_remove_timer(&inquiry_retry);
    retry_armed = false;
}

static void retry_inquiry(btstack_timer_source_t *timer)
{
    (void)timer;
    retry_armed = false;
    reconnect_grace = false;
    if (working && !idle && !hid_cid && !incoming_acl && !le_busy && !inquiring &&
        selected_acl == HCI_CON_HANDLE_INVALID && le_scan == LE_SCAN_OFF)
    {
        if (pending_connect)
        {
            connect_selected();
            return;
        }
        if (!manual && bonds.count)
        {
            unsigned index = next_bonded_peer(reconnect_cursor);
            if (index < bonds.count)
            {
                struct classic_hid_candidate peer;
                bond_candidate(&bonds.entries[index], &peer);
                select_target(&peer);
                reconnect_cursor = index + 1;
                connect_selected();
                return;
            }
        }
        else if (!manual && session_preferred.ble && selected_valid)
        {
            connect_selected();
            return;
        }
    }
    start_inquiry();
}

static void retry_after(unsigned ms)
{
    if (retry_armed) return;
    retry_armed = true;
    btstack_run_loop_set_timer(&inquiry_retry, ms);
    btstack_run_loop_add_timer(&inquiry_retry);
}

static void connect_pending_peer(void)
{
    if (saved_classic_peer(&le_target) && selected_acl == HCI_CON_HANDLE_INVALID)
    {
        /* Leave the radio available for a bonded HID device that pages its
         * host on wake. The timer falls back to host-initiated paging. */
        retry_after(CLASSIC_RECONNECT_WAIT_MS);
        status("waiting for bonded Classic HID", CLASSIC_RECONNECT_WAIT_MS);
    }
    else
        connect_selected();
}

static void start_inquiry(void)
{
    if (!working || idle || hid_cid || le_busy || reconnect_grace || incoming_acl ||
        pending_connect || inquiring || le_scan != LE_SCAN_OFF)
        return;
    bool wait_for_selected = manual && selected_valid && saved_classic_peer(&le_target);
    if ((has_preference() && !manual) || wait_for_selected)
    {
        /* The controller can carry only one ACL. Release a Classic ACL left
         * behind by a closed HID session before paging the next peer. */
        if (selected_acl != HCI_CON_HANDLE_INVALID)
        {
            gap_disconnect(selected_acl);
            return;
        }
        if (retry_armed || wait_for_selected) return;
        unsigned delay_ms = page_delay_ms;
        bool round_complete = true;
        if (bonds.count)
        {
            unsigned index = next_bonded_peer(reconnect_cursor);
            round_complete = index >= bonds.count;
            if (round_complete)
            {
                index = next_bonded_peer(0);
                if (index >= bonds.count) return;
                reconnect_cursor = 0;
            }
            else
                delay_ms = 250;

            /* Give Classic peers a chance to page us on the first attempt
             * after boot or a link loss, and before each Classic page. */
            bool wait_for_classic = !bonds.entries[index].ble ||
                                    (listen_before_reconnect && has_saved_classic_peer());
            if (wait_for_classic && delay_ms < CLASSIC_RECONNECT_WAIT_MS)
                delay_ms = CLASSIC_RECONNECT_WAIT_MS;
            listen_before_reconnect = false;
        }
        retry_after(delay_ms);
        if (round_complete && page_delay_ms < 16000) page_delay_ms *= 2;
        return;
    }
    inquiring = true;
    int rc = gap_inquiry_start(7); /* A BR/EDR inquiry round lasts 8.96 s. */
    if (rc)
    {
        status("inquiry failed", rc);
        after_inquiry(rc);
    }
}

/* Indices are append-only until an explicit scan/forget. A busy LE band
 * cannot consume the BR/EDR slots, even when addresses look identical. */
static unsigned record_candidate(struct classic_hid_candidate *candidate)
{
    unsigned count = 0, i;
    for (i = 0; i < peer_count; ++i)
    {
        if (peers[i].ble != candidate->ble) continue;
        ++count;
        if (peers[i].address_type == candidate->address_type &&
            !memcmp(peers[i].address, candidate->address, sizeof(candidate->address)))
            break;
    }
    bool added = i == peer_count;
    if (added)
    {
        if (count >= CLASSIC_HID_SLOTS_PER_TRANSPORT) return 0;
        /* Reserve the last four slots for peripherals in a crowded inquiry,
         * and for meaningful scan responses amid anonymous LE advertisements. */
        if (count >= CLASSIC_HID_SLOTS_PER_TRANSPORT - 4 &&
            (candidate->ble ? !candidate->name[0] && !candidate->hid_service && !candidate->appearance :
             (candidate->class_of_device & 0x1f00) != 0x0500))
            return 0;
    }
    else
    {
        const struct classic_hid_candidate *old = &peers[i];
        if (!candidate->name[0] || (old->name_complete && !candidate->name_complete))
        {
            memcpy(candidate->name, old->name, sizeof(candidate->name));
            candidate->name_complete = old->name_complete;
        }
        if (!candidate->appearance) candidate->appearance = old->appearance;
        candidate->hid_service |= old->hid_service;
        if (candidate->rssi == 127) candidate->rssi = old->rssi;
    }
    bool changed = added || peers[i].class_of_device != candidate->class_of_device ||
                   peers[i].appearance != candidate->appearance || peers[i].hid_service != candidate->hid_service ||
                   strcmp(peers[i].name, candidate->name);
    rt_base_t level = rt_hw_interrupt_disable();
    peers[i] = *candidate;
    if (added) ++peer_count;
    rt_hw_interrupt_enable(level);
    if (report_discoveries && changed && app.discovered)
        app.discovered(i + 1, candidate);
    return i + 1;
}

static void observe_peer(const uint8_t *packet)
{
    struct classic_hid_candidate candidate = {.rssi = 127};
    gap_event_inquiry_result_get_bd_addr(packet, candidate.address);
    candidate.class_of_device = gap_event_inquiry_result_get_class_of_device(packet);
    if (gap_event_inquiry_result_get_rssi_available(packet))
        candidate.rssi = (int8_t)gap_event_inquiry_result_get_rssi(packet);
    if (gap_event_inquiry_result_get_name_available(packet))
    {
        const uint8_t *name = gap_event_inquiry_result_get_name(packet);
        size_t len = gap_event_inquiry_result_get_name_len(packet);
        if (len >= sizeof(candidate.name)) len = sizeof(candidate.name) - 1;
        for (size_t n = 0; n < len; ++n)
            candidate.name[n] = name[n] < 32 || name[n] == 127 ? '?' : (char)name[n];
        candidate.name[len] = '\0';
    }
    unsigned index = record_candidate(&candidate);
    if (index && !storage.load && !manual && !reconnect_grace && !pending_connect && !hid_cid && !incoming_acl &&
        selected_acl == HCI_CON_HANDLE_INVALID && is_gamepad(candidate.class_of_device))
    {
        memcpy(selected, candidate.address, sizeof(selected));
        selected_valid = pending_connect = true;
        status("selected", index);
    }
}

static void connect_selected(void)
{
    pending_connect = false;
    int rc;
    if (le_target.ble)
    {
        le_busy = true;
        rc = dual_ble_hid_connect(&le_target, !manual && bonds.count ? 3500u : 10000u);
        if (rc) le_busy = false;
    }
    else
    {
        restore_classic_key(selected);
        rc = hid_host_connect(selected, HID_PROTOCOL_MODE_REPORT, &hid_cid);
    }
    if (rc || manual) status("connecting", rc);
    else publish_status();
    if (rc)
    {
        hid_cid = 0;
        start_inquiry();
    }
}

static void le_connected(void)
{
    le_link = true;
    status("BLE link connected", 0);
}

static bool le_descriptor(const struct classic_hid_candidate *peer, const uint8_t *map, size_t length)
{
    if (!le_busy || !selected_valid || !le_target.ble ||
        peer->address_type != le_target.address_type || memcmp(peer->address, le_target.address, 6)) return false;
    le_target = *peer;
    le_layout = !app.probe || app.probe(&le_target, map, length);
    if (!le_layout)
    {
        if (!bonds.count) manual = true;
        selected_valid = false;
        status("unsupported BLE HID layout", 0);
    }
    else if (app.descriptor)
        app.descriptor(map, length);
    return le_layout;
}

static void le_ready(void)
{
    if (!le_layout || !selected_valid) return;
    le_input_ready = true;
    publish_status();
    if (!storage.save)
    {
        session_preferred = le_target;
        manual = false;
        page_delay_ms = 1000;
        publish_status();
        return;
    }

    struct classic_hid_bond_entry entry = { .ble = true, .address_type = le_target.address_type & 1 };
    memcpy(entry.address, le_target.address, sizeof(entry.address));
    memcpy(entry.name, le_target.name, sizeof(entry.name));
    int rc = dual_ble_hid_export_bond(&entry.keys.le);
    if (rc && rc != -RT_ENOENT)
    {
        status("read BLE keys failed", rc);
        return;
    }
    if (!stable_le_address(entry.address_type, entry.address))
    {
        if (!entry.keys.le.key_size)
        {
            status("BLE private address requires a new scan", 0);
            return;
        }
        entry.address_type = entry.keys.le.identity_type;
        memcpy(entry.address, entry.keys.le.identity, sizeof(entry.address));
    }
    bond_candidate(&entry, &le_identity);
    remember_bond(&entry);
}

static void le_input(uint8_t id, const uint8_t *data, size_t length)
{
    if (le_layout && selected_valid)
        app.input(id, data, length);
}

static void clear_le_link(void)
{
    if (le_layout && app.probe) app.probe(NULL, NULL, 0);
    if (le_layout && app.descriptor) app.descriptor(NULL, 0);
    le_layout = le_busy = le_link = le_input_ready = false;
    le_identity.ble = false;
}

static void le_closed(int reason)
{
    clear_le_link();
    if (storage.load)
    {
        /* Keep space in the RAM database for pairing another peer even after a
         * failed setup or when the permanent list is full. */
        for (int i = 0; i < le_device_db_max_count(); ++i)
        {
            int type;
            struct classic_hid_candidate peer = {.ble = true};
            le_device_db_info(i, &type, peer.address, NULL);
            if (type != BD_ADDR_TYPE_LE_PUBLIC && type != BD_ADDR_TYPE_LE_RANDOM) continue;
            peer.address_type = (uint8_t)type;
            if (find_bond(&peer) < 0) le_device_db_remove(i);
        }
    }
    status("BLE link closed", reason);
    start_inquiry();
}

static void le_timer_after(unsigned ms)
{
    btstack_run_loop_remove_timer(&le_scan_timer);
    btstack_run_loop_set_timer(&le_scan_timer, ms);
    btstack_run_loop_add_timer(&le_scan_timer);
}

static void resume_classic(void)
{
    btstack_run_loop_remove_timer(&le_scan_timer);
    le_scan = LE_SCAN_OFF;
    if (pending_connect && !hid_cid && !incoming_acl)
        connect_pending_peer();
    else
        start_inquiry();
}

static void stop_le_window(void)
{
    le_scan = le_scan == LE_SCAN_STARTING ? LE_SCAN_CANCEL_START : LE_SCAN_STOPPING;
    le_timer_after(1500);
    gap_stop_scan();
}

static void le_window_timeout(btstack_timer_source_t *timer)
{
    (void)timer;
    if (le_scan == LE_SCAN_STOPPING || le_scan == LE_SCAN_CANCEL_START)
    {
        /* A start that never reached the controller can be cancelled by GAP
         * without a disable command. Do not strand a pending Classic target. */
        status("BLE scan stop timeout", 0);
        resume_classic();
    }
    else if (le_scan == LE_SCAN_ACTIVE || le_scan == LE_SCAN_STARTING)
        stop_le_window();
}

static void le_scan_complete(uint8_t rc)
{
    if (le_scan == LE_SCAN_STARTING)
    {
        le_scan = LE_SCAN_ACTIVE; /* The enable command was acknowledged. */
        if (rc) status("BLE scan start failed", rc);
        if (rc || !report_discoveries)
            stop_le_window();
        else
            le_timer_after(8000);
    }
    else if (le_scan == LE_SCAN_CANCEL_START)
    {
        /* Enable and disable share one opcode. This is the delayed enable
         * completion; wait for disable before resuming the Classic host. */
        le_scan = LE_SCAN_STOPPING;
        le_timer_after(1500);
    }
    else if (le_scan == LE_SCAN_STOPPING)
    {
        if (rc) status("BLE scan stop failed", rc);
        resume_classic();
    }
}

static void after_inquiry(int rc)
{
    inquiring = inquiry_active = false;
    if (!hid_cid && !incoming_acl && pending_connect)
        connect_pending_peer();
    else if (report_discoveries && !idle && !hid_cid && !incoming_acl && hci_le_supported())
    {
        le_scan = LE_SCAN_STARTING;
        le_timer_after(1500);
        gap_start_scan();
    }
    else if (rc && !idle)
        retry_after(1000);
    else if (!rc)
        start_inquiry();
}

static void inquiry_command_status(uint8_t rc)
{
    inquiry_active = inquiring = rc == ERROR_CODE_SUCCESS;
    if (rc)
    {
        /* BTstack does not emit GAP_EVENT_INQUIRY_COMPLETE when the
         * controller rejects Inquiry after gap_inquiry_start queued it. */
        status("controller rejected inquiry", rc);
        after_inquiry(rc);
    }
    else if (idle || (pending_connect && le_target.ble))
        gap_inquiry_stop();
}

static void end_connection(void);

static void finish_descriptor_session(void)
{
    if (!storage.load || !cached.length) return;
    /* A preference or a failed save cannot authenticate a descriptor for
     * a subsequent ACL. Keep only the map belonging to the committed key. */
    link_key_t key = {0};
    link_key_type_t type = INVALID_LINK_KEY;
    struct classic_hid_candidate peer = {0};
    memcpy(peer.address, cached.address, sizeof(peer.address));
    int index = find_bond(&peer);
    bool trusted = index >= 0 && has_saved_key(&bonds.entries[index]) &&
                   gap_get_link_key_for_bd_addr(cached.address, key, &type) &&
                   type == (link_key_type_t)bonds.entries[index].keys.classic.type &&
                   !memcmp(bonds.entries[index].keys.classic.key, key, sizeof(key));
    if (!trusted) cached.length = 0;
}

static void accept_control(void)
{
    if (!waiting_control)
        return;
    btstack_run_loop_remove_timer(&control_wait);
    waiting_control = false;
    int rc = hid_host_accept_connection(hid_cid, HID_PROTOCOL_MODE_REPORT);
    if (rc)
    {
        status("incoming connection failed", rc);
        end_connection();
    }
}

static void control_timeout(btstack_timer_source_t *timer)
{
    (void)timer;
    status("SDP wait expired, accepting HID control", 0);
    accept_control();
}

/* HIDDescriptorList = sequence of (uint8 descriptor type, text string). */
static void extract_report_map(uint16_t size)
{
    const uint8_t *p = prefetch.attribute;
    const uint8_t *end = p + size;
    if (!size || de_get_len_safe(p, size) != size || de_get_element_type(p) != DE_DES)
        return;
    p += de_get_header_size(p);
    while (p < end)
    {
        uint32_t n = de_get_len_safe(p, end - p);
        if (!n || de_get_element_type(p) != DE_DES)
            return;
        const uint8_t *entry_end = p + n;
        const uint8_t *entry = p + de_get_header_size(p);
        if (de_get_len_safe(entry, entry_end - entry) == 2 && entry[0] == 0x08 && entry[1] == 0x22)
        {
            entry += 2;
            n = de_get_len_safe(entry, entry_end - entry);
            if (n && n == (uint32_t)(entry_end - entry) && de_get_element_type(entry) == DE_STRING &&
                de_get_data_size(entry) <= sizeof(cached.map))
            {
                prefetch.map_length = de_get_data_size(entry);
                prefetch.map_offset = entry + de_get_header_size(entry) - prefetch.attribute;
                if (prefetch.map_length) return;
            }
        }
        p = entry_end;
    }
}

static void sdp_result(uint8_t type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel;
    (void)size;
    if (type != HCI_EVENT_PACKET || !prefetch.busy) return;
    bool current = prefetch.live && selected_valid && prefetch.acl == selected_acl &&
                   !memcmp(prefetch.address, selected, sizeof(selected));
    switch (hci_event_packet_get_type(packet))
    {
    case SDP_EVENT_QUERY_ATTRIBUTE_VALUE:
        if (current && !prefetch.map_length &&
            sdp_event_query_attribute_byte_get_attribute_id(packet) == BLUETOOTH_ATTRIBUTE_HID_DESCRIPTOR_LIST)
        {
            uint16_t total = sdp_event_query_attribute_byte_get_attribute_length(packet);
            uint16_t offset = sdp_event_query_attribute_byte_get_data_offset(packet);
            uint16_t record = sdp_event_query_attribute_byte_get_record_id(packet);
            if (!offset)
            {
                prefetch.record = record;
                prefetch.filled = 0;
            }
            /* BTstack emits the DE header (1..5 bytes) before its length is
             * known. These bytes are part of the attribute, starting at 0. */
            if (record == prefetch.record && offset == prefetch.filled &&
                total <= sizeof(prefetch.attribute) &&
                (total ? offset < total : offset < 5))
            {
                prefetch.attribute[prefetch.filled++] = sdp_event_query_attribute_byte_get_data(packet);
                if (prefetch.filled == total)
                    extract_report_map(total);
            }
        }
        break;
    case SDP_EVENT_QUERY_COMPLETE:
        prefetch.busy = prefetch.live = false;
        if (!current) break;
        if (!sdp_event_query_complete_get_status(packet) && prefetch.map_length &&
            gap_security_level(selected_acl) >= LEVEL_2)
        {
            memcpy(cached.map, prefetch.attribute + prefetch.map_offset, prefetch.map_length);
            memcpy(cached.address, selected, sizeof(selected));
            cached.length = prefetch.map_length;
        }
        else if (sdp_event_query_complete_get_status(packet))
            status("SDP prefetch failed", sdp_event_query_complete_get_status(packet));
        else
            status("SDP report descriptor missing or unusable, attribute bytes", prefetch.filled);
        accept_control();
        break;
    }
}

static void start_sdp_on_acl(void *context)
{
    (void)context;
    if (!sdp_queued)
        return;
    sdp_queued = false;
    if (working && incoming_acl && selected_valid && selected_acl != HCI_CON_HANDLE_INVALID &&
        (!hid_cid || waiting_control) && !prefetch.busy && !discard_connection)
    {
        pending_connect = false;
        if (inquiring)
            gap_inquiry_stop();
        /* Use the same service search as normal HID discovery, then keep
         * only the Report descriptor from attribute 0x0206. */
        prefetch.busy = prefetch.live = true;
        prefetch.filled = prefetch.map_length = 0;
        prefetch.acl = selected_acl;
        memcpy(prefetch.address, selected, sizeof(selected));
        int rc = sdp_client_query_uuid16(sdp_result, selected,
                         BLUETOOTH_SERVICE_CLASS_HUMAN_INTERFACE_DEVICE_SERVICE);
        if (!rc) return;
        status("SDP prefetch failed", rc);
        prefetch.busy = prefetch.live = false;
    }
    accept_control();
}

static void end_connection(void)
{
    bool wait_for_peer = !storage.load && selected_valid && !manual && !discard_connection &&
                         cached.length && !memcmp(cached.address, selected, sizeof(selected));
    link_open = false;
    finish_descriptor_session();
    hid_cid = 0;
    btstack_run_loop_remove_timer(&control_wait);
    waiting_control = false;
    discard_connection = numbered_reports = layout_ready = false;
    if (early_count || early_dropped)
        status("input before SDP discarded", early_count + early_dropped);
    early_count = early_dropped = 0;
    if (app.probe)
        app.probe(NULL, NULL, 0);
    if (app.descriptor)
        app.descriptor(NULL, 0);
    publish_status();
    if (wait_for_peer)
    {
        /* Allow a device-initiated virtual-cable reconnect before we page it. */
        reconnect_grace = true;
        retry_after(CLASSIC_RECONNECT_WAIT_MS);
        return;
    }
    start_inquiry();
}

static bool selected_peer(const bd_addr_t address)
{
    return selected_valid && !le_target.ble && !memcmp(selected, address, sizeof(selected));
}

static bool selected_saved_peer(void)
{
    if (!has_preference() || !selected_valid) return false;
    struct classic_hid_candidate peer = current_peer(), saved = preferred_peer();
    return same_peer(&peer, &saved) || find_bond(&peer) >= 0;
}

static int accept_selected_acl(bd_addr_t address, hci_link_type_t type)
{
    if (!working || type != HCI_LINK_TYPE_ACL || idle || le_busy || discard_connection)
        return 0;
    if (manual || hid_cid || incoming_acl || pending_connect || selected_acl != HCI_CON_HANDLE_INVALID)
        return selected_peer(address);

    /* A bonded Classic HID device may initiate the ACL as it wakes. BTstack
     * calls this filter before delivering HCI_EVENT_CONNECTION_REQUEST. */
    struct classic_hid_candidate peer = {0};
    memcpy(peer.address, address, sizeof(peer.address));
    int index = find_bond(&peer);
    if (index < 0 || !has_saved_key(&bonds.entries[index])) return 0;
    bond_candidate(&bonds.entries[index], &peer);
    select_target(&peer);
    restore_classic_key(address);
    cancel_retry();
    return 1;
}

static void deliver_input(const uint8_t *data, uint16_t length)
{
    if (numbered_reports && !length)
        return;
    if (numbered_reports)
        app.input(data[0], data + 1, length - 1);
    else
        app.input(0, data, length);
}

static bool accept_layout(const uint8_t *map, size_t length)
{
    if (!app.probe) return true;
    struct classic_hid_candidate peer = {.rssi = 127};
    memcpy(peer.address, active_peer, sizeof(peer.address));
    for (unsigned i = 0; i < peer_count; ++i)
    {
        if (!peers[i].ble && !memcmp(peers[i].address, active_peer, sizeof(active_peer)))
        {
            peer = peers[i];
            break;
        }
    }
    if (app.probe(&peer, map, length)) return true;
    /* Keep the saved binding so the user can reconnect after changing the
     * enabled models. A failed automatic attempt can try the next bond. */
    if (!bonds.count) manual = true;
    discard_connection = true;
    selected_valid = pending_connect = false;
    layout_ready = false;
    status("unsupported HID layout", 0);
    hid_host_disconnect(hid_cid);
    return false;
}

static void hid_event(const uint8_t *packet, uint16_t size)
{
    uint16_t cid;
    bd_addr_t address;
    uint8_t rc;
    switch (hci_event_hid_meta_get_subevent_code(packet))
    {
    case HID_SUBEVENT_INCOMING_CONNECTION:
        cid = hid_subevent_incoming_connection_get_hid_cid(packet);
        hid_subevent_incoming_connection_get_address(packet, address);
        if (hid_cid || !selected_peer(address))
            hid_host_decline_connection(cid);
        else
        {
            hid_cid = cid;
            pending_connect = false;
            waiting_control = true;
            if (sdp_queued || prefetch.live)
            {
                btstack_run_loop_set_timer(&control_wait, 1000);
                btstack_run_loop_add_timer(&control_wait);
            }
            else
                accept_control();
        }
        break;
    case HID_SUBEVENT_CONNECTION_OPENED:
        if (hid_subevent_connection_opened_get_hid_cid(packet) != hid_cid)
            break;
        rc = hid_subevent_connection_opened_get_status(packet);
        if (rc)
        {
            status(discard_connection ? "previous connection attempt ended" : "connect failed", rc);
            end_connection();
        }
        else if (discard_connection)
            hid_host_disconnect(hid_cid);
        else
        {
            selected_acl = hid_subevent_connection_opened_get_con_handle(packet);
            hid_subevent_connection_opened_get_bd_addr(packet, address);
            memcpy(active_peer, address, sizeof(active_peer));
            link_open = true;
            status("connected", hid_cid);
            if (hid_subevent_connection_opened_get_incoming(packet) && selected_peer(address) &&
                cached.length && !memcmp(address, cached.address, sizeof(address)) &&
                gap_security_level(selected_acl) >= LEVEL_2)
                hid_host_set_cached_report_descriptor(hid_cid, cached.map, cached.length);
        }
        break;
    case HID_SUBEVENT_DESCRIPTOR_AVAILABLE:
        if (hid_subevent_descriptor_available_get_hid_cid(packet) != hid_cid || discard_connection)
            break;
        rc = hid_subevent_descriptor_available_get_status(packet);
        if (!rc)
        {
            const uint8_t *map = hid_descriptor_storage_get_descriptor_data(hid_cid);
            uint16_t len = hid_descriptor_storage_get_descriptor_len(hid_cid);
            if (!accept_layout(map, len)) break;
            numbered_reports = len && btstack_hid_report_id_declared(map, len);
            if (selected_valid && len && len <= sizeof(cached.map) &&
                gap_security_level(selected_acl) >= LEVEL_2)
            {
                memcpy(cached.map, map, len);
                memcpy(cached.address, selected, sizeof(selected));
                cached.length = len;
            }
            if (app.descriptor) app.descriptor(map, len);
            status("ready", len);
            if (len) remember_ready_peer();
        }
        else if (!accept_layout(NULL, 0))
            break;
        else
            status("descriptor unavailable, raw input", rc);
        layout_ready = true;
        for (unsigned i = 0; i < early_count; ++i)
            deliver_input(early[i].data, early[i].length);
        if (early_dropped)
            status("input before SDP dropped", early_dropped);
        early_count = early_dropped = 0;
        break;
    case HID_SUBEVENT_REPORT:
        if (size >= 8 && hid_subevent_report_get_hid_cid(packet) == hid_cid && !discard_connection)
        {
            const uint8_t *data = hid_subevent_report_get_report(packet);
            uint16_t len = hid_subevent_report_get_report_len(packet);
            if (len && len <= size - 7 && data[0] == 0xa1)
            {
                --len;
                ++data;
                if (layout_ready)
                    deliver_input(data, len);
                else if (early_count < sizeof(early)/sizeof(early[0]) && len <= sizeof(early[0].data))
                {
                    early[early_count].length = len;
                    memcpy(early[early_count++].data, data, len);
                }
                else
                    ++early_dropped;
            }
        }
        break;
    case HID_SUBEVENT_VIRTUAL_CABLE_UNPLUG:
        if (hid_subevent_virtual_cable_unplug_get_hid_cid(packet) == hid_cid && selected_peer(cached.address))
            cached.length = 0;
        break;
    case HID_SUBEVENT_CONNECTION_CLOSED:
        if (hid_subevent_connection_closed_get_hid_cid(packet) == hid_cid)
        {
            status("disconnected", hid_cid);
            end_connection();
        }
        break;
    }
}

static void controller_stopped(void)
{
    working = false;
    dual_ble_hid_reset();
    clear_le_link();
    btstack_run_loop_remove_timer(&inquiry_retry);
    btstack_run_loop_remove_timer(&control_wait);
    btstack_run_loop_remove_timer(&le_scan_timer);
    if (le_scan != LE_SCAN_OFF) gap_stop_scan();
    le_scan = LE_SCAN_OFF;
    if ((layout_ready || link_open) && app.descriptor)
        app.descriptor(NULL, 0);
    if ((layout_ready || link_open) && app.probe)
        app.probe(NULL, NULL, 0);
    command_cancelled = command_pending;
    retry_armed = reconnect_grace = pending_connect = selected_valid = listen_before_reconnect = false;
    inquiring = incoming_acl = attempted_on_acl = sdp_queued = waiting_control = false;
    inquiry_active = report_discoveries = false;
    prefetch.busy = prefetch.live = false;
    link_open = discard_connection = layout_ready = numbered_reports = false;
    idle = false;
    manual = storage.load != NULL;
    hid_cid = 0;
    selected_acl = HCI_CON_HANDLE_INVALID;
    peer_count = early_count = early_dropped = 0;
    cached.length = 0;
    rt_base_t level = rt_hw_interrupt_disable();
    memset(&bonds, 0, sizeof(bonds));
    rt_hw_interrupt_enable(level);
    session_preferred.ble = false;
    publish_status();
}

static void packet_handler(uint8_t type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    (void)channel;
    if (type != HCI_EVENT_PACKET || (!working && hci_event_packet_get_type(packet) != BTSTACK_EVENT_STATE))
        return;
    bd_addr_t address;
    switch (hci_event_packet_get_type(packet))
    {
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING)
        {
            if (working) break;
            working = true;
            gap_local_bd_addr(local_address);
            if (hci_le_supported())
            {
                gap_set_scan_params(1, 0x0060, 0x0040, 0); /* Active scan, 40 ms per 60 ms. */
                gap_set_scan_duplicate_filter(false); /* Merge names arriving in scan responses. */
            }
            if (storage.load) restore_bindings();
            if (!has_preference()) status("select a peer", 0);
            start_inquiry();
        }
        else if (working)
            controller_stopped();
        break;
    case GAP_EVENT_INQUIRY_RESULT:
        observe_peer(packet);
        break;
    case GAP_EVENT_ADVERTISING_REPORT:
        if (report_discoveries && (le_scan == LE_SCAN_STARTING || le_scan == LE_SCAN_ACTIVE))
        {
            struct classic_hid_candidate candidate;
            if (classic_le_advertisement(packet, size, &candidate))
                record_candidate(&candidate);
        }
        break;
    case HCI_EVENT_COMMAND_COMPLETE:
        if (size >= 6 && hci_event_command_complete_get_command_opcode(packet) == HCI_OPCODE_HCI_LE_SET_SCAN_ENABLE)
            le_scan_complete(hci_event_command_complete_get_return_parameters(packet)[0]);
        break;
    case HCI_EVENT_COMMAND_STATUS:
        if (hci_event_command_status_get_command_opcode(packet) == HCI_OPCODE_HCI_INQUIRY)
            inquiry_command_status(hci_event_command_status_get_status(packet));
        break;
    case GAP_EVENT_INQUIRY_COMPLETE:
        if (inquiring) after_inquiry(0);
        break;
    case HCI_EVENT_CONNECTION_REQUEST:
        if (!storage.load && !manual && !selected_valid && !hid_cid &&
            hci_event_connection_request_get_link_type(packet) == HCI_LINK_TYPE_ACL &&
            is_gamepad(hci_event_connection_request_get_class_of_device(packet)))
        {
            hci_event_connection_request_get_bd_addr(packet, selected);
            selected_valid = true;
            status("incoming gamepad", 0);
        }
        hci_event_connection_request_get_bd_addr(packet, address);
        if (!hid_cid && selected_peer(address) &&
            hci_event_connection_request_get_link_type(packet) == HCI_LINK_TYPE_ACL)
        {
            cancel_retry();
            incoming_acl = true;
            pending_connect = false;
            if (saved_classic_peer(&le_target))
                status("Classic HID peer initiated reconnect", 0);
            else
                publish_status();
        }
        break;
    case HCI_EVENT_CONNECTION_COMPLETE:
        hci_event_connection_complete_get_bd_addr(packet, address);
        if (selected_peer(address) && hci_event_connection_complete_get_link_type(packet) == HCI_LINK_TYPE_ACL)
        {
            uint8_t rc = hci_event_connection_complete_get_status(packet);
            if (!rc)
            {
                selected_acl = hci_event_connection_complete_get_connection_handle(packet);
                attempted_on_acl = false;
                incoming_acl = !hid_cid;
            }
            else
                incoming_acl = false;
            if (rc) status("ACL link failed", rc);
            else publish_status();
            if (rc && !hid_cid) start_inquiry();
        }
        break;
    case HCI_EVENT_AUTHENTICATION_COMPLETE:
        if (hci_event_authentication_complete_get_connection_handle(packet) == selected_acl &&
            hci_event_authentication_complete_get_status(packet))
            status("authentication failed", hci_event_authentication_complete_get_status(packet));
        break;
    case HCI_EVENT_ENCRYPTION_CHANGE:
    case HCI_EVENT_ENCRYPTION_CHANGE_V2:
        if (hci_event_encryption_change_get_connection_handle(packet) == selected_acl)
        {
            uint8_t rc = hci_event_encryption_change_get_status(packet);
            if (rc)
                status("encryption failed", rc);
            else
            {
                uint8_t enabled = hci_event_encryption_change_get_encryption_enabled(packet);
                status("encryption enabled", enabled);
                if (enabled && layout_ready) remember_ready_peer();
                /* Some devices stop answering SDP after their HIDP channels
                 * open. On a cold ACL, start SDP while it is still available. */
                if (enabled && incoming_acl && selected_valid && (!hid_cid || waiting_control) && !discard_connection &&
                    !attempted_on_acl && (!cached.length || !selected_peer(cached.address)))
                {
                    sdp_queued = attempted_on_acl = true;
                    btstack_run_loop_execute_on_main_thread(&start_sdp);
                }
            }
        }
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        if (hci_event_disconnection_complete_get_connection_handle(packet) == selected_acl &&
            hci_event_disconnection_complete_get_status(packet) == ERROR_CODE_SUCCESS)
        {
            selected_acl = HCI_CON_HANDLE_INVALID;
            incoming_acl = sdp_queued = prefetch.live = false;
            finish_descriptor_session();
            status("ACL disconnected", hci_event_disconnection_complete_get_reason(packet));
            if (!hid_cid) start_inquiry();
        }
        break;
    case HCI_EVENT_PIN_CODE_REQUEST:
        hci_event_pin_code_request_get_bd_addr(packet, address);
        if (selected_peer(address)) gap_pin_code_response(address, "0000");
        else gap_pin_code_negative(address);
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        hci_event_user_confirmation_request_get_bd_addr(packet, address);
        if (selected_peer(address))
            gap_ssp_confirmation_response(address);
        else gap_ssp_confirmation_negative(address);
        break;
    case GAP_EVENT_PAIRING_COMPLETE:
        gap_event_pairing_complete_get_bd_addr(packet, address);
        if (selected_peer(address))
            status("pairing", gap_event_pairing_complete_get_status(packet));
        break;
    case GAP_EVENT_PAIRING_STARTED:
        gap_event_pairing_started_get_bd_addr(packet, address);
        if (!memcmp(address, cached.address, sizeof(address)))
            cached.length = 0;
        if (storage.load && selected_peer(address))
            gap_drop_link_key_for_bd_addr(address);
        break;
    case HCI_EVENT_LINK_KEY_NOTIFICATION:
        reverse_bd_addr(packet + 2, address);
        /* BTstack updates its key DB before forwarding this event, preserving
         * the original key type for CHANGED_COMBINATION_KEY. A late key can
         * complete a binding once the descriptor and encryption are ready. */
        if (storage.save && selected_peer(address) && layout_ready && link_open)
            remember_ready_peer();
        else if (!memcmp(address, cached.address, sizeof(address)))
            cached.length = 0;
        /* The controller can finish an old pairing while a manual switch or
         * forget is disconnecting it. Keep only selected or committed keys. */
        struct classic_hid_candidate peer = {0};
        memcpy(peer.address, address, sizeof(peer.address));
        if (storage.load && !selected_peer(address) && find_bond(&peer) < 0)
            gap_drop_link_key_for_bd_addr(address);
        break;
    case HCI_EVENT_HID_META:
        hid_event(packet, size);
        break;
    }
}

static int remove_bond(const struct classic_hid_candidate *peer)
{
    int index = find_bond(peer);
    if (index < 0) return -RT_ENOENT;
    struct classic_hid_bond_entry entry = bonds.entries[index];
    staged_bonds = bonds;
    --staged_bonds.count;
    memmove(&staged_bonds.entries[index], &staged_bonds.entries[index + 1],
            (staged_bonds.count - (unsigned)index) * sizeof(entry));
    memset(&staged_bonds.entries[staged_bonds.count], 0, sizeof(entry));
    int rc = commit_bonds();
    if (rc) return rc;
    reconnect_cursor = 0;
    if (entry.ble)
    {
        struct classic_hid_candidate identity;
        bond_candidate(&entry, &identity);
        if (entry.keys.le.key_size)
        {
            identity.address_type = entry.keys.le.identity_type;
            memcpy(identity.address, entry.keys.le.identity, sizeof(identity.address));
        }
        dual_ble_hid_forget(&identity);
    }
    else
    {
        gap_drop_link_key_for_bd_addr(entry.address);
        if (!memcmp(cached.address, entry.address, sizeof(entry.address))) cached.length = 0;
    }
    return 0;
}

static void run_command(void *context)
{
    (void)context;
    enum user_action action = requested_action;
    if (!working || command_cancelled)
    {
        command_pending = command_cancelled = false;
        return;
    }
    if (action == PAIR_PEER)
    {
        if (le_busy && le_link && selected_valid && requested_peer.ble &&
            requested_peer.address_type == le_target.address_type &&
            !memcmp(selected, requested_peer.address, sizeof(selected)))
        {
            int rc = dual_ble_hid_pair();
            if (rc) status("BLE pairing request failed", rc);
        }
        else if (!requested_peer.ble && selected_peer(requested_peer.address) &&
                 !discard_connection && selected_acl != HCI_CON_HANDLE_INVALID)
            gap_request_security_level(selected_acl, LEVEL_2);
        command_pending = false;
        return;
    }
    if (action == RECONNECT_SAVED && !has_preference())
    {
        status("no remembered peer", -RT_ENOENT);
        command_pending = false;
        return;
    }

    bool forget = action == FORGET_SAVED || action == FORGET_BOND;
    if (forget)
    {
        struct classic_hid_candidate current = current_peer();
        bool have_current = selected_valid || le_busy;
        struct classic_hid_candidate peer = action == FORGET_BOND ? requested_peer :
                                            have_current ? current : preferred_peer();
        int index = find_bond(&peer);
        bool disconnect = action == FORGET_SAVED || (have_current &&
                          (same_peer(&peer, &current) || (index >= 0 && index == find_bond(&current))));
        int rc = remove_bond(&peer);
        if (rc && (rc != -RT_ENOENT || action == FORGET_BOND))
        {
            status("forget peer failed", rc);
            command_pending = false;
            return;
        }
        if (session_preferred.ble && same_peer(&peer, &session_preferred))
            session_preferred.ble = false;
        if (disconnect && le_busy && le_target.ble)
            dual_ble_hid_forget(&le_target); /* Also catch pairing that finishes during teardown. */
        else if (disconnect && !peer.ble && valid_address(peer.address))
            gap_drop_link_key_for_bd_addr(peer.address);
        status(valid_address(peer.address) ? "peer forgotten" : "no remembered peer", 0);
        if (!disconnect)
        {
            command_pending = false;
            return;
        }
    }
    cancel_retry();
    reconnect_grace = false;
    idle = action == DISCONNECT_PEER;
    report_discoveries = action == MANUAL_SCAN;
    if (action == MANUAL_SCAN || forget || idle)
    {
        hci_con_handle_t old_acl = selected_acl;
        manual = true;
        pending_connect = selected_valid = incoming_acl = sdp_queued = prefetch.live = false;
        selected_acl = HCI_CON_HANDLE_INVALID;
        if (!idle)
            peer_count = 0;
        else if (inquiring)
            gap_inquiry_stop();
        if (forget && !storage.load && !le_target.ble)
        {
            cached.length = 0;
            gap_delete_all_link_keys();
        }
        if (report_discoveries && (hid_cid || le_busy))
            status("scan waiting for previous connection", 0);
        if (waiting_control)
        {
            hid_host_decline_connection(hid_cid);
            end_connection();
        }
        else if (hid_cid)
        {
            /* A still-pending SDP operation can only be disconnected after
             * its channels open. Discard its later completion in that case. */
            discard_connection = true;
            hid_host_disconnect(hid_cid);
        }
        if (old_acl != HCI_CON_HANDLE_INVALID)
            gap_disconnect(old_acl);
        if (le_busy)
        {
            int rc = dual_ble_hid_disconnect();
            if (rc) status("BLE disconnect request failed", rc);
        }
        if (!hid_cid) start_inquiry();
        if (idle) status("manual disconnect", 0);
    }
    else if (action == RECONNECT_SAVED && (hid_cid || le_busy) && selected_saved_peer())
    {
        manual = false;
        status(link_open || le_input_ready ? "saved peer connected" : "connecting to saved peer", 0);
    }
    else if (!hid_cid && !le_busy)
    {
        struct classic_hid_candidate peer = action == RECONNECT_SAVED ? preferred_peer() : requested_peer;
        if (peer.ble || !selected_peer(peer.address))
        {
            if (selected_acl != HCI_CON_HANDLE_INVALID)
                gap_disconnect(selected_acl);
            selected_acl = HCI_CON_HANDLE_INVALID;
            sdp_queued = incoming_acl = prefetch.live = false;
        }
        if (action == RECONNECT_SAVED)
            select_saved();
        else
        {
            manual = true;
            select_target(&peer);
        }
        pending_connect = true;
        /* Waiting for this short inquiry round avoids racing inquiry cancel
         * with the SDP client's new outgoing ACL connection. */
        if (!inquiring && !incoming_acl && le_scan == LE_SCAN_OFF)
            connect_pending_peer();
        else if (peer.ble && inquiry_active)
            gap_inquiry_stop(); /* Dial only after GAP_EVENT_INQUIRY_COMPLETE. */
    }
    else
        status("select peer failed", -RT_EBUSY);
    if (!report_discoveries && le_scan == LE_SCAN_ACTIVE)
        stop_le_window();
    publish_status();
    command_pending = false;
}

static int submit_command(enum user_action action, unsigned index, const struct classic_hid_candidate *peer)
{
    int rc = RT_EOK;
    rt_base_t level = rt_hw_interrupt_disable();
    if (!working || command_pending || (action == CONNECT_SELECTED && (hid_cid || le_busy)))
        rc = -RT_EBUSY;
    else if (action == RECONNECT_SAVED && !has_preference())
        rc = -RT_ENOENT;
    else if (action == RECONNECT_SAVED && (hid_cid || le_busy) && !selected_saved_peer())
        rc = -RT_EBUSY;
    else if (action == PAIR_PEER && (!selected_valid || discard_connection ||
             (le_target.ble ? !le_link : selected_acl == HCI_CON_HANDLE_INVALID)))
        rc = -RT_EINVAL;
    else if (index > peer_count)
        rc = -RT_EINVAL;
    else
    {
        command_pending = true;
        requested_action = action;
        if (index)
            peer = &peers[index - 1];
        if (action == PAIR_PEER)
        {
            requested_peer = le_target;
            memcpy(requested_peer.address, selected, sizeof(selected));
        }
        else if (peer)
            requested_peer = *peer;
    }
    rt_hw_interrupt_enable(level);
    if (!rc)
        btstack_run_loop_execute_on_main_thread(&command);
    return rc;
}

int classic_hid_scan(void)
{
    return submit_command(MANUAL_SCAN, 0, NULL);
}

int classic_hid_connect(unsigned index)
{
    return index ? submit_command(CONNECT_SELECTED, index, NULL) : -RT_EINVAL;
}

int classic_hid_forget_bonded(const struct classic_hid_candidate *peer)
{
    if (!peer || !valid_address(peer->address)) return -RT_EINVAL;
    return submit_command(FORGET_BOND, 0, peer);
}

int classic_hid_connect_address(const uint8_t address[6])
{
    if (!address) return -RT_EINVAL;
    struct classic_hid_candidate peer = {.rssi = 127};
    memcpy(peer.address, address, sizeof(peer.address));
    return classic_hid_connect_peer(&peer);
}

int classic_hid_connect_peer(const struct classic_hid_candidate *peer)
{
    if (!peer || !valid_address(peer->address) || (peer->ble ? peer->address_type > 3 : peer->address_type != 0))
        return -RT_EINVAL;
    return submit_command(CONNECT_SELECTED, 0, peer);
}

int classic_hid_pair(void)
{
    return submit_command(PAIR_PEER, 0, NULL);
}

int classic_hid_disconnect(void)
{
    return submit_command(DISCONNECT_PEER, 0, NULL);
}

int classic_hid_reconnect(void)
{
    return submit_command(RECONNECT_SAVED, 0, NULL);
}

int classic_hid_forget(void)
{
    return submit_command(FORGET_SAVED, 0, NULL);
}

unsigned classic_hid_candidates(struct classic_hid_candidate *out, unsigned max)
{
    rt_base_t level = rt_hw_interrupt_disable();
    unsigned n = max < peer_count ? max : peer_count;
    if (out) memcpy(out, peers, n * sizeof(*out));
    rt_hw_interrupt_enable(level);
    return n;
}

bool classic_hid_candidate_at(unsigned index, struct classic_hid_candidate *out)
{
    rt_base_t level = rt_hw_interrupt_disable();
    bool valid = out && index && index <= peer_count;
    if (valid)
        *out = peers[index - 1];
    rt_hw_interrupt_enable(level);
    return valid;
}

static void classic_hid_host_init(void)
{
    static const struct classic_le_hid_callbacks le_callbacks = {
        .link = le_connected, .descriptor = le_descriptor, .input = le_input,
        .closed = le_closed, .status = status, .ready = le_ready
    };
    hci_set_link_key_db(btstack_link_key_db_memory_instance());
    l2cap_init();
    dual_ble_hid_init(&le_callbacks);
    /* A gamepad can query our SDP server while our client reads its HID record. */
    sdp_init();
    gap_set_security_level(LEVEL_2);
    /* Authenticate and enable encryption before opening the SDP channel. */
    l2cap_require_security_level_2_for_outgoing_sdp();
    gap_set_bondable_mode(1);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    gap_ssp_set_authentication_requirement(SSP_IO_AUTHREQ_MITM_PROTECTION_NOT_REQUIRED_GENERAL_BONDING);
    gap_ssp_set_auto_accept(0);
    if (storage.load)
        gap_register_classic_connection_filter(accept_selected_acl);
    hid_host_init(descriptor_storage, sizeof(descriptor_storage));
    hid_host_register_packet_handler(packet_handler);
    hci_handler.callback = packet_handler;
    hci_add_event_handler(&hci_handler);
    command.callback = run_command;
    start_sdp.callback = start_sdp_on_acl;
    btstack_run_loop_set_timer_handler(&inquiry_retry, retry_inquiry);
    btstack_run_loop_set_timer_handler(&control_wait, control_timeout);
    btstack_run_loop_set_timer_handler(&le_scan_timer, le_window_timeout);
    hci_set_inquiry_mode(INQUIRY_MODE_RSSI_AND_EIR);
    gap_set_page_timeout(0x2000); /* 5.12 s to page an unavailable Classic peer before moving on. */
    gap_set_local_name("Edgi-Talk Classic HID");
    gap_set_default_link_policy_settings(LM_LINK_POLICY_ENABLE_SNIFF_MODE | LM_LINK_POLICY_ENABLE_ROLE_SWITCH);
    gap_connectable_control(1);
}

int classic_hid_start_with_uart(const struct classic_hid_callbacks *callbacks,
                                const struct bluetooth_hci_uart *io)
{
    return classic_hid_start_with_store(callbacks, io, NULL);
}

int classic_hid_start_with_store(const struct classic_hid_callbacks *callbacks,
                                 const struct bluetooth_hci_uart *io,
                                 const struct classic_hid_store *store)
{
    if (!callbacks || !callbacks->input) return -RT_EINVAL;
    if (store && (!store->load || !store->save)) return -RT_EINVAL;
    rt_base_t level = rt_hw_interrupt_disable();
    if (started)
    {
        rt_hw_interrupt_enable(level);
        return -RT_EBUSY;
    }
    started = true;
    app = *callbacks;
    if (store) storage = *store;
    else memset(&storage, 0, sizeof(storage));
    rt_hw_interrupt_enable(level);
    int rc = rtthread_btstack_start(io, classic_hid_host_init);
    if (rc)
    {
        level = rt_hw_interrupt_disable();
        started = false;
        rt_hw_interrupt_enable(level);
    }
    return rc;
}
