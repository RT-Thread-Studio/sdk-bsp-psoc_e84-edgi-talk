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
#include "cy_sysclk.h"
#include "cy_tcpwm.h"
//#define DRV_DEBUG
#define LOG_TAG "drv.pwm"
#include <drv_log.h>

struct ifx_pwm
{
    struct rt_device_pwm pwm_device;
    const cy_stc_tcpwm_pwm_config_t *tcpwm_pwm_config;
    const mtb_hal_pwm_configurator_t *hal_cfg;
    const char *name;
};

static struct ifx_pwm ifx_pwm_obj[] =
{
    IFX_PWM_DEVICE_LIST
    {
        .tcpwm_pwm_config = RT_NULL,
        .hal_cfg = RT_NULL,
        .name = RT_NULL,
    }
};


#ifndef IFX_PWM_MAX_TICKS
#define IFX_PWM_MAX_TICKS 65535U
#endif

#ifndef IFX_PWM_DEFAULT_CHANNEL
#define IFX_PWM_DEFAULT_CHANNEL 1U
#endif

static inline TCPWM_Type *ifx_pwm_get_base(struct ifx_pwm *pwm)
{
    if ((pwm == RT_NULL) || (pwm->hal_cfg == RT_NULL))
        return RT_NULL;

    return pwm->hal_cfg->base;
}

static inline uint32_t ifx_pwm_get_cntnum(struct ifx_pwm *pwm)
{
    if ((pwm == RT_NULL) || (pwm->hal_cfg == RT_NULL))
        return 0U;

    return pwm->hal_cfg->cntnum;
}

static inline const mtb_hal_peri_div_t *ifx_pwm_get_clock_ref(struct ifx_pwm *pwm)
{
    if ((pwm == RT_NULL) || (pwm->hal_cfg == RT_NULL) ||
        (pwm->hal_cfg->clock == RT_NULL) || (pwm->hal_cfg->clock->clock_ref == RT_NULL))
    {
        return RT_NULL;
    }

    return (const mtb_hal_peri_div_t *)pwm->hal_cfg->clock->clock_ref;
}

static inline uint32_t ifx_pwm_get_clock(struct ifx_pwm *pwm)
{
    const mtb_hal_peri_div_t *clock_ref = ifx_pwm_get_clock_ref(pwm);

    if (clock_ref == RT_NULL)
        return 0U;

    return Cy_SysClk_PeriPclkGetFrequency((en_clk_dst_t)clock_ref->clk_dst,
                                          clock_ref->div_type,
                                          clock_ref->div_num);
}

static inline int ifx_pwm_check_clk(struct ifx_pwm *pwm, uint32_t *clk)
{
    *clk = ifx_pwm_get_clock(pwm);
    if (*clk == 0U)
    {
        LOG_E("%s: invalid clk (0 Hz)", pwm->name ? pwm->name : "ifx_pwm");
        return -RT_ERROR;
    }
    return RT_EOK;
}

static inline rt_err_t ifx_pwm_check_cfg(struct ifx_pwm *pwm, const struct rt_pwm_configuration *cfg)
{
    if (!pwm || ifx_pwm_get_base(pwm) == RT_NULL)
        return -RT_ERROR;

    if (cfg == RT_NULL)
        return -RT_EINVAL;

    /* Single-channel PWM devices keep compatibility with channel 0 and default channel(1). */
    if ((cfg->channel != 0U) && (cfg->channel != IFX_PWM_DEFAULT_CHANNEL))
    {
        LOG_E("%s: unsupported channel %u", pwm->name ? pwm->name : "ifx_pwm", cfg->channel);
        return -RT_EINVAL;
    }

    return RT_EOK;
}

static inline uint64_t ns_to_ticks(uint64_t ns, uint32_t clk)
{
    if (clk == 0U)
        return 0ULL;

    uint64_t ticks = (ns * (uint64_t)clk) / 1000000000ULL;

    if (ticks > IFX_PWM_MAX_TICKS)
        ticks = IFX_PWM_MAX_TICKS;

    return ticks;
}

static rt_err_t drv_pwm_enable(struct rt_device_pwm *device, struct rt_pwm_configuration *configuration, rt_bool_t enable)
{
    struct ifx_pwm *pwm = (struct ifx_pwm *)device->parent.user_data;
    rt_err_t ret;
    TCPWM_Type *base;
    uint32_t cntNum;

    ret = ifx_pwm_check_cfg(pwm, configuration);
    if (ret != RT_EOK)
        return ret;

    base = ifx_pwm_get_base(pwm);
    cntNum = ifx_pwm_get_cntnum(pwm);

    if (!enable)
    {
        Cy_TCPWM_PWM_Disable(base, cntNum);
    }
    else
    {
        Cy_TCPWM_PWM_Enable(base, cntNum);
        Cy_TCPWM_TriggerStart_Single(base, cntNum);
    }

    return RT_EOK;
}

