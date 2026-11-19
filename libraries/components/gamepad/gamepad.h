/* SPDX-License-Identifier: Apache-2.0 */
#ifndef HID_HOST_INPUT_API_H
#define HID_HOST_INPUT_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "gamepad_report.h"
#include "hid_pnp_id.h"

#ifdef __cplusplus
extern "C" {
#endif

enum gamepad_transport { HID_TRANSPORT_BR_EDR, HID_TRANSPORT_BLE };

struct gamepad_address
{
    uint8_t bytes[6]; /* Display order, most significant byte first. */
    uint8_t type;     /* BLE public/random address type; zero for Classic. */
};

struct gamepad_peer
{
    enum gamepad_transport transport;
    struct gamepad_address address;
    char name[32];
    int8_t rssi;
    uint32_t class_of_device;
    uint16_t appearance;
    bool hid_service;
    struct hid_pnp_id pnp;
};

struct gamepad_status
{
    enum gamepad_transport transport;
    bool connecting, connected, has_target, has_preferred;
    struct gamepad_address target, active, preferred;
    const char *model_name; /* Confirmed by the active peer's descriptor. */
    enum gamepad_transport preferred_transport;
    uint32_t input_reports; /* Accepted reports since this layout was installed. */
};

/* One host owns the controller at a time. Candidates have stable one-based
 * indices until the next scan. Commands may queue work on the host thread. */
struct gamepad_platform
{
    const char *name;
    enum gamepad_transport transport; /* Default for an unqualified direct address. */
    int (*start)(void);
    int (*scan)(void);
    bool (*candidate)(unsigned index, struct gamepad_peer *out);
    int (*connect)(const struct gamepad_address *address);
    int (*pair)(void);
    int (*disconnect)(void);
    int (*reconnect)(void);
    int (*forget)(void); /* Optional for a platform without stored preferences. */
    void (*status)(struct gamepad_status *out);
    bool dual_scan; /* Candidates may come from both BR/EDR and BLE. */
    /* Optional for a dual-mode host. The peer is a snapshot, including the
     * transport and address type. Copy it before queueing an async request. */
    int (*connect_peer)(const struct gamepad_peer *peer);
    bool (*bonded)(unsigned index, struct gamepad_peer *out, bool *has_key);
    int (*forget_peer)(const struct gamepad_peer *peer);
};

struct gamepad_model
{
    const char *name;
    unsigned priority; /* Higher wins; ties use ascending model name. */
    bool (*match)(const struct gamepad_peer *peer); /* Discovery hint only. */
    bool (*identify)(const struct gamepad_peer *peer, const uint8_t *map, size_t length);
    const struct gamepad_mapping *mapping;
    /* NULL uses the shared HID descriptor decoder. */
    void (*decode)(uint8_t id, const uint8_t *data, size_t length,
                   gamepad_emit_fn emit, void *context);
};

/* Register static tables before start. Input callbacks execute on the host
 * thread. Copy any data needed after the callback and avoid blocking it. */
typedef void (*gamepad_input_fn)(const struct gamepad_peer *peer, const struct gamepad_model *model,
                                 const struct gamepad_input *input, void *context);
typedef void (*gamepad_discovery_fn)(unsigned index, const struct gamepad_peer *peer, void *context);
int gamepad_register_platform(const struct gamepad_platform *platform);
int gamepad_register_model(const struct gamepad_model *model);
int gamepad_register_builtin_models(void);
/* Optional, before start. Reports new or updated candidates on the host thread. */
int gamepad_set_discovery_callback(gamepad_discovery_fn callback, void *context);
int gamepad_start(const char *platform, gamepad_input_fn input, void *context);
const struct gamepad_model *gamepad_model_at(unsigned index); /* Zero-based. */
const struct gamepad_model *gamepad_match(const struct gamepad_peer *peer);
bool gamepad_is_iine_app_peer(const struct gamepad_peer *peer);
int gamepad_scan(void);
bool gamepad_candidate(unsigned index, struct gamepad_peer *out);
bool gamepad_bonded(unsigned index, struct gamepad_peer *out, bool *has_key);
bool gamepad_can_connect(enum gamepad_transport transport);
int gamepad_connect_index(unsigned index);
int gamepad_connect_peer(const struct gamepad_peer *peer);
/* Direct connection on the platform's HID transport. */
int gamepad_connect_address(const struct gamepad_address *address);
int gamepad_pair(void);
int gamepad_disconnect(void);
int gamepad_reconnect(void);
int gamepad_reconnect_index(unsigned index);
int gamepad_forget_index(unsigned index);
int gamepad_forget(void);
void gamepad_get_status(struct gamepad_status *out);
void gamepad_show_raw(bool enabled);

/* Publish a candidate after inserting or updating its one-based scan slot. */
void gamepad_discovered(unsigned index, const struct gamepad_peer *peer);

/* Called serially by the active host. A new descriptor replaces any previous
 * layout. Pass a zero length for a confirmed HID endpoint without a map.
 * A false return means no enabled model accepted this endpoint. */
bool gamepad_descriptor(const struct gamepad_peer *peer, const uint8_t *map, size_t length);
void gamepad_report(const struct gamepad_peer *peer, uint8_t id, const uint8_t *data, size_t length);
void gamepad_detached(const struct gamepad_peer *peer);

/* Example entry: register the board platform first, then run the input loop. */
int gamepad_demo_run(const char *platform);
#ifdef __cplusplus
}
#endif
#endif
