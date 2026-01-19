
/**
 * @file lv_example_virtual3d_animated_emoji.h
 * @brief 3D表情动画显示示例头文件
 */

#ifndef __LV_EXAMPLE_VIRTUAL3D_EMOJI_H__
#define __LV_EXAMPLE_VIRTUAL3D_EMOJI_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "lx_vglite_api.h"


/*********************
 *      DEFINES
 *********************/

#define EMOJI_BG 0xFF000000  // DEFAULT BG COLOR

#define EMOJI_IMG_CRACK_SEQ_START_IND 14  // 有规律的图片起始位置: 设置这个起始位置,预防 icon_group CRACK之前的位置变动,判断的IND全部要调整

#define __FOLLOW_DEMO_ON__
//#define __PRESS_DEMO_ON__
/**********************
 *      TYPEDEFS
 **********************/

typedef struct lx_action_cfg_item
{
    const char* name;//对外的动作名称，比如“Smile”
    uint32_t modelId;//所在模型的ID
    uint16_t actionId;//在模型中的动作ID
    
    int16_t iconFirstId;//显示icon的序列第一帧，-1表示不显示icon
    int16_t iconScale;//icon的缩放百分比，原始大小为100
    int16_t iconNum;//icon序列帧数量, 等于1时表示单张图片
    int16_t iconPosX;//icon显示的x坐标
    int16_t iconPosY;//icon显示的y坐标
    int8_t iconStartPoint;//icon开始显示的时间点，对应动作的百分比，即0表示动作开始的点，100表示动作结束的点
    int8_t iconEndPoint;//icon结束显示的时间点

    uint32_t bgColor;//背景色
    uint32_t layerColor[4];//层的颜色，注：第25~32位如果不是0xff，则表示层缩放的百分比，原始大小为0x64(即100)
} lx_action_cfg_item_t;

/**********************
 * GLOBAL VARIABLES
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * @brief 创建3D表情动画显示示例
 */
lv_obj_t * lv_example_virtual3d_animated_emoji(lv_obj_t * p_container) ;

void lv_example_virtual3d_animated_emoji_popup(void);

void qday_show_emoji_by_rtt_info(int index);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __LV_EXAMPLE_VIRTUAL3D_EMOJI_H__ */
