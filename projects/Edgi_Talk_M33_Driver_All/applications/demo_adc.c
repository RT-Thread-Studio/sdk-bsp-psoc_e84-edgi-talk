/*
 * demo_adc.c - ADC sample (ported from Edgi_Talk_M33_ADC)
 *
 * MSH command: demo_adc
 *   Enables ADC1 channel 1 and reads the voltage once. The ADC power-enable
 *   MOSFET on P8.4 is driven high on the first call.
 *
 * Kconfig: RT_USING_ADC + BSP_USING_ADC + BSP_USING_ADC1 + BSP_USING_ADC_CHANNEL1
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#define DEMO_ADC_DEV_NAME       "adc1"
#define DEMO_ADC_CHANNEL        1
#define DEMO_ADC_CTRL_PIN       GET_PIN(8, 4)

#ifdef RT_USING_ADC
static int demo_adc(int argc, char *argv[])
{
    static rt_adc_device_t adc_dev = RT_NULL;
    static rt_bool_t ctrl_enabled = RT_FALSE;

    if (adc_dev == RT_NULL)
    {
        adc_dev = (rt_adc_device_t)rt_device_find(DEMO_ADC_DEV_NAME);
        if (adc_dev == RT_NULL)
        {
            rt_kprintf("ADC device %s not found\n", DEMO_ADC_DEV_NAME);
            return -RT_ERROR;
        }
    }

    if (!ctrl_enabled)
    {
        rt_pin_mode(DEMO_ADC_CTRL_PIN, PIN_MODE_OUTPUT);
        rt_pin_write(DEMO_ADC_CTRL_PIN, PIN_HIGH);
        ctrl_enabled = RT_TRUE;
        rt_thread_mdelay(5);
    }

    rt_adc_enable(adc_dev, DEMO_ADC_CHANNEL);

    rt_uint32_t value = rt_adc_read(adc_dev, DEMO_ADC_CHANNEL);
    /* The original demo uses this scaling to convert raw 12-bit count to mV. */
    rt_uint32_t mv = (value * 4200 + 900) / 1800;
    rt_uint32_t v_part = mv / 1000;
    rt_uint32_t mv_part = mv % 1000;

    rt_kprintf("CH%d: %d.%03d V (raw=%d)\n",
               DEMO_ADC_CHANNEL, v_part, mv_part, value);
    return RT_EOK;
}
#else
static int demo_adc(int argc, char *argv[])
{
    rt_kprintf("ADC framework not enabled. Enable in menuconfig:\n");
    rt_kprintf("  RT_USING_ADC + BSP_USING_ADC + BSP_USING_ADC1 + BSP_USING_ADC_CHANNEL1\n");
    return -RT_ERROR;
}
#endif
MSH_CMD_EXPORT(demo_adc, read adc1 ch1 voltage);
