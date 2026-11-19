/* SPDX-License-Identifier: Apache-2.0 */
#include "gamepad_report.h"

#include <rtthread.h>
#include <stdbool.h>
#include <string.h>

/* Report IDs are carried by the Report Reference descriptor on BLE. Each ID
 * has its own bit cursor, including constant padding and vendor fields. */
#define MAX_FIELDS 96
#define MAX_USAGES 32
#define MAX_REPORT_BITS (512u * 8u)

struct field
{
    uint16_t offset;
    uint16_t page;
    uint16_t usage;
    uint8_t id;
    uint8_t width;
    bool is_signed;
    bool relative;
    bool seen;
    uint8_t held_directions;
    uint32_t previous;
};

struct globals
{
    uint32_t page, width, count;
    uint8_t id;
    bool is_signed;
};

struct usage_range
{
    uint32_t first, last;
};

struct usages
{
    struct usage_range range[MAX_USAGES];
    unsigned count;
    bool open;
};

static struct field fields[MAX_FIELDS];
static unsigned field_count;
static uint16_t report_bits[256];
static bool raw_report[256];
static uint8_t previous_raw[32];
static uint8_t previous_raw_id;
static size_t previous_raw_length = SIZE_MAX;
static const struct gamepad_mapping *mapping;

static uint32_t usage_at(const struct usages *local, unsigned index)
{
    for (unsigned i = 0; i < local->count; i++)
    {
        const struct usage_range *r = &local->range[i];
        uint32_t count = r->last - r->first + 1u;
        if (index < count)
            return r->first + index;
        index -= count;
    }
    /* HID repeats the final local Usage when Report Count is larger. */
    return local->count ? local->range[local->count - 1].last : 0;
}

static bool add_input(const struct globals *g, const struct usages *local, uint32_t flags)
{
    uint16_t offset = report_bits[g->id];
    if (!g->width || !g->count || g->count > (MAX_REPORT_BITS - offset) / g->width)
        return false;
    report_bits[g->id] += g->width * g->count;
    if (flags & 1)  /* constant padding */
        return true;

    /* Array selectors, buffered bytes and vendor reports retain their raw
     * values. Do not interpret an array (e.g. keyboard scancodes) as bits. */
    if (!(flags & 2) || (flags & 0x100) || g->width > 32)
    {
        raw_report[g->id] = true;
        return true;
    }
    for (unsigned i = 0; i < g->count; i++)
    {
        uint32_t full_usage = usage_at(local, i);
        unsigned page = full_usage >> 16;
        uint16_t usage = full_usage;
        bool axis = page == 1 && usage >= 0x30 && usage <= 0x39;
        bool simulation_axis = page == 2 && (usage == 0xc4 || usage == 0xc5);
        if (!axis && !simulation_axis && page != 9)
        {
            raw_report[g->id] = true;
            continue;
        }
        if (field_count == MAX_FIELDS)
            return false;
        fields[field_count++] = (struct field){
            .id = g->id, .offset = offset + i * g->width, .width = g->width,
            .page = page, .usage = usage, .is_signed = g->is_signed,
            .relative = (flags & 4) != 0,
        };
    }
    return true;
}

