/*
 * Copyright (c) 2006-2025, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-12-14     RT-Thread    First version
 */
#include <rtthread.h>
#include <rtdevice.h>

#define PWM_DEV_NAME        "pwm18"
#define PWM_DEV_CHANNEL     0
#define PWM_PERIOD_NS       200000

static struct rt_device_pwm *pwm_dev = RT_NULL;

static rt_uint8_t current_brightness = 0;

void set_brightness(rt_uint8_t percent)
{
    if (percent > 100)
        percent = 100;

    current_brightness = percent;

    rt_uint32_t pulse = (PWM_PERIOD_NS * percent) / 100;

    rt_pwm_set(pwm_dev, PWM_DEV_CHANNEL, PWM_PERIOD_NS, pulse);
}

rt_uint8_t get_brightness(void)
{
    return current_brightness;
}

static int pwm_bl_init(void)
{
    if (pwm_dev == RT_NULL)
    {
        pwm_dev = (struct rt_device_pwm *)rt_device_find(PWM_DEV_NAME);
        if (pwm_dev == RT_NULL)
        {
            rt_kprintf("Cannot find %s device!\n", PWM_DEV_NAME);
            return -1;
        }

        rt_pwm_set(pwm_dev, PWM_DEV_CHANNEL, PWM_PERIOD_NS, 0);
        rt_pwm_enable(pwm_dev, PWM_DEV_CHANNEL);
    }

    set_brightness(80);

    return 0;
}
INIT_DEVICE_EXPORT(pwm_bl_init);
