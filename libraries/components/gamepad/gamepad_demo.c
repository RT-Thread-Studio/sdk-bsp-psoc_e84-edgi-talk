/* SPDX-License-Identifier: Apache-2.0 */
#include "gamepad.h"
#include <rtthread.h>
#include <stdlib.h>
#include <string.h>

struct sample
{
    enum gamepad_transport transport;
    struct gamepad_address address;
    struct gamepad_input input;
    uint8_t preview[32];
    unsigned dropped;
};

struct demo_event
{
    unsigned candidate_index; /* Zero for an input event. */
    union {
        struct sample sample;
        struct gamepad_peer peer;
    };
};

static rt_mq_t samples;
static rt_atomic_t refresh_list;

static void queue_input(const struct gamepad_peer *peer, const struct gamepad_model *model,
                        const struct gamepad_input *input, void *context)
{
    static unsigned dropped;
    struct demo_event event = { .sample = { .transport = peer->transport, .address = peer->address,
                                            .input = *input, .dropped = dropped } };
    (void)context;
    (void)model;
    if (input->kind == HID_INPUT_RAW && input->length)
        memcpy(event.sample.preview, input->raw, input->length < sizeof(event.sample.preview) ?
               input->length : sizeof(event.sample.preview));
    event.sample.input.raw = NULL;
    if (rt_mq_send(samples, &event, sizeof(event)) == RT_EOK)
        dropped = 0;
    else
        ++dropped;
}

static void print_address(const struct gamepad_address *a)
{
    const uint8_t *b = a->bytes;
    rt_kprintf("%02x:%02x:%02x:%02x:%02x:%02x", b[0], b[1], b[2], b[3], b[4], b[5]);
}

static const char *transport_name(enum gamepad_transport transport)
{
    return transport == HID_TRANSPORT_BLE ? "BLE" : "BR/EDR";
}

static void print_candidate(unsigned index, const struct gamepad_peer *peer)
{
    const struct gamepad_model *hint = gamepad_match(peer);
    rt_kprintf("%u  [%s]  ", index, transport_name(peer->transport));
    print_address(&peer->address);
    if (peer->transport == HID_TRANSPORT_BLE)
        rt_kprintf(" type=%u", peer->address.type);
    const char *match = gamepad_is_iine_app_peer(peer) ? "IINE app endpoint" : hint ? hint->name : "(none)";
    rt_kprintf("  %s  RSSI=%d  match=%s%s\n", peer->name[0] ? peer->name : "(unnamed)",
               (int)peer->rssi, match,
               gamepad_can_connect(peer->transport) ? "" : "  (scan only)");
}

static void show_list(void)
{
    struct gamepad_peer peer;
    unsigned i = 1;
    for (; gamepad_candidate(i, &peer); ++i)
        print_candidate(i, &peer);
    if (i == 1) rt_kprintf("No candidates yet. Put the controller in pairing mode.\n");
}

static void queue_candidate(unsigned index, const struct gamepad_peer *peer, void *context)
{
    (void)context;
    struct demo_event event = { .candidate_index = index, .peer = *peer };
    if (rt_mq_send(samples, &event, sizeof(event)) != RT_EOK)
        rt_atomic_store(&refresh_list, 1);
}