static bool parse_map(const uint8_t *data, size_t size)
{
    struct globals g = {0}, stack[4];
    struct usages local = {0};
    unsigned depth = 0;
    while (size)
    {
        uint8_t prefix = *data++;
        unsigned count = prefix & 3u;
        if (count == 3) count = 4;
        size--;
        if (prefix == 0xfe || count > size)
            return false;
        uint32_t value = 0;
        for (unsigned i = 0; i < count; i++)
            value |= (uint32_t)data[i] << (8 * i);
        data += count;
        size -= count;
        uint32_t usage = count == 4 ? value : (g.page << 16) | value;
        switch (prefix & 0xfc)
        {
        case 0x04: /* Usage Page */
            if (value > 0xffff) return false;
            g.page = value;
            break;
        case 0x14: /* Logical Minimum sets the signedness of the input */
            g.is_signed = count && (value & (1u << (8 * count - 1)));
            break;
        case 0x74: g.width = value; break;
        case 0x84:
            if (!value || value > 255) return false;
            g.id = value;
            break;
        case 0x94: g.count = value; break;
        case 0xa4: /* Push */
            if (depth == sizeof(stack) / sizeof(stack[0])) return false;
            stack[depth++] = g;
            break;
        case 0xb4: /* Pop */
            if (!depth) return false;
            g = stack[--depth];
            break;
        case 0x08: /* Usage */
        case 0x18: /* Usage Minimum */
            if (local.count == MAX_USAGES || local.open) return false;
            local.range[local.count++] = (struct usage_range){usage, usage};
            local.open = (prefix & 0xfc) == 0x18;
            break;
        case 0x28: /* Usage Maximum */
            if (!local.open) return false;
            if (usage < local.range[local.count - 1].first ||
                (usage >> 16) != (local.range[local.count - 1].first >> 16))
                return false;
            local.range[local.count - 1].last = usage;
            local.open = false;
            break;
        case 0xa8: /* Alternative usage sets are not decoded */
            return false;
        case 0x80: /* Input */
            if (!add_input(&g, &local, value)) return false;
            break;
        }
        if ((prefix & 0x0c) == 0) /* Local state expires on every Main item. */
        {
            if (local.open) return false;
            local.count = 0;
        }
    }
    return !depth && !local.open;
}

void gamepad_set_report_map(const uint8_t *data, size_t length)
{
    field_count = 0;
    mapping = NULL;
    previous_raw_length = SIZE_MAX;
    memset(report_bits, 0, sizeof(report_bits));
    memset(raw_report, 0, sizeof(raw_report));
    if (length && !parse_map(data, length))
    {
        field_count = 0;
        rt_kprintf("HID map unsupported; displaying raw input\n");
    }
}

void gamepad_set_mapping(const struct gamepad_mapping *new_mapping)
{
    if (mapping == new_mapping)
        return;
    mapping = new_mapping;
    previous_raw_length = SIZE_MAX;
    for (unsigned i = 0; i < field_count; ++i)
    {
        fields[i].seen = false;
        fields[i].held_directions = 0;
    }
}

static const char *control_name(const struct field *f)
{
    if (mapping)
        for (size_t i = 0; i < mapping->control_count; ++i)
        {
            const struct gamepad_control_name *n = &mapping->controls[i];
            if (n->report_id == f->id && n->page == f->page && n->usage == f->usage)
                return n->name;
        }
    return NULL;
}

static bool emit_hat_keys(struct field *f, uint32_t value, gamepad_emit_fn emit, void *context)
{
    static const char *const names[] = {"Up", "Right", "Down", "Left"};
    if (!mapping || f->id != mapping->hat_report_id)
        return false;
    for (size_t i = 0; i < mapping->hat_count; ++i)
    {
        const struct gamepad_hat_position *position = &mapping->hat[i];
        if (position->value != value)
            continue;
        uint8_t changed = f->held_directions ^ position->directions;
        for (unsigned j = 0; j < sizeof(names) / sizeof(names[0]); ++j)
        {
            unsigned mask = 1u << j;
            if (changed & mask)
            {
                const struct gamepad_input key = {
                    .kind = HID_INPUT_BUTTON, .report_id = f->id, .page = f->page,
                    .usage = f->usage, .name = names[j], .value = !!(position->directions & mask)
                };
                emit(&key, context);
            }
        }
        f->held_directions = position->directions;
        return true;
    }
    return false;
}

static void emit_raw(uint8_t id, const uint8_t *data, size_t length, bool every_packet,
                     gamepad_emit_fn emit, void *context)
{
    /* Some gamepads stream the idle state continuously. Compare only when
     * the entire report fits, so changes beyond the preview are never lost. */
    if (length <= sizeof(previous_raw))
    {
        if (!every_packet && previous_raw_length == length && previous_raw_id == id &&
            (!length || memcmp(previous_raw, data, length) == 0))
            return;
        if (length)
            memcpy(previous_raw, data, length);
    }
    previous_raw_id = id;
    previous_raw_length = length;
    const struct gamepad_input input = {
        .kind = HID_INPUT_RAW, .report_id = id, .raw = data, .length = length
    };
    emit(&input, context);
}

