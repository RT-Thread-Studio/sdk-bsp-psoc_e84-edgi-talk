/**
 * @file lv_example_virtual3d_animated_emoji.c
 * @brief 3D表情动画显示示例实现
 */

#include "lvgl.h"
#include <stdio.h>  
#include "lx_resource.h"
#include "lv_example_virtual3d_animated_emoji.h"   

#include "lv_font.h"

#define DRV_INFO
#define LOG_TAG         "virtual3d"
#include <drv_log.h>
/* 资源声明 */      

LX_IMG_DECLARE(LX_ANIMATEDEMOJI_EARPHONE_NORMAL_128_128)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_EARPHONE_TRANSPARENCY_128_129)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_EARPHONE_DENOISE_128_128)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_MUSIC_EQ_128_128)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_MUSIC_LAST_128_128)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_MUSIC_MUTE_128_128)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_MUSIC_NEXT_128_128)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_MUSIC_PLAY_128_128)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_MUSIC_STOP_128_128)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_MUSIC_VOL_ADD_128_128)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_MUSIC_VOL_SUB_128_128)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_DREAM_DRINK_80_64)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_DREAM_PLANET_80_64)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_DREAM_STAR_80_64)
#if 0
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_00_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_01_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_02_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_03_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_04_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_05_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_06_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_07_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_08_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_09_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_10_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_11_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_12_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_13_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_14_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_15_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_16_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_17_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_18_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_19_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_20_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_21_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_22_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_23_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_24_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_25_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_26_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_27_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_28_256_256)
LX_IMG_DECLARE(LX_ANIMATEDEMOJI_CRACK_29_256_256)

#endif



LX_MOD_DECLARE(LX_MODEL_EMOJI_00)
LX_MOD_DECLARE(LX_MODEL_EMOJI_01)
LX_MOD_DECLARE(LX_MODEL_EMOJI_02)
LX_MOD_DECLARE(LX_MODEL_EMOJI_03)
LX_MOD_DECLARE(LX_MODEL_EMOJI_04)
LX_MOD_DECLARE(LX_MODEL_EMOJI_05)

LX_MOD_DECLARE(LX_MODEL_EMOJI_06)
LX_MOD_DECLARE(LX_MODEL_EMOJI_07)
LX_MOD_DECLARE(LX_MODEL_EMOJI_08)
LX_MOD_DECLARE(LX_MODEL_EMOJI_09)
LX_MOD_DECLARE(LX_MODEL_EMOJI_10)

LX_MOD_DECLARE(LX_MODEL_EMOJI_11)
LX_MOD_DECLARE(LX_MODEL_EMOJI_12)
LX_MOD_DECLARE(LX_MODEL_EMOJI_13)
LX_MOD_DECLARE(LX_MODEL_EMOJI_14)
LX_MOD_DECLARE(LX_MODEL_EMOJI_15)
LX_MOD_DECLARE(LX_MODEL_EMOJI_16)
LX_MOD_DECLARE(LX_MODEL_EMOJI_17)
LX_MOD_DECLARE(LX_MODEL_EMOJI_18)
LX_MOD_DECLARE(LX_MODEL_EMOJI_19)
LX_MOD_DECLARE(LX_MODEL_EMOJI_20)
LX_MOD_DECLARE(LX_MODEL_EMOJI_21)
#if 0
LX_MOD_DECLARE(LX_MODEL_EMOJI_22)
LX_MOD_DECLARE(LX_MODEL_EMOJI_23)
LX_MOD_DECLARE(LX_MODEL_EMOJI_24)
LX_MOD_DECLARE(LX_MODEL_EMOJI_25)
LX_MOD_DECLARE(LX_MODEL_EMOJI_26)
LX_MOD_DECLARE(LX_MODEL_EMOJI_27)
LX_MOD_DECLARE(LX_MODEL_EMOJI_28)
LX_MOD_DECLARE(LX_MODEL_EMOJI_29)
LX_MOD_DECLARE(LX_MODEL_EMOJI_30)
LX_MOD_DECLARE(LX_MODEL_EMOJI_31)
LX_MOD_DECLARE(LX_MODEL_EMOJI_32)
LX_MOD_DECLARE(LX_MODEL_EMOJI_33)
LX_MOD_DECLARE(LX_MODEL_EMOJI_34)
LX_MOD_DECLARE(LX_MODEL_EMOJI_35)
LX_MOD_DECLARE(LX_MODEL_EMOJI_36)
LX_MOD_DECLARE(LX_MODEL_EMOJI_37)
LX_MOD_DECLARE(LX_MODEL_EMOJI_38)
LX_MOD_DECLARE(LX_MODEL_EMOJI_39)
LX_MOD_DECLARE(LX_MODEL_EMOJI_40)
LX_MOD_DECLARE(LX_MODEL_EMOJI_41)
LX_MOD_DECLARE(LX_MODEL_EMOJI_42)
LX_MOD_DECLARE(LX_MODEL_EMOJI_43)
LX_MOD_DECLARE(LX_MODEL_EMOJI_44)
LX_MOD_DECLARE(LX_MODEL_EMOJI_45)
LX_MOD_DECLARE(LX_MODEL_EMOJI_46)
LX_MOD_DECLARE(LX_MODEL_EMOJI_47)
LX_MOD_DECLARE(LX_MODEL_EMOJI_48)
LX_MOD_DECLARE(LX_MODEL_EMOJI_49)
LX_MOD_DECLARE(LX_MODEL_EMOJI_50)
LX_MOD_DECLARE(LX_MODEL_EMOJI_51)
LX_MOD_DECLARE(LX_MODEL_EMOJI_52)
LX_MOD_DECLARE(LX_MODEL_EMOJI_53)
#endif
lv_obj_t * emoji_container;
lv_obj_t* emoji_virtual3d = NULL;
static bool  emoji_enable_change = true;

static bool emoji_popup_mode = false;
static lv_obj_t** emoji_visible_obj_list = NULL;
static uint32_t emoji_visible_obj_count = 0;
static lv_obj_t *emoji_snapshot_img = NULL;
static lv_img_dsc_t *emoji_snapshot_dsc = NULL;

static lx_vglite_model_t CURR_MODEL = { 0 };


static char lx_action_name[128] = { 0 };

static const char* next_action_name = NULL;

#define EMOJI_BASE_COLOR 0xffffffff //可根据需要进行修改

#define EMOJI_TAP_ICON_X 290 //点击动画显示图标的x坐标，此值为466X466分辨率下的坐标，请根据实际分辨率修改
#define EMOJI_TAP_ICON_Y 300 //点击动画显示图标的y坐标，此值为466X466分辨率下的坐标，请根据实际分辨率修改

