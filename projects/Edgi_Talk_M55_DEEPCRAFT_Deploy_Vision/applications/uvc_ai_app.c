#include <rtthread.h>
#include <string.h>
#include <stdio.h>

#include "uvc_ai_app.h"
#include "uvc_ai.h"
#include "usbh_uvc_display_hook.h"
#include "usb_config.h"

#undef USB_DBG_TAG
#define USB_DBG_TAG "uvc_ai_app"
#include "usb_log.h"

#define UVC_RGB565(r, g, b) (uint16_t)((((uint16_t)(r) & 0xF8U) << 8) | (((uint16_t)(g) & 0xFCU) << 3) | ((uint16_t)(b) >> 3))

#define UVC_AI_BOX_THICKNESS       2U
#define UVC_AI_FONT_WIDTH          5U
#define UVC_AI_FONT_HEIGHT         7U
#define UVC_AI_FONT_SPACING        1U
#define UVC_AI_TEXT_PADDING_X      2U
#define UVC_AI_TEXT_PADDING_Y      2U
#define UVC_AI_LABEL_MAX_LEN       32U
#define UVC_AI_FRAME_BYTES         ((uint32_t)UVC_AI_CAMERA_WIDTH * UVC_AI_CAMERA_HEIGHT * 2U)
#define UVC_AI_WORKER_STACK_SIZE   65536U
#define UVC_AI_DTCM_SECTION        __attribute__((section(".cy_dtcm")))
/*
 * Keep AI worker close to official demo behavior (high-priority inference task),
 * so NPU completion callback can be handled without long scheduler latency.
 */
#define UVC_AI_WORKER_PRIORITY     8U
#define UVC_AI_WORKER_TICK         5U
#define UVC_AI_WORKER_NAME         "uvc_aiwk"

typedef struct {
    char ch;
    uint8_t rows[UVC_AI_FONT_HEIGHT];
} uvc_font5x7_t;

typedef struct {
    rt_bool_t started;
    rt_bool_t ai_ready;
    rt_bool_t worker_running;
    rt_bool_t worker_busy;
    uint32_t frame_counter;
    uint16_t src_width;
    uint16_t src_height;
    rt_thread_t worker_thread;
    rt_sem_t worker_sem;
    uint8_t *worker_frame_buf;
    uvc_ai_result_t latest_result;
} uvc_ai_app_ctx_t;

static uvc_ai_app_ctx_t g_uvc_ai_app;
/* Put AI worker stack in DTCM to reduce VLA-heavy postprocess latency. */
static UVC_AI_DTCM_SECTION rt_uint8_t g_uvc_ai_worker_stack[UVC_AI_WORKER_STACK_SIZE];
static struct rt_thread g_uvc_ai_worker_tcb;

static const uint16_t g_uvc_ai_class_colors[UVC_AI_NUM_CLASSES] = {
    UVC_RGB565(227, 66, 24),  /* class 0 */
    UVC_RGB565(8, 24, 168),   /* class 1 */
    UVC_RGB565(0, 120, 0),    /* class 2 */
};
static const uint16_t g_uvc_ai_unknown_color = UVC_RGB565(255, 215, 0);
static const uint16_t g_uvc_ai_text_color = UVC_RGB565(255, 255, 255);
static const uint16_t g_uvc_ai_model_bg = UVC_RGB565(0, 0, 0);

static const uvc_font5x7_t g_uvc_font5x7[] = {
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {'%', {0x19, 0x19, 0x02, 0x04, 0x08, 0x13, 0x13}},
    {'-', {0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04}},
    {':', {0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00}},
    {'?', {0x0e, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}},
    {'0', {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e}},
    {'1', {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e}},
    {'2', {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f}},
    {'3', {0x1e, 0x01, 0x01, 0x0e, 0x01, 0x01, 0x1e}},
    {'4', {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02}},
    {'5', {0x1f, 0x10, 0x1e, 0x01, 0x01, 0x11, 0x0e}},
    {'6', {0x06, 0x08, 0x10, 0x1e, 0x11, 0x11, 0x0e}},
    {'7', {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e}},
    {'9', {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x02, 0x0c}},
    {'A', {0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}},
    {'B', {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e}},
    {'C', {0x0e, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0e}},
    {'D', {0x1c, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1c}},
    {'E', {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f}},
    {'F', {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10}},
    {'G', {0x0e, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0e}},
    {'H', {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}},
    {'I', {0x0e, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e}},
    {'J', {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0e}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f}},
    {'M', {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11}},
    {'O', {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}},
    {'P', {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10}},
    {'Q', {0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d}},
    {'R', {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11}},
    {'S', {0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e}},
    {'T', {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0a}},
    {'X', {0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f}},
};

