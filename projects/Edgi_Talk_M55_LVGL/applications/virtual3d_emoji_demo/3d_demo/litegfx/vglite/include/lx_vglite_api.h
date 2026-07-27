/******************************************************************************
* Copyright (c) 2023 Shanghai QDay Technology Co., Ltd.
* All rights reserved.
*
* This file is part of the LiteGFX 0.0.1 distribution.
*
* This software is licensed under terms that can be found in the LICENSE file in
* the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
* Author:LiteGFX Team
* Date:2023.11.08
*******************************************************************************/

#ifndef LX_VGLITE_API_H
#define LX_VGLITE_API_H

#if defined(__cplusplus)
extern "C" {
#endif

/*********************
*      INCLUDES
*********************/
#include <stdint.h>
#include <vg_lite.h>
//#include <lx_platform_log.h>

/*********************
*      DEFINES
*********************/
#define LX_VGLITE_LIB_VERSION   "V26.02.02.02"

#define LX_VGWIDGET_INS_DEMO lx_vglite_load_demo_widget()/*仅供调试*/

#define LX_VGWIDGET_INS_EMOJI_ANIMATION lx_vglite_load_emoji_animation_widget()/*表情动画（3D拓展版）*/

/**********************
*      TYPEDEFS
**********************/


enum LX_CMD_ID {
    LX_CMD_ID_BASE = 0,
    LX_CMD_ID_GET_IMAGE,
    LX_CMD_ID_FREE_IMAGE,
    LX_CMD_ID_SET_SELECTED,//在多个场景里 从来设置进入时停在哪个icon上
    LX_CMD_ID_SET_NUMBER,//在多个场景里 从来设置场景中总共有几个icon
    LX_CMD_ID_CLICK,
    LX_CMD_ID_REMOVE,
    LX_CMD_ID_BATTERY,
    LX_CMD_ID_HEARTRATE,
    LX_CMD_ID_KAL,
    LX_CMD_ID_STEP,
    LX_CMD_ID_TEMPR_LOW,//最低温度
    LX_CMD_ID_TEMPR_HI,//最高温度
    LX_CMD_ID_TEMPR_CURR,//当前温度
    LX_CMD_ID_TEMPR_UINT,//温度单位，0：摄氏度，1：华氏度
    LX_CMD_ID_WEATHER,//天气图标的ID
    LX_CMD_ID_PROGRESS,//进度
    LX_CMD_ID_COLOR,//颜色
    LX_CMD_ID_SHOW_TIME,//是否显示时间
    LX_CMD_ID_FIXED_BUTTON,//是否显示固定表盘的按钮
    LX_CMD_ID_TIME_FORMAT,//时间制式，0：24小时制，1：12小时制
    LX_CMD_ID_UPDATE_STATUS,//状态更新
    LX_CMD_ID_LONG_PRESSED,
    LX_CMD_ID_SET_LOOP,//设置循环，0：关闭循环，1：开启循环
    LX_CMD_ID_SET_NODELETE,//标记zoomlist里某个物体是不能被删除的
    LX_CMD_ID_SET_POS,//设置特效显示的坐标位置
    LX_CMD_ID_SET_SCALE,//设置缩放比例
    LX_CMD_ID_SET_MODE,//设置模式
    LX_CMD_ID_SET_FRAMES,//设置帧数
    LX_CMD_ID_SET_CURENT_FRAME,//设置当前为第几帧
    LX_CMD_ID_SET_VALUE,//设置数值
    LX_CMD_ID_SET_TURN_SPEED,//设置旋转速度，可配合编码器使用
    LX_CMD_ID_SET_SCREEN_MASK,//设置屏幕遮罩
    LX_CMD_ID_DO_ACTION,//动一动
    LX_CMD_ID_GET_ENABLE,//获取可用状态
    LX_CMD_ID_GET_MODEL,//动态获取模型
    LX_CMD_ID_FREE_MODEL,//动态释放模型
    LX_CMD_ID_GET_TEXTURE,//动态获取纹理
    LX_CMD_ID_FREE_TEXTURE,//动态释放纹理
    LX_CMD_ID_INIT = 0xfe,
    LX_CMD_ID_DEINIT = 0xff,	
};

typedef bool (*lx_vglite_event_cb_t)(void* context_p, uint8_t event, uint32_t param1, uint32_t param2);

typedef struct lx_vglite_context {
    uint32_t magic_number;
    lx_vglite_event_cb_t event_callback;
    uint32_t user_data;
} lx_vglite_context_t;

enum LX_VGLITE_EVENT {
    LX_VGLITE_EVENT_INIT = 0,
    LX_VGLITE_EVENT_DEINIT,
    LX_VGLITE_EVENT_SETUP,
    LX_VGLITE_EVENT_TEARDOWN,
    LX_VGLITE_EVENT_RESUME,
    LX_VGLITE_EVENT_PAUSE,
    LX_VGLITE_EVENT_UPDATE,
    LX_VGLITE_EVENT_RENDER,
    LX_VGLITE_EVENT_TOUCH,
    LX_VGLITE_EVENT_SET_IMAGE,
    LX_VGLITE_EVENT_SET_PARAM,
    LX_VGLITE_EVENT_SET_USER_CALLBACK,
    LX_VGLITE_EVENT_SET_TEXTURE,
    LX_VGLITE_EVENT_SET_PARAM2,
    LX_VGLITE_EVENT_SET_MODEL,
};

typedef struct lx_canvas_buffer {
    vg_lite_buffer_format_t format;
    int16_t w;
    int16_t h;
    void* buffer; 
} lx_canvas_buffer_t;

typedef struct lx_vglite_image {
    vg_lite_buffer_format_t format;
    int16_t w;
    int16_t h;
    void* data; 
    uint32_t prop;//用于特定的特效
} lx_vglite_image_t;

typedef struct lx_vglite_model {
    void* data; 
    uint32_t size;
} lx_vglite_model_t;

typedef struct lx_point {
	int16_t x; /* x coord */
	int16_t y; /* y coord */
} lx_point_t;

typedef struct lx_obj_point {
    int16_t index;
    int16_t x; /* relative x coord */
    int16_t y; /* relative y coord */
} lx_obj_point_t;

typedef struct lx_rect {
	int16_t x; /* x coord */
	int16_t y; /* y coord */
	int16_t w; /* row width in pixel */
	int16_t h; /* column height in pixel */
} lx_rect_t;

typedef enum {
    LX_TOUCH_UP = 0,
    LX_TOUCH_DOWN,
    LX_TOUCH_MOVE
} lx_touch_state_t;

typedef uint32_t (*lx_vglite_user_cb_t)(uint16_t cmd, uint32_t param, uint32_t user_data);

/**********************
* GLOBAL PROTOTYPES
**********************/
uint32_t lx_vglite_load_demo_widget();

uint32_t lx_vglite_load_emoji_animation_widget();

/*
    接口名称：lx_vglite_buffer_init
    功能：将 buffer 转化为 vglite buffer
    返回类型：vg_lite_error_t（参考《vg_lite.h》）
    参数 1：lx_canvas_buffer_t* cb，该参数将 buffer 的地址、宽、高和格式（16 位或 24 位）打
    包传入方法
    参数 2：vg_lite_buffer_t* vb，该参数为转化后的 vglite buffer
*/
vg_lite_error_t lx_vglite_buffer_init(lx_canvas_buffer_t* cb, vg_lite_buffer_t* vb);

/*
    功能：释放 vglite buffer
    返回类型：vg_lite_error_t（参考《vg_lite.h》）
    参数：vg_lite_buffer_t* vb，该参数为待释放的 vglite buffer
*/
vg_lite_error_t lx_vglite_buffer_deinit(vg_lite_buffer_t* vb);

/*
    功能：特效实例初始化
    返回类型：uint32_t，该方法返回的为该特效实例的 handler，通过 handler 可以对该特效实例
    进行系列操作
    参 数 1 ： uint32_t instance，该参数为特效类型实例，例如传入参数“LX_VGWIDGET_INS_PRISM”
    参数 2：vg_lite_buffer_t *buf，该参数为转换后的 vglite buffer
    参数 3、4：vglite buffer 对应的宽和高
*/
uint32_t lx_vglite_init(uint32_t instance, vg_lite_buffer_t *buf, int16_t width, int16_t height);

/*
    功能：释放特效实例
    参数：uint32_t handler，待释放特效实例对应的 handler
    注：特效如果不被释放，则特效的运行状态会被保留
*/
void lx_vglite_deinit(uint32_t handler);

/*
    功能：特效界面构建、布局
    参数：uint32_t handler，该特效实例对应的 handler
*/
void lx_vglite_setup(uint32_t handler);

/*
    功能：特效界面恢复
    参数：uint32_t handler，该特效实例对应的 handler
*/
void lx_vglite_resume(uint32_t handler);

/*
    功能：特效界面暂停
    参数：uint32_t handler，该特效实例对应的 handler
*/
void lx_vglite_pause(uint32_t handler);

/*
    功能：特效界面销毁
    参数：uint32_t handler，该特效实例对应的 handler
*/
void lx_vglite_teardown(uint32_t handler);

/*
    功能：特效界面更新
    参数：uint32_t handler，该特效实例对应的 handler
    注：须以每秒 30 帧（默认值，可根据特效情况微调）的频率调用本接口，调用完本接口后需要触发界面渲染
*/
void lx_vglite_update(uint32_t handler);

/*
    功能：特效界面更新
    参数 1：uint32_t handler，该特效实例对应的 handler
    参数 2：vg_lite_buffer_t *buf，该 vglite buffer 用于渲染过场特效的某一帧画面,主要用于双framebuffer的情况
    注：须以每秒 30 帧（默认值，可根据特效情况微调）的频率调用本接口，调用完本接口后需要触发界面渲染
*/
void lx_vglite_update_ex(uint32_t handler, vg_lite_buffer_t *buf);

/*
    功能：特效界面渲染
    参数 1：uint32_t handler，该特效实例对应的 handler
    参数 2～5：渲染区域坐标（通过 lvgl 接口渲染时，会进行分块）
*/
void lx_vglite_render(uint32_t handler, int16_t x1, int16_t x2, int16_t y1, int16_t y2);

/*
    功能：填充特效所需的图像
    参数 1：uint32_t handler，该特效实例对应的 handler
    参数 2：uint16_t index，图像对应的索引值
    参数 3：void* data，图像 buffer 的地址
    参数 4：vg_lite_buffer_format_t format，图像格式（见《vg_lite.h》）
    参数 5、6：图像的宽和高
    注：须在 lx_vglite_setup()方法调用前填充
*/
void lx_vglite_set_image(uint32_t handler, uint16_t index, void* data, vg_lite_buffer_format_t format, int16_t w, int16_t h);

/*
    功能：填充特效所需的图像(增强版)
    参数 1：uint32_t handler，该特效实例对应的 handler
    参数 2：uint16_t index，图像对应的索引值
    参数 3：void* data，图像 buffer 的地址
    参数 4：vg_lite_buffer_format_t format，图像格式（见《vg_lite.h》）
    参数 5、6：图像的宽和高
    参数 7: 图像对应的属性,应用于特定的特效,具体见每个特效的说明
    注：须在 lx_vglite_setup()方法调用前填充
*/
void lx_vglite_set_image_ex(uint32_t handler, uint16_t index, void* data, vg_lite_buffer_format_t format, int16_t w, int16_t h, uint32_t prop);

/*
    功能：填充3D拓展版中模型所需的纹理
    参数 1：uint32_t handler，该特效实例对应的 handler
    参数 2：uint16_t index，纹理对应的索引值
    参数 3：void* data，纹理 buffer 的地址
    参数 4：vg_lite_buffer_format_t format，纹理格式（见《vg_lite.h》）
    参数 5、6：纹理的宽和高
    注：须在 lx_vglite_setup()方法调用前填充
*/
void lx_vglite_set_texture(uint32_t handler, uint16_t index, void* data, vg_lite_buffer_format_t format, int16_t w, int16_t h);


/*
    功能：填充3D拓展版中模型数据
    参数 1：uint32_t handler，该特效实例对应的 handler
    参数 2：uint16_t index，模型对应的索引值
    参数 3：void* data，模型 buffer 的地址
    参数 4：uint32_t size，模型 buffer大小
    注：须在 lx_vglite_setup()方法调用前填充
*/
void lx_vglite_set_model(uint32_t handler, uint16_t index, void* data, uint32_t size);

/*
    功能：填充3D拓展版中模型所需的纹理(增强版)
    参数 1：uint32_t handler，该特效实例对应的 handler
    参数 2：uint16_t index，纹理对应的索引值
    参数 3：void* data，纹理 buffer 的地址
    参数 4：vg_lite_buffer_format_t format，纹理格式（见《vg_lite.h》）
    参数 5、6：纹理的宽和高
    参数 7: 纹理对应的属性,应用于特定的特效,具体见每个特效的说明
    注：须在 lx_vglite_setup()方法调用前填充
*/
void lx_vglite_set_texture_ex(uint32_t handler, uint16_t index, void* data, vg_lite_buffer_format_t format, int16_t w, int16_t h, uint32_t prop);

/*
    功能：传递触摸屏事件
    参数 1：uint32_t handler，该特效实例对应的 handler
    参数 2： lx_touch_state_t state,触摸状态（lx_touch_state_t 定义 见《lx_vglite_api.h》）
    参数 3、4：触摸坐标
*/
void lx_vglite_touch(uint32_t handler, lx_touch_state_t state, int16_t x, int16_t y);

/*
    功能：向特效传递需要的参数，比如当前选中项，电池电量等
    参数 1：uint32_t handler，该特效实例对应的 handler
    参数 2：uint16_t cmd，参数 id，（见《lx_vglite_api.h》中定义的“LX_CMD_ID”）
    参数 3：uint32_t param，参数值
*/
void lx_vglite_set_param(uint32_t handler, uint16_t cmd, uint32_t param);

/*
    功能：向特效传递需要的参数
    参数 1：uint32_t handler，该特效实例对应的 handler
    参数 2：const char* cmd，指令字符串
    参数 3：uint32_t param，参数值
*/
void lx_vglite_set_param2(uint32_t handler, const char* cmd, uint32_t param);

/*
    功能：向特效注册回调，以便接受特效传递的参数/指令，比如当前选中项等
    参数 1：uint32_t handler，该特效实例对应的 handler
    参数 2：lx_vglite_user_cb_t callback，需要注册的回调函数(lx_vglite_user_cb_t 定义见《lx_vglite_api.h》)
    参数 3：uint32_t user_data，用户数据，特效调用回调函数时会带上该值    
*/
void lx_vglite_set_user_callback(uint32_t handler, lx_vglite_user_cb_t callback, uint32_t user_data);



/*
    功能：过场特效构建
    参数 1：uint32_t handler，该特效实例对应的 handler
    参数 bool return_back，false时为入场特效，true时为出场特效
*/
void lx_vglite_trans_setup(uint32_t handler, bool return_back);

/*
    功能：过场特效销毁
    参数：uint32_t handler，该特效实例对应的 handler
*/
void lx_vglite_trans_teardown(uint32_t handler);

/*
    功能：过场特效更新
    参数 1：uint32_t handler，该特效实例对应的 handler
    参数 2：vg_lite_buffer_t *buf，该 vglite buffer 用于渲染过场特效的某一帧画面
    参数 3：uint8_t percent，过场特效进展的百分比，特效会根据当前进展的百分比渲染画面
*/
void lx_vglite_trans_update(uint32_t handler, vg_lite_buffer_t *buf, uint8_t percent);

/*
    功能：过场特效渲染
    参数 1：uint32_t handler，该特效实例对应的 handler
    参数 2～5：渲染区域坐标
*/
void lx_vglite_trans_render(uint32_t handler, int16_t x1, int16_t y1, int16_t x2, int16_t y2);

/*
    功能：设置过场特效旧界面的图像与新界面的图像
    参数 1：uint32_t handler，该特效实例对应的 handler
    参数 2：uint16_t index，index 为 0 则为旧的界面，index 为 1 则为新的界面
    参数 3：void* data,图像 buffer 的地址
    参数 vg_lite_buffer_format_t format，图像格式（见《vg_lite.h》）
    参数 5、6：图像的宽和高
    注：须在 lx_vglite_trans_setup()方法调用前设置
*/
void lx_vglite_trans_set_src(uint32_t handler, uint16_t index, void* data, vg_lite_buffer_format_t format, int16_t w, int16_t h);

/*
    功能：判断设备是否已经授权
*/
bool lx_is_auth_existed();

/**********************
* INTERNAL PROTOTYPES
**********************/
void* lx_vglite_malloc(size_t sz);
void lx_vglite_free(void *ptr);
lx_vglite_context_t* lx_vglite_context_create();
void lx_vglite_context_free(lx_vglite_context_t* context_p);
bool lx_vglite_context_check(lx_vglite_context_t *context_p);
void lx_vglite_set_event_callback(lx_vglite_context_t* ctx, lx_vglite_event_cb_t callback, uint32_t user_data);

#if defined(__cplusplus)
}
#endif

#endif // LX_VGLITE_API_H
