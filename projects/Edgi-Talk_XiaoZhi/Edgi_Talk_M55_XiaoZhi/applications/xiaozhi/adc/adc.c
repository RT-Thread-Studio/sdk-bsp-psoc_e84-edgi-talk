#include <rtdevice.h>
#include <rtthread.h>
#include "drv_gpio.h"

#define ADC_CTRL           GET_PIN(8, 4)
#define ADC_DEV_NAME       "adc1"
#define ADC_CHANNEL        0

#define LCD_BL_GPIO_NUM    GET_PIN(15, 7)
#define LCD_DISP_GPIO_NUM  GET_PIN(15, 6)
#define BL_PWM_DISP_CTRL   GET_PIN(20, 6)

#define ADC_AVG_NUM        15
#define DISCARD_NUM        10

#define ADC_RAW_4V2        1800
#define ADC_RAW_4V0        1714
#define ADC_RAW_3V9        1671
#define ADC_RAW_3V6        1543
#define ADC_RAW_3V7        1586
#define ADC_RAW_3V8        1629
#define ADC_RAW_3V0        1286
#define ADC_RAW_2V0        800

static rt_thread_t adc_check_tid = RT_NULL;

extern void xiaozhi_ui_update_adc(char* string);
extern void poweroff(void);

static void lcd_backlight_on(void)
{
    rt_pin_write(BL_PWM_DISP_CTRL, PIN_HIGH);
    rt_pin_write(LCD_DISP_GPIO_NUM, PIN_HIGH);
    rt_pin_write(LCD_BL_GPIO_NUM, PIN_HIGH);
}

static void adc_check_entry(void* parameter)
{
    RT_ASSERT(parameter != RT_NULL);

    rt_adc_device_t adc_dev;
    rt_uint32_t value;
    static rt_uint32_t adc_buf[ADC_AVG_NUM] = {0};
    rt_uint32_t sum = 0;
    rt_uint8_t idx = 0;
    rt_uint8_t count = 0;

    rt_pin_mode(ADC_CTRL, PIN_MODE_OUTPUT);
    rt_pin_write(ADC_CTRL, PIN_HIGH);

    adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("ADC check failed: can't find %s device!\n", ADC_DEV_NAME);
        return;
    }

    rt_adc_enable(adc_dev, ADC_CHANNEL);

    rt_uint32_t init_sum = 0;

    for (int i = 0; i < DISCARD_NUM; i++)
    {
        rt_adc_read(adc_dev, ADC_CHANNEL);
        rt_thread_mdelay(10);
    }

    for (int i = 0; i < ADC_AVG_NUM; i++)
    {
        init_sum += rt_adc_read(adc_dev, ADC_CHANNEL);
        rt_thread_mdelay(10);
    }

    int init_avg = init_sum / ADC_AVG_NUM;
    rt_kprintf("Init ADC avg = %d\n", init_avg);

    if (init_avg >= ADC_RAW_3V9)
    {
        lcd_backlight_on();
    }
    else if (init_avg > ADC_RAW_2V0 && init_avg < ADC_RAW_3V9)
    {
        poweroff();
    }

    while (1)
    {
        value = rt_adc_read(adc_dev, ADC_CHANNEL);

        sum -= adc_buf[idx];
        adc_buf[idx] = value;
        sum += value;
        idx = (idx + 1) % ADC_AVG_NUM;
        if (count < ADC_AVG_NUM) count++;

        if (count >= ADC_AVG_NUM)
        {
            rt_uint32_t avg_val = sum / ADC_AVG_NUM;

            if (avg_val > ADC_RAW_2V0 && avg_val < ADC_RAW_3V9)
            {
                poweroff();
            }
            if (avg_val > ADC_RAW_2V0 && avg_val < ADC_RAW_4V0)
            {
                xiaozhi_ui_update_adc("请充电");
            }
            else
            {
                xiaozhi_ui_update_adc(" ");
            }
        }

        rt_thread_mdelay(500);
    }
}

static int adc_check_entry_init(void)
{
    adc_check_tid = rt_thread_create(
                        "adc_check_thread",
                        adc_check_entry,
                        (void*)0x01,
                        1024,
                        25,
                        5
                    );

    if (RT_NULL == adc_check_tid)
    {
        rt_kprintf("[%s] Create failed!\n", __FUNCTION__);
        return -RT_ENOMEM;
    }

    if (RT_EOK != rt_thread_startup(adc_check_tid))
    {
        rt_kprintf("[%s] Startup failed!\n", __FUNCTION__);
        return -RT_ERROR;
    }

    rt_kprintf("[%s] Created successfully\n", __FUNCTION__);
    return RT_EOK;
}

//INIT_APP_EXPORT(adc_check_entry_init);
