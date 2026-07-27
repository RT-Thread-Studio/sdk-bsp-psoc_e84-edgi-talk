#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#ifdef RT_CHERRYUSB_HOST
#include "usbh_core.h"
#endif

#define LED_PIN_B                 GET_PIN(16, 5)

int main(void)
{
    rt_kprintf("Hello RT-Thread\r\n");
    rt_kprintf("This core is cortex-m33\n");

#ifdef RT_CHERRYUSB_HOST
    usbh_initialize(0, USBHS_BASE, NULL);
#endif

    rt_pin_mode(LED_PIN_B, PIN_MODE_OUTPUT);
    while (1)
    {
        rt_pin_write(LED_PIN_B, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED_PIN_B, PIN_LOW);
        rt_thread_mdelay(500);
    }
    return 0;
}

#if defined(RT_CHERRYUSB_DEVICE) && defined(RT_CHERRYUSB_DEVICE_CDC_ACM)
static int cherryusb_device_cdc_acm_init(void)
{
    extern void cdc_acm_init(uint8_t busid, uint32_t reg_base);
    cdc_acm_init(0, USBHS_BASE);
    return 0;
}
INIT_APP_EXPORT(cherryusb_device_cdc_acm_init);
#endif