static const lv_image_dsc_t * icon_group[] =
{
    &LX_ANIMATEDEMOJI_EARPHONE_NORMAL_128_128,
    &LX_ANIMATEDEMOJI_EARPHONE_TRANSPARENCY_128_129,
    &LX_ANIMATEDEMOJI_EARPHONE_DENOISE_128_128,
    &LX_ANIMATEDEMOJI_MUSIC_EQ_128_128,
    &LX_ANIMATEDEMOJI_MUSIC_LAST_128_128,
    &LX_ANIMATEDEMOJI_MUSIC_MUTE_128_128,
    &LX_ANIMATEDEMOJI_MUSIC_NEXT_128_128,
    &LX_ANIMATEDEMOJI_MUSIC_PLAY_128_128,
    &LX_ANIMATEDEMOJI_MUSIC_STOP_128_128,
    &LX_ANIMATEDEMOJI_MUSIC_VOL_SUB_128_128,
    &LX_ANIMATEDEMOJI_MUSIC_VOL_ADD_128_128,
    &LX_ANIMATEDEMOJI_DREAM_DRINK_80_64,
    &LX_ANIMATEDEMOJI_DREAM_PLANET_80_64,
    &LX_ANIMATEDEMOJI_DREAM_STAR_80_64,
#if 0
    &LX_ANIMATEDEMOJI_CRACK_00_256_256,//14
    &LX_ANIMATEDEMOJI_CRACK_01_256_256,
    &LX_ANIMATEDEMOJI_CRACK_02_256_256,
    &LX_ANIMATEDEMOJI_CRACK_03_256_256,
    &LX_ANIMATEDEMOJI_CRACK_04_256_256,
    &LX_ANIMATEDEMOJI_CRACK_05_256_256,
    &LX_ANIMATEDEMOJI_CRACK_06_256_256,
    &LX_ANIMATEDEMOJI_CRACK_07_256_256,
    &LX_ANIMATEDEMOJI_CRACK_08_256_256,
    &LX_ANIMATEDEMOJI_CRACK_09_256_256,
    &LX_ANIMATEDEMOJI_CRACK_10_256_256,
    &LX_ANIMATEDEMOJI_CRACK_11_256_256,
    &LX_ANIMATEDEMOJI_CRACK_12_256_256,
    &LX_ANIMATEDEMOJI_CRACK_13_256_256,
    &LX_ANIMATEDEMOJI_CRACK_14_256_256,
    &LX_ANIMATEDEMOJI_CRACK_15_256_256,
    &LX_ANIMATEDEMOJI_CRACK_16_256_256,
    &LX_ANIMATEDEMOJI_CRACK_17_256_256,
    &LX_ANIMATEDEMOJI_CRACK_18_256_256,
    &LX_ANIMATEDEMOJI_CRACK_19_256_256,
    &LX_ANIMATEDEMOJI_CRACK_20_256_256,
    &LX_ANIMATEDEMOJI_CRACK_21_256_256,
    &LX_ANIMATEDEMOJI_CRACK_22_256_256,
    &LX_ANIMATEDEMOJI_CRACK_23_256_256,
    &LX_ANIMATEDEMOJI_CRACK_24_256_256,
    &LX_ANIMATEDEMOJI_CRACK_25_256_256,
    &LX_ANIMATEDEMOJI_CRACK_26_256_256,
    &LX_ANIMATEDEMOJI_CRACK_27_256_256,
    &LX_ANIMATEDEMOJI_CRACK_28_256_256,
    &LX_ANIMATEDEMOJI_CRACK_29_256_256,
    &LX_ANIMATEDEMOJI_CRACK_25_256_256,
    &LX_ANIMATEDEMOJI_CRACK_21_256_256,
    &LX_ANIMATEDEMOJI_CRACK_17_256_256,
    &LX_ANIMATEDEMOJI_CRACK_13_256_256,
    &LX_ANIMATEDEMOJI_CRACK_09_256_256,
    &LX_ANIMATEDEMOJI_CRACK_05_256_256,
    &LX_ANIMATEDEMOJI_CRACK_00_256_256,
#endif
};


static lx_action_cfg_item_t lx_action_cfg_table[] =
{
    {"follow",
        0, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xffffffff, 0xffffffff, 0xffffffff},//跟随
    {"hearing",
        1, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xffffb310, 0xffffffff, 0xffffffff},//倾听
    {"thinking",
        2, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xff00f0f0, 0xffffffff, 0xffffffff},//思考
    {"speaking",
        3, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xff000000, EMOJI_BASE_COLOR, 0xffffffff},//说话
    {"error_asr",
        4, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xff00ffff, 0xffcfcfcf, EMOJI_BASE_COLOR},//错误识别
    {"yawn",
        5, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xffffb310, 0xff000000, EMOJI_BASE_COLOR},//哈欠
    {"say_hello",
        6, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xff8f8fff, 0xff000000, EMOJI_BASE_COLOR},//打招呼
    {"thumbup",
        7, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xff8f8fff, 0xff000000, EMOJI_BASE_COLOR},//点赞
    {"sleepy",
        8, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xff8f8fff, 0xffffffff, 0xffffffff},//困倦
    {"anticipation",
        9, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xffffffff, 0xffffffff, 0xffffffff},//期待
    {"shy",
        10, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xff8f8fff, 0xff000000, EMOJI_BASE_COLOR},//害羞
    {"tap_earphonemode_normal",
        11, 1, 0, 100, 1, EMOJI_TAP_ICON_X, EMOJI_TAP_ICON_Y, 25, 65, 0x00000000, EMOJI_BASE_COLOR, 0xff000000, EMOJI_BASE_COLOR, 0xffffffff}, // 开关降噪-正常
    {"tap_earphonemode_transparencymode",
        11, 1, 1, 100, 1, EMOJI_TAP_ICON_X, EMOJI_TAP_ICON_Y, 25, 65, 0x00000000, EMOJI_BASE_COLOR, 0xff000000, EMOJI_BASE_COLOR, 0xffffffff}, // 开关降噪-通透
    {"tap_earphonemode_denoise",
        11, 1, 2, 100, 1, EMOJI_TAP_ICON_X, EMOJI_TAP_ICON_Y, 25, 65, 0x00000000, EMOJI_BASE_COLOR, 0xff000000, EMOJI_BASE_COLOR, 0xffffffff}, // 开关降噪-降噪
    {"tap_music_eq",
        11, 1, 3, 100, 1, EMOJI_TAP_ICON_X, EMOJI_TAP_ICON_Y, 25, 65, 0x00000000, EMOJI_BASE_COLOR, 0xff000000, EMOJI_BASE_COLOR, 0xffffffff}, // 音乐控制-EQ
    {"tap_music_last",
        11, 1, 4, 100, 1, EMOJI_TAP_ICON_X, EMOJI_TAP_ICON_Y, 25, 65, 0x00000000, EMOJI_BASE_COLOR, 0xff000000, EMOJI_BASE_COLOR, 0xffffffff}, // 音乐控制-上一首
    {"tap_music_mute",
        11, 1, 5, 100, 1, EMOJI_TAP_ICON_X, EMOJI_TAP_ICON_Y, 25, 65, 0x00000000, EMOJI_BASE_COLOR, 0xff000000, EMOJI_BASE_COLOR, 0xffffffff}, // 音乐控制-静音
    {"tap_music_next",
        11, 1, 6, 100, 1, EMOJI_TAP_ICON_X, EMOJI_TAP_ICON_Y, 25, 65, 0x00000000, EMOJI_BASE_COLOR, 0xff000000, EMOJI_BASE_COLOR, 0xffffffff}, // 音乐控制-下一首
    {"tap_music_play",
        11, 1, 7, 100, 1, EMOJI_TAP_ICON_X, EMOJI_TAP_ICON_Y, 25, 65, 0x00000000, EMOJI_BASE_COLOR, 0xff000000, EMOJI_BASE_COLOR, 0xffffffff}, // 音乐控制-播放
    {"tap_music_stop",
        11, 1, 8, 100, 1, EMOJI_TAP_ICON_X, EMOJI_TAP_ICON_Y, 25, 65, 0x00000000, EMOJI_BASE_COLOR, 0xff000000, EMOJI_BASE_COLOR, 0xffffffff}, // 音乐控制-停止
    {"tap_music_volume-",
        11, 1, 9, 100, 1, EMOJI_TAP_ICON_X, EMOJI_TAP_ICON_Y, 25, 65, 0x00000000, EMOJI_BASE_COLOR, 0xff000000, EMOJI_BASE_COLOR, 0xffffffff}, // 音乐控制-降低音量
    {"tap_music_volume+",
        11, 1, 10, 100, 1, EMOJI_TAP_ICON_X, EMOJI_TAP_ICON_Y, 25, 65, 0x00000000, EMOJI_BASE_COLOR, 0xff000000, EMOJI_BASE_COLOR, 0xffffffff}, // 音乐控制-调高音量
    {"blink",
        12, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xffffffff, 0xffffffff, 0xffffffff},//眨眼
    {"angry",
        13, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xff8f8fff, 0xffffffff, 0xffffffff},//愤怒
    {"poke",
        14, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xff8f8fff, 0xffffffff, 0xffffffff},//戳脸
    {"laugh",
        15, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xff8f8fff, 0xffffffff, 0xffffffff},//大笑
    {"proud",
        16, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xff8f8fff, 0xffffffff, 0xffffffff},//得意
    {"fear",
        17, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xffffb30f, 0xffffffff, 0xffffffff},//害怕
    {"cry",
        18, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xffffb30f, 0xffffffff, 0xffffffff},//哭泣
    {"awkward",
        19, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xffffb30f, 0xffffffff, 0xffffffff},//哭笑不得
    {"error_general_1/3",
        20, 2, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xffffffff, 0xffffffff, 0xffffffff},//状态异常 1
    {"error_general_2/3",
        20, 3, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xffffffff, 0xffffffff, 0xffffffff},//状态异常 2
    {"error_general_3/3",
        20, 4, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xffffffff, 0xffffffff, 0xffffffff},//状态异常 3
    {"press",
        21, 1, -1, -1, -1, 0, 0, 0, 0, 0x00000000, EMOJI_BASE_COLOR, 0xffffffff, 0xffffffff, 0xffffffff},//按住屏幕左右/上下滑
}; 

