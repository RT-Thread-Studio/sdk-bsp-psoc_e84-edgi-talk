#ifndef USBH_UVC_DISPLAY_HOOK_H
#define USBH_UVC_DISPLAY_HOOK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t *framebuffer;
    uint16_t lcd_width;
    uint16_t lcd_height;
    uint16_t src_width;
    uint16_t src_height;
    uint16_t dst_width;
    uint16_t dst_height;
    uint16_t dst_y_offset;
} uvc_display_overlay_info_t;

typedef void (*uvc_display_overlay_cb_t)(const uvc_display_overlay_info_t *info, void *user_ctx);

void uvc_display_set_overlay_callback(uvc_display_overlay_cb_t cb, void *user_ctx);

#ifdef __cplusplus
}
#endif

#endif /* USBH_UVC_DISPLAY_HOOK_H */
