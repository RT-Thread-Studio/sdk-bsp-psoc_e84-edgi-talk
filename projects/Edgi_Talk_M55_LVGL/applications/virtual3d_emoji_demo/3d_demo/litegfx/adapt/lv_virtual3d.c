/**
 * @file lv_virtual3d.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_virtual3d.h"
#include <stdint.h>
#include <string.h>
#include "lv_vg_lite_utils.h"

#if 1//LV_USE_LX_VIRTUAL3D
// #include "lv_vglite_buf.h"
#include "lx_vglite_api.h"
#include <rtthread.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_virtual3d_class

#ifndef LX_VIRTUAL3D_EXT_DRAW_SIZE
#define LX_VIRTUAL3D_EXT_DRAW_SIZE LV_MAX(LV_HOR_RES, LV_VER_RES)
#endif

/**********************
 *      TYPEDEFS
 **********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/
static void virtual3d_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void virtual3d_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void virtual3d_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void virtual3d_timer_cb(lv_timer_t* timer);
static void virtual3d_update_pos(lv_obj_t * obj);
static void virtual3d_clear_bg(lv_obj_t * obj);
static void virtual3d_set_fullscreen_click_area(lv_obj_t * obj);
static void virtual3d_get_local_point(lv_obj_t * obj, lv_point_t * pt);
static void virtual3d_get_draw_area(lv_obj_t * obj, lv_area_t * draw_area);
static bool virtual3d_get_render_area(lv_obj_t * obj, const lv_area_t * layer_area, lv_area_t * render_area);

/**********************
 *  STATIC VARIABLES
 **********************/
static uint32_t s_virtual3d_draw_calls;
static uint32_t s_virtual3d_skip_calls;
static uint32_t s_virtual3d_no_layer_calls;
static lv_area_t s_virtual3d_last_layer_area;
static lv_area_t s_virtual3d_last_render_area;
static lv_area_t s_virtual3d_last_obj_area;
static uint32_t s_virtual3d_last_draw_w;
static uint32_t s_virtual3d_last_draw_h;
static uint32_t s_virtual3d_last_draw_stride;
static uint32_t s_virtual3d_last_draw_cf;
static uint32_t s_virtual3d_last_vg_w;
static uint32_t s_virtual3d_last_vg_h;
static uint32_t s_virtual3d_last_vg_stride;
static uint32_t s_virtual3d_last_handler;
static uint32_t s_virtual3d_last_setuped;

const lv_obj_class_t lv_virtual3d_class = {
    .constructor_cb = virtual3d_constructor,
    .destructor_cb = virtual3d_destructor,
    .event_cb = virtual3d_event,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_virtual3d_t),
    .base_class = &lv_obj_class
};


/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
* @brief Create virtual 3d object.
* @param parent: parent obj. @ref lv_obj_create
* @return object created.
*/
lv_obj_t * lv_virtual3d_create(lv_obj_t * parent, uint32_t instance, uint32_t bg_color)
{
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);

    lv_virtual3d_t * virtual3d = (lv_virtual3d_t *)obj;

    virtual3d->vg_buf.width = LX_HOR_RES;
    virtual3d->vg_buf.height = LX_VER_RES;
    virtual3d->handler = 0;
    virtual3d->callback = NULL;
    virtual3d->setuped = false;
    virtual3d->instance = instance;
    virtual3d->bg_color = bg_color;
    virtual3d->last_pos_x = 0;
    virtual3d->last_pos_y = 0;


    lv_obj_class_init_obj(obj);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_width(obj, LX_HOR_RES);
    lv_obj_set_height(obj, LX_VER_RES);
    virtual3d_set_fullscreen_click_area(obj);
    lv_obj_refresh_ext_draw_size(obj);

    return obj;
}

void lv_virtual3d_setup(lv_obj_t * obj)
{
    lv_virtual3d_t * virtual3d = (lv_virtual3d_t *)obj;

    if(virtual3d->handler && virtual3d->setuped == false)
    {
        if(virtual3d->callback)
        {
            lx_vglite_user_cb_t cb = (lx_vglite_user_cb_t)virtual3d->callback;
            cb(LX_CMD_ID_INIT, 0, (uint32_t)obj);
        }

        virtual3d_update_pos(obj);
        lx_vglite_setup(virtual3d->handler);
        virtual3d->setuped = true;

        if(virtual3d->anim_timer == NULL)
        {
            virtual3d->anim_timer = lv_timer_create(virtual3d_timer_cb, 50, (void*)virtual3d);
            lv_timer_set_repeat_count(virtual3d->anim_timer, -1);            
        }
    }
}

