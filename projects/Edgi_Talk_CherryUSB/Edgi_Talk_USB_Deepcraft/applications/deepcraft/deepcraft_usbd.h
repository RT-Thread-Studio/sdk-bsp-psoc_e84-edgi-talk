#ifndef APP_DEEPCRAFT_USBD_H
#define APP_DEEPCRAFT_USBD_H

#include <stdint.h>

#include "protocol/protocol.h"
#include "protocol/pb_decode.h"
#include "protocol/pb_encode.h"

typedef struct
{
    protocol_t *protocol;
    pb_istream_t istream;
    pb_ostream_t ostream;
} deepcraft_usbd_t;

deepcraft_usbd_t *deepcraft_usbd_create(protocol_t *protocol);
void deepcraft_usbd_destroy(deepcraft_usbd_t *usb);
void deepcraft_usbd_resync_rx(void);

#endif
