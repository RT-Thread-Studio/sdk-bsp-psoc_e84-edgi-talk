#include "system.h"

#include <rtthread.h>

#include "deepcraft/deepcraft_mic.h"
#include "deepcraft/deepcraft_lsm6ds3.h"

void system_load_device_drivers(protocol_t *protocol)
{
    if (!deepcraft_mic_register(protocol))
    {
        rt_kprintf("deepcraft: failed to register microphone device.\r\n");
    }

    if (!deepcraft_lsm6ds3_register(protocol))
    {
        rt_kprintf("deepcraft: failed to register lsm6ds3 device.\r\n");
    }
}