static inline int16_t uvc_ai_app_clamp_i16(int32_t value, int32_t min_v, int32_t max_v)
{
    if (value < min_v) {
        value = min_v;
    }

    if (value > max_v) {
        value = max_v;
    }

    return (int16_t)value;
}

static void uvc_ai_app_publish_result(const uvc_ai_result_t *result)
{
    rt_base_t level;

    if (result == RT_NULL) {
        return;
    }

    level = rt_hw_interrupt_disable();
    memcpy(&g_uvc_ai_app.latest_result, result, sizeof(g_uvc_ai_app.latest_result));
    rt_hw_interrupt_enable(level);
}

static void uvc_ai_app_snapshot_result(uvc_ai_result_t *result)
{
    rt_base_t level;

    if (result == RT_NULL) {
        return;
    }

    level = rt_hw_interrupt_disable();
    memcpy(result, &g_uvc_ai_app.latest_result, sizeof(*result));
    rt_hw_interrupt_enable(level);
}

static inline char uvc_ai_app_to_upper_ascii(char ch)
{
    if ((ch >= 'a') && (ch <= 'z')) {
        return (char)(ch - ('a' - 'A'));
    }

    return ch;
}

static const uint8_t *uvc_ai_app_get_glyph5x7(char ch)
{
    uint32_t i;
    uint32_t glyph_count = sizeof(g_uvc_font5x7) / sizeof(g_uvc_font5x7[0]);
    char upper = uvc_ai_app_to_upper_ascii(ch);

    for (i = 0U; i < glyph_count; i++) {
        if (g_uvc_font5x7[i].ch == upper) {
            return g_uvc_font5x7[i].rows;
        }
    }

    return g_uvc_font5x7[5].rows; /* '?' */
}

static void uvc_ai_app_fill_rect(const uvc_display_overlay_info_t *info,
                                 int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                 uint16_t color)
{
    int16_t x_start = x0;
    int16_t y_start = y0;
    int16_t x_end = x1;
    int16_t y_end = y1;
    int16_t y;

    if ((info == RT_NULL) || (info->framebuffer == RT_NULL)) {
        return;
    }

    if (x_start > x_end) {
        int16_t tmp = x_start;
        x_start = x_end;
        x_end = tmp;
    }

    if (y_start > y_end) {
        int16_t tmp = y_start;
        y_start = y_end;
        y_end = tmp;
    }

    x_start = uvc_ai_app_clamp_i16(x_start, 0, (int32_t)info->lcd_width - 1);
    y_start = uvc_ai_app_clamp_i16(y_start, 0, (int32_t)info->lcd_height - 1);
    x_end = uvc_ai_app_clamp_i16(x_end, 0, (int32_t)info->lcd_width - 1);
    y_end = uvc_ai_app_clamp_i16(y_end, 0, (int32_t)info->lcd_height - 1);
    if ((x_start > x_end) || (y_start > y_end)) {
        return;
    }

    for (y = y_start; y <= y_end; y++) {
        uint16_t *row = info->framebuffer + (uint32_t)y * info->lcd_width + (uint16_t)x_start;
        int16_t x;

        for (x = x_start; x <= x_end; x++) {
            *row++ = color;
        }
    }
}

