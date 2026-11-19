/* SPDX-License-Identifier: Apache-2.0 */
#ifndef CLASSIC_HID_HOST_API_H
#define CLASSIC_HID_HOST_API_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "hid_pnp_id.h"
#include "hid_le_bond.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLASSIC_HID_SLOTS_PER_TRANSPORT 16
#define CLASSIC_HID_MAX_CANDIDATES (2 * CLASSIC_HID_SLOTS_PER_TRANSPORT)
#define CLASSIC_HID_MAX_DESCRIPTOR 2048
#define CLASSIC_HID_NO_LINK_KEY 0xffu
#define CLASSIC_HID_MAX_BONDS 8

struct bluetooth_hci_uart;

/* HID-ready peers, ordered by most recent successful connection. */
struct classic_hid_bond_entry
{
    uint8_t address[6], address_type;
    char name[32];
    bool ble;
    union {
        struct {
            uint8_t key[16], type;
        } classic;
        struct hid_le_bond le;
    } keys;
};

struct classic_hid_bonds
{
    uint8_t local[6];
    unsigned count;
    struct classic_hid_bond_entry entries[CLASSIC_HID_MAX_BONDS];
};

struct classic_hid_store
{
    int (*load)(struct classic_hid_bonds *out); /* -RT_EEMPTY when unbound. */
    int (*save)(const struct classic_hid_bonds *records); /* Atomically replace, including an empty list. */
};

struct classic_hid_status
{
    uint8_t local_valid, local_address[6];
    uint8_t bound, bound_address[6], has_link_key;
    uint8_t connected, peer_address[6];
    uint8_t manual, connecting, target_valid, target_address[6];
    bool target_ble, peer_ble, bound_ble;
    uint8_t target_type, peer_type, bound_type;
};

struct classic_hid_candidate
{
    uint8_t address[6]; /* Display order, most significant byte first. */
    int8_t rssi;        /* 127 when the inquiry result omits RSSI. */
    uint32_t class_of_device;
    char name[32];
    bool ble, hid_service, name_complete;
    uint8_t address_type; /* LE public/random (0/1) or identity (2/3). Zero for BR/EDR. */
    uint16_t appearance;
    struct hid_pnp_id pnp;
};

/* Event buffers are valid during the callback. An empty descriptor clears
 * the layout. Report ID is supplied separately when declared by the peer. */
struct classic_hid_callbacks
{
    void (*status)(const char *stage, int detail);
    void (*descriptor)(const uint8_t *data, size_t length);
    void (*input)(uint8_t id, const uint8_t *data, size_t length);
    /* Optional model probe after SDP, before input or committing a preference.
     * A peer with an unavailable descriptor has length zero. NULL peer ends
     * this session. Return false to decline an unsupported connected peer. */
    bool (*probe)(const struct classic_hid_candidate *peer, const uint8_t *map, size_t length);
    /* Explicit dual-mode scan only, once per transport/address type/address
     * and on name or device metadata changes.
     * The one-based index stays valid until a new scan; copy peer if retaining it. */
    void (*discovered)(unsigned index, const struct classic_hid_candidate *peer);
};

/* Edgi-Talk board entry; the BSP provides its configured controller. */
int classic_hid_start(const struct classic_hid_callbacks *callbacks);
/* Portable entry for another board or explicit controller binding. */
int classic_hid_start_with_uart(const struct classic_hid_callbacks *callbacks,
                                const struct bluetooth_hci_uart *io);
int classic_hid_start_with_store(const struct classic_hid_callbacks *callbacks,
                                 const struct bluetooth_hci_uart *io,
                                 const struct classic_hid_store *store);
void classic_hid_get_status(struct classic_hid_status *out);
int classic_hid_reconnect(void); /* Resume connecting to the saved peer. */
int classic_hid_forget(void);    /* Erase the selected or most recent local binding. */
bool classic_hid_bonded_at(unsigned index, struct classic_hid_candidate *out, bool *has_key);
int classic_hid_forget_bonded(const struct classic_hid_candidate *peer);
int classic_hid_scan(void);
unsigned classic_hid_candidates(struct classic_hid_candidate *out, unsigned max);
bool classic_hid_candidate_at(unsigned index, struct classic_hid_candidate *out);
int classic_hid_connect(unsigned index); /* One-based index in the mixed scan list. */
int classic_hid_connect_peer(const struct classic_hid_candidate *peer);
int classic_hid_connect_address(const uint8_t address[6]); /* Display order. */
int classic_hid_pair(void);       /* Authenticate the selected ACL; connect also requests security. */
int classic_hid_disconnect(void); /* Suspend paging and inquiry, preserve the saved peer. */

int edgi_classic_gamepad_register(void);

#ifdef __cplusplus
}
#endif
#endif
