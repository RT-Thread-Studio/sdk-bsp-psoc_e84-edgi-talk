#include "rtthread.h"
#include "string.h"
#include "lvgl.h"

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

extern const unsigned char xiaozhi_font[];
extern const int xiaozhi_font_size;

static lv_obj_t *global_label1;
static lv_obj_t *global_label2;
static lv_obj_t *global_label3;
static lv_obj_t *global_label4;
static lv_style_t style;
static lv_style_t style24;
#define EMOJI_NUM   18
static lv_obj_t *emoji_objs[EMOJI_NUM];

static const lv_image_dsc_t *emoji_imgs[EMOJI_NUM] =
{
    &color_neutral, &color_happy, &color_laughing, &color_funny, &color_sad,
    &color_angry, &color_crying, &color_loving, &color_sleepy, &color_surprised,
    &color_shocked, &color_thinking, &color_winking, &color_cool, &color_relaxed,
    &color_delicious, &color_kissy, &color_confident
};

static const char *emoji_names[EMOJI_NUM] =
{
    "neutral", "happy", "laughing", "funny", "sad", "angry",
    "crying", "loving", "sleepy", "surprised", "shocked",
    "thinking", "winking", "cool", "relaxed", "delicious",
    "kissy", "confident"
};


typedef enum
{
    UI_CMD_SET_STATUS,
    UI_CMD_SET_OUTPUT,
    UI_CMD_SET_EMOJI,
    UI_CMD_SET_ADC
} ui_cmd_t;

typedef struct
{
    ui_cmd_t cmd;
    char data[128];
} ui_msg_t;

struct rt_messagequeue ui_mq;
static char mq_pool[10 * sizeof(ui_msg_t)];

rt_err_t xiaozhi_ui_obj_init(void)
{
    lv_obj_clear_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);

    global_label1 = lv_label_create(lv_screen_active());
    lv_obj_set_x(global_label1, 485);
    lv_obj_set_y(global_label1, 300);
    lv_obj_add_style(global_label1, &style, 0);
    lv_obj_set_style_transform_angle(global_label1, 900, LV_PART_MAIN | LV_STATE_DEFAULT);

    global_label2 = lv_label_create(lv_screen_active());
    lv_obj_set_x(global_label2, 80);
    lv_obj_set_y(global_label2, 20);
    lv_obj_add_style(global_label2, &style24, 0);
    lv_obj_set_style_transform_angle(global_label2, 900, LV_PART_MAIN | LV_STATE_DEFAULT);

    global_label3 = lv_label_create(lv_screen_active());
    lv_obj_set_x(global_label3, 490);
    lv_obj_set_y(global_label3, 30);
    lv_obj_add_style(global_label3, &style, 0);
    lv_obj_set_style_text_color(global_label3, lv_color_hex(0xFF0000), 0);  // 红色
    lv_obj_set_style_transform_angle(global_label3, 900, LV_PART_MAIN | LV_STATE_DEFAULT);

    global_label4 = lv_label_create(lv_screen_active());
    lv_obj_set_x(global_label4, 40);
    lv_obj_set_y(global_label4, 20);
    lv_obj_add_style(global_label4, &style24, 0);
//    lv_obj_set_style_text_color(global_label4, lv_color_hex(0xFF0000), 0);  // 红色
    lv_obj_set_style_transform_angle(global_label4, 900, LV_PART_MAIN | LV_STATE_DEFAULT);

    for (int i = 0; i < EMOJI_NUM; i++)
    {
        emoji_objs[i] = lv_img_create(lv_screen_active());
        lv_img_set_src(emoji_objs[i], emoji_imgs[i]);
        lv_obj_set_x(emoji_objs[i], 403);
        lv_obj_set_y(emoji_objs[i], 300);
        lv_obj_set_style_transform_angle(emoji_objs[i], 900, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_flag(emoji_objs[i], LV_OBJ_FLAG_HIDDEN); // 初始隐藏
    }

    return RT_EOK;
}

void xiaozhi_ui_chat_status(char *string)
{
    ui_msg_t msg;
    msg.cmd = UI_CMD_SET_STATUS;

    if (string)
        strncpy(msg.data, string, sizeof(msg.data) - 1);
    else
        strncpy(msg.data, "", sizeof(msg.data) - 1);

    msg.data[sizeof(msg.data) - 1] = '\0';
    rt_mq_send(&ui_mq, &msg, sizeof(msg));
}

