/*
 * demo_key.c - GPIO key interrupt sample (ported from Edgi_Talk_M33_Key_Irq)
 *
 * MSH command: demo_key
 *   Registers a falling-edge IRQ on P8.3 (button). Each press prints a
 *   message and toggles the blue LED on P16.5.
 *
 * Kconfig: none (RT_USING_PIN + BSP_USING_GPIO already enabled in AHT20).
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#define DEMO_KEY_BUTTON_PIN   GET_PIN(8, 3)
#define DEMO_KEY_LED_PIN      GET_PIN(16, 5)
#define DEMO_KEY_DEBOUNCE_MS  50

static rt_timer_t demo_key_timer;
static rt_uint8_t demo_key_led_state;

static void demo_key_timer_callback(void *args)
{
    if (rt_pin_read(DEMO_KEY_BUTTON_PIN) == PIN_LOW)
    {
        demo_key_led_state = !demo_key_led_state;
        rt_pin_write(DEMO_KEY_LED_PIN, demo_key_led_state ? PIN_HIGH : PIN_LOW);
        rt_kprintf("button pressed (led %s)\n", demo_key_led_state ? "ON" : "OFF");
    }

    rt_pin_irq_enable(DEMO_KEY_BUTTON_PIN, PIN_IRQ_ENABLE);
}

static void demo_key_callback(void *args)
{
    rt_pin_irq_enable(DEMO_KEY_BUTTON_PIN, PIN_IRQ_DISABLE);

    if (demo_key_timer != RT_NULL)
    {
        rt_timer_start(demo_key_timer);
    }
}

static int demo_key(int argc, char *argv[])
{
    rt_err_t ret;

    rt_pin_mode(DEMO_KEY_LED_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(DEMO_KEY_LED_PIN, PIN_LOW);
    demo_key_led_state = 0;

    rt_pin_mode(DEMO_KEY_BUTTON_PIN, PIN_MODE_INPUT_PULLUP);

    if (demo_key_timer == RT_NULL)
    {
        demo_key_timer = rt_timer_create("key_db",
                                         demo_key_timer_callback,
                                         RT_NULL,
                                         rt_tick_from_millisecond(DEMO_KEY_DEBOUNCE_MS),
                                         RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
        if (demo_key_timer == RT_NULL)
        {
            rt_kprintf("create key debounce timer failed\n");
            return -RT_ERROR;
        }
    }

    ret = rt_pin_attach_irq(DEMO_KEY_BUTTON_PIN, PIN_IRQ_MODE_FALLING,
                            demo_key_callback, RT_NULL);
    if (ret != RT_EOK)
    {
        rt_kprintf("attach key irq failed: %d\n", ret);
        return ret;
    }

    ret = rt_pin_irq_enable(DEMO_KEY_BUTTON_PIN, PIN_IRQ_ENABLE);
    if (ret != RT_EOK)
    {
        rt_kprintf("enable key irq failed: %d\n", ret);
        return ret;
    }

    rt_kprintf("Key IRQ ready. Press the button on P8.3.\n");
    return RT_EOK;
}
MSH_CMD_EXPORT(demo_key, register button irq on P8.3 and toggle blue LED);
