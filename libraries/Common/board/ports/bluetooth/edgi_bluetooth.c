/* SPDX-License-Identifier: Apache-2.0 */
#include <rtconfig.h>

#ifdef BSP_USING_BTSTACK_HID_HOST
#include "cyw55512_hci.h"
#include "classic_hid_host.h"
#include "edgi_hid_store.h"
#include "gamepad.h"
#include <rtthread.h>
#include <string.h>

int classic_hid_start(const struct classic_hid_callbacks *callbacks)
{
    return classic_hid_start_with_store(callbacks, &cyw55512_hci_uart, &edgi_classic_hid_store);
}

static struct gamepad_peer classic_peer;
static bool classic_ready;

static void classic_peer_info(struct gamepad_peer *out, const struct classic_hid_candidate *device)
{
    memset(out, 0, sizeof(*out));
    out->transport = device->ble ? HID_TRANSPORT_BLE : HID_TRANSPORT_BR_EDR;
    out->address.type = device->address_type;
    out->appearance = device->appearance;
    out->hid_service = device->hid_service;
    out->pnp = device->pnp;
    memcpy(out->address.bytes, device->address, sizeof(out->address.bytes));
    memcpy(out->name, device->name, sizeof(out->name));
    out->class_of_device = device->class_of_device;
    out->rssi = device->rssi;
}

static bool classic_candidate(unsigned index, struct gamepad_peer *out)
{
    struct classic_hid_candidate device;
    if (!out || !classic_hid_candidate_at(index, &device)) return false;
    classic_peer_info(out, &device);
    return true;
}

static bool classic_bonded(unsigned index, struct gamepad_peer *out, bool *has_key)
{
    struct classic_hid_candidate device;
    if (!classic_hid_bonded_at(index, &device, has_key)) return false;
    classic_peer_info(out, &device);
    return true;
}

static void classic_discovered(unsigned index, const struct classic_hid_candidate *device)
{
    struct gamepad_peer peer;
    classic_peer_info(&peer, device);
    gamepad_discovered(index, &peer);
}

static bool classic_layout(const struct classic_hid_candidate *device, const uint8_t *map, size_t length)
{
    if (classic_ready)
        gamepad_detached(&classic_peer);
    classic_ready = false;
    if (!device) return false;
    classic_peer_info(&classic_peer, device);
    classic_peer.hid_service = true;
    classic_ready = gamepad_descriptor(&classic_peer, map, length);
    return classic_ready;
}

static void classic_input(uint8_t id, const uint8_t *data, size_t length)
{
    if (classic_ready)
        gamepad_report(&classic_peer, id, data, length);
}

static void classic_log(const char *stage, int detail)
{
    rt_kprintf("Bluetooth HID %s (%d)\n", stage, detail);
}

static int start_classic(void)
{
    static const struct classic_hid_callbacks callbacks = {
        .status = classic_log, .input = classic_input, .probe = classic_layout,
        .discovered = classic_discovered
    };
    return classic_hid_start(&callbacks);
}

static int connect_classic(const struct gamepad_address *address)
{
    return address && !address->type ? classic_hid_connect_address(address->bytes) : -RT_EINVAL;
}

static int use_peer(const struct gamepad_peer *peer, int (*action)(const struct classic_hid_candidate *))
{
    if (!peer || (peer->transport != HID_TRANSPORT_BR_EDR && peer->transport != HID_TRANSPORT_BLE))
        return -RT_EINVAL;
    struct classic_hid_candidate device = {
        .ble = peer->transport == HID_TRANSPORT_BLE, .address_type = peer->address.type,
        .rssi = peer->rssi, .class_of_device = peer->class_of_device,
        .appearance = peer->appearance, .hid_service = peer->hid_service
    };
    memcpy(device.address, peer->address.bytes, sizeof(device.address));
    memcpy(device.name, peer->name, sizeof(device.name));
    return action(&device);
}

static int connect_peer(const struct gamepad_peer *peer)
{
    return use_peer(peer, classic_hid_connect_peer);
}

static int forget_peer(const struct gamepad_peer *peer)
{
    return use_peer(peer, classic_hid_forget_bonded);
}

static void classic_status(struct gamepad_status *out)
{
    struct classic_hid_status state;
    classic_hid_get_status(&state);
    memset(out, 0, sizeof(*out));
    out->connecting = state.connecting;
    out->connected = state.connected;
    out->has_target = state.target_valid;
    out->has_preferred = state.bound;
    out->transport = (state.connected ? state.peer_ble : state.target_ble) ? HID_TRANSPORT_BLE : HID_TRANSPORT_BR_EDR;
    out->preferred_transport = state.bound_ble ? HID_TRANSPORT_BLE : HID_TRANSPORT_BR_EDR;
    out->target.type = state.target_type;
    out->active.type = state.peer_type;
    out->preferred.type = state.bound_type;
    memcpy(out->target.bytes, state.target_address, sizeof(out->target.bytes));
    memcpy(out->active.bytes, state.peer_address, sizeof(out->active.bytes));
    memcpy(out->preferred.bytes, state.bound_address, sizeof(out->preferred.bytes));
}

int edgi_classic_gamepad_register(void)
{
    static const struct gamepad_platform platform = {
        .name = "edgi-classic", .transport = HID_TRANSPORT_BR_EDR, .dual_scan = true,
        .start = start_classic, .scan = classic_hid_scan, .candidate = classic_candidate,
        .connect_peer = connect_peer,
        .connect = connect_classic, .pair = classic_hid_pair, .disconnect = classic_hid_disconnect,
        .reconnect = classic_hid_reconnect, .forget = classic_hid_forget, .status = classic_status,
        .bonded = classic_bonded, .forget_peer = forget_peer
    };
    return gamepad_register_platform(&platform);
}
#endif