int16_t lastPosX;
int16_t lastPosY;

void* model_cache_list[128];



#define M_PI    3.1415926

static int get_angle_from_pos(int x, int y, int width, int height) {
    // 计算屏幕中心点
    double center_x = width / 2.0;
    double center_y = height / 2.0;
    
    // 计算相对于中心点的偏移
    double delta_x = x - center_x;
    double delta_y = y - center_y;
    
    // 使用atan2计算角度（弧度），注意坐标系的转换
    // 由于屏幕坐标系Y轴向下，需要调整计算方式
    double angle_rad = atan2(delta_x, -delta_y);
    
    // 转换为角度
    double angle_deg = angle_rad * 180.0 / M_PI;
    
    // 标准化角度到0-360范围
    if (angle_deg < 0) {
        angle_deg += 360.0;
    }

	return (int)angle_deg;
}


lx_vglite_model_t* get_model(uint16_t index)
{

    CURR_MODEL.data = NULL;
    CURR_MODEL.size = 0;
    LOG_D(" LX_VGWIDGET_INS_EMOJI_ANIMATION ->get_model start index:%d",index);

    CURR_MODEL.data = NULL;
    CURR_MODEL.size = 0;

	switch (index) 
    {
        case 0:

            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_00);
            LOG_D(" LX_VGWIDGET_INS_EMOJI_ANIMATION ->get_model LX_MODEL_EMOJI_00 model_cache_list[0]:%d",model_cache_list[index]);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_00);

                LOG_D(" LX_VGWIDGET_INS_EMOJI_ANIMATION ->get_model LX_MODEL_EMOJI_00 CURR_MODEL.size:%d",CURR_MODEL.size);
            }

            break;
        case 1:

            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_01);
            LOG_D(" LX_VGWIDGET_INS_EMOJI_ANIMATION ->get_model LX_MODEL_EMOJI_01 model_cache_list[1]:%d",model_cache_list[index]);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_01);
                LOG_D(" LX_VGWIDGET_INS_EMOJI_ANIMATION ->get_model LX_MODEL_EMOJI_01 CURR_MODEL.size:%d",CURR_MODEL.size);
            }
            
            break;
        case 2:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_02);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_02);
            }
            break;
        case 3:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_03);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_03);
            }
            break;
        case 4:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_04);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_04);
            break;
            }
        case 5:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_05);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_05);
            }
            break;

        case 6:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_06);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_06);
            }
            break;
        case 7:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_07);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_07);
            }
            break;
        case 8:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_08);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_08);
            }
            break;
        case 9:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_09);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_09);
            }
            break;
        case 10:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_10);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_10);
            }
            break;

        case 11:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_11);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_11);
            }
            break;
        case 12:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_12);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_12);
            }
            break;
        case 13:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_13);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_13);
            }
            break;   
        case 14:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_14);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_14);
            }
            break;   
        case 15:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_15);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_15);
            }
            break;
        case 16:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_16);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_16);
            }
            break;
        case 17:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_17);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_17);
            }
            break;   
        case 18:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_18);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_18);
            }            
            break;    
            
        case 19:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_19);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_19);
            }            
            break;        
            
        case 20:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_20);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_20);
            }            
            break;    
        case 21:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_21);
            if(model_cache_list[index])  
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_21);
            }            
            break;        
        /*
        case 22:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_22);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_22);
            }
            break;
        case 23:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_23);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_23);
            }
            break;
         case 24:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_24);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_24);
            }
            break;
        case 25:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_25);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_25);
            }
            break;
        case 26:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_26);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_26);
            }
            break;
        case 27:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_27);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_27);
            }
            break;
        case 28:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_28);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_28);
            }
            break;

        case 29:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_29);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_29);
            }
            break;

        case 30:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_30);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_30);
            }
            break;
        case 31:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_31);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_31);
            }
            break;
        case 32:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_32);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_32);
            }
            break;
        case 33:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_33);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_33);
            }
            break;
        case 34:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_34);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_34);
            }
            break;
        case 35:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_35);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_35);
            }
            break;
        case 36:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_36);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_36);
            }
            break;
        case 37:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_37);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_37);
            }
            break;
        case 38:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_38);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_38);
            }
            break;

        case 39:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_39);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_39);
            }
            break;

        case 40:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_40);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_40);
            }
            break;
        case 41:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_41);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_41);
            }
            break;

        case 42:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_42);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_42);
            }
            break;
        case 43:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_43);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_43);
            }
            break;
         case 44:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_44);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_44);
            }
            break;
        case 45:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_45);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_45);
            }
            break;
        case 46:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_46);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_46);
            }
            break;
        case 47:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_47);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_47);
            }
            break;
        case 48:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_48);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_48);
            }
            break;
        case 49:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_49);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_49);
            }
            break;
        case 50:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_50);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_50);
            }
            break;

        case 51:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_51);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_51);
            }
            break;
        case 52:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_52);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_52);
            }
            break;
        case 53:
            model_cache_list[index] = LX_LOAD_MODEL(LX_MODEL_EMOJI_53);
            if(model_cache_list[index])
            {
                CURR_MODEL.data = model_cache_list[index];
                CURR_MODEL.size = LX_GET_MODEL_ORIGINAL_SIZE(LX_MODEL_EMOJI_53);
            }
            break;
        */
    }



    return &CURR_MODEL;
}

