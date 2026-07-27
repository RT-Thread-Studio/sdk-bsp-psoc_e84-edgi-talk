/**
 * @file lv_virtual3d.h
 *
 */

#ifndef __LV_VIRTUAL3D_H__
#define __LV_VIRTUAL3D_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "vg_lite.h"
#include "lv_obj_private.h"
#include "lv_obj_class_private.h"
#include "lv_timer_private.h"

#if 1//LV_USE_LX_VIRTUAL3D
/*********************
 *      DEFINES
 *********************/
#ifndef LX_HOR_RES
#define LX_HOR_RES  480
#endif

#ifndef LX_VER_RES
#define LX_VER_RES  480
#endif

/**********************
 *      TYPEDEFS
 **********************/
typedef struct _lv_virtual3d_t{
    lv_obj_t obj;
    vg_lite_buffer_t vg_buf;
    uint32_t bg_color;
    uint32_t handler;
    uint32_t instance;
    void* callback;
    bool setuped;
    lv_timer_t *anim_timer;

    int32_t last_pos_x;
    int32_t last_pos_y;
} lv_virtual3d_t;

 extern const lv_obj_class_t lv_virtual3d_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
lv_obj_t * lv_virtual3d_create(lv_obj_t * parent, uint32_t instance, uint32_t bg_color);

void lv_virtual3d_setup(lv_obj_t * obj);

void lv_virtual3d_teardown(lv_obj_t * obj);

void lv_virtual3d_handle_indev_event(lv_obj_t * obj, lv_event_code_t code);

/*=====================
 * Setter functions
 *====================*/
void lv_virtual3d_set_image(lv_obj_t * obj, uint8_t index, const lv_image_dsc_t* image);

void lv_virtual3d_set_image_ex(lv_obj_t * obj, uint8_t index, const lv_image_dsc_t* image, uint32_t prop);

void lv_virtual3d_set_param(lv_obj_t * obj, uint16_t cmd, uint32_t param);

void lv_virtual3d_set_param2(lv_obj_t * obj,  const char* cmd, uint32_t param);

void lv_virtual3d_set_user_callback(lv_obj_t * obj, void* callback, uint32_t user_data);

void lv_virtual3d_set_model(lv_obj_t * obj, uint8_t index, void* data, uint32_t size);

/*=====================
 * Getter functions
 *====================*/
uint32_t lx_virtual3d_get_vglite_format(lv_color_format_t cf);

/**********************
 *      MACROS
 **********************/



#endif /*LV_USE_LX_VIRTUAL3D*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*__LV_VIRTUAL3D_H__*/
