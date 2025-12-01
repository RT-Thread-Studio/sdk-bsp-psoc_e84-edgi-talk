#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#define LED_PIN_B                 GET_PIN(16, 5)
#define BUTTON_PIN                GET_PIN(8, 3)

void button_callback(void *args)
{
    rt_kprintf("The button is pressed \r\n");
}

int main(void)
{
    rt_kprintf("Hello RT-Thread\r\n");
    rt_kprintf("This core is cortex-m33\n");
    rt_pin_mode(LED_PIN_B, PIN_MODE_OUTPUT);
    rt_pin_mode(BUTTON_PIN, PIN_MODE_INPUT_PULLUP);
    rt_pin_attach_irq(BUTTON_PIN, PIN_IRQ_MODE_FALLING, button_callback, RT_NULL);
    rt_pin_irq_enable(BUTTON_PIN, PIN_IRQ_ENABLE);
    while (1)
    {
        rt_pin_write(LED_PIN_B, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED_PIN_B, PIN_LOW);
        rt_thread_mdelay(500);
    }
    return 0;
}
