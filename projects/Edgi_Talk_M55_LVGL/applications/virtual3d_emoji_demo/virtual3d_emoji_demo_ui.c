#include <rtthread.h>
#include <lvgl.h>
#include "virtual3d_emoji_demo_ui.h"
#include "lv_example_virtual3d_animated_emoji.h"
#include "lv_virtual3d.h"

#define DBG_TAG "v3d.demo"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define VIRTUAL3D_DEMO_TITLE "Virtual3D Animated Emoji"
#define VIRTUAL3D_DEMO_HINT "Drag the 3D emoji"

static lv_obj_t *s_emoji;
static lv_obj_t *s_title_label;
static lv_obj_t *s_hint_label;

static void title_create(lv_obj_t *screen, lv_coord_t scr_w)
{
    s_title_label = lv_label_create(screen);
    lv_obj_remove_flag(s_title_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_text_color(s_title_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(s_title_label, &lv_font_montserrat_20, 0);
    lv_obj_set_width(s_title_label, scr_w);
    lv_obj_set_style_text_align(s_title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_title_label, VIRTUAL3D_DEMO_TITLE);
    lv_obj_align(s_title_label, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_move_foreground(s_title_label);
}

static void hint_create(lv_obj_t *screen, lv_coord_t scr_w)
{
    s_hint_label = lv_label_create(screen);
    lv_obj_remove_flag(s_hint_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_text_color(s_hint_label, lv_color_hex(0xb8c7d9), 0);
    lv_obj_set_style_text_font(s_hint_label, &lv_font_montserrat_16, 0);
    lv_obj_set_width(s_hint_label, scr_w);
    lv_obj_set_style_text_align(s_hint_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_hint_label, VIRTUAL3D_DEMO_HINT);
    lv_obj_align(s_hint_label, LV_ALIGN_BOTTOM_MID, 0, -18);
    lv_obj_move_foreground(s_hint_label);
}

void virtual3d_emoji_demo_init(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_coord_t scr_w = lv_display_get_horizontal_resolution(NULL);
    lv_coord_t scr_h = lv_display_get_vertical_resolution(NULL);

    lv_obj_clean(screen);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    s_emoji = lv_example_virtual3d_animated_emoji(screen);
    if (s_emoji != NULL) {
        qday_show_emoji_by_rtt_info(0);
    }
    title_create(screen, scr_w);
    hint_create(screen, scr_w);

    LOG_I("Virtual3D emoji initialized, screen=%dx%d, lx=%dx%d",
          scr_w, scr_h, LX_HOR_RES, LX_VER_RES);
}

#ifdef RT_USING_FINSH
#include <finsh.h>
static void virtual3d_demo_stat(void)
{
    lv_area_t area;
    lv_coord_t scr_w = lv_display_get_horizontal_resolution(NULL);
    lv_coord_t scr_h = lv_display_get_vertical_resolution(NULL);

    rt_kprintf("Virtual3D demo: screen=%dx%d lx=%dx%d offset=(%d,%d)\n",
               scr_w, scr_h, LX_HOR_RES, LX_VER_RES,
               VIRTUAL3D_EMOJI_DEMO_EMOJI_OFFSET_X,
               VIRTUAL3D_EMOJI_DEMO_EMOJI_OFFSET_Y);
    if (s_emoji != NULL)
    {
        lv_obj_get_coords(s_emoji, &area);
        rt_kprintf("emoji: (%d,%d)-(%d,%d) size=%dx%d hidden=%u\n",
                   area.x1, area.y1, area.x2, area.y2,
                   lv_obj_get_width(s_emoji), lv_obj_get_height(s_emoji),
                   (uint32_t)lv_obj_has_flag(s_emoji, LV_OBJ_FLAG_HIDDEN));
    }
    if (s_title_label != NULL)
    {
        lv_obj_get_coords(s_title_label, &area);
        rt_kprintf("title: (%d,%d)-(%d,%d) size=%dx%d hidden=%u\n",
                   area.x1, area.y1, area.x2, area.y2,
                   lv_obj_get_width(s_title_label), lv_obj_get_height(s_title_label),
                   (uint32_t)lv_obj_has_flag(s_title_label, LV_OBJ_FLAG_HIDDEN));
    }
    if (s_hint_label != NULL)
    {
        lv_obj_get_coords(s_hint_label, &area);
        rt_kprintf("hint: (%d,%d)-(%d,%d) size=%dx%d hidden=%u\n",
                   area.x1, area.y1, area.x2, area.y2,
                   lv_obj_get_width(s_hint_label), lv_obj_get_height(s_hint_label),
                   (uint32_t)lv_obj_has_flag(s_hint_label, LV_OBJ_FLAG_HIDDEN));
    }
}
MSH_CMD_EXPORT(virtual3d_demo_stat, show Virtual3D demo layout state);
#endif