static void uvc_ai_app_draw_hline(const uvc_display_overlay_info_t *info,
                                  int16_t x0, int16_t x1, int16_t y, uint16_t color)
{
    int16_t x_start = x0;
    int16_t x_end = x1;
    int16_t x;
    uint16_t *row;

    if ((info == RT_NULL) || (info->framebuffer == RT_NULL) ||
        (y < 0) || (y >= info->lcd_height)) {
        return;
    }

    if (x_start > x_end) {
        int16_t tmp = x_start;
        x_start = x_end;
        x_end = tmp;
    }

    x_start = uvc_ai_app_clamp_i16(x_start, 0, (int32_t)info->lcd_width - 1);
    x_end = uvc_ai_app_clamp_i16(x_end, 0, (int32_t)info->lcd_width - 1);
    if (x_start > x_end) {
        return;
    }

    row = info->framebuffer + (uint32_t)y * info->lcd_width + (uint16_t)x_start;
    for (x = x_start; x <= x_end; x++) {
        *row++ = color;
    }
}

static void uvc_ai_app_draw_vline(const uvc_display_overlay_info_t *info,
                                  int16_t x, int16_t y0, int16_t y1, uint16_t color)
{
    int16_t y_start = y0;
    int16_t y_end = y1;
    int16_t y;

    if ((info == RT_NULL) || (info->framebuffer == RT_NULL) ||
        (x < 0) || (x >= info->lcd_width)) {
        return;
    }

    if (y_start > y_end) {
        int16_t tmp = y_start;
        y_start = y_end;
        y_end = tmp;
    }

    y_start = uvc_ai_app_clamp_i16(y_start, 0, (int32_t)info->lcd_height - 1);
    y_end = uvc_ai_app_clamp_i16(y_end, 0, (int32_t)info->lcd_height - 1);
    if (y_start > y_end) {
        return;
    }

    for (y = y_start; y <= y_end; y++) {
        info->framebuffer[(uint32_t)y * info->lcd_width + (uint16_t)x] = color;
    }
}

static void uvc_ai_app_draw_rect(const uvc_display_overlay_info_t *info,
                                 int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                                 uint16_t color, uint8_t thickness)
{
    uint8_t i;
    int16_t x_start = x0;
    int16_t y_start = y0;
    int16_t x_end = x1;
    int16_t y_end = y1;

    if (x_start > x_end) {
        int16_t tmp = x_start;
        x_start = x_end;
        x_end = tmp;
    }
    if (y_start > y_end) {
        int16_t tmp = y_start;
        y_start = y_end;
        y_end = tmp;
    }

    for (i = 0U; i < thickness; i++) {
        int16_t left = (int16_t)(x_start + i);
        int16_t right = (int16_t)(x_end - i);
        int16_t top = (int16_t)(y_start + i);
        int16_t bottom = (int16_t)(y_end - i);

        if ((left > right) || (top > bottom)) {
            break;
        }

        uvc_ai_app_draw_hline(info, left, right, top, color);
        uvc_ai_app_draw_hline(info, left, right, bottom, color);
        uvc_ai_app_draw_vline(info, left, top, bottom, color);
        uvc_ai_app_draw_vline(info, right, top, bottom, color);
    }
}

static void uvc_ai_app_draw_char5x7(const uvc_display_overlay_info_t *info,
                                    int16_t x, int16_t y, char ch, uint16_t color)
{
    const uint8_t *rows = uvc_ai_app_get_glyph5x7(ch);
    uint32_t row;

    if ((info == RT_NULL) || (info->framebuffer == RT_NULL)) {
        return;
    }

    for (row = 0U; row < UVC_AI_FONT_HEIGHT; row++) {
        int16_t py = (int16_t)(y + (int16_t)row);
        uint16_t *dst_row;
        uint8_t bits = rows[row];
        uint32_t col;

        if ((py < 0) || (py >= info->lcd_height)) {
            continue;
        }

        dst_row = info->framebuffer + (uint32_t)py * info->lcd_width;
        for (col = 0U; col < UVC_AI_FONT_WIDTH; col++) {
            if ((bits & (uint8_t)(1U << (UVC_AI_FONT_WIDTH - 1U - col))) != 0U) {
                int16_t px = (int16_t)(x + (int16_t)col);

                if ((px >= 0) && (px < info->lcd_width)) {
                    dst_row[px] = color;
                }
            }
        }
    }
}