void gamepad_decode_input(uint8_t id, const uint8_t *data, size_t length, bool show_raw,
                          gamepad_emit_fn emit, void *context)
{
    static const char *const axes[] = {
        "X", "Y", "Z", "Rx", "Ry", "Rz", "slider", "dial", "wheel", "hat"
    };
    if (!length || length < (report_bits[id] + 7u) / 8u)
    {
        emit_raw(id, data, length, show_raw, emit, context);
        return;
    }

    bool decoded = false;
    bool unmapped_axis = false;
    for (unsigned i = 0; i < field_count; i++)
    {
        struct field *f = &fields[i];
        if (f->id != id) continue;
        const char *name = control_name(f);
        if (f->page == 2 && !name)
        {
            /* Preserve the raw fallback for unprofiled simulation controls. */
            if (!mapping)
            {
                unmapped_axis = true;
                continue;
            }
            name = f->usage == 0xc4 ? "Accelerator" : "Brake";
        }
        decoded = true;
        uint32_t value = 0;
        for (unsigned bit = 0; bit < f->width; bit++)
        {
            unsigned at = f->offset + bit;
            value |= (uint32_t)((data[at / 8] >> (at % 8)) & 1u) << bit;
        }
        /* Equal absolute samples repeat state. Equal nonzero relative
         * samples (e.g. wheel movement) represent additional motion. */
        if (f->relative ? value == 0 : (f->seen && f->previous == value))
            continue;
        bool first_sample = !f->seen;
        f->seen = true;
        f->previous = value;
        struct gamepad_input input = {
            .report_id = id, .page = f->page, .usage = f->usage, .value = value,
            .name = name, .is_signed = f->is_signed, .relative = f->relative
        };
        if (f->page == 9)
        {
            if (mapping && first_sample && !value)
                continue;
            input.kind = HID_INPUT_BUTTON;
        }
        else if (f->page == 1 && f->usage == 0x39 && emit_hat_keys(f, value, emit, context))
            continue;
        else
        {
            input.kind = HID_INPUT_AXIS;
            input.name = name ? name : axes[f->usage - 0x30];
            if (f->is_signed && f->width < 32 && (value & (1u << (f->width - 1))))
                input.value |= UINT32_MAX << f->width;
        }
        emit(&input, context);
    }
    if (show_raw || !decoded || raw_report[id] || unmapped_axis)
        emit_raw(id, data, length, show_raw, emit, context);
}

void gamepad_print_event(const struct gamepad_input *input)
{
    uint8_t id = input->report_id;
    if (input->kind == HID_INPUT_RAW)
    {
        rt_kprintf("report=%u raw[%u]", id, (unsigned)input->length);
        for (size_t i = 0; i < input->length && i < 32; ++i)
            rt_kprintf(" %02x", input->raw[i]);
        rt_kprintf("%s\n", input->length > 32 ? " ..." : "");
    }
    else if (input->kind == HID_INPUT_BUTTON)
    {
        if (input->name)
            rt_kprintf("report=%u key=%s %s\n", id, input->name, input->value ? "down" : "up");
        else
            rt_kprintf("report=%u button=%u %s\n", id, input->usage, input->value ? "down" : "up");
    }
    else if (input->is_signed)
        rt_kprintf("report=%u %s%s=%ld\n", id, input->name, input->relative ? "_delta" : "",
                   (long)(int32_t)input->value);
    else
        rt_kprintf("report=%u %s%s=%lu\n", id, input->name, input->relative ? "_delta" : "",
                   (unsigned long)input->value);
}

static void print_event(const struct gamepad_input *input, void *context)
{
    (void)context;
    gamepad_print_event(input);
}

void gamepad_print_input(uint8_t id, const uint8_t *data, size_t length, bool show_raw)
{
    gamepad_decode_input(id, data, length, show_raw, print_event, NULL);
}