#if 0
void load_crack_img(uint16_t index)
{
    uint16_t crack_index = index - EMOJI_IMG_CRACK_SEQ_START_IND;
    switch (crack_index)
    {
        case 0:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_00_256_256);
        break;
        case 1:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_01_256_256);
        break;
        case 2:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_02_256_256);
        break;
        case 3:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_03_256_256);
        break;
        case 4:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_04_256_256);
        break;
        case 5:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_05_256_256);
        break;
        case 6:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_06_256_256);
        break;
        case 7:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_07_256_256);
        break;
        case 8:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_08_256_256);
        break;
        case 9:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_09_256_256);
        break;
        case 10:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_10_256_256);
        break;
        case 11:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_11_256_256);
        break;
        case 12:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_12_256_256);
        break;
        case 13:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_13_256_256);
        break;
        case 14:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_14_256_256);
        break;
        case 15:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_15_256_256);
        break;
        case 16:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_16_256_256);
        break;
        case 17:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_17_256_256);
        break;
        case 18:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_18_256_256);
        break;
        case 19:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_19_256_256);
        break;
        case 20:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_20_256_256);
        break;
        case 21:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_21_256_256);
        break;
        case 22:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_22_256_256);
        break;
        case 23:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_23_256_256);
        break;
        case 24:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_24_256_256);
        break;
        case 25:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_25_256_256);
        break;
        case 26:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_26_256_256);
        break;
        case 27:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_27_256_256);
        break;
        case 28:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_28_256_256);
        break;
        case 29:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_29_256_256);
        break;
        case 30:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_25_256_256);
        break;
        case 31:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_21_256_256);
        break;
        case 32:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_17_256_256);
        break;
        case 33:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_13_256_256);
        break;
        case 34:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_09_256_256);
        break;
        case 35:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_05_256_256);
        break;
        case 36:
            LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_00_256_256);
        break;

    }


}

#endif

void load_img(uint16_t index)
{

    if( index >=  EMOJI_IMG_CRACK_SEQ_START_IND )
    {
        //tangrao_debug load_crack_img(index);
    }
    else if( index <  EMOJI_IMG_CRACK_SEQ_START_IND ) 
    {
        switch (index) 
        {
            case 0:
                LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_EARPHONE_NORMAL_128_128);
            break;
            case 1:
                LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_EARPHONE_TRANSPARENCY_128_129);
            break;
            case 2:
                LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_EARPHONE_DENOISE_128_128);
            break;
            case 3:
                LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_EQ_128_128);
            break;
            case 4:
                LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_LAST_128_128);
            break;
            case 5:
                LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_MUTE_128_128);
            break;
            case 6:
                LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_NEXT_128_128);
            break;
            case 7:
                LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_PLAY_128_128);
            break;
            case 8:
                LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_STOP_128_128);
            break;
            case 9:
                LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_VOL_SUB_128_128);
            break;
            case 10:
                LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_VOL_ADD_128_128);
            break;
            case 11:
                LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_DREAM_DRINK_80_64);
            break;
            case 12:
                LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_DREAM_PLANET_80_64);
            break;
            case 13:
                LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_DREAM_STAR_80_64);
            break;            
        }
        
    }
    
}

#if 0
void free_crack_img(uint16_t index)
{
    uint16_t crack_index = index - EMOJI_IMG_CRACK_SEQ_START_IND;
    switch (crack_index)
    {
        case 0:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_00_256_256);
        break;
        case 1:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_01_256_256);
        break;
        case 2:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_02_256_256);
        break;
        case 3:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_03_256_256);
        break;
        case 4:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_04_256_256);
        break;
        case 5:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_05_256_256);
        break;
        case 6:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_06_256_256);
        break;
        case 7:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_07_256_256);
        break;
        case 8:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_08_256_256);
        break;
        case 9:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_09_256_256);
        break;
        case 10:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_10_256_256);
        break;
        case 11:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_11_256_256);
        break;
        case 12:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_12_256_256);
        break;
        case 13:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_13_256_256);
        break;
        case 14:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_14_256_256);
        break;
        case 15:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_15_256_256);
        break;
        case 16:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_16_256_256);
        break;
        case 17:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_17_256_256);
        break;
        case 18:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_18_256_256);
        break;
        case 19:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_19_256_256);
        break;
        case 20:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_20_256_256);
        break;
        case 21:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_21_256_256);
        break;
        case 22:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_22_256_256);
        break;
        case 23:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_23_256_256);
        break;
        case 24:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_24_256_256);
        break;
        case 25:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_25_256_256);
        break;
        case 26:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_26_256_256);
        break;
        case 27:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_27_256_256);
        break;
        case 28:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_28_256_256);
        break;
        case 29:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_29_256_256);
        break;
        case 30:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_25_256_256);
        break;
        case 31:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_21_256_256);
        break;
        case 32:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_17_256_256);
        break;
        case 33:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_13_256_256);
        break;
        case 34:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_09_256_256);
        break;
        case 35:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_05_256_256);
        break;
        case 36:
            LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_00_256_256);
        break;

    }


}
#endif

void  free_img(uint16_t index)
{

    if( index >=  EMOJI_IMG_CRACK_SEQ_START_IND )
    {
        //tangrao_debug free_crack_img(index);
    }
    else if( index <  EMOJI_IMG_CRACK_SEQ_START_IND ) 
    {
        switch (index) 
        {
            case 0:
                LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_EARPHONE_NORMAL_128_128);
            break;
            case 1:
                LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_EARPHONE_TRANSPARENCY_128_129);
            break;
            case 2:
                LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_EARPHONE_DENOISE_128_128);
            break;
            case 3:
                LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_EQ_128_128);
            break;
            case 4:
                LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_LAST_128_128);
            break;
            case 5:
                LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_MUTE_128_128);
            break;
            case 6:
                LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_NEXT_128_128);
            break;
            case 7:
                LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_PLAY_128_128);
            break;
            case 8:
                LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_STOP_128_128);
            break;
            case 9:
                LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_VOL_SUB_128_128);
            break;
            case 10:
                LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_VOL_ADD_128_128);
            break;
            case 11:
                LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_DREAM_DRINK_80_64);
            break;
            case 12:
                LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_DREAM_PLANET_80_64);
            break;
            case 13:
                LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_DREAM_STAR_80_64);
            break;
        }
        
    }
    
}