static void uvc_ai_app_draw_text5x7(const uvc_display_overlay_info_t *info,
                                    int16_t x, int16_t y, const char *text, uint16_t color)
{
    int16_t cursor_x = x;

    if (text == RT_NULL) {
        return;
    }

    while (*text != '\0') {
        uvc_ai_app_draw_char5x7(info, cursor_x, y, *text, color);
        cursor_x = (int16_t)(cursor_x + UVC_AI_FONT_WIDTH + UVC_AI_FONT_SPACING);
        text++;
    }
}

static uint16_t uvc_ai_app_text_width5x7(const char *text)
{
    uint32_t len;

    if ((text == RT_NULL) || (*text == '\0')) {
        return 0U;
    }

    len = (uint32_t)strlen(text);
    return (uint16_t)(len * (UVC_AI_FONT_WIDTH + UVC_AI_FONT_SPACING) - UVC_AI_FONT_SPACING);
}

static int16_t uvc_ai_app_map_x_to_lcd(const uvc_display_overlay_info_t *info, int16_t ai_x)
{
    int32_t x;

    if ((info == RT_NULL) || (info->src_width == 0U) || (info->dst_width == 0U)) {
        return 0;
    }

    x = uvc_ai_app_clamp_i16(ai_x, 0, (int32_t)UVC_AI_IMAGE_WIDTH - 1);
    x = (x * (int32_t)info->src_width) / (int32_t)UVC_AI_IMAGE_WIDTH;
    x = (x * (int32_t)info->dst_width) / (int32_t)info->src_width;

    return uvc_ai_app_clamp_i16(x, 0, (int32_t)info->lcd_width - 1);
}

static int16_t uvc_ai_app_map_y_to_lcd(const uvc_display_overlay_info_t *info, int16_t ai_y)
{
    int32_t y;

    if ((info == RT_NULL) || (info->src_height == 0U) || (info->dst_height == 0U)) {
        return 0;
    }

    y = uvc_ai_app_clamp_i16(ai_y, 0, (int32_t)UVC_AI_IMAGE_HEIGHT - 1);
    y = (y * (int32_t)info->src_height) / (int32_t)UVC_AI_IMAGE_HEIGHT;
    y = (y * (int32_t)info->dst_height) / (int32_t)info->src_height + (int32_t)info->dst_y_offset;

    return uvc_ai_app_clamp_i16(y, 0, (int32_t)info->lcd_height - 1);
}

static void uvc_ai_app_draw_text_box(const uvc_display_overlay_info_t *info,
                                     int16_t x, int16_t y, const char *text,
                                     uint16_t bg_color, uint16_t fg_color)
{
    uint16_t text_w = uvc_ai_app_text_width5x7(text);
    uint16_t box_w;
    uint16_t box_h;
    int16_t x0;
    int16_t y0;
    int16_t x1;
    int16_t y1;
    int16_t min_y;
    int16_t max_y;

    if ((info == RT_NULL) || (text_w == 0U)) {
        return;
    }

    box_w = (uint16_t)(text_w + UVC_AI_TEXT_PADDING_X * 2U);
    box_h = (uint16_t)(UVC_AI_FONT_HEIGHT + UVC_AI_TEXT_PADDING_Y * 2U);

    x0 = x;
    y0 = y;

    if ((int32_t)x0 + (int32_t)box_w > info->lcd_width) {
        x0 = (int16_t)(info->lcd_width - box_w);
    }
    if (x0 < 0) {
        x0 = 0;
    }

    min_y = (int16_t)info->dst_y_offset;
    max_y = (int16_t)(info->dst_y_offset + info->dst_height);
    if (max_y > info->lcd_height) {
        max_y = info->lcd_height;
    }
    max_y = (int16_t)(max_y - box_h);

    if (y0 < min_y) {
        y0 = min_y;
    }
    if (y0 > max_y) {
        y0 = max_y;
    }

    x1 = (int16_t)(x0 + box_w - 1);
    y1 = (int16_t)(y0 + box_h - 1);

    uvc_ai_app_fill_rect(info, x0, y0, x1, y1, bg_color);
    uvc_ai_app_draw_text5x7(info,
                            (int16_t)(x0 + UVC_AI_TEXT_PADDING_X),
                            (int16_t)(y0 + UVC_AI_TEXT_PADDING_Y),
                            text,
                            fg_color);
}

