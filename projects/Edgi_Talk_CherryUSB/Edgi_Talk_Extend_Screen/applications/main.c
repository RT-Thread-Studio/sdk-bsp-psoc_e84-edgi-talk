#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "usb_display.h"

int main(void)
{
    rt_kprintf("Hello RT-Thread\n");
    rt_kprintf("It's cortex-m55 cherryusb demo\n");

    usb_display_init();

    return 0;
}