void  free_all_img()
{
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_EARPHONE_NORMAL_128_128);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_EARPHONE_TRANSPARENCY_128_129);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_EARPHONE_DENOISE_128_128);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_EQ_128_128);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_LAST_128_128);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_MUTE_128_128);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_NEXT_128_128);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_PLAY_128_128);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_STOP_128_128);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_VOL_ADD_128_128);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_VOL_SUB_128_128);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_DREAM_DRINK_80_64);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_DREAM_PLANET_80_64);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_DREAM_STAR_80_64);
#if 0
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_00_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_01_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_02_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_03_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_04_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_05_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_06_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_07_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_08_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_09_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_10_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_11_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_12_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_13_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_14_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_15_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_16_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_17_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_18_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_19_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_20_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_21_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_22_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_23_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_24_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_25_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_26_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_27_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_28_256_256);
    LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_CRACK_29_256_256);
#endif
}

void  free_model(uint16_t index)
{
    if(model_cache_list[index])
    {
        LOG_D(" LX_VGWIDGET_INS_EMOJI_ANIMATION ->free_model  (uint16_t)index:%d",(uint16_t)index);
        LX_UNLOAD_MODEL(model_cache_list[index]);
        model_cache_list[index] = NULL;  
    }
    
}

void  free_all_model()
{
   	for(uint8_t i = 0; i < sizeof(model_cache_list)/sizeof(void*); i++)
	{
		if(model_cache_list[i])
		{
			LX_UNLOAD_MODEL(model_cache_list[i]);
			model_cache_list[i] = NULL;
		}
	}

    CURR_MODEL.data = NULL;
    CURR_MODEL.size = 0;
    
}




/**
 * @brief 保存当前screen可见的所有obj指针
 */
static void emoji_save_screen_visible_objs(void)
{
    #if 0 
    lv_obj_t *scr = lv_screen_active();
    uint32_t obj_count = lv_obj_get_child_cnt(scr);
    
    emoji_visible_obj_list = lv_malloc(sizeof(lv_obj_t *) * obj_count);

    if(emoji_visible_obj_list)
    {
        lv_memset(emoji_visible_obj_list, 0x0, sizeof(lv_obj_t *) * obj_count);

        for (int i = 0; i < obj_count; i++)
        {
            lv_obj_t* obj = lv_obj_get_child(scr, i);

            if(lv_obj_is_visible(obj))
            {
                emoji_visible_obj_list[emoji_visible_obj_count] = obj;
                emoji_visible_obj_count++;
            }
        }
    }
    #endif

}

/**
 * @brief 隐藏当前screen可见的所有obj指针
 */
static void emoji_hide_screen_visible_objs(void)
{
    #if 0 
    for (int i = 0; i < emoji_visible_obj_count; i++)
    {
        lv_obj_add_flag(emoji_visible_obj_list[i], LV_OBJ_FLAG_HIDDEN);
    }
    #endif    
}

/**
 * @brief 恢复被隐藏的obj指针
 */
static void emoji_restore_screen_visible_objs(void)
{
    #if 0
    for (int i = 0; i < emoji_visible_obj_count; i++)
    {
        lv_obj_clear_flag(emoji_visible_obj_list[i], LV_OBJ_FLAG_HIDDEN);
    }

    if(emoji_visible_obj_list)
    {
        lv_free(emoji_visible_obj_list);
        emoji_visible_obj_list = NULL;
        emoji_visible_obj_count = 0;
    }
    #endif
}

/**
 * @brief 将当前屏幕的所有对象以及遮罩、黑色圆圈背景截屏后作为背景,同时将其他obj对象隐藏,目的是为了减少LVGL其他obj刷新对帧率的影响
 */
static void emoji_screenshot_replace_popup_bg(void)
{
    lv_obj_t *scr = lv_screen_active();

    emoji_save_screen_visible_objs();

	lv_style_value_t v = { 0 };

	/* 创建遮罩层 */
	lv_obj_t *popup_mask = lv_obj_create(scr);
	lv_obj_set_x(popup_mask, 0);
	lv_obj_set_y(popup_mask, 0);
	lv_obj_set_width(popup_mask, LV_PCT(100));
	lv_obj_set_height(popup_mask, LV_PCT(100));
	//Set Local Styles for popup_mask
	v.num = (int32_t)0x1F1F1F;
	lv_obj_set_local_style_prop(popup_mask, LV_STYLE_BG_COLOR, v, LV_PART_MAIN|LV_STATE_DEFAULT);
	v.num = (int32_t)0xcc;
	lv_obj_set_local_style_prop(popup_mask, LV_STYLE_BG_OPA, v, LV_PART_MAIN|LV_STATE_DEFAULT);

	/* 创建黑色圆圈背景 */
	lv_obj_t *popup_bg = lv_obj_create(scr);
	lv_obj_set_x(popup_bg, 0);
	lv_obj_set_y(popup_bg, 0);
	lv_obj_set_width(popup_bg, 350);
	lv_obj_set_height(popup_bg, 350);
	lv_obj_set_align(popup_bg, LV_ALIGN_CENTER);
	//Set Local Styles for emoji_popup_bg
	v.num = (int32_t)0x0000;
	lv_obj_set_local_style_prop(popup_bg, LV_STYLE_BG_COLOR, v, LV_PART_MAIN|LV_STATE_DEFAULT);
	v.num = (int32_t)0x0;
	lv_obj_set_local_style_prop(popup_bg, LV_STYLE_BORDER_WIDTH, v, LV_PART_MAIN|LV_STATE_DEFAULT);
	v.num = (int32_t)175;
	lv_obj_set_local_style_prop(popup_bg, LV_STYLE_RADIUS, v, LV_PART_MAIN|LV_STATE_DEFAULT);  

    emoji_snapshot_dsc = lv_snapshot_take(scr, LV_COLOR_FORMAT_ARGB8888);

    lv_obj_delete(popup_mask);
    lv_obj_delete(popup_bg);

    emoji_hide_screen_visible_objs();

    if(!emoji_snapshot_dsc) {
        LOG_E_WARN("Failed to take screenshot");
        return;
    }



    emoji_snapshot_img = lv_img_create(scr);
    lv_image_set_src(emoji_snapshot_img, emoji_snapshot_dsc);
    lv_obj_set_size(emoji_snapshot_img, lv_obj_get_width(scr), lv_obj_get_height(scr));
    lv_obj_center(emoji_snapshot_img);
}

/**
 * @brief 删除截屏并恢复被隐藏的obj指针
 */