static void uvc_ai_app_worker_thread_entry(void *parameter)
{
    uvc_ai_result_t result;
    int ret;

    (void)parameter;

    while (g_uvc_ai_app.worker_running) {
        if (rt_sem_take(g_uvc_ai_app.worker_sem, RT_WAITING_FOREVER) != RT_EOK) {
            continue;
        }

        if (!g_uvc_ai_app.worker_running) {
            break;
        }

        if ((!g_uvc_ai_app.ai_ready) || (g_uvc_ai_app.worker_frame_buf == RT_NULL)) {
            g_uvc_ai_app.worker_busy = RT_FALSE;
            continue;
        }

        ret = uvc_ai_process_yuyv(g_uvc_ai_app.worker_frame_buf,
                                  UVC_AI_FRAME_BYTES,
                                  &result);
        if (ret == RT_EOK) {
            uvc_ai_app_publish_result(&result);
        } else {
            USB_LOG_WRN("AI processing failed\r\n");
            uvc_ai_reset_result(&result);
            uvc_ai_app_publish_result(&result);
        }

        g_uvc_ai_app.worker_busy = RT_FALSE;
    }
}

static void uvc_ai_app_overlay_cb(const uvc_display_overlay_info_t *info, void *user_ctx)
{
    uvc_ai_result_t result_snapshot;
    const uvc_ai_result_t *result = &result_snapshot;
    uint8_t det_count;
    uint32_t i;
    char model_text[24];

    (void)user_ctx;

    if ((info == RT_NULL) || (!g_uvc_ai_app.started) || (!g_uvc_ai_app.ai_ready)) {
        return;
    }

    uvc_ai_app_snapshot_result(&result_snapshot);
    if (!result->valid) {
        return;
    }

    det_count = result->count;
    if (det_count > UVC_AI_MAX_PREDICTIONS) {
        det_count = UVC_AI_MAX_PREDICTIONS;
    }

    for (i = 0U; i < det_count; i++) {
        const int16_t *bbox = &result->bbox_int16[i * 4U];
        int16_t x0 = uvc_ai_app_map_x_to_lcd(info, bbox[0]);
        int16_t y0 = uvc_ai_app_map_y_to_lcd(info, bbox[1]);
        int16_t x1 = uvc_ai_app_map_x_to_lcd(info, bbox[2]);
        int16_t y1 = uvc_ai_app_map_y_to_lcd(info, bbox[3]);
        uint16_t color;
        char label_text[UVC_AI_LABEL_MAX_LEN];
        int16_t label_y;

        if (x0 > x1) {
            int16_t tmp = x0;
            x0 = x1;
            x1 = tmp;
        }
        if (y0 > y1) {
            int16_t tmp = y0;
            y0 = y1;
            y1 = tmp;
        }

        if (result->class_id[i] < UVC_AI_NUM_CLASSES) {
            color = g_uvc_ai_class_colors[result->class_id[i]];
        } else {
            color = g_uvc_ai_unknown_color;
        }

        uvc_ai_app_draw_rect(info, x0, y0, x1, y1, color, UVC_AI_BOX_THICKNESS);

        (void)snprintf(label_text, sizeof(label_text), "%s %.0f%%",
                       result->class_string[i], result->conf[i] * 100.0f);
        label_y = (int16_t)(y0 - (int16_t)(UVC_AI_FONT_HEIGHT + UVC_AI_TEXT_PADDING_Y * 2U + 2U));
        if (label_y < (int16_t)info->dst_y_offset) {
            label_y = (int16_t)(y0 + 2);
        }
        uvc_ai_app_draw_text_box(info, x0, label_y, label_text, color, g_uvc_ai_text_color);
    }

    (void)snprintf(model_text, sizeof(model_text), "Model %.1f ms", result->inference_ms);
    uvc_ai_app_draw_text_box(info, 4, (int16_t)(info->dst_y_offset + 4U),
                             model_text, g_uvc_ai_model_bg, g_uvc_ai_text_color);
}