void xiaozhi_ui_chat_output(char *string)
{
    ui_msg_t msg;
    msg.cmd = UI_CMD_SET_OUTPUT;

    if (string)
        strncpy(msg.data, string, sizeof(msg.data) - 1);
    else
        strncpy(msg.data, "", sizeof(msg.data) - 1);

    msg.data[sizeof(msg.data) - 1] = '\0';
    rt_mq_send(&ui_mq, &msg, sizeof(msg));
}

void xiaozhi_ui_update_emoji(char *string)
{
    ui_msg_t msg;
    msg.cmd = UI_CMD_SET_EMOJI;

    if (string)
        strncpy(msg.data, string, sizeof(msg.data) - 1);
    else
        strncpy(msg.data, "neutral", sizeof(msg.data) - 1);

    msg.data[sizeof(msg.data) - 1] = '\0';
    rt_mq_send(&ui_mq, &msg, sizeof(msg));
}

void xiaozhi_ui_update_adc(char *string)
{
    ui_msg_t msg;
    msg.cmd = UI_CMD_SET_ADC;

    if (string)
        strncpy(msg.data, string, sizeof(msg.data) - 1);
    else
        strncpy(msg.data, "neutral", sizeof(msg.data) - 1);

    msg.data[sizeof(msg.data) - 1] = '\0';
    rt_mq_send(&ui_mq, &msg, sizeof(msg));
}


extern void lv_port_disp_init(void);

void xiaozhi_ui_task(void *args)
{
    ui_msg_t msg;
    rt_uint32_t ms;

    lv_init();
    lv_tick_set_cb(&rt_tick_get_millisecond);
    lv_port_disp_init();


    lv_style_init(&style);
    lv_font_t *font = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, 30);
    lv_style_set_text_font(&style, font);

    lv_style_init(&style24);
    lv_font_t *font2 = lv_tiny_ttf_create_data(xiaozhi_font, xiaozhi_font_size, 24);
    lv_style_set_text_font(&style24, font2);

    if (xiaozhi_ui_obj_init() != RT_EOK) return;

    lv_label_set_text(global_label1, "  Connecting");
    lv_label_set_text(global_label2, "Use a Phone or Computer to connect to the Hotspot");
    lv_label_set_text(global_label3, " ");
    lv_label_set_text(global_label4, "SSID: RT-Thread-AP Password: 123456789 IP:192.168.169.1");
    lv_obj_clear_flag(emoji_objs[0], LV_OBJ_FLAG_HIDDEN);
    lv_task_handler();
    while (1)
    {
        if (rt_mq_recv(&ui_mq, &msg, sizeof(msg), RT_WAITING_FOREVER))
        {
            switch (msg.cmd)
            {
            case UI_CMD_SET_STATUS:
                lv_label_set_text(global_label1, msg.data);
                break;
            case UI_CMD_SET_OUTPUT:
                lv_label_set_text(global_label4, msg.data);
                break;
            case UI_CMD_SET_ADC:
                lv_label_set_text(global_label3, msg.data);
                break;
            case UI_CMD_SET_EMOJI:
            {
                int found = -1;
                for (int i = 0; i < EMOJI_NUM; i++)
                {
                    if (strcmp(msg.data, emoji_names[i]) == 0)
                    {
                        found = i;
                        break;
                    }
                }
                for (int i = 0; i < EMOJI_NUM; i++)
                {
                    lv_obj_add_flag(emoji_objs[i], LV_OBJ_FLAG_HIDDEN);
                }
                if (found >= 0)
                    lv_obj_clear_flag(emoji_objs[found], LV_OBJ_FLAG_HIDDEN);
                else
                    lv_obj_clear_flag(emoji_objs[0], LV_OBJ_FLAG_HIDDEN);
                break;
            }
            }
        }

        ms = lv_task_handler();
        rt_thread_mdelay(ms);
    }
}
void init_ui(void)
{
    rt_mq_init(&ui_mq, "ui_mq", mq_pool, sizeof(ui_msg_t), sizeof(mq_pool), RT_IPC_FLAG_FIFO);
    rt_thread_t tid = rt_thread_create("xz_ui", xiaozhi_ui_task, NULL, 1024 * 10, 25, 10);
    if (tid) rt_thread_startup(tid);
}

void clean_info(void){
    lv_label_set_text(global_label2, " ");
}
