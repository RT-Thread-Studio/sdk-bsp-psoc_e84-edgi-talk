#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "usbh_core.h"

#define LED_PIN_G               GET_PIN(16, 6)

int main(void)
{
    rt_kprintf("Hello RT-Thread\r\n");
    rt_kprintf("It's cortex-m55\r\n");

    /* USB host initialize (same as msh: usbh_init 0 0x44900000) */
    usbh_initialize(0, USBHS_BASE, NULL);

    rt_pin_mode(LED_PIN_G, PIN_MODE_OUTPUT);

    while (1)
    {
        rt_pin_write(LED_PIN_G, PIN_LOW);
        rt_thread_mdelay(500);
        rt_pin_write(LED_PIN_G, PIN_HIGH);
        rt_thread_mdelay(500);
    }
    return 0;
}