static void emoji_screen_restore(void)
{
    emoji_restore_screen_visible_objs();

    if(emoji_snapshot_img) {
        lv_obj_del(emoji_snapshot_img);
        emoji_snapshot_img = NULL;
    }
    
    if(emoji_snapshot_dsc) {
        lv_draw_buf_destroy(emoji_snapshot_dsc);
        emoji_snapshot_dsc = NULL;
    }
}

static void touch_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    LOG_D(" lv_example_virtual3d_animated_emoji ->touch_event_handler  (uint16_t)LV_HOR_RES:%d",(uint16_t)LV_HOR_RES);
    LOG_D(" lv_example_virtual3d_animated_emoji ->touch_event_handler  (uint16_t)LV_VER_RES:%d",(uint16_t)LV_VER_RES);
    LOG_D(" lv_example_virtual3d_animated_emoji ->touch_event_handler  (uint16_t)code:%d",(uint16_t)code);
    if(code == LV_EVENT_PRESSED) {
        lv_point_t pt = { 0 };	
        LOG_D(" lv_example_virtual3d_animated_emoji ->touch_event_handler  (uint16_t)LV_EVENT_PRESSED:%d",(uint16_t)LV_EVENT_PRESSED);
        lv_indev_get_point(lv_indev_get_act(), &pt);
        #if defined(__FOLLOW_DEMO_ON__)
            lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"animate_system_standby_smartring_follow");
            lv_virtual3d_set_param2(emoji_virtual3d, "followRange", (uint32_t)80);
        #elif defined(__PRESS_DEMO_ON__)
            lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"animate_system_standby_touchscreen_press");
            //记录其实触摸点
            lastPosX = pt.x;
            lastPosY = pt.y;
		#endif

			
    } 
    else if(code == LV_EVENT_PRESSING) {
        lv_point_t pt = { 0 };	
        LOG_D(" lv_example_virtual3d_animated_emoji ->touch_event_handler  (uint16_t)LV_EVENT_PRESSING:%d",(uint16_t)LV_EVENT_PRESSING);
        lv_indev_get_point(lv_indev_get_act(), &pt);
        #if defined(__FOLLOW_DEMO_ON__)
            lv_virtual3d_set_param2(emoji_virtual3d,  "followAngle", (uint32_t) get_angle_from_pos(pt.x, pt.y, LV_HOR_RES, LV_VER_RES));
        #elif defined(__PRESS_DEMO_ON__)
			int16_t offsetX = pt.x - lastPosX;
			int16_t offsetY = pt.y - lastPosY;

			//根据X和Y方向的位移差，判断是上下滑动，还是左右滑动
			if(abs(offsetX) > abs(offsetY))
			{
				lv_virtual3d_set_param2(emoji_virtual3d, "offsetX", (uint32_t)offsetX);//左右滑动
			}
			else
			{
				lv_virtual3d_set_param2(emoji_virtual3d, "offsetY", (uint32_t)offsetY);//上下滑动
			}
		#endif            
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        lv_point_t pt = { 0 };	
        LOG_D(" lv_example_virtual3d_animated_emoji ->touch_event_handler  (uint16_t)LV_EVENT_RELEASED:%d",(uint16_t)LV_EVENT_RELEASED);
        LOG_D(" lv_example_virtual3d_animated_emoji ->touch_event_handler  (uint16_t)LV_EVENT_PRESS_LOST:%d",(uint16_t)LV_EVENT_PRESS_LOST);
        lv_indev_get_point(lv_indev_get_act(), &pt);
        #if defined(__FOLLOW_DEMO_ON__)
            lv_virtual3d_set_param2(emoji_virtual3d, "followRange", (uint32_t)0);
        #elif defined(__PRESS_DEMO_ON__)
			int16_t offsetX = pt.x - lastPosX;
			int16_t offsetY = pt.y - lastPosY;	
			bool abandon = false;	
			
			if(abs(offsetX) > abs(offsetY))
			{
				if(offsetX > LV_HOR_RES / 2)//滑动超过半屏（请根据实际情况调整）即认为要切换界面，将Emoji移出屏幕
				{
					lv_virtual3d_set_param2(emoji_virtual3d, "offsetX", (uint32_t)LV_HOR_RES);
				}
				else if(offsetX < -LV_HOR_RES / 2)
				{
					lv_virtual3d_set_param2(emoji_virtual3d, "offsetX", (uint32_t)-LV_HOR_RES);
				}
				else
				{
					abandon = true;
				}
			}
			else
			{
				if(offsetY > LV_VER_RES / 2)
				{
					lv_virtual3d_set_param2(emoji_virtual3d, "offsetY", (uint32_t)LV_VER_RES);
				}
				else if(offsetY < -LV_VER_RES / 2)
				{
					lv_virtual3d_set_param2(emoji_virtual3d, "offsetY", (uint32_t)-LV_VER_RES);
				}
				else
				{
					abandon = true;
				}
			}			

			//如果放弃切界面，则加载其他动画Emoji即可居中回正
			if(abandon)
			{
				lv_virtual3d_set_param2(emoji_virtual3d, "setActionByName", (uint32_t)"animate_system_standby_timer_squint");
			}
		#endif

    }
}
/**
 * @brief 受特效传递的参数/指令
 * @param cmd 命令ID
 * @param param 命令参数
 * @param user_data 用户数据
 * @return 处理结果
 */
