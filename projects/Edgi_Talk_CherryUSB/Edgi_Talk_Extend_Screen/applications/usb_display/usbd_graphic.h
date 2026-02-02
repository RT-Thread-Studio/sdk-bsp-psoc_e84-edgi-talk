#ifndef USBD_GRAPHIC_H
#define USBD_GRAPHIC_H

#include <stdint.h>
#include "usbd_core.h"

struct usb_display_res
{
    uint16_t width;
    uint16_t height;
    uint16_t fps;
} __attribute__((packed));

void usbd_graphic_set_info(uint16_t width, uint16_t height, uint16_t fps, uint8_t format);
void usbd_graphic_set_edid(const uint8_t *edid, uint16_t len);
struct usbd_interface *usbd_graphic_init_intf(struct usbd_interface *intf);

#endif /* USBD_GRAPHIC_H */