void uvc_ai_app_enforce_mode(uint8_t *fmt, uint16_t *width, uint16_t *height)
{
    if ((fmt == RT_NULL) || (width == RT_NULL) || (height == RT_NULL)) {
        return;
    }

    if (*fmt != USBH_VIDEO_FORMAT_UNCOMPRESSED) {
        USB_LOG_WRN("AI mode requires YUYV, force fmt=0\r\n");
        *fmt = USBH_VIDEO_FORMAT_UNCOMPRESSED;
    }

    if ((*width != UVC_AI_CAMERA_WIDTH) || (*height != UVC_AI_CAMERA_HEIGHT)) {
        USB_LOG_WRN("AI mode requires 320x240, force %ux%u\r\n",
                    (unsigned)UVC_AI_CAMERA_WIDTH, (unsigned)UVC_AI_CAMERA_HEIGHT);
        *width = UVC_AI_CAMERA_WIDTH;
        *height = UVC_AI_CAMERA_HEIGHT;
    }
}

int uvc_ai_app_start(uint16_t src_width, uint16_t src_height)
{
    uvc_ai_config_t config;
    int ret;
    rt_err_t thread_ret;

    memset(&g_uvc_ai_app, 0, sizeof(g_uvc_ai_app));
    g_uvc_ai_app.src_width = src_width ? src_width : UVC_AI_CAMERA_WIDTH;
    g_uvc_ai_app.src_height = src_height ? src_height : UVC_AI_CAMERA_HEIGHT;
    g_uvc_ai_app.started = RT_TRUE;

    uvc_ai_reset_result(&g_uvc_ai_app.latest_result);

    config.initialized = 1U;
    config.src_width = g_uvc_ai_app.src_width;
    config.src_height = g_uvc_ai_app.src_height;

    ret = uvc_ai_init(&config);
    if (ret != RT_EOK) {
        USB_LOG_ERR("AI init failed, render-only mode\r\n");
        return RT_EOK;
    }

    g_uvc_ai_app.ai_ready = RT_TRUE;
    g_uvc_ai_app.worker_frame_buf = rt_malloc(UVC_AI_FRAME_BYTES);
    if (g_uvc_ai_app.worker_frame_buf != RT_NULL) {
        g_uvc_ai_app.worker_sem = rt_sem_create("uvc_aiwk", 0, RT_IPC_FLAG_FIFO);
        if (g_uvc_ai_app.worker_sem != RT_NULL) {
            g_uvc_ai_app.worker_running = RT_TRUE;
            g_uvc_ai_app.worker_busy = RT_FALSE;
            thread_ret = rt_thread_init(&g_uvc_ai_worker_tcb,
                                        UVC_AI_WORKER_NAME,
                                        uvc_ai_app_worker_thread_entry,
                                        RT_NULL,
                                        g_uvc_ai_worker_stack,
                                        sizeof(g_uvc_ai_worker_stack),
                                        UVC_AI_WORKER_PRIORITY,
                                        UVC_AI_WORKER_TICK);
            if (thread_ret == RT_EOK) {
                g_uvc_ai_app.worker_thread = &g_uvc_ai_worker_tcb;
                rt_thread_startup(g_uvc_ai_app.worker_thread);
            } else {
                g_uvc_ai_app.worker_thread = RT_NULL;
                g_uvc_ai_app.worker_running = RT_FALSE;
                rt_sem_delete(g_uvc_ai_app.worker_sem);
                g_uvc_ai_app.worker_sem = RT_NULL;
                USB_LOG_WRN("AI worker thread init failed (%d), fallback to sync\r\n", thread_ret);
            }
        } else {
            USB_LOG_WRN("AI worker semaphore create failed, fallback to sync\r\n");
        }
    } else {
        USB_LOG_WRN("AI worker buffer alloc failed, fallback to sync\r\n");
    }

    if ((g_uvc_ai_app.worker_thread == RT_NULL) || (g_uvc_ai_app.worker_sem == RT_NULL)) {
        if (g_uvc_ai_app.worker_sem != RT_NULL) {
            rt_sem_delete(g_uvc_ai_app.worker_sem);
            g_uvc_ai_app.worker_sem = RT_NULL;
        }
        if (g_uvc_ai_app.worker_frame_buf != RT_NULL) {
            rt_free(g_uvc_ai_app.worker_frame_buf);
            g_uvc_ai_app.worker_frame_buf = RT_NULL;
        }
    }

    uvc_display_set_overlay_callback(uvc_ai_app_overlay_cb, RT_NULL);
    USB_LOG_INFO("AI app started (%s)\r\n",
                 (g_uvc_ai_app.worker_thread != RT_NULL) ? "async" : "sync");

    return RT_EOK;
}

