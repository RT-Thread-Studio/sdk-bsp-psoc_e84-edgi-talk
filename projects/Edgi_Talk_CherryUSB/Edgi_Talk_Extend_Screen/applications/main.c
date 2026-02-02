#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "usb_display.h"

#define LED_PIN_G               GET_PIN(16, 6)
int main(void)
{
    rt_kprintf("Hello RT-Thread\r\n");
    rt_kprintf("It's cortex-m55\r\n");
    rt_pin_mode(LED_PIN_G, PIN_MODE_OUTPUT);

    usb_display_init();

    while (1)
    {
        rt_thread_mdelay(1000);
    }
    return 0;
}

