/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-01-01     RT-Thread    First version
 */

#include <rtthread.h>
#include <string.h>
#include <stdlib.h>
#include <lvgl.h>
#include "xiaozhi_ui.h"

/*****************************************************************************
 * Macro Definitions
 *****************************************************************************/
#define DBG_TAG    "xz.ui"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

#define EMOJI_NUM           18
#define UI_MSG_DATA_SIZE    128
#define UI_MSG_POOL_SIZE    10
#define UI_THREAD_STACK     (1024 * 10)
#define UI_THREAD_PRIORITY  25
#define UI_THREAD_TICK      10

/* Screen resolution specific definitions */
#define SCREEN_WIDTH        800
#define SCREEN_HEIGHT       512
#define BASE_WIDTH          390
#define BASE_HEIGHT         450
#define SCALE_DPX(val)      LV_DPX((val) * g_scale)

/* Animation and timing */
#define ANIM_TIMEOUT        300
#define IDLE_TIME_LIMIT     20000

/* Layout constants */
#define HEADER_HEIGHT       40
#define BATTERY_OUTLINE_W   58
#define BATTERY_OUTLINE_H   33

/*****************************************************************************
 * Type Definitions
 *****************************************************************************/
typedef enum
{
    UI_CMD_SET_STATUS = 0,
    UI_CMD_SET_OUTPUT,
    UI_CMD_SET_EMOJI,
    UI_CMD_SET_ADC,
    UI_CMD_CLEAR_INFO,
    UI_CMD_SHOW_AP_INFO,
    UI_CMD_SHOW_CONNECTING,
    UI_CMD_UPDATE_BATTERY,
    UI_CMD_UPDATE_CHARGE_STATUS
    /* UI_CMD_UPDATE_BLE_STATUS - Bluetooth not used */
} ui_cmd_t;

typedef struct
{
    ui_cmd_t cmd;
    char data[UI_MSG_DATA_SIZE];
} ui_msg_t;

/* Container status flags */
#define CONT_IDLE           0x01
#define CONT_HIDDEN         0x02
#define CONT_DEFAULT_STATUS (CONT_IDLE | CONT_HIDDEN)

/*****************************************************************************
 * External Declarations
 *****************************************************************************/
/* Emoji images */
extern const lv_image_dsc_t color_neutral;
extern const lv_image_dsc_t color_happy;
extern const lv_image_dsc_t color_laughing;
extern const lv_image_dsc_t color_funny;
extern const lv_image_dsc_t color_sad;
extern const lv_image_dsc_t color_angry;
extern const lv_image_dsc_t color_crying;
extern const lv_image_dsc_t color_loving;
extern const lv_image_dsc_t color_sleepy;
extern const lv_image_dsc_t color_surprised;
extern const lv_image_dsc_t color_shocked;
extern const lv_image_dsc_t color_thinking;
extern const lv_image_dsc_t color_winking;
extern const lv_image_dsc_t color_cool;
extern const lv_image_dsc_t color_relaxed;
extern const lv_image_dsc_t color_delicious;
extern const lv_image_dsc_t color_kissy;
extern const lv_image_dsc_t color_confident;

/* Status icons - Bluetooth not used */
/* extern const lv_image_dsc_t ble; */
/* extern const lv_image_dsc_t ble_close; */

/* Font data */
extern const unsigned char xiaozhi_font[];
extern const int xiaozhi_font_size;

/* Display port */
extern void lv_port_disp_init(void);

/*****************************************************************************
 * Static Variables
 *****************************************************************************/
/* Synchronization */
static struct rt_semaphore s_ui_init_sem;
static struct rt_messagequeue s_ui_mq;
static char s_mq_pool[UI_MSG_POOL_SIZE * sizeof(ui_msg_t)];

/* Scale factor for different screen sizes */
static float g_scale = 1.0f;

/* Container and animation */
static lv_obj_t *s_cont = NULL;
/* static uint8_t s_cont_status = CONT_DEFAULT_STATUS; */ /* Reserved for future use */
/* static uint32_t s_anim_tick = 0; */                    /* Reserved for future use */

