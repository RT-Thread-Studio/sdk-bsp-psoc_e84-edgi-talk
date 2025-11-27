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
    UI_CMD_SHOW_CONNECTING
} ui_cmd_t;

typedef struct
{
    ui_cmd_t cmd;
    char data[UI_MSG_DATA_SIZE];
} ui_msg_t;

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

/* LVGL objects */
static lv_obj_t *s_label_status;    /* Status label */
static lv_obj_t *s_label_info;      /* Info label */
static lv_obj_t *s_label_adc;       /* ADC label */
static lv_obj_t *s_label_output;    /* Output label */
static lv_obj_t *s_emoji_objs[EMOJI_NUM];

/* LVGL styles */
static lv_style_t s_style_30;
static lv_style_t s_style_24;

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
 * @brief Initialize LVGL UI objects
 * @return RT_EOK on success
 */
static rt_err_t ui_objects_init(void)
{
    lv_obj_t *screen = lv_screen_active();

    /* Configure screen */
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Create status label */
    s_label_status = lv_label_create(screen);
    lv_obj_set_pos(s_label_status, 485, 300);
    lv_obj_add_style(s_label_status, &s_style_30, 0);
    lv_obj_set_style_transform_angle(s_label_status, 900, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Create info label */
    s_label_info = lv_label_create(screen);
    lv_obj_set_pos(s_label_info, 80, 20);
    lv_obj_add_style(s_label_info, &s_style_24, 0);
    lv_obj_set_style_transform_angle(s_label_info, 900, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Create ADC label */
    s_label_adc = lv_label_create(screen);
    lv_obj_set_pos(s_label_adc, 490, 30);
    lv_obj_add_style(s_label_adc, &s_style_30, 0);
    lv_obj_set_style_text_color(s_label_adc, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_transform_angle(s_label_adc, 900, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Create output label */
    s_label_output = lv_label_create(screen);
    lv_obj_set_pos(s_label_output, 40, 20);
    lv_obj_add_style(s_label_output, &s_style_24, 0);
    lv_obj_set_style_transform_angle(s_label_output, 900, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* Create emoji objects */
    for (int i = 0; i < EMOJI_NUM; i++)
    {
        s_emoji_objs[i] = lv_img_create(screen);
        lv_img_set_src(s_emoji_objs[i], s_emoji_imgs[i]);
        lv_obj_set_pos(s_emoji_objs[i], 403, 300);
        lv_obj_set_style_transform_angle(s_emoji_objs[i], 900, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_flag(s_emoji_objs[i], LV_OBJ_FLAG_HIDDEN);
    }

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
        lv_label_set_text(s_label_status, msg->data);
        break;

    case UI_CMD_SET_OUTPUT:
        lv_label_set_text(s_label_output, msg->data);
        break;

    case UI_CMD_SET_ADC:
        lv_label_set_text(s_label_adc, msg->data);
        break;

    case UI_CMD_SET_EMOJI:
        ui_show_emoji(ui_find_emoji_index(msg->data));
        break;

    case UI_CMD_CLEAR_INFO:
        lv_label_set_text(s_label_info, " ");
        lv_label_set_text(s_label_output, " ");
        break;

    case UI_CMD_SHOW_AP_INFO:
        lv_label_set_text(s_label_status, "  Connecting");
        lv_label_set_text(s_label_info, "Use a Phone or Computer to connect to the Hotspot");
        lv_label_set_text(s_label_output, "SSID: RT-Thread-AP Password: 123456789 IP:192.168.169.1");
        break;

    case UI_CMD_SHOW_CONNECTING:
        lv_label_set_text(s_label_status, "  Connecting");
        lv_label_set_text(s_label_info, "Connecting to saved WiFi...");
        lv_label_set_text(s_label_output, " ");
        break;

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
    lv_font_t *font_30;
    lv_font_t *font_24;

    (void)args;

    /* Initialize LVGL */
    lv_init();
    lv_tick_set_cb(&rt_tick_get_millisecond);
    lv_port_disp_init();

    /* Initialize styles */
    lv_style_init(&s_style_30);
    font_30 = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, 30);
    lv_style_set_text_font(&s_style_30, font_30);

    lv_style_init(&s_style_24);
    font_24 = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, 24);
    lv_style_set_text_font(&s_style_24, font_24);

    /* Initialize UI objects */
    if (ui_objects_init() != RT_EOK)
    {
        LOG_E("UI objects init failed");
        return;
    }

    /* Set initial display - neutral state, WiFi manager will update */
    lv_label_set_text(s_label_status, "  Initializing");
    lv_label_set_text(s_label_info, " ");
    lv_label_set_text(s_label_adc, " ");
    lv_label_set_text(s_label_output, " ");
    ui_show_emoji(0);
    lv_task_handler();

    /* Signal initialization complete */
    rt_sem_release(&s_ui_init_sem);
    LOG_I("UI initialized");

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