static rt_err_t drv_pwm_set_period(struct ifx_pwm *pwm, struct rt_pwm_configuration *configuration)
{
    uint32_t clk;
    uint32_t cntNum;
    TCPWM_Type *base;
    rt_err_t ret;

    ret = ifx_pwm_check_cfg(pwm, configuration);
    if (ret != RT_EOK)
        return ret;

    if (ifx_pwm_check_clk(pwm, &clk) != RT_EOK)
        return -RT_ERROR;

    base = ifx_pwm_get_base(pwm);
    cntNum = ifx_pwm_get_cntnum(pwm);

    uint64_t period_ns = configuration->period;
    uint64_t ticks = ns_to_ticks(period_ns, clk);

    if (period_ns > 0 && ticks == 0)
        ticks = 1;

    if (ticks == 0)
    {
        LOG_E("%s: requested period is 0 ns", pwm->name ? pwm->name : "ifx_pwm");
        return -RT_EINVAL;
    }

    if (ticks > IFX_PWM_MAX_TICKS)
        ticks = IFX_PWM_MAX_TICKS;

    Cy_TCPWM_PWM_SetPeriod0(base, cntNum, (uint32_t)ticks);

    return RT_EOK;
}

static rt_err_t drv_pwm_set_pulse(struct ifx_pwm *pwm, struct rt_pwm_configuration *configuration)
{
    uint32_t clk;
    uint32_t cntNum;
    TCPWM_Type *base;
    rt_err_t ret;

    ret = ifx_pwm_check_cfg(pwm, configuration);
    if (ret != RT_EOK)
        return ret;

    if (ifx_pwm_check_clk(pwm, &clk) != RT_EOK)
        return -RT_ERROR;

    base = ifx_pwm_get_base(pwm);
    cntNum = ifx_pwm_get_cntnum(pwm);

    uint64_t pulse_ns = configuration->pulse;
    uint64_t ticks = ns_to_ticks(pulse_ns, clk);

    uint32_t period_ticks = Cy_TCPWM_PWM_GetPeriod0(base, cntNum);

    if (period_ticks == 0)
    {
        LOG_W("%s: period not configured, setting pulse to 0", pwm->name ? pwm->name : "ifx_pwm");
        ticks = 0;
    }
    else
    {
        if (ticks > period_ticks)
            ticks = period_ticks;
    }

    Cy_TCPWM_PWM_SetCompare0Val(base, cntNum, (uint32_t)ticks);

    return RT_EOK;
}

static rt_err_t drv_pwm_set(struct rt_device_pwm *device, struct rt_pwm_configuration *configuration)
{
    struct ifx_pwm *pwm = (struct ifx_pwm *)device->parent.user_data;
    uint32_t clk;
    uint32_t cntNum;
    TCPWM_Type *base;
    rt_err_t ret;

    ret = ifx_pwm_check_cfg(pwm, configuration);
    if (ret != RT_EOK)
        return ret;

    if (ifx_pwm_check_clk(pwm, &clk) != RT_EOK)
        return -RT_ERROR;

    base = ifx_pwm_get_base(pwm);
    cntNum = ifx_pwm_get_cntnum(pwm);

    uint64_t period_ticks = ns_to_ticks(configuration->period, clk);
    uint64_t pulse_ticks  = ns_to_ticks(configuration->pulse, clk);

    if (configuration->period > 0 && period_ticks == 0)
        period_ticks = 1;

    if (period_ticks == 0)
    {
        LOG_E("%s: requested period is 0 ns", pwm->name ? pwm->name : "ifx_pwm");
        return -RT_EINVAL;
    }

    if (pulse_ticks > period_ticks)
        pulse_ticks = period_ticks;

    if (period_ticks > IFX_PWM_MAX_TICKS)
        period_ticks = IFX_PWM_MAX_TICKS;

    Cy_TCPWM_PWM_SetPeriod0(base, cntNum, (uint32_t)period_ticks);
    Cy_TCPWM_PWM_SetCompare0Val(base, cntNum, (uint32_t)pulse_ticks);

    return RT_EOK;
}