void uvc_ai_app_process_frame(const struct usbh_videoframe *frame)
{
    int ret;

    if ((!g_uvc_ai_app.started) || (!g_uvc_ai_app.ai_ready) || (frame == RT_NULL)) {
        return;
    }

    if (frame->frame_format != USBH_VIDEO_FORMAT_UNCOMPRESSED) {
        return;
    }

    g_uvc_ai_app.frame_counter++;
    if ((g_uvc_ai_app.frame_counter % UVC_AI_FRAMES_TO_SKIP) != 0U) {
        return;
    }

    if ((g_uvc_ai_app.worker_thread != RT_NULL) &&
        (g_uvc_ai_app.worker_sem != RT_NULL) &&
        (g_uvc_ai_app.worker_frame_buf != RT_NULL)) {
        if (frame->frame_size < UVC_AI_FRAME_BYTES) {
            return;
        }

        if (g_uvc_ai_app.worker_busy) {
            return;
        }

        g_uvc_ai_app.worker_busy = RT_TRUE;
        memcpy(g_uvc_ai_app.worker_frame_buf, frame->frame_buf, UVC_AI_FRAME_BYTES);
        if (rt_sem_release(g_uvc_ai_app.worker_sem) != RT_EOK) {
            g_uvc_ai_app.worker_busy = RT_FALSE;
        }
        return;
    }

    ret = uvc_ai_process_yuyv(frame->frame_buf, frame->frame_size, &g_uvc_ai_app.latest_result);
    if (ret != RT_EOK) {
        USB_LOG_WRN("AI processing failed\r\n");
    }
}

void uvc_ai_app_stop(void)
{
    uint32_t wait_ms = 200U;
    uint32_t detach_wait_ms = 200U;

    if (!g_uvc_ai_app.started) {
        return;
    }

    uvc_display_set_overlay_callback(RT_NULL, RT_NULL);

    if (g_uvc_ai_app.worker_thread != RT_NULL) {
        g_uvc_ai_app.worker_running = RT_FALSE;
        if (g_uvc_ai_app.worker_sem != RT_NULL) {
            (void)rt_sem_release(g_uvc_ai_app.worker_sem);
        }

        while (g_uvc_ai_app.worker_busy && (wait_ms > 0U)) {
            rt_thread_mdelay(10);
            if (wait_ms >= 10U) {
                wait_ms -= 10U;
            } else {
                wait_ms = 0U;
            }
        }

        while (detach_wait_ms > 0U) {
            rt_err_t detach_ret = rt_thread_detach(&g_uvc_ai_worker_tcb);

            if (detach_ret == RT_EOK) {
                break;
            }

            rt_thread_mdelay(10);
            detach_wait_ms -= 10U;
        }
        g_uvc_ai_app.worker_thread = RT_NULL;
    }

    if (g_uvc_ai_app.worker_sem != RT_NULL) {
        rt_sem_delete(g_uvc_ai_app.worker_sem);
        g_uvc_ai_app.worker_sem = RT_NULL;
    }

    if (g_uvc_ai_app.worker_frame_buf != RT_NULL) {
        rt_free(g_uvc_ai_app.worker_frame_buf);
        g_uvc_ai_app.worker_frame_buf = RT_NULL;
    }
    g_uvc_ai_app.worker_busy = RT_FALSE;

    if (g_uvc_ai_app.ai_ready) {
        uvc_ai_deinit();
        g_uvc_ai_app.ai_ready = RT_FALSE;
    }

    g_uvc_ai_app.started = RT_FALSE;
    uvc_ai_reset_result(&g_uvc_ai_app.latest_result);
    USB_LOG_INFO("AI app stopped\r\n");
}
