/* SPDX-License-Identifier: Apache-2.0 */
#ifndef HID_REPORT_DECODER_H
#define HID_REPORT_DECODER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
enum gamepad_direction
{
    HID_DPAD_UP = 1, HID_DPAD_RIGHT = 2, HID_DPAD_DOWN = 4, HID_DPAD_LEFT = 8
};

struct gamepad_control_name
{
    uint16_t page, usage;
    uint8_t report_id;
    const char *name;
};

struct gamepad_hat_position
{
    uint8_t value, directions;
};

struct gamepad_mapping
{
    const struct gamepad_control_name *controls;
    size_t control_count;
    const struct gamepad_hat_position *hat;
    size_t hat_count;
    uint8_t hat_report_id;
};

enum gamepad_input_kind { HID_INPUT_BUTTON, HID_INPUT_AXIS, HID_INPUT_RAW };

struct gamepad_input
{
    enum gamepad_input_kind kind;
    uint8_t report_id;
    uint16_t page, usage;
    const char *name; /* Static physical label or axis name, NULL if unknown. */
    uint32_t value;  /* Sign-extended for signed axes. */
    bool is_signed, relative;
    const uint8_t *raw;
    size_t length;
};

typedef void (*gamepad_emit_fn)(const struct gamepad_input *input, void *context);
void gamepad_decode_input(uint8_t id, const uint8_t *data, size_t length, bool show_raw,
                          gamepad_emit_fn emit, void *context);

void gamepad_set_report_map(const uint8_t *data, size_t length);
/* Optional physical labels, owned by the caller for the lifetime of the map.
 * Setting a new report descriptor returns the decoder to generic HID usages. */
void gamepad_set_mapping(const struct gamepad_mapping *mapping);
void gamepad_print_input(uint8_t id, const uint8_t *data, size_t length, bool show_raw);
void gamepad_print_event(const struct gamepad_input *input);

#ifdef __cplusplus
}
#endif
#endif