void lv_virtual3d_teardown(lv_obj_t * obj)
{
    lv_virtual3d_t * virtual3d = (lv_virtual3d_t *)obj;

     if(virtual3d->handler && virtual3d->setuped)
     {
          lx_vglite_teardown(virtual3d->handler);
          virtual3d->setuped = false;

          if(virtual3d->callback)
          {
              lx_vglite_user_cb_t cb = (lx_vglite_user_cb_t)virtual3d->callback;
              cb(LX_CMD_ID_DEINIT, 0, (uint32_t)obj);
          }

          if(virtual3d->anim_timer)
          {
              lv_timer_del(virtual3d->anim_timer);
              virtual3d->anim_timer = NULL;
          }
     }    
}

void lv_virtual3d_handle_indev_event(lv_obj_t * obj, lv_event_code_t code)
{
    lv_virtual3d_t *virtual3d = (lv_virtual3d_t*)obj;
    lv_point_t pt = { 0 };
    uint32_t touch_type;

    if(virtual3d == NULL || !virtual3d->handler || !virtual3d->setuped) {
        return;
    }

    if(code == LV_EVENT_PRESSED) {
        touch_type = LX_TOUCH_DOWN;
    }
    else if(code == LV_EVENT_PRESSING) {
        touch_type = LX_TOUCH_MOVE;
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        touch_type = LX_TOUCH_UP;
    }
    else {
        return;
    }

    lv_indev_get_point(lv_indev_get_act(), &pt);
    lx_vglite_touch(virtual3d->handler, touch_type, pt.x, pt.y);
}

/*=====================
 * Setter functions
 *====================*/
 /**
* @brief  add 1 item.
* @param  obj: virtual 3d object
* @param  image: item image.
* @return int16_t : item id added or -1 if error happend.
*/
void lv_virtual3d_set_image_ex(lv_obj_t * obj, uint8_t index, const lv_image_dsc_t* image, uint32_t prop)
{
    lv_virtual3d_t * virtual3d = (lv_virtual3d_t *)obj;

    if(virtual3d->handler)
    {
        LV_LOG("lv_virtual3d_set_image_ex  image->header.cf =%d\n",image->header.cf);
        vg_lite_buffer_format_t format = (vg_lite_buffer_format_t)lv_vg_lite_vg_fmt((lv_color_format_t)image->header.cf);
        void* image_data = (void*)image->data;
        LV_LOG("lv_virtual3d_set_image_ex  format =%d\n",format);

         /*
        if(image->header.cf == LV_COLOR_FORMAT_RGB565A8)
        {
            format = vglite_get_buf_format(LV_COLOR_FORMAT_RGB565A8);
        }
        else if(image->header.cf == LV_COLOR_FORMAT_RGB888)
        {
            format = vglite_get_buf_format(LV_COLOR_FORMAT_RGB888);
        }        
        else if(image->header.cf == LV_COLOR_FORMAT_ARGB8888)
        {
            format = vglite_get_buf_format(LV_COLOR_FORMAT_ARGB8888);;
        }
        else if(image->header.cf == LV_COLOR_FORMAT_ETC2)
        {
            format = vglite_get_buf_format(LV_COLOR_FORMAT_ETC2);
        }
         */

        lx_vglite_set_image_ex(virtual3d->handler, index, image_data, format, image->header.w, image->header.h, prop);
    }

}

void lv_virtual3d_set_image(lv_obj_t * obj, uint8_t index, const lv_image_dsc_t* image)
{
    lv_virtual3d_set_image_ex(obj, index, image, 0);
}


void lv_virtual3d_set_texture_ex(lv_obj_t * obj, uint8_t index, const lv_image_dsc_t* image, uint32_t prop)
{
    lv_virtual3d_t * virtual3d = (lv_virtual3d_t *)obj;

    if(virtual3d->handler)
    {
        vg_lite_buffer_format_t format = (vg_lite_buffer_format_t)lv_vg_lite_vg_fmt(image->header.cf);
        void* image_data = (void*)image->data;

        lx_vglite_set_texture_ex(virtual3d->handler, index, image_data, format, image->header.w, image->header.h, prop);
    }

}