int gamepad_demo_run(const char *platform)
{
    int rc = gamepad_register_builtin_models();
    if (rc) return rc;
    samples = rt_mq_create("padinput", sizeof(struct demo_event), 48, RT_IPC_FLAG_FIFO);
    if (!samples) return -RT_ENOMEM;
    rc = gamepad_set_discovery_callback(queue_candidate, NULL);
    if (!rc) rc = gamepad_start(platform, queue_input, NULL);
    if (rc)
    {
        rt_kprintf("%s startup failed (%d)\n", platform, rc);
        rt_mq_delete(samples);
        return rc;
    }
    rt_kprintf("%s started. Use gamepad scan to find controllers.\n", platform);
    struct demo_event event;
    struct gamepad_address last;
    enum gamepad_transport last_transport = HID_TRANSPORT_BR_EDR;
    bool have_last = false;
    for (;;)
    {
        if (rt_mq_recv(samples, &event, sizeof(event), RT_WAITING_FOREVER) < 0) continue;
        if (rt_atomic_exchange(&refresh_list, 0))
            show_list();
        if (event.candidate_index)
        {
            struct gamepad_peer peer;
            /* A later scan may have reused this slot while UART was busy. */
            if (gamepad_candidate(event.candidate_index, &peer) && peer.transport == event.peer.transport &&
                peer.address.type == event.peer.address.type &&
                !memcmp(peer.address.bytes, event.peer.address.bytes, sizeof(peer.address.bytes)))
                print_candidate(event.candidate_index, &event.peer);
            continue;
        }
        struct sample *s = &event.sample;
        if (!have_last || last_transport != s->transport || last.type != s->address.type ||
            memcmp(last.bytes, s->address.bytes, sizeof(last.bytes)))
        {
            rt_kprintf("Input from [%s] ", transport_name(s->transport));
            print_address(&s->address);
            rt_kprintf(" (type %u)\n", s->address.type);
            last = s->address;
            last_transport = s->transport;
            have_last = true;
        }
        if (s->dropped) rt_kprintf("UART backlog: %u input events skipped\n", s->dropped);
        s->input.raw = s->preview;
        gamepad_print_event(&s->input);
    }
}

#ifdef RT_USING_MSH
static void show_bonds(void)
{
    struct gamepad_peer peer;
    bool has_key;
    unsigned i = 1;
    for (; gamepad_bonded(i, &peer, &has_key); ++i)
    {
        rt_kprintf("%u  [%s]  ", i, transport_name(peer.transport));
        print_address(&peer.address);
        if (peer.transport == HID_TRANSPORT_BLE)
            rt_kprintf(" type=%u", peer.address.type);
        rt_kprintf("  %s  %s%s\n", peer.name[0] ? peer.name : "(unnamed)",
                   has_key ? "bonded" : "pairing required",
                   gamepad_is_iine_app_peer(&peer) ? "  (IINE app endpoint)" : "");
    }
    if (i == 1) rt_kprintf("No saved peers. Use gamepad scan to find controllers.\n");
}

static unsigned parse_index(const char *arg)
{
    char *end;
    unsigned long index = strtoul(arg, &end, 10);
    return *arg >= '0' && *arg <= '9' && !*end && index <= 65535 ? (unsigned)index : 0;
}

static int hex_digit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int connect_target(int argc, char **argv)
{
    const char *arg = argv[2];
    if (!strchr(arg, ':'))
    {
        unsigned index = parse_index(arg);
        return argc == 3 && index ? gamepad_connect_index(index) : -RT_EINVAL;
    }
    struct gamepad_address address = {0};
    if (strlen(arg) != 17) return -RT_EINVAL;
    for (unsigned i = 0; i < 6; ++i)
    {
        int hi = hex_digit(arg[3*i]), lo = hex_digit(arg[3*i + 1]);
        if (hi < 0 || lo < 0 || (i < 5 && arg[3*i + 2] != ':')) return -RT_EINVAL;
        address.bytes[i] = (uint8_t)((hi << 4) | lo);
    }
    if (argc == 4)
    {
        if (strcmp(argv[3], "0") && strcmp(argv[3], "1")) return -RT_EINVAL;
        address.type = argv[3][0] - '0';
        const struct gamepad_peer peer = { .transport = HID_TRANSPORT_BLE, .address = address };
        return gamepad_connect_peer(&peer);
    }
    else
    {
        struct gamepad_peer peer, selected;
        bool found = false;
        for (unsigned i = 1; gamepad_candidate(i, &peer); ++i)
        {
            if (memcmp(address.bytes, peer.address.bytes, 6)) continue;
            if (found && (selected.transport != peer.transport || selected.address.type != peer.address.type))
            {
                rt_kprintf("Address occurs in multiple scan entries. Use the list index.\n");
                return -RT_EINVAL;
            }
            found = true;
            selected = peer;
        }
        if (found) return gamepad_connect_peer(&selected);
        if (!gamepad_can_connect(HID_TRANSPORT_BR_EDR))
        {
            rt_kprintf("Use gamepad scan or specify the BLE address type (0 public, 1 random).\n");
            return -RT_ENOENT;
        }
    }
    return gamepad_connect_address(&address);
}

