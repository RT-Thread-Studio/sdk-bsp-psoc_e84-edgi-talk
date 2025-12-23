#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#define LED_PIN_G               GET_PIN(16, 6)
extern int lcd_test(void);
int main(void)
{
    rt_kprintf("Hello RT-Thread\r\n");
    rt_kprintf("It's cortex-m55\r\n");
    rt_pin_mode(LED_PIN_G, PIN_MODE_OUTPUT);

    while (1)
    {
        lcd_test();
    }
    return 0;
}