void lv_virtual3d_set_texture(lv_obj_t * obj, uint8_t index, const lv_image_dsc_t* image)
{
    lv_virtual3d_set_texture_ex(obj, index, image, 0);
}


void lv_virtual3d_set_model(lv_obj_t * obj, uint8_t index, void* data, uint32_t size)
{
    lv_virtual3d_t * virtual3d = (lv_virtual3d_t *)obj;

    if(virtual3d->handler)
    {
        lx_vglite_set_model(virtual3d->handler, index, data, size);
    }
}

void lv_virtual3d_set_param(lv_obj_t * obj, uint16_t cmd, uint32_t param)
{
    lv_virtual3d_t * virtual3d = (lv_virtual3d_t *)obj;

    if(virtual3d->handler)
    {
        lx_vglite_set_param(virtual3d->handler, cmd, param);
    }

}

void lv_virtual3d_set_param2(lv_obj_t * obj,  const char* cmd, uint32_t param)
{
    lv_virtual3d_t * virtual3d = (lv_virtual3d_t *)obj;

    if(virtual3d->handler)
    {
        lx_vglite_set_param2(virtual3d->handler, cmd, param);
    }

}

void lv_virtual3d_set_user_callback(lv_obj_t * obj, void* callback, uint32_t user_data)
{
     lv_virtual3d_t * virtual3d = (lv_virtual3d_t *)obj;

     if(virtual3d->handler)
     {
        virtual3d->callback = callback;
        lx_vglite_set_user_callback(virtual3d->handler, (lx_vglite_user_cb_t)callback, user_data);
     }
}

/*=====================
 * Getter functions
 *====================*/
uint32_t lx_virtual3d_get_vglite_format(lv_color_format_t cf)
{
    vg_lite_buffer_format_t vg_buffer_format = VG_LITE_RGB565;

    switch(cf) {
        case LV_COLOR_FORMAT_L8:
            vg_buffer_format = VG_LITE_L8;
            break;
        case LV_COLOR_FORMAT_A4:
            vg_buffer_format = VG_LITE_A4;
            break;
        case LV_COLOR_FORMAT_A8:
            vg_buffer_format = VG_LITE_A8;
            break;
        case LV_COLOR_FORMAT_I1:
            vg_buffer_format = VG_LITE_INDEX_1;
            break;
        case LV_COLOR_FORMAT_I2:
            vg_buffer_format = VG_LITE_INDEX_2;
            break;
        case LV_COLOR_FORMAT_I4:
            vg_buffer_format = VG_LITE_INDEX_4;
            break;
        case LV_COLOR_FORMAT_I8:
            vg_buffer_format = VG_LITE_INDEX_8;
            break;
        case LV_COLOR_FORMAT_RGB565:
            vg_buffer_format = VG_LITE_RGB565;
            break;
        case LV_COLOR_FORMAT_RGB565A8:
            vg_buffer_format = VG_LITE_ARGB8565;
            break;
        case LV_COLOR_FORMAT_RGB888:
            vg_buffer_format = VG_LITE_RGB888;
            break;
        case LV_COLOR_FORMAT_ARGB8888:
            vg_buffer_format = VG_LITE_RGBA8888;
            break;
        case LV_COLOR_FORMAT_XRGB8888:
            vg_buffer_format = VG_LITE_RGBX8888;
            break;
/*        case LV_COLOR_FORMAT_ETC2:
            vg_buffer_format = VG_LITE_RGBA8888_ETC2_EAC;
            break;*/

        default:
            break;
    }

    return (uint32_t)vg_buffer_format;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void virtual3d_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
     lv_virtual3d_t * virtual3d = (lv_virtual3d_t *)obj;

     virtual3d->handler = lx_vglite_init(virtual3d->instance, (vg_lite_buffer_t *)&virtual3d->vg_buf, LX_HOR_RES, LX_VER_RES);

}

static void virtual3d_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
     lv_virtual3d_t * virtual3d = (lv_virtual3d_t *)obj;

     if(virtual3d->handler)
     {
          lv_virtual3d_teardown(obj);

          lx_vglite_deinit(virtual3d->handler);
          virtual3d->handler = 0;
     }
}