static uint32_t virtual3d_user_callback(uint16_t cmd, uint32_t param, uint32_t user_data)
{
    lv_obj_t *obj = (lv_obj_t *)user_data;
    uint32_t ret = 0;
    switch (cmd) {
        case LX_CMD_ID_INIT:

            memset(model_cache_list, 0x0, sizeof(model_cache_list));
            next_action_name = NULL;

            lv_virtual3d_set_param2(obj,  "actionCfgTable", (uint32_t)&lx_action_cfg_table);
            lv_virtual3d_set_param2(obj,  "actionCfgTableSize", sizeof(lx_action_cfg_table)/sizeof(lx_action_cfg_item_t));
            lv_virtual3d_set_param2(obj,   "actionSpeed", (uint32_t)100);//设置动作速度，数值越大，速度越快，默认值：100
            lv_virtual3d_set_param2(obj,  "cameraDistance", (uint32_t)70);//设置视距，视距越小，画面越大，默认值：36

            lv_virtual3d_set_param2(obj, "followRange",  (uint32_t)0);//设置头部转向幅度，0~100，默认值：0，仅设置动作"animate_system_standby_smartring_follow"后有效
	        lv_virtual3d_set_param2(obj, "followAngle", (uint32_t)0);//设置头部转向角度，0~360，默认值：0，仅设置动作"animate_system_standby_smartring_follow"后有效
	
            lv_virtual3d_set_param2(obj, "offsetX", (uint32_t)0);//设置头部移动的X偏移，屏幕中心点为0，默认值：0，仅动作"animate_system_standby_touchscreen_press"有效
            lv_virtual3d_set_param2(obj, "offsetY", (uint32_t)0);//设置头部移动的Y偏移，屏幕中心点为0，默认值：0，仅动作"animate_system_standby_touchscreen_press"有效

            lv_virtual3d_set_param2(obj, "antialiasScale", (uint32_t)1200);//填充像素时放大画布，送显时缩小画布达到抗锯齿的效果，但是越大需要的内存越多，帧率降的越多，参数为倍数X1000，小于等于2000时有效，默认值：1073

            if(emoji_popup_mode)//弹出框状态时缩小画面
	        {
	            lv_virtual3d_set_param2(obj, "cameraDistance", (uint32_t)60);//设置视距，视距越小，画面越大，默认值：36
	        }
            break;
        case LX_CMD_ID_DEINIT:

            free_all_model();

            free_all_img();
	        if(emoji_popup_mode)
	        {
	            emoji_screen_restore();
	        }   
	        else
	        {
	            emoji_restore_screen_visible_objs();
	        } 
            lv_obj_remove_event_cb(emoji_virtual3d, touch_event_handler);
            break;

        case LX_CMD_ID_GET_IMAGE:
        {
            static lx_vglite_image_t img;
            uint32_t index =   param ;
            load_img(index);
            lv_img_dsc_t * pDsc = icon_group[index];
        
            img.data = (void*)pDsc->data;
            img.w = pDsc->header.w;
            img.h = pDsc->header.h;
            
            img.format = lx_virtual3d_get_vglite_format(pDsc->header.cf); 

            ret = (uint32_t)&img;  
            break;
        }
        case LX_CMD_ID_GET_MODEL:    

            LOG_D(" LX_VGWIDGET_INS_EMOJI_ANIMATION ->LX_CMD_ID_GET_MODEL  (uint16_t)param:%d",(uint16_t)param);
            return get_model((uint16_t)param);

        case LX_CMD_ID_FREE_MODEL:
            LOG_D(" LX_VGWIDGET_INS_EMOJI_ANIMATION ->LX_CMD_ID_FREE_MODEL  (uint16_t)param:%d",(uint16_t)param);
            free_model((uint16_t)param);

            return 1;

        case LX_CMD_ID_FREE_IMAGE:

            LOG_D("LX_CMD_ID_FREE_IMAGE param:%d\n", param);

            free_img((uint16_t)param);

            return 1;    

        case LX_CMD_ID_UPDATE_STATUS:
        {

            uint8_t percent = (uint8_t)param;

           // LOG_D(" LX_VGWIDGET_INS_EMOJI_ANIMATION ->LX_CMD_ID_UPDATE_STATUS  percent:%d",percent);

            /* 删除界面处理参考
            if(percent == 100)
            {
                if (emoji_virtual3d)
                {
                     lv_obj_del(emoji_virtual3d);  
                     return 1;//必须返回1，通知库当前特效实例被销毁了,库中的方法会及时返回，否则会引起崩溃
                }
            }
            */

            break;
        }

        }



    
    return ret;
}


void red_background_demo(void)
{
    /* 获取当前活动屏幕 */
    lv_obj_t * scr = lv_screen_active();

    /* 创建一个基础对象作为红色背景的容器 */
    lv_obj_t * obj = lv_obj_create(scr);

    /* 设置对象的大小，例如占满整个屏幕 */
    lv_obj_set_size(obj, LV_HOR_RES, LV_VER_RES);

    /* 设置对象的背景颜色为红色 */
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xFF0000), LV_PART_MAIN);

    /* 设置背景完全不透明（覆盖） */
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);

    /* 将对象对齐到屏幕中央（可选） */
    lv_obj_center(obj);
}



static uint32_t virtual3d_user_callback_test(uint16_t cmd, uint32_t param, uint32_t user_data)
{
    lv_obj_t* obj = (lv_obj_t*)user_data;

    switch(cmd) {
    case LX_CMD_ID_INIT:
        LV_LOG("Loading LX_VGWIDGET_INS_DEMO resources...");
        lv_virtual3d_set_image(obj, 0, LX_LOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_MUTE_128_128));
        break;

    case LX_CMD_ID_DEINIT:
        LV_LOG_INFO("Unloading LX_VGWIDGET_INS_DEMO resources...");
        LX_UNLOAD_IMAGE(LX_ANIMATEDEMOJI_MUSIC_MUTE_128_128);
        break;
    }
    return 0;
}



/**
 * @brief 创建3D表情动画显示示例
 */