/* LVGL objects - Main Screen */
static lv_obj_t *s_label_status;    /* Status label */
static lv_obj_t *s_label_info;      /* Info label */
static lv_obj_t *s_label_adc;       /* ADC label */
static lv_obj_t *s_label_output;    /* Output label */
static lv_obj_t *s_emoji_objs[EMOJI_NUM];
static lv_obj_t *s_main_container;
static lv_obj_t *s_header_row;
static lv_obj_t *s_img_container;

/* Battery and status */
static lv_obj_t *s_battery_fill = NULL;
static lv_obj_t *s_battery_label = NULL;
/* static lv_obj_t *s_img_ble = NULL; */ /* Bluetooth not used */
static int s_battery_level = 100;

/* LVGL styles */
static lv_style_t s_style_30;
static lv_style_t s_style_24;
static lv_style_t s_style_20;

/* Emoji resources */
static const lv_image_dsc_t *s_emoji_imgs[EMOJI_NUM] =
{
    &color_neutral, &color_happy, &color_laughing, &color_funny, &color_sad,
    &color_angry, &color_crying, &color_loving, &color_sleepy, &color_surprised,
    &color_shocked, &color_thinking, &color_winking, &color_cool, &color_relaxed,
    &color_delicious, &color_kissy, &color_confident
};

static const char *s_emoji_names[EMOJI_NUM] =
{
    "neutral", "happy", "laughing", "funny", "sad", "angry",
    "crying", "loving", "sleepy", "surprised", "shocked",
    "thinking", "winking", "cool", "relaxed", "delicious",
    "kissy", "confident"
};

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

/**
 * @brief Calculate scale factor for different screen resolutions
 * @return Scale factor
 */
static float get_scale_factor(void)
{
    lv_disp_t *disp = lv_disp_get_default();
    lv_coord_t scr_width = lv_disp_get_hor_res(disp);
    lv_coord_t scr_height = lv_disp_get_ver_res(disp);

    float scale_x = (float)scr_width / BASE_WIDTH;
    float scale_y = (float)scr_height / BASE_HEIGHT;

    return (scale_x < scale_y) ? scale_x : scale_y;
}

/**
 * @brief Update battery display
 * @param level Battery level (0-100)
 */