static rt_err_t drv_pwm_get(struct rt_device_pwm *device, struct rt_pwm_configuration *configuration)
{
    struct ifx_pwm *pwm = (struct ifx_pwm *)device->parent.user_data;
    uint32_t clk;
    uint32_t cntNum;
    TCPWM_Type *base;
    rt_err_t ret;

    ret = ifx_pwm_check_cfg(pwm, configuration);
    if (ret != RT_EOK)
        return ret;

    if (ifx_pwm_check_clk(pwm, &clk) != RT_EOK)
        return -RT_ERROR;

    base = ifx_pwm_get_base(pwm);
    cntNum = ifx_pwm_get_cntnum(pwm);

    uint32_t period_ticks = Cy_TCPWM_PWM_GetPeriod0(base, cntNum);
    uint32_t cmp_ticks    = Cy_TCPWM_PWM_GetCompare0Val(base, cntNum);

    configuration->period = (uint64_t)period_ticks * 1000000000ULL / (uint64_t)clk;
    configuration->pulse  = (uint64_t)cmp_ticks * 1000000000ULL / (uint64_t)clk;

    return RT_EOK;
}

static rt_err_t drv_pwm_control(struct rt_device_pwm *device, int cmd, void *arg)
{
    struct rt_pwm_configuration *configuration = (struct rt_pwm_configuration *)arg;

    if ((cmd == PWM_CMD_ENABLE) || (cmd == PWM_CMD_DISABLE) ||
        (cmd == PWM_CMD_SET) || (cmd == PWM_CMD_GET) ||
        (cmd == PWM_CMD_SET_PERIOD) || (cmd == PWM_CMD_SET_PULSE))
    {
        if (configuration == RT_NULL)
            return -RT_EINVAL;
    }

    switch (cmd)
    {
    case PWM_CMD_ENABLE:
        return drv_pwm_enable(device, configuration, RT_TRUE);

    case PWM_CMD_DISABLE:
        return drv_pwm_enable(device, configuration, RT_FALSE);

    case PWM_CMD_SET:
        return drv_pwm_set(device, configuration);

    case PWM_CMD_GET:
        return drv_pwm_get(device, configuration);

    case PWM_CMD_SET_PERIOD:
        return drv_pwm_set_period((struct ifx_pwm *)device->parent.user_data, configuration);

    case PWM_CMD_SET_PULSE:
        return drv_pwm_set_pulse((struct ifx_pwm *)device->parent.user_data, configuration);

    default:
        return -RT_EINVAL;
    }
}

static struct rt_pwm_ops drv_ops = { drv_pwm_control };

static rt_err_t ifx_hw_pwm_init(struct ifx_pwm *device)
{
    cy_en_tcpwm_status_t tcpwm_status;
    TCPWM_Type *base;
    uint32_t cntNum;

    RT_ASSERT(device != RT_NULL);

    if (!device->tcpwm_pwm_config || !device->hal_cfg)
    {
        LOG_W("%s: tcpwm config or base missing", device->name ? device->name : "ifx_pwm");
        return -RT_ERROR;
    }

    base = ifx_pwm_get_base(device);
    cntNum = ifx_pwm_get_cntnum(device);

    tcpwm_status = Cy_TCPWM_PWM_Init(base, cntNum, device->tcpwm_pwm_config);
    if (CY_TCPWM_SUCCESS != tcpwm_status)
    {
        LOG_E("%s: Initialize the TCPWM block failed", device->name ? device->name : "ifx_pwm");
        return -RT_ERROR;
    }

    return RT_EOK;
}

static int rt_hw_pwm_init(void)
{
    int i;
    int result = RT_EOK;
    int count = sizeof(ifx_pwm_obj) / sizeof(ifx_pwm_obj[0]);
    int registered = 0;

    for (i = 0; i < count; i++)
    {
        struct ifx_pwm *obj = &ifx_pwm_obj[i];

        if (obj->tcpwm_pwm_config == RT_NULL || obj->hal_cfg == RT_NULL)
        {
            continue;
        }

        if (ifx_hw_pwm_init(obj) != RT_EOK)
        {
            LOG_E("%s init failed", obj->name ? obj->name : "ifx_pwm");
            result = -RT_ERROR;
            goto __exit;
        }

        obj->pwm_device.parent.user_data = obj;

        if (rt_device_pwm_register(&obj->pwm_device, obj->name, &drv_ops, obj) == RT_EOK)
        {
            LOG_D("%s register success", obj->name ? obj->name : "ifx_pwm");
            registered++;
        }
        else
        {
            LOG_E("%s register failed", obj->name ? obj->name : "ifx_pwm");
            result = -RT_ERROR;
        }
    }

    if (registered == 0)
    {
        LOG_W("No PWM instances configured");
    }

__exit:
    return result;
}
INIT_BOARD_EXPORT(rt_hw_pwm_init);

#endif /* RT_USING_PWM */