static void virtual3d_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
      LV_UNUSED(class_p);
      lv_res_t res;
      /*Call the ancestor's event handler*/
      res = lv_obj_event_base(MY_CLASS, e);
      if(res != LV_RES_OK)
      {
          return;
      }
      lv_event_code_t code = lv_event_get_code(e);
      lv_obj_t* obj = lv_event_get_target(e);

      lv_virtual3d_t *virtual3d = (lv_virtual3d_t*)obj;

      if(code == LV_EVENT_PRESSED)
      {
          lv_virtual3d_handle_indev_event(obj, code);
      }
      else if(code == LV_EVENT_PRESSING)
      {
          lv_virtual3d_handle_indev_event(obj, code);
      }
      else if(code == LV_EVENT_RELEASED)
      {
          lv_virtual3d_handle_indev_event(obj, code);
      }
      else if(code == LV_EVENT_DRAW_MAIN)
      {
          if(virtual3d->setuped)
          {
              lv_layer_t* layer = lv_event_get_layer(e);
              lv_area_t render_area;

              if(layer == NULL || layer->draw_buf == NULL) {
                  s_virtual3d_no_layer_calls++;
                  return;
              }

              if(!virtual3d_get_render_area(obj, &layer->buf_area, &render_area)) {
                  s_virtual3d_skip_calls++;
                  return;
              }

              lv_obj_get_coords(obj, &s_virtual3d_last_obj_area);
              s_virtual3d_last_layer_area = layer->buf_area;
              s_virtual3d_last_render_area = render_area;
              s_virtual3d_last_draw_w = layer->draw_buf->header.w;
              s_virtual3d_last_draw_h = layer->draw_buf->header.h;
              s_virtual3d_last_draw_stride = layer->draw_buf->header.stride;
              s_virtual3d_last_draw_cf = layer->draw_buf->header.cf;

              lv_vg_lite_buffer_from_draw_buf(&virtual3d->vg_buf, layer->draw_buf);

              if(virtual3d->bg_color) {
                  virtual3d_clear_bg(obj);
              }

              s_virtual3d_draw_calls++;
              s_virtual3d_last_vg_w = virtual3d->vg_buf.width;
              s_virtual3d_last_vg_h = virtual3d->vg_buf.height;
              s_virtual3d_last_vg_stride = virtual3d->vg_buf.stride;
              s_virtual3d_last_handler = virtual3d->handler;
              s_virtual3d_last_setuped = virtual3d->setuped;
              
              lx_vglite_render(virtual3d->handler, 
                              layer->buf_area.x1,
                              layer->buf_area.y1,
                              layer->buf_area.x2,
                              layer->buf_area.y2);
              vg_lite_finish();
          }
      }
      else if(code == LV_EVENT_REFR_EXT_DRAW_SIZE)
      {
          lv_event_set_ext_draw_size(e, LX_VIRTUAL3D_EXT_DRAW_SIZE);
      }
      else if(code == LV_EVENT_GET_SELF_SIZE)
      {
          lv_point_t * p = lv_event_get_param(e);
          p->x = LX_HOR_RES;
          p->y = LX_VER_RES;
      }
}

/**
* @brief  rotation anim timer callback.
* @param  timer
* @return none
*/
static void virtual3d_timer_cb(lv_timer_t* timer)
{
    lv_virtual3d_t * virtual3d = (lv_virtual3d_t *)timer->user_data;

    if(virtual3d->handler)
    {
        virtual3d_update_pos((lv_obj_t *)virtual3d);
        
        lx_vglite_update(virtual3d->handler);

        lv_obj_invalidate((const lv_obj_t *)virtual3d);
    }
}

static void virtual3d_update_pos(lv_obj_t * obj)
{
    lv_virtual3d_t * virtual3d = (lv_virtual3d_t *)obj;

    lv_area_t area = { 0 };

    lv_obj_get_coords(obj, &area);

    if(virtual3d->last_pos_x != area.x1 || virtual3d->last_pos_y != area.y1)
    {
        uint32_t pos = (area.x1 << 16) | (area.y1 & 0xffff);
        lx_vglite_set_param(virtual3d->handler, LX_CMD_ID_SET_POS, (uintptr_t)pos);

        virtual3d->last_pos_x = area.x1;
        virtual3d->last_pos_y = area.y1;
    }
}

