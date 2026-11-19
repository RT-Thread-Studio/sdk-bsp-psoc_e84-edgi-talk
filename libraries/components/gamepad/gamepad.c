/* SPDX-License-Identifier: Apache-2.0 */
#include "gamepad.h"
#include <rtthread.h>
#include <rthw.h>
#include <string.h>

static const struct gamepad_platform *platforms[4];
static const struct gamepad_model *models[16];
static unsigned platform_count, model_count;
static const struct gamepad_platform *host;
static gamepad_input_fn listener;
static void *listener_context;
static gamepad_discovery_fn discovery_callback;
static void *discovery_context;
static rt_atomic_t raw_enabled;

/* The active host serializes descriptor, report and detach callbacks. */
static const struct gamepad_model *active_model;
static struct gamepad_peer active_peer;
static uint32_t input_reports;

int gamepad_register_platform(const struct gamepad_platform *p)
{
    if (!p || !p->name || !*p->name || !p->start || !p->scan || !p->candidate ||
        !p->connect || !p->pair || !p->disconnect || !p->reconnect || !p->status)
        return -RT_EINVAL;
    if (host) return -RT_EBUSY;
    for (unsigned i = 0; i < platform_count; ++i)
        if (!strcmp(p->name, platforms[i]->name))
            return p == platforms[i] ? 0 : -RT_EINVAL;
    if (platform_count == sizeof(platforms) / sizeof(platforms[0])) return -RT_EFULL;
    platforms[platform_count++] = p;
    return 0;
}

int gamepad_register_model(const struct gamepad_model *m)
{
    if (!m || !m->name || !*m->name || !m->match || !m->identify) return -RT_EINVAL;
    if (host) return -RT_EBUSY;
    unsigned pos = 0;
    for (unsigned i = 0; i < model_count; ++i)
    {
        int order = strcmp(models[i]->name, m->name);
        if (!order) return m == models[i] ? 0 : -RT_EINVAL;
        if (models[i]->priority > m->priority || (models[i]->priority == m->priority && order < 0))
            ++pos;
    }
    if (model_count == sizeof(models) / sizeof(models[0])) return -RT_EFULL;
    memmove(models + pos + 1, models + pos, (model_count - pos) * sizeof(models[0]));
    models[pos] = m;
    ++model_count;
    return 0;
}

int gamepad_register_builtin_models(void)
{
    int rc = 0;
#ifdef HID_HOST_MODEL_IINE_1001
    extern const struct gamepad_model gamepad_iine_1001;
    rc = gamepad_register_model(&gamepad_iine_1001);
#endif
#ifdef HID_HOST_MODEL_IINE_1003
    extern const struct gamepad_model gamepad_iine_1003;
    if (!rc) rc = gamepad_register_model(&gamepad_iine_1003);
#endif
#ifdef HID_HOST_MODEL_GENERIC_HID
    extern const struct gamepad_model gamepad_generic_hid;
    if (!rc) rc = gamepad_register_model(&gamepad_generic_hid);
#endif
    return rc;
}

int gamepad_set_discovery_callback(gamepad_discovery_fn callback, void *context)
{
    if (host) return -RT_EBUSY;
    discovery_callback = callback;
    discovery_context = context;
    return 0;
}

void gamepad_discovered(unsigned index, const struct gamepad_peer *peer)
{
    if (host && peer && index && discovery_callback &&
        (peer->transport == host->transport || (host->dual_scan &&
         (peer->transport == HID_TRANSPORT_BR_EDR || peer->transport == HID_TRANSPORT_BLE))))
        discovery_callback(index, peer, discovery_context);
}

int gamepad_start(const char *platform, gamepad_input_fn input, void *context)
{
    if (!platform || !input) return -RT_EINVAL;
    if (host) return -RT_EBUSY;
    if (!model_count) return -RT_ENOENT;
    for (unsigned i = 0; i < platform_count; ++i)
    {
        if (strcmp(platform, platforms[i]->name)) continue;
        host = platforms[i];
        listener = input;
        listener_context = context;
        int rc = host->start();
        if (rc)
        {
            active_model = NULL;
            listener = NULL;
            host = NULL;
        }
        return rc;
    }
    return -RT_ENOENT;
}

const struct gamepad_model *gamepad_model_at(unsigned index)
{
    return index < model_count ? models[index] : NULL;
}

bool gamepad_is_iine_app_peer(const struct gamepad_peer *peer)
{
    return peer && peer->transport == HID_TRANSPORT_BLE && !strcmp(peer->name, "IG05 Gamepad(BLE)");
}

const struct gamepad_model *gamepad_match(const struct gamepad_peer *peer)
{
    if (gamepad_is_iine_app_peer(peer)) return NULL;
    for (unsigned i = 0; peer && i < model_count; ++i)
        if (models[i]->match(peer)) return models[i];
    return NULL;
}