lv_obj_t * lv_example_virtual3d_animated_emoji(lv_obj_t * p_container)
{

#if 1
    LOG_D("Creating 3D :LX_VGWIDGET_INS_EMOJI_ANIMATION   ");
    emoji_popup_mode = false;

    /* 全屏模式下,隐藏其他obj对象,以免影响刷新帧率 */
    emoji_save_screen_visible_objs();
    emoji_hide_screen_visible_objs();

    /* 创建3D表情动画对象 */
    emoji_virtual3d = lv_virtual3d_create(p_container,
                                            LX_VGWIDGET_INS_EMOJI_ANIMATION, 
                                            EMOJI_BG);
    lv_obj_center(emoji_virtual3d);
    /* 设置回调函数 */
    lv_virtual3d_set_user_callback(emoji_virtual3d, virtual3d_user_callback, 
                                 (uint32_t)emoji_virtual3d);



    lv_obj_add_event_cb(emoji_virtual3d, touch_event_handler, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(emoji_virtual3d, touch_event_handler, LV_EVENT_RELEASED, NULL);    
    lv_obj_add_event_cb(emoji_virtual3d, touch_event_handler, LV_EVENT_PRESSING, NULL);   
    lv_obj_add_event_cb(emoji_virtual3d, touch_event_handler, LV_EVENT_PRESS_LOST, NULL);    

    LOG_D("Creating 3D :LX_VGWIDGET_INS_EMOJI_ANIMATION AAA LV_HOR_RES =%d",LV_HOR_RES);

    LOG_D("Creating 3D :LX_VGWIDGET_INS_EMOJI_ANIMATION BB LV_VER_RES =%d",LV_VER_RES);

    /* 初始化3D效果 */                             
    lv_virtual3d_setup(emoji_virtual3d);

    LOG_D("LX_VGWIDGET_INS_EMOJI_ANIMATION created successfully");
#else
    //red_background_demo();
    LOG_D("Creating 3D :LX_VGWIDGET_INS_DEMO   ");
    emoji_virtual3d = lv_virtual3d_create(lv_screen_active(),
                                            LX_VGWIDGET_INS_DEMO,
                                            EMOJI_BG);
    lv_obj_center(emoji_virtual3d);
    /* 设置回调函数 */
    lv_virtual3d_set_user_callback(emoji_virtual3d, virtual3d_user_callback_test,
                                 (uint32_t)emoji_virtual3d);
    lv_virtual3d_setup(emoji_virtual3d);
#endif


}

void lv_example_virtual3d_animated_emoji_popup(void) 
{
    LOG_D("Creating 3D :LX_VGWIDGET_INS_EMOJI_ANIMATION popup");

    emoji_popup_mode = true;

    /* 用截屏方式替换背景 */
    emoji_screenshot_replace_popup_bg();

    /* 创建3D表情动画对象 */
    emoji_virtual3d = lv_virtual3d_create(lv_screen_active(), 
                                            LX_VGWIDGET_INS_EMOJI_ANIMATION, 
                                            0);
    lv_obj_center(emoji_virtual3d);
    /* 设置回调函数 */
    lv_virtual3d_set_user_callback(emoji_virtual3d, virtual3d_user_callback, 
                                 (uint32_t)emoji_virtual3d);
    lv_obj_add_event_cb(emoji_virtual3d, touch_event_handler, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(emoji_virtual3d, touch_event_handler, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(emoji_virtual3d, touch_event_handler, LV_EVENT_PRESSING, NULL);   
    lv_obj_add_event_cb(emoji_virtual3d, touch_event_handler, LV_EVENT_PRESS_LOST, NULL);
    /* 初始化3D效果 */                             
    lv_virtual3d_setup(emoji_virtual3d);

    //lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"animate_chat_expression_fear_1");


    LOG_D("LX_VGWIDGET_INS_EMOJI_ANIMATION popup created successfully");
}

//RTT STRING
/*static const char *s_emoji_names[EMOJI_NUM] =
{
    "neutral", "happy", "laughing", "funny", "sad", "angry",
    "crying", "loving", "sleepy", "surprised", "shocked",
    "thinking", "winking", "cool", "relaxed", "delicious",
    "kissy", "confident"
};*/

void qday_show_emoji_need_skeep_once()
{
    LOG_D("qday_show_emoji_need_skeep_once  ,set emoji_enable_change = false");
    emoji_enable_change = false;
}

 void qday_show_emoji_by_rtt_info(int index)
 {
     LOG_D("qday_show_emoji_by_rtt_info  index =%d",index);
     LOG_D("qday_show_emoji_by_rtt_info  emoji_enable_change =%d",emoji_enable_change);
     if(emoji_enable_change == false)
     {
         emoji_enable_change = true;
         LOG_D("qday_show_emoji_by_rtt_info  skeep once ,set  emoji_enable_change = true;");
         return;
     }

     switch (index)
     {
     case 0: //neutral‌：中性的
         LOG_D("qday_show_emoji_by_rtt_info  set => follow");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"follow");
         break;

     case 1: //happy‌：快乐的
         LOG_D("qday_show_emoji_by_rtt_info  set => proud");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"proud");
         break;
     case 2://‌laughing‌：大笑的
         LOG_D("qday_show_emoji_by_rtt_info  set => laugh");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"laugh");
         break;
     case 3: //funny‌：有趣的
         LOG_D("qday_show_emoji_by_rtt_info  set => awkward");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"awkward");
         break;
     case 4: //‌sad‌：悲伤的
         LOG_D("qday_show_emoji_by_rtt_info  set => cry");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"cry");
         break;
     case 5: //angry‌：生气的
         LOG_D("qday_show_emoji_by_rtt_info  set => angry");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"angry");
         break;

     case 6: //crying‌：哭泣的
         LOG_D("qday_show_emoji_by_rtt_info  set => cry");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"cry");
         break;
     case 7://loving‌：充满爱的
         LOG_D("qday_show_emoji_by_rtt_info  set => shy");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"shy");
         break;
     case 8://sleepy‌：困倦的
         LOG_D("qday_show_emoji_by_rtt_info  set => sleepy");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"sleepy");
         break;
     case 9://surprised‌：惊讶的
         LOG_D("qday_show_emoji_by_rtt_info  set => anticipation");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"anticipation");
         break;
     case 10://shocked‌：震惊的
         LOG_D("qday_show_emoji_by_rtt_info  set => fear");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"fear");
         break;

     case 11://thinking‌：思考的
         LOG_D("qday_show_emoji_by_rtt_info  set => thinking");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"thinking");
         break;
     case 12://winking‌：眨眼的
         LOG_D("qday_show_emoji_by_rtt_info  set => blink");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"blink");
         break;
     case 13://cool‌：酷的
         LOG_D("qday_show_emoji_by_rtt_info  set => speaking");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"speaking");
         break;
     case 14://relaxed‌：放松的
         LOG_D("qday_show_emoji_by_rtt_info  set => yawn");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"yawn");
         break;
     case 15://delicious‌：美味的
         LOG_D("qday_show_emoji_by_rtt_info  set => thumbup");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"thumbup");
         break;
     case 16://kissy‌：亲吻的
         LOG_D("qday_show_emoji_by_rtt_info  set => poke");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"poke");
         break;
     case 17://confident‌：自信的
         LOG_D("qday_show_emoji_by_rtt_info  set => proud");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"proud");
         break;

     //自定义
     case 100://hearing：倾听
         LOG_D("qday_show_emoji_by_rtt_info  set => hearing");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"hearing");
         break;

     case 101:// 开关降噪-正常
         LOG_D("qday_show_emoji_by_rtt_info  set => tap_earphonemode_normal");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"tap_earphonemode_normal");
         break;
     case 102:// 开关降噪-通透
         LOG_D("qday_show_emoji_by_rtt_info  set => tap_earphonemode_transparencymode");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"tap_earphonemode_transparencymode");
         break;
     case 103:// 开关降噪-降噪
         LOG_D("qday_show_emoji_by_rtt_info  set => tap_earphonemode_denoise");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"tap_earphonemode_denoise");
         break;
     case 104:// 音乐控制-EQ
         LOG_D("qday_show_emoji_by_rtt_info  set => tap_music_eq");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"tap_music_eq");
         break;
     case 105:// 音乐控制-上一首
         LOG_D("qday_show_emoji_by_rtt_info  set => tap_music_last");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"tap_music_last");
         break;
     case 106:// 音乐控制-静音
         LOG_D("qday_show_emoji_by_rtt_info  set => tap_music_mute");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"tap_music_mute");
         break;
     case 107:// 音乐控制-下一首
         LOG_D("qday_show_emoji_by_rtt_info  set => tap_music_next");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"tap_music_next");
         break;
     case 108:// 音乐控制-播放
         LOG_D("qday_show_emoji_by_rtt_info  set => tap_music_play");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"tap_music_play");
         break;
     case 109:// 音乐控制-停止
         LOG_D("qday_show_emoji_by_rtt_info  set => tap_music_stop");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"tap_music_stop");
         break;
     case 110:// 音乐控制-降低音量
         LOG_D("qday_show_emoji_by_rtt_info  set => tap_music_volume-");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionByName", (uint32_t)"tap_music_volume-");
         qday_show_emoji_need_skeep_once();
         break;
     case 111://音乐控制-调高音量
         LOG_D("qday_show_emoji_by_rtt_info  set => tap_music_volume+");
         lv_virtual3d_set_param2(emoji_virtual3d, "setActionByName", (uint32_t)"tap_music_volume+");
         qday_show_emoji_need_skeep_once();
         break;
      default:
          LOG_D("qday_show_emoji_by_rtt_info   default set => speaking");
          lv_virtual3d_set_param2(emoji_virtual3d, "setActionLoopByName", (uint32_t)"speaking");
          break;

     }


 }
