#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#define LED_PIN_G               GET_PIN(16, 6)

int main(void)
{
    rt_kprintf("Hello RT-Thread\r\n");
    rt_kprintf("It's cortex-m55\r\n");
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

static int cherryusb_device_cdc_acm_init(void)
{
    extern void cdc_acm_init(uint8_t busid, uint32_t reg_base);
    cdc_acm_init(0, USBHS_BASE);
    return 0;
}
INIT_APP_EXPORT(cherryusb_device_cdc_acm_init);
