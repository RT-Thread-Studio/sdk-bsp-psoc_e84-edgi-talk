/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-07-13     Rbb666       first version
 */
#include "drv_pwm.h"
#ifdef RT_USING_PWM
#include <rtdevice.h>
#include "drv_gpio.h"

// #define DRV_DEBUG
#define LOG_TAG "drv.pwm"
#include <drv_log.h>

struct rt_device_pwm pwm_device;

struct ifx_pwm
{
    struct rt_device_pwm pwm_device;
    mtb_hal_pwm_t *pwm_obj;
    const cy_stc_tcpwm_pwm_config_t *tcpwm_pwm_config;
    const mtb_hal_pwm_configurator_t *hal_pwm_configurator;
    TCPWM_Type *base;
    mtb_hal_clock_t pwm_clock;
    rt_uint8_t channel;
    char *name;
};

static struct ifx_pwm ifx_pwm_obj[] =
{
#ifdef TCPWM_0_GRP_0_PWM_5_CONFIG
    TCPWM_0_GRP_0_PWM_5_CONFIG
#endif
};

static rt_err_t drv_pwm_enable(mtb_hal_pwm_t *htim, struct rt_pwm_configuration *configuration, rt_bool_t enable)
{
    rt_uint32_t channel = configuration->channel;

    if (!configuration->complementary || configuration->complementary)
    {
        if (!enable)
        {
            if (channel == 5)
            {
                htim->tcpwm.cntnum = channel;
            }
            mtb_hal_pwm_stop(htim);
        }
        else
        {
            if (channel == 5)
            {
                htim->tcpwm.cntnum = channel;
            }
            mtb_hal_pwm_start(htim);
        }
    }

    return RT_EOK;
}

static rt_err_t drv_pwm_set(mtb_hal_pwm_t *htim, struct rt_pwm_configuration *configuration)
{
    rt_uint32_t period, pulse;

    period = (unsigned long long)configuration->period / 1000;

    pulse = (unsigned long long)configuration->pulse / 1000;

    mtb_hal_pwm_set_period(htim, period, pulse);

    return RT_EOK;
}

static rt_err_t drv_pwm_get(mtb_hal_pwm_t *htim, struct rt_pwm_configuration *configuration)
{
    uint32_t Period = Cy_TCPWM_PWM_GetPeriod0(htim->tcpwm.base, htim->tcpwm.cntnum);

    uint32_t Compare = Cy_TCPWM_PWM_GetCompare0Val(htim->tcpwm.base, htim->tcpwm.cntnum);

    configuration->period = Period * 1000;

    configuration->pulse = Compare * 1000;

    return RT_EOK;
}

static rt_err_t drv_pwm_control(struct rt_device_pwm *device, int cmd, void *arg)
{
    struct rt_pwm_configuration *configuration = (struct rt_pwm_configuration *)arg;
    mtb_hal_pwm_t *htim = (mtb_hal_pwm_t *)device->parent.user_data;

    switch (cmd)
    {
    case PWMN_CMD_ENABLE:
        configuration->complementary = RT_TRUE;

    case PWM_CMD_ENABLE:
        return drv_pwm_enable(htim, configuration, RT_TRUE);

    case PWMN_CMD_DISABLE:
        configuration->complementary = RT_FALSE;

    case PWM_CMD_DISABLE:
        return drv_pwm_enable(htim, configuration, RT_FALSE);

    case PWM_CMD_SET:
        return drv_pwm_set(htim, configuration);

    case PWM_CMD_GET:
        return drv_pwm_get(htim, configuration);

    default:
        return -RT_EINVAL;
    }
}

static struct rt_pwm_ops drv_ops = {drv_pwm_control};

static rt_err_t ifx_hw_pwm_init(struct ifx_pwm *device)
{
    cy_en_tcpwm_status_t tcpwm_status;
    cy_rslt_t rslt;
    RT_ASSERT(device != RT_NULL);

    /* Initialize the TCPWM block */
    tcpwm_status = Cy_TCPWM_PWM_Init(device->base,
                                     device->channel, device->tcpwm_pwm_config);
    if (CY_RSLT_SUCCESS != tcpwm_status)
    {
        LOG_E("Initialize the TCPWM block failed");
    }
    rslt = mtb_hal_pwm_setup(device->pwm_obj, device->hal_pwm_configurator, NULL);
    if (rslt != RT_EOK)
        return -RT_ERROR;

    return RT_EOK;
}

static int rt_hw_pwm_init(void)
{
    int i;
    int result = RT_EOK;

    for (i = 0; i < sizeof(ifx_pwm_obj) / sizeof(ifx_pwm_obj[0]); i++)
    {
        ifx_pwm_obj[i].pwm_obj = rt_malloc(sizeof(mtb_hal_pwm_t));
        RT_ASSERT(ifx_pwm_obj[i].pwm_obj != RT_NULL);

        /* pwm init */
        if (ifx_hw_pwm_init(&ifx_pwm_obj[i]) != RT_EOK)
        {
            LOG_E("%s init failed", ifx_pwm_obj[i].name);
            result = -RT_ERROR;
            goto __exit;
        }
        else
        {
            if (rt_device_pwm_register(&ifx_pwm_obj[i].pwm_device, ifx_pwm_obj[i].name, &drv_ops, ifx_pwm_obj[i].pwm_obj) == RT_EOK)
            {
                LOG_D("%s register success", ifx_pwm_obj[i].name);
            }
            else
            {
                LOG_D("%s register failed", ifx_pwm_obj[i].name);
                result = -RT_ERROR;
            }
        }
    }

__exit:
    return result;
}
INIT_BOARD_EXPORT(rt_hw_pwm_init);
#endif /* RT_USING_PWM */
