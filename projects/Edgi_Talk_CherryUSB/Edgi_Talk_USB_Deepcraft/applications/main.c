#include <rtthread.h>

#include <string.h>

#include "board.h"

#include "build.h"
#include "deepcraft/deepcraft_usbd.h"
#include "protocol/protocol.h"
#include "system.h"

static uint8_t *deepcraft_board_get_serial_uuid(void)
{
    uint64_t serial64 = Cy_SysLib_GetUniqueId();
    static uint8_t serial[16] = {
        0x29, 0x0D, 0xE5, 0xCB, 0x46, 0x0B, 0x41, 0xBF,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    rt_memcpy(serial + 8, &serial64, sizeof(serial64));
    return serial;
}

static void deepcraft_board_reset(protocol_t *protocol)
{
    (void)protocol;
    NVIC_SystemReset();
}

int main(void)
{
    protocol_Version firmware_version = {
        .major = 1,
        .minor = 0,
        .build = BUILD_DATE,
        .revision = BUILD_TIME,
    };

    uint8_t *serial = deepcraft_board_get_serial_uuid();

    protocol_t *protocol = protocol_create("PSOC Edge E84 RT-Thread Kit", serial, firmware_version);
    if (protocol == RT_NULL)
    {
        rt_kprintf("deepcraft: protocol_create failed.\r\n");
        return -1;
    }

    protocol->board_reset = deepcraft_board_reset;

    system_load_device_drivers(protocol);

    deepcraft_usbd_t *usb = deepcraft_usbd_create(protocol);
    if (usb == RT_NULL)
    {
        rt_kprintf("deepcraft: usb init failed.\r\n");
        return -1;
    }

    rt_kprintf("deepcraft: ready, waiting for host commands.\r\n");

    while (1)
    {
        int status = protocol_process_request(protocol, &usb->istream, &usb->ostream);
        if (status != PROTOCOL_STATUS_SUCCESS)
        {
            rt_kprintf("deepcraft: protocol_process_request failed: %d\r\n", status);
            if (status == PROTOCOL_STATUS_FAILED_TO_DECODE_REQUEST)
            {
                deepcraft_usbd_resync_rx();
            }
        }
    }
}
