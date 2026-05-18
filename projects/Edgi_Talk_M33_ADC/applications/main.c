#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "drv_adc.h"

#define ADC_CTRL                  GET_PIN(8, 4)
#define LED_PIN_B                 GET_PIN(16, 5)
#define ADC_DEV_NAME              "adc1"

#if !defined(BSP_USING_ADC_CHANNEL0) && \
    !defined(BSP_USING_ADC_CHANNEL1) && \
    !defined(BSP_USING_ADC_CHANNEL2) && \
    !defined(BSP_USING_ADC_CHANNEL3)
    #error "Please enable at least one ADC channel"
#endif

static const rt_uint8_t adc_channels[] =
{
#if defined(BSP_USING_ADC_CHANNEL0)
    ADC_CHANNEL0,
#endif
#if defined(BSP_USING_ADC_CHANNEL1)
    ADC_CHANNEL1,
#endif
#if defined(BSP_USING_ADC_CHANNEL2)
    ADC_CHANNEL2,
#endif
#if defined(BSP_USING_ADC_CHANNEL3)
    ADC_CHANNEL3,
#endif
};

#define ADC_CHANNEL_COUNT (sizeof(adc_channels) / sizeof(adc_channels[0]))

rt_adc_device_t adc_dev;

int main(void)
{
    rt_size_t i;

    rt_kprintf("Hello RT-Thread\r\n");
    rt_kprintf("This core is cortex-m33\n");
    rt_pin_mode(LED_PIN_B, PIN_MODE_OUTPUT);
    rt_pin_mode(ADC_CTRL, PIN_MODE_OUTPUT);
    rt_pin_write(ADC_CTRL, PIN_HIGH);

    adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("adc sample run failed! can't find %s device!\n", ADC_DEV_NAME);
        return RT_ERROR;
    }

    for (i = 0; i < ADC_CHANNEL_COUNT; i++)
    {
        rt_adc_enable(adc_dev, (rt_int8_t)adc_channels[i]);
    }

    while (1)
    {
        rt_uint32_t value, mv, v, mv_frac;

        rt_pin_write(LED_PIN_B, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED_PIN_B, PIN_LOW);
        rt_thread_mdelay(500);

        for (i = 0; i < ADC_CHANNEL_COUNT; i++)
        {
            value = rt_adc_read(adc_dev, (rt_int8_t)adc_channels[i]);
            mv = (value * 4200 + 900) / 1800;
            v = mv / 1000;
            mv_frac = mv % 1000;

            rt_kprintf("CH%d Value is: %d.%03d V\n", adc_channels[i], v, mv_frac);
        }
    }
    return 0;
}