static void update_battery_display(int level)
{
    s_battery_level = level;

    if (s_battery_fill)
    {
        int width = (BATTERY_OUTLINE_W - 4) * level / 100;
        if (width < 2 && level > 0) width = 2;
        lv_obj_set_width(s_battery_fill, width);

        if (level <= 20)
        {
            lv_obj_set_style_bg_color(s_battery_fill, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        else
        {
            lv_obj_set_style_bg_color(s_battery_fill, lv_color_hex(0x00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    if (s_battery_label)
    {
        lv_label_set_text_fmt(s_battery_label, "%d%%", level);
    }
}





/**
 * @brief Switch container animation
 * @param hidden Whether to hide container
 * @note This function is reserved for future use
 */
static void switch_cont_anim(bool hidden) __attribute__((unused));
static void switch_cont_anim(bool hidden)
{
    if (!s_cont) return;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_cont);
    lv_anim_del(s_cont, NULL);

    if (hidden)
    {
        lv_anim_set_values(&a, lv_obj_get_y(s_cont), -lv_obj_get_height(s_cont));
    }
    else
    {
        lv_anim_set_values(&a, lv_obj_get_y(s_cont), 0);
    }
    lv_anim_set_duration(&a, 200);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_start(&a);
}

/**
 * @brief Initialize LVGL UI objects
 * @return RT_EOK on success
 */
static rt_err_t ui_objects_init(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_coord_t scr_width = lv_disp_get_hor_res(NULL);
    lv_coord_t scr_height = lv_disp_get_ver_res(NULL);

    /* Calculate scale factor */
    g_scale = get_scale_factor();

    /* Configure screen */
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Create main container - Flex Column layout */
    s_main_container = lv_obj_create(screen);
    lv_obj_remove_flag(s_main_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_main_container, scr_width, scr_height);
    lv_obj_set_style_pad_all(s_main_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_margin_all(s_main_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s_main_container, LV_OPA_0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_main_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(s_main_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_main_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(s_main_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Header row container */
    s_header_row = lv_obj_create(s_main_container);
    lv_obj_remove_flag(s_header_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_header_row, scr_width, SCALE_DPX(HEADER_HEIGHT));

    /* Clear header_row's padding and margin */
    lv_obj_set_style_pad_all(s_header_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_margin_all(s_header_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    /* Set header_row's background transparent and border width to 0 */
    lv_obj_set_style_bg_opa(s_header_row, LV_OPA_0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_header_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(s_header_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_header_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Left spacer */
    lv_obj_t *spacer_left = lv_obj_create(s_header_row);
    lv_obj_remove_flag(spacer_left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(spacer_left, LV_OPA_0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(spacer_left, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(spacer_left, SCALE_DPX(40), LV_SIZE_CONTENT);

    /* BLE status icon - removed */
    /* s_img_ble = lv_img_create(s_header_row); */
    /* lv_img_set_src(s_img_ble, &ble_close); */
    /* lv_obj_set_size(s_img_ble, SCALE_DPX(24), SCALE_DPX(24)); */
    /* lv_img_set_zoom(s_img_ble, (int)(LV_SCALE_NONE * g_scale)); */

    /* Status label - centered */
    s_label_status = lv_label_create(s_header_row);
    lv_label_set_long_mode(s_label_status, LV_LABEL_LONG_WRAP);
    lv_obj_add_style(s_label_status, &s_style_24, 0);
    lv_obj_set_width(s_label_status, LV_PCT(60));
    lv_obj_set_style_text_align(s_label_status, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Battery container */
    lv_obj_t *battery_outline = lv_obj_create(s_header_row);
    lv_obj_set_style_border_width(battery_outline, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(battery_outline, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(battery_outline, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(battery_outline, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(battery_outline, BATTERY_OUTLINE_W * g_scale, BATTERY_OUTLINE_H * g_scale);

    /* Battery fill */
    s_battery_fill = lv_obj_create(battery_outline);
    lv_obj_set_style_outline_width(s_battery_fill, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(s_battery_fill, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_battery_fill, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(s_battery_fill, lv_color_hex(0x00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(s_battery_fill, BATTERY_OUTLINE_W * g_scale - 4, BATTERY_OUTLINE_H * g_scale - 4);
    lv_obj_set_style_border_width(s_battery_fill, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s_battery_fill, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(s_battery_fill, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_clear_flag(s_battery_fill, LV_OBJ_FLAG_SCROLLABLE);

    /* Battery label */
    s_battery_label = lv_label_create(battery_outline);
    lv_obj_add_style(s_battery_label, &s_style_20, 0);
    lv_label_set_text_fmt(s_battery_label, "%d%%", s_battery_level);
    lv_obj_align(s_battery_label, LV_ALIGN_CENTER, 0, 0);

    /* Right spacer */
    lv_obj_t *spacer_right = lv_obj_create(s_header_row);
    lv_obj_remove_flag(spacer_right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(spacer_right, LV_OPA_0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(spacer_right, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(spacer_right, SCALE_DPX(40), LV_SIZE_CONTENT);

    /* Image container for emoji */
    s_img_container = lv_obj_create(s_main_container);
    lv_obj_remove_flag(s_img_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(s_img_container, scr_width, scr_height * 0.5);
    lv_obj_set_style_bg_opa(s_img_container, LV_OPA_0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s_img_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s_img_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Create emoji objects - centered in img_container */
    for (int i = 0; i < EMOJI_NUM; i++)
    {
        s_emoji_objs[i] = lv_img_create(s_img_container);
        lv_img_set_src(s_emoji_objs[i], s_emoji_imgs[i]);
        lv_obj_align(s_emoji_objs[i], LV_ALIGN_CENTER, 0, 0);
        lv_img_set_zoom(s_emoji_objs[i], (int)(LV_SCALE_NONE * g_scale));
        lv_obj_add_flag(s_emoji_objs[i], LV_OBJ_FLAG_HIDDEN);
    }
    /* Show neutral emoji by default */
    lv_obj_clear_flag(s_emoji_objs[0], LV_OBJ_FLAG_HIDDEN);

    /* Text container for output */
    lv_obj_t *text_container = lv_obj_create(s_main_container);
    lv_obj_remove_flag(text_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(text_container, scr_width, scr_height * 0.4);
    lv_obj_set_style_bg_opa(text_container, LV_OPA_0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(text_container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(text_container, SCALE_DPX(20), LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Output label */
    s_label_output = lv_label_create(text_container);
    lv_label_set_long_mode(s_label_output, LV_LABEL_LONG_WRAP);
    lv_obj_add_style(s_label_output, &s_style_24, 0);
    lv_obj_set_width(s_label_output, LV_PCT(90));
    lv_obj_set_style_text_align(s_label_output, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(s_label_output, LV_ALIGN_TOP_MID, 0, 0);

    /* Info label */
    s_label_info = lv_label_create(text_container);
    lv_label_set_long_mode(s_label_info, LV_LABEL_LONG_WRAP);
    lv_obj_add_style(s_label_info, &s_style_20, 0);
    lv_obj_set_width(s_label_info, LV_PCT(90));
    lv_obj_set_style_text_align(s_label_info, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(s_label_info, LV_ALIGN_BOTTOM_MID, 0, 0);

    /* ADC label - positioned in top right */
    s_label_adc = lv_label_create(screen);
    lv_obj_add_style(s_label_adc, &s_style_30, 0);
    lv_obj_set_style_text_color(s_label_adc, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT); /* Dark gray for better contrast */
    lv_obj_align(s_label_adc, LV_ALIGN_TOP_RIGHT, -SCALE_DPX(20), SCALE_DPX(20));

    return RT_EOK;
}

/**
 * @brief Send UI message to queue
 * @param cmd Command type
 * @param data Data string (can be NULL)
 * @param default_data Default value if data is NULL
 */
static void ui_send_message(ui_cmd_t cmd, const char *data, const char *default_data)
{
    ui_msg_t msg;

    msg.cmd = cmd;
    if (data != RT_NULL)
    {
        rt_strncpy(msg.data, data, sizeof(msg.data) - 1);
    }
    else if (default_data != RT_NULL)
    {
        rt_strncpy(msg.data, default_data, sizeof(msg.data) - 1);
    }
    else
    {
        msg.data[0] = '\0';
    }
    msg.data[sizeof(msg.data) - 1] = '\0';

    rt_mq_send(&s_ui_mq, &msg, sizeof(msg));
}

/**
 * @brief Find emoji index by name
 * @param name Emoji name
 * @return Index if found, -1 otherwise
 */
static int ui_find_emoji_index(const char *name)
{
    for (int i = 0; i < EMOJI_NUM; i++)
    {
        if (rt_strcmp(name, s_emoji_names[i]) == 0)
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Show specified emoji, hide others
 * @param index Emoji index to show
 */
static void ui_show_emoji(int index)
{
    for (int i = 0; i < EMOJI_NUM; i++)
    {
        lv_obj_add_flag(s_emoji_objs[i], LV_OBJ_FLAG_HIDDEN);
    }

    if (index >= 0 && index < EMOJI_NUM)
    {
        lv_obj_clear_flag(s_emoji_objs[index], LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        /* Default to neutral */
        lv_obj_clear_flag(s_emoji_objs[0], LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * @brief Process UI message
 * @param msg Message to process
 */
static void ui_process_message(const ui_msg_t *msg)
{
    switch (msg->cmd)
    {
    case UI_CMD_SET_STATUS:
        if (s_label_status)
        {
            lv_label_set_text(s_label_status, msg->data);
        }
        break;

    case UI_CMD_SET_OUTPUT:
        if (s_label_output)
        {
            lv_label_set_text(s_label_output, msg->data);
        }
        break;

    case UI_CMD_SET_ADC:
        if (s_label_adc)
        {
            lv_label_set_text(s_label_adc, msg->data);
        }
        break;

    case UI_CMD_SET_EMOJI:
        ui_show_emoji(ui_find_emoji_index(msg->data));
        break;

    case UI_CMD_CLEAR_INFO:
        if (s_label_info) lv_label_set_text(s_label_info, " ");
        if (s_label_output) lv_label_set_text(s_label_output, " ");
        break;

    case UI_CMD_SHOW_AP_INFO:
        if (s_label_status) lv_label_set_text(s_label_status, "连接中...");
        if (s_label_info) lv_label_set_text(s_label_info, "使用手机或电脑连接热点");
        if (s_label_output) lv_label_set_text(s_label_output, "SSID: RT-Thread-AP 密码: 123456789 IP:192.168.169.1");
        break;

    case UI_CMD_SHOW_CONNECTING:
        if (s_label_status) lv_label_set_text(s_label_status, "连接中...");
        if (s_label_info) lv_label_set_text(s_label_info, "正在连接已保存的WiFi...");
        if (s_label_output) lv_label_set_text(s_label_output, " ");
        break;

    case UI_CMD_UPDATE_BATTERY:
    {
        int level = atoi(msg->data);
        update_battery_display(level);
    }
    break;

    case UI_CMD_UPDATE_CHARGE_STATUS:
        if (strcmp(msg->data, "charging") == 0)
        {
            /* Update UI to show charging status */
            if (s_label_status)
            {
                const char *current_text = lv_label_get_text(s_label_status);
                char new_text[128];
                rt_snprintf(new_text, sizeof(new_text), "%s [充电中]", current_text);
                lv_label_set_text(s_label_status, new_text);
            }
        }
        else
        {
            /* Remove charging indicator */
            if (s_label_status)
            {
                const char *current_text = lv_label_get_text(s_label_status);
                char *charging_pos = rt_strstr(current_text, " [充电中]");
                if (charging_pos)
                {
                    char new_text[128];
                    int len = charging_pos - current_text;
                    rt_memcpy(new_text, current_text, len);
                    new_text[len] = '\0';
                    lv_label_set_text(s_label_status, new_text);
                }
            }
        }
        break;

    /* UI_CMD_UPDATE_BLE_STATUS - Bluetooth not used */
    /* case UI_CMD_UPDATE_BLE_STATUS: */
    /*     if (s_img_ble) { */
    /*         if (strcmp(msg->data, "open") == 0) { */
    /*             lv_img_set_src(s_img_ble, &ble); */
    /*         } else { */
    /*             lv_img_set_src(s_img_ble, &ble_close); */
    /*         } */
    /*     } */
    /*     break; */

    default:
        LOG_W("Unknown UI command: %d", msg->cmd);
        break;
    }
}

/**
 * @brief UI thread entry
 * @param args Thread arguments (unused)
 */
static void ui_thread_entry(void *args)
{
    ui_msg_t msg;
    rt_uint32_t period_ms;
    lv_font_t *font_36;
    lv_font_t *font_28;
    lv_font_t *font_22;

    (void)args;

    /* Initialize LVGL */
    lv_init();
    lv_tick_set_cb(&rt_tick_get_millisecond);
    lv_port_disp_init();

    /* Get screen resolution for scaling */
    lv_coord_t scr_width = lv_disp_get_hor_res(NULL);
    lv_coord_t scr_height = lv_disp_get_ver_res(NULL);
    LOG_I("Screen resolution: %d x %d", scr_width, scr_height);

    /* Calculate scale factor */
    g_scale = get_scale_factor();
    LOG_I("Scale factor: %.2f", g_scale);

    /* Initialize styles with scaled fonts for 800x512 resolution */
    lv_style_init(&s_style_30);
    font_36 = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, (int)(36 * g_scale));
    lv_style_set_text_font(&s_style_30, font_36);
    lv_style_set_text_align(&s_style_30, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_color(&s_style_30, lv_color_hex(0x000000));

    lv_style_init(&s_style_24);
    font_28 = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, (int)(28 * g_scale));
    lv_style_set_text_font(&s_style_24, font_28);
    lv_style_set_text_align(&s_style_24, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_color(&s_style_24, lv_color_hex(0x000000));

    lv_style_init(&s_style_20);
    font_22 = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, (int)(22 * g_scale));
    lv_style_set_text_font(&s_style_20, font_22);
    lv_style_set_text_align(&s_style_20, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_color(&s_style_20, lv_color_hex(0x000000));

    /* Initialize UI objects */
    if (ui_objects_init() != RT_EOK)
    {
        LOG_E("UI objects init failed");
        return;
    }

    /* Set initial display */
    if (s_label_status) lv_label_set_text(s_label_status, "初始化中...");
    if (s_label_info) lv_label_set_text(s_label_info, " ");
    if (s_label_adc) lv_label_set_text(s_label_adc, " ");
    if (s_label_output) lv_label_set_text(s_label_output, " ");
    ui_show_emoji(0);
    lv_task_handler();

    /* Signal initialization complete */
    rt_sem_release(&s_ui_init_sem);
    LOG_I("UI initialized for 800x512 resolution");

    /* Main loop */
    while (1)
    {
        if (rt_mq_recv(&s_ui_mq, &msg, sizeof(msg), RT_WAITING_FOREVER) > 0)
        {
            ui_process_message(&msg);
        }

        period_ms = lv_task_handler();
        rt_thread_mdelay(period_ms);
    }
}

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

void xiaozhi_ui_init(void)
{
    rt_thread_t tid;

    /* Initialize synchronization primitives */
    rt_sem_init(&s_ui_init_sem, "ui_sem", 0, RT_IPC_FLAG_PRIO);
    rt_mq_init(&s_ui_mq, "ui_mq", s_mq_pool, sizeof(ui_msg_t),
               sizeof(s_mq_pool), RT_IPC_FLAG_FIFO);

    /* Create UI thread */
    tid = rt_thread_create("xz_ui", ui_thread_entry, RT_NULL,
                           UI_THREAD_STACK, UI_THREAD_PRIORITY, UI_THREAD_TICK);
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
    else
    {
        LOG_E("Create UI thread failed");
    }
}

rt_err_t xiaozhi_ui_wait_ready(rt_int32_t timeout)
{
    return rt_sem_take(&s_ui_init_sem, timeout);
}

void xiaozhi_ui_set_status(const char *status)
{
    ui_send_message(UI_CMD_SET_STATUS, status, "");
}

void xiaozhi_ui_set_output(const char *output)
{
    ui_send_message(UI_CMD_SET_OUTPUT, output, "");
}

void xiaozhi_ui_set_emoji(const char *emoji)
{
    ui_send_message(UI_CMD_SET_EMOJI, emoji, "neutral");
}

void xiaozhi_ui_set_adc(const char *adc_str)
{
    ui_send_message(UI_CMD_SET_ADC, adc_str, "");
}

void xiaozhi_ui_clear_info(void)
{
    ui_send_message(UI_CMD_CLEAR_INFO, RT_NULL, RT_NULL);
}

void xiaozhi_ui_show_ap_config(void)
{
    ui_send_message(UI_CMD_SHOW_AP_INFO, RT_NULL, RT_NULL);
}

void xiaozhi_ui_show_connecting(void)
{
    ui_send_message(UI_CMD_SHOW_CONNECTING, RT_NULL, RT_NULL);
}

void xiaozhi_ui_update_battery(int level)
{
    char battery_str[16];
    rt_snprintf(battery_str, sizeof(battery_str), "%d", level);
    ui_send_message(UI_CMD_UPDATE_BATTERY, battery_str, "100");
}

void xiaozhi_ui_update_charging_status(bool is_charging)
{
    const char *status = is_charging ? "charging" : "discharging";
    ui_send_message(UI_CMD_UPDATE_CHARGE_STATUS, status, "discharging");
}

void xiaozhi_ui_update_ble_status(bool connected)
{
    /* Bluetooth functionality not implemented */
    LOG_W("Bluetooth status update not implemented");
    /* If needed in future, implement with different approach */
}


/*****************************************************************************
 * Legacy API Compatibility
 *****************************************************************************/

/* Keep old function names for backward compatibility */
void init_ui(void)
{
    xiaozhi_ui_init();
}

rt_err_t wait_ui_ready(rt_int32_t timeout)
{
    return xiaozhi_ui_wait_ready(timeout);
}

void clean_info(void)
{
    xiaozhi_ui_clear_info();
}

void xiaozhi_ui_chat_status(char *string)
{
    xiaozhi_ui_set_status(string);
}

void xiaozhi_ui_chat_output(char *string)
{
    xiaozhi_ui_set_output(string);
}

void xiaozhi_ui_update_emoji(char *string)
{
    xiaozhi_ui_set_emoji(string);
}

void xiaozhi_ui_update_adc(char *string)
{
    xiaozhi_ui_set_adc(string);
}