static void virtual3d_clear_bg(lv_obj_t * obj)
{
    lv_virtual3d_t * virtual3d = (lv_virtual3d_t *)obj;

    vg_lite_clear((vg_lite_buffer_t *)&virtual3d->vg_buf, NULL, virtual3d->bg_color);
}

static void virtual3d_set_fullscreen_click_area(lv_obj_t * obj)
{
    int32_t ext_x = LV_MAX((LV_HOR_RES - LX_HOR_RES + 1) / 2, 0);
    int32_t ext_y = LV_MAX((LV_VER_RES - LX_VER_RES + 1) / 2, 0);
    int32_t ext = LV_MAX(ext_x, ext_y);

    if(ext > 0) {
        lv_obj_set_ext_click_area(obj, ext);
    }
}

static void virtual3d_get_local_point(lv_obj_t * obj, lv_point_t * pt)
{
    lv_area_t area;

    lv_obj_get_coords(obj, &area);
    pt->x -= area.x1;
    pt->y -= area.y1;

    if(pt->x < 0) pt->x = 0;
    if(pt->y < 0) pt->y = 0;
    if(pt->x >= LX_HOR_RES) pt->x = LX_HOR_RES - 1;
    if(pt->y >= LX_VER_RES) pt->y = LX_VER_RES - 1;
}

static void virtual3d_get_draw_area(lv_obj_t * obj, lv_area_t * draw_area)
{
    LV_UNUSED(obj);

    draw_area->x1 = 0;
    draw_area->y1 = 0;
    draw_area->x2 = LV_HOR_RES - 1;
    draw_area->y2 = LV_VER_RES - 1;
}

static bool virtual3d_get_render_area(lv_obj_t * obj, const lv_area_t * layer_area, lv_area_t * render_area)
{
    lv_area_t draw_area;

    if((layer_area == NULL) || (render_area == NULL)) {
        return false;
    }

    virtual3d_get_draw_area(obj, &draw_area);

    render_area->x1 = LV_MAX(layer_area->x1, draw_area.x1);
    render_area->y1 = LV_MAX(layer_area->y1, draw_area.y1);
    render_area->x2 = LV_MIN(layer_area->x2, draw_area.x2);
    render_area->y2 = LV_MIN(layer_area->y2, draw_area.y2);

    return (render_area->x1 <= render_area->x2) && (render_area->y1 <= render_area->y2);
}

#ifdef RT_USING_FINSH
#include <finsh.h>
static void virtual3d_render_stat(void)
{
    rt_kprintf("Virtual3D render: draw=%u skip=%u no_layer=%u handler=0x%08x setuped=%u lx=%dx%d\n",
               s_virtual3d_draw_calls, s_virtual3d_skip_calls, s_virtual3d_no_layer_calls,
               s_virtual3d_last_handler, s_virtual3d_last_setuped, LX_HOR_RES, LX_VER_RES);
    rt_kprintf("obj: (%d,%d)-(%d,%d)\n",
               s_virtual3d_last_obj_area.x1, s_virtual3d_last_obj_area.y1,
               s_virtual3d_last_obj_area.x2, s_virtual3d_last_obj_area.y2);
    rt_kprintf("layer: (%d,%d)-(%d,%d) draw_buf=%ux%u stride=%u cf=%u\n",
               s_virtual3d_last_layer_area.x1, s_virtual3d_last_layer_area.y1,
               s_virtual3d_last_layer_area.x2, s_virtual3d_last_layer_area.y2,
               s_virtual3d_last_draw_w, s_virtual3d_last_draw_h,
               s_virtual3d_last_draw_stride, s_virtual3d_last_draw_cf);
    rt_kprintf("render: (%d,%d)-(%d,%d) vg=%ux%u stride=%u\n",
               s_virtual3d_last_render_area.x1, s_virtual3d_last_render_area.y1,
               s_virtual3d_last_render_area.x2, s_virtual3d_last_render_area.y2,
               s_virtual3d_last_vg_w, s_virtual3d_last_vg_h,
               s_virtual3d_last_vg_stride);
}
MSH_CMD_EXPORT(virtual3d_render_stat, show Virtual3D render state);
#endif

#endif //LV_USE_LX_VIRTUAL3D
