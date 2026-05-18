/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-07-04     Rbb666       first version
 */

#include <rtthread.h>
#include "drv_adc.h"
#include "board.h"
#include "cycfg_peripherals.h"

#if defined(BSP_USING_ADC1)

//#define DRV_DEBUG
#define LOG_TAG             "drv.adc"
#include <drv_log.h>

struct ifx_adc
{
    struct rt_adc_device ifx_adc_device;
    const struct ifx_sar_adc_config *cfg;
    const char *name;
};

static struct ifx_adc ifx_adc_obj[] =
{
#ifdef BSP_USING_ADC1
    ADC1_CONFIG,
#endif
};

static rt_bool_t autonomous_initialized = RT_FALSE;

static uint8_t ifx_resolve_phy_channel(const struct ifx_sar_adc_config *cfg, uint8_t logical_channel)
{
#if defined(CYBSP_SAR_ADC_ENABLED)
    uint8_t sar_channel;

    for (sar_channel = 0; sar_channel < 8; sar_channel++)
    {
        const cy_stc_autanalog_sar_hs_chan_t *hs_channel = CYBSP_SAR_ADC_sta_hs_cfg.hsGpioChan[sar_channel];

        if (hs_channel == NULL)
        {
            continue;
        }

        switch (logical_channel)
        {
        case ADC_CHANNEL0:
            if (hs_channel->posPin == CY_AUTANALOG_SAR_PIN_GPIO4)
            {
                return sar_channel;
            }
            break;

        case ADC_CHANNEL1:
            if (hs_channel->posPin == CY_AUTANALOG_SAR_PIN_GPIO5)
            {
                return sar_channel;
            }
            break;

        case ADC_CHANNEL2:
            if (hs_channel->posPin == CY_AUTANALOG_SAR_PIN_GPIO6)
            {
                return sar_channel;
            }
            break;

        case ADC_CHANNEL3:
            if (hs_channel->posPin == CY_AUTANALOG_SAR_PIN_GPIO7)
            {
                return sar_channel;
            }
            break;

        default:
            break;
        }
    }
#endif

    return cfg->channel_map[logical_channel];
}

static rt_err_t ifx_autonomous_analog_init_once(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if (autonomous_initialized)
    {
        return RT_EOK;
    }

    result = Cy_AutAnalog_Init(&autonomous_analog_init);
    if (result != CY_AUTANALOG_SUCCESS)
    {
        LOG_E("Autonomous analog init failed");
        return -RT_ERROR;
    }

    Cy_AutAnalog_SetInterruptMask(CY_AUTANALOG_INT_SAR0_RESULT);
    Cy_AutAnalog_StartAutonomousControl();
    autonomous_initialized = RT_TRUE;

    return RT_EOK;
}

static rt_err_t ifx_adc_enabled(struct rt_adc_device *device, rt_int8_t channel, rt_bool_t enabled)
{
    const struct ifx_sar_adc_config *cfg;
    uint32_t channel_mask;

    RT_UNUSED(enabled);

    if ((device == RT_NULL) || (channel < 0))
    {
        return -RT_EINVAL;
    }

    cfg = (const struct ifx_sar_adc_config *)device->parent.user_data;
    channel_mask = (uint32_t)(1u << (uint32_t)channel);
    if ((cfg == RT_NULL) || ((uint32_t)channel >= cfg->max_channels))
    {
        return -RT_EINVAL;
    }

    if ((cfg->channel_mask & channel_mask) == 0u)
    {
        return -RT_EINVAL;
    }

    if (!autonomous_initialized)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}


static rt_err_t ifx_get_adc_value(struct rt_adc_device *device, rt_int8_t channel, rt_uint32_t *value)
{
    const struct ifx_sar_adc_config *cfg;
    int32_t sar_adc_count;
    int32_t sar_adc_mv;
    uint8_t phy_channel;
    uint32_t channel_mask;

    if ((device == RT_NULL) || (value == RT_NULL) || (channel < 0))
    {
        return -RT_EINVAL;
    }

    cfg = (const struct ifx_sar_adc_config *)device->parent.user_data;
    channel_mask = (uint32_t)(1u << (uint32_t)channel);
    if ((cfg == RT_NULL) || ((uint32_t)channel >= cfg->max_channels))
    {
        return -RT_EINVAL;
    }

    if ((cfg->channel_mask & channel_mask) == 0u)
    {
        return -RT_EINVAL;
    }

    phy_channel = ifx_resolve_phy_channel(cfg, (uint8_t)channel);
    if ((phy_channel == IFX_ADC_PHY_INVALID) || (phy_channel >= 8))
    {
        return -RT_EINVAL;
    }

    /* Force one fresh conversion result to avoid stale data. */
    {
        uint8_t ch_mask = (uint8_t)(1u << phy_channel);
        uint32_t timeout = 100000U;

        Cy_AutAnalog_SAR_ClearHSchanResultStatus(cfg->sar_idx, ch_mask);
        Cy_AutAnalog_StartAutonomousControl();

        while (timeout-- > 0U)
        {
            if ((Cy_AutAnalog_SAR_GetHSchanResultStatus(cfg->sar_idx) & ch_mask) != 0U)
            {
                break;
            }
        }

        if (timeout == 0U)
        {
            return -RT_ETIMEOUT;
        }
    }

    sar_adc_count = Cy_AutAnalog_SAR_ReadResult(cfg->sar_idx,
                    cfg->input,
                    phy_channel);

    sar_adc_mv = Cy_AutAnalog_SAR_CountsTo_mVolts(cfg->sar_idx,
                 cfg->low_power,
                 cfg->sequencer,
                 cfg->input,
                 phy_channel,
                 cfg->vref_mv,
                 sar_adc_count);

    if (sar_adc_mv < 0)
    {
        sar_adc_mv = 0;
    }

    *value = (rt_uint32_t)sar_adc_mv;
    return RT_EOK;
}

static const struct rt_adc_ops ifx_adc_ops =
{
    .enabled = ifx_adc_enabled,
    .convert = ifx_get_adc_value,
};

static int rt_hw_adc_init(void)
{
    rt_err_t result = RT_EOK;
    int i;

    result = ifx_autonomous_analog_init_once();
    if (result != RT_EOK)
    {
        return result;
    }

    for (i = 0; i < sizeof(ifx_adc_obj) / sizeof(ifx_adc_obj[0]); i++)
    {
        /* register ADC device */
        if (rt_hw_adc_register(&ifx_adc_obj[i].ifx_adc_device,
                               ifx_adc_obj[i].name,
                               &ifx_adc_ops,
                               ifx_adc_obj[i].cfg) == RT_EOK)
        {
            LOG_D("%s register success", ifx_adc_obj[i].name);
        }
        else
        {
            LOG_E("%s register failed", ifx_adc_obj[i].name);
            result = -RT_ERROR;
        }
    }

    return result;
}
INIT_BOARD_EXPORT(rt_hw_adc_init);

#endif /* BSP_USING_ADC1 */