bool gamepad_candidate(unsigned index, struct gamepad_peer *out)
{
    return host && index && out && host->candidate(index, out);
}

bool gamepad_bonded(unsigned index, struct gamepad_peer *out, bool *has_key)
{
    return host && host->bonded && index && out && host->bonded(index, out, has_key);
}

bool gamepad_can_connect(enum gamepad_transport transport)
{
    return host && (host->transport == transport || (host->connect_peer &&
           (transport == HID_TRANSPORT_BR_EDR || transport == HID_TRANSPORT_BLE)));
}

int gamepad_connect_peer(const struct gamepad_peer *peer)
{
    if (!peer) return -RT_EINVAL;
    if (!gamepad_can_connect(peer->transport)) return -RT_ENOSYS;
    return host->connect_peer ? host->connect_peer(peer) : host->connect(&peer->address);
}

int gamepad_connect_index(unsigned index)
{
    struct gamepad_peer peer;
    if (!gamepad_candidate(index, &peer)) return -RT_ENOENT;
    return gamepad_connect_peer(&peer);
}

int gamepad_connect_address(const struct gamepad_address *address)
{
    if (!address || !host) return -RT_EINVAL;
    return host->connect(address);
}

int gamepad_scan(void)       { return host ? host->scan() : -RT_EBUSY; }
int gamepad_pair(void)       { return host ? host->pair() : -RT_EBUSY; }
int gamepad_disconnect(void) { return host ? host->disconnect() : -RT_EBUSY; }
int gamepad_reconnect(void)  { return host ? host->reconnect() : -RT_EBUSY; }
int gamepad_forget(void)     { return host && host->forget ? host->forget() : -RT_ENOSYS; }

int gamepad_reconnect_index(unsigned index)
{
    struct gamepad_peer peer;
    if (!host || !host->bonded) return -RT_ENOSYS;
    return gamepad_bonded(index, &peer, NULL) ? gamepad_connect_peer(&peer) : -RT_ENOENT;
}

int gamepad_forget_index(unsigned index)
{
    struct gamepad_peer peer;
    if (!host || !host->forget_peer) return -RT_ENOSYS;
    return gamepad_bonded(index, &peer, NULL) ? host->forget_peer(&peer) : -RT_ENOENT;
}

void gamepad_get_status(struct gamepad_status *out)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (host)
    {
        host->status(out);
        if (!host->connect_peer)
            out->transport = out->preferred_transport = host->transport;
        rt_base_t level = rt_hw_interrupt_disable();
        if (out->connected && active_model && active_peer.transport == out->transport &&
            active_peer.address.type == out->active.type &&
            !memcmp(active_peer.address.bytes, out->active.bytes, sizeof(out->active.bytes)))
        {
            out->model_name = active_model->name;
            out->input_reports = input_reports;
        }
        rt_hw_interrupt_enable(level);
    }
}

void gamepad_show_raw(bool enabled)
{
    rt_atomic_store(&raw_enabled, enabled);
}

static bool same_peer(const struct gamepad_peer *peer)
{
    return peer && active_model && peer->transport == active_peer.transport &&
           peer->address.type == active_peer.address.type &&
           !memcmp(peer->address.bytes, active_peer.address.bytes, sizeof(peer->address.bytes));
}

bool gamepad_descriptor(const struct gamepad_peer *peer, const uint8_t *map, size_t length)
{
    if (!peer || !gamepad_can_connect(peer->transport) || (!map && length)) return false;
    active_model = NULL;
    if (!length && gamepad_is_iine_app_peer(peer))
    {
        rt_kprintf("IG05 Gamepad(BLE) has no HID Report Map. Scan for the controller's gaming endpoint.\n");
        return false;
    }
    for (unsigned i = 0; i < model_count; ++i)
    {
        if (!models[i]->identify(peer, map, length)) continue;
        active_peer = *peer;
        input_reports = 0;
        gamepad_set_report_map(map, length);
        gamepad_set_mapping(models[i]->mapping);
        active_model = models[i];
        return true;
    }
    return false;
}

static void emit(const struct gamepad_input *input, void *context)
{
    (void)context;
    listener(&active_peer, active_model, input, listener_context);
}

void gamepad_report(const struct gamepad_peer *peer, uint8_t id, const uint8_t *data, size_t length)
{
    if (!same_peer(peer) || (!data && length)) return;
    ++input_reports;
    if (active_model->decode)
        active_model->decode(id, data, length, emit, NULL);
    else
        gamepad_decode_input(id, data, length, rt_atomic_load(&raw_enabled) != 0, emit, NULL);
}

void gamepad_detached(const struct gamepad_peer *peer)
{
    if (!same_peer(peer)) return;
    active_model = NULL;
    gamepad_set_report_map(NULL, 0);
}