static void show_address(const char *label, bool valid, enum gamepad_transport transport,
                         const struct gamepad_address *address)
{
    if (valid)
    {
        rt_kprintf("%s [%s] ", label, transport_name(transport));
        print_address(address);
        rt_kprintf(" (type %u)\n", address->type);
    }
}

static int gamepad(int argc, char **argv)
{
    int rc = 0;
    if ((argc == 3 || argc == 4) && !strcmp(argv[1], "connect"))
    {
        rc = connect_target(argc, argv);
        if (rc == -RT_ENOSYS)
            rt_kprintf("This transport is scan only in this firmware.\n");
    }
    else if (argc == 3 && !strcmp(argv[1], "raw") &&
             (!strcmp(argv[2], "on") || !strcmp(argv[2], "off")))
        gamepad_show_raw(!strcmp(argv[2], "on"));
    else if (argc == 3 && (!strcmp(argv[1], "reconnect") || !strcmp(argv[1], "forget")))
    {
        unsigned index = parse_index(argv[2]);
        if (!index) goto usage;
        rc = !strcmp(argv[1], "reconnect") ? gamepad_reconnect_index(index) : gamepad_forget_index(index);
        if (rc == -RT_EBUSY && !strcmp(argv[1], "reconnect"))
            rt_kprintf("Use gamepad disconnect and wait for the current link to close.\n");
    }
    else if (argc != 2) goto usage;
    else if (!strcmp(argv[1], "models"))
    {
        const struct gamepad_model *m;
        for (unsigned i = 0; (m = gamepad_model_at(i)) != NULL; ++i)
            rt_kprintf("%s\n", m->name);
    }
    else if (!strcmp(argv[1], "scan"))
    {
        rc = gamepad_scan();
        if (!rc) rt_kprintf("Scan requested. Devices appear here; use gamepad connect <index>.\n");
    }
    else if (!strcmp(argv[1], "list")) show_list();
    else if (!strcmp(argv[1], "bonds")) show_bonds();
    else if (!strcmp(argv[1], "pair")) rc = gamepad_pair();
    else if (!strcmp(argv[1], "disconnect")) rc = gamepad_disconnect();
    else if (!strcmp(argv[1], "reconnect")) rc = gamepad_reconnect();
    else if (!strcmp(argv[1], "forget")) rc = gamepad_forget();
    else if (!strcmp(argv[1], "status"))
    {
        struct gamepad_status s;
        gamepad_get_status(&s);
        rt_kprintf("HID link %s\n", s.connected ? "connected" : s.connecting ? "connecting" : "disconnected");
        show_address("Preferred", s.has_preferred, s.preferred_transport, &s.preferred);
        show_address("Target", s.has_target, s.transport, &s.target);
        show_address("Connected", s.connected, s.transport, &s.active);
        if (s.model_name)
            rt_kprintf("Model %s, input reports=%lu\n", s.model_name, (unsigned long)s.input_reports);
    }
    else goto usage;
    if (rc) rt_kprintf("gamepad %s failed (%d)\n", argv[1], rc);
    return rc;
usage:
    rt_kprintf("gamepad models|scan|list|bonds|connect <index|address [type]>|pair|disconnect|reconnect [index]|forget [index]|status|raw on|off\n");
    return -RT_EINVAL;
}
MSH_CMD_EXPORT(gamepad, Discover and connect HID gamepads);
#endif
