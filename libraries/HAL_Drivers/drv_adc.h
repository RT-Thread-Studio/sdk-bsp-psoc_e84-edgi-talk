/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-28     shelton      first version
 */

#ifndef __ADC_CONFIG_H__
#define __ADC_CONFIG_H__

#include <rtthread.h>
#include "cy_autanalog.h"



#ifdef __cplusplus
extern "C"
{
#endif

#if defined(BSP_USING_ADC1)

struct ifx_sar_adc_config
{
    uint8_t sar_idx;
    uint8_t sequencer;
    uint8_t max_channels;
    uint32_t channel_mask;
    bool low_power;
    cy_en_autanalog_sar_input_t input;
    uint32_t vref_mv;
    const uint8_t *channel_map;
};

#endif

#if defined(BSP_USING_ADC1)

/* Logical ADC channels. */
#define ADC_CHANNEL0    0
#define ADC_CHANNEL1    1
#define ADC_CHANNEL2    2
#define ADC_CHANNEL3    3

#define IFX_ADC_MAX_CHANNELS    4
#define IFX_ADC_PHY_INVALID     0xFF

/* Logical channel -> SAR result channel mapping (P15[4]..P15[7]). */
#if defined(BSP_USING_ADC_CHANNEL0)
#define ADC_CHANNEL0_GPIO  0
#define ADC_CHANNEL0_MASK  (1u << ADC_CHANNEL0)
#else
#define ADC_CHANNEL0_GPIO  IFX_ADC_PHY_INVALID
#define ADC_CHANNEL0_MASK  0u
#endif

#if defined(BSP_USING_ADC_CHANNEL1)
#define ADC_CHANNEL1_GPIO  1
#define ADC_CHANNEL1_MASK  (1u << ADC_CHANNEL1)
#else
#define ADC_CHANNEL1_GPIO  IFX_ADC_PHY_INVALID
#define ADC_CHANNEL1_MASK  0u
#endif

#if defined(BSP_USING_ADC_CHANNEL2)
#define ADC_CHANNEL2_GPIO  2
#define ADC_CHANNEL2_MASK  (1u << ADC_CHANNEL2)
#else
#define ADC_CHANNEL2_GPIO  IFX_ADC_PHY_INVALID
#define ADC_CHANNEL2_MASK  0u
#endif

#if defined(BSP_USING_ADC_CHANNEL3)
#define ADC_CHANNEL3_GPIO  3
#define ADC_CHANNEL3_MASK  (1u << ADC_CHANNEL3)
#else
#define ADC_CHANNEL3_GPIO  IFX_ADC_PHY_INVALID
#define ADC_CHANNEL3_MASK  0u
#endif

static const uint8_t adc1_channel_map[IFX_ADC_MAX_CHANNELS] =
{
    ADC_CHANNEL0_GPIO,
    ADC_CHANNEL1_GPIO,
    ADC_CHANNEL2_GPIO,
    ADC_CHANNEL3_GPIO,
};

#define ADC1_CHANNEL_MASK (ADC_CHANNEL0_MASK | ADC_CHANNEL1_MASK | ADC_CHANNEL2_MASK | ADC_CHANNEL3_MASK)

static const struct ifx_sar_adc_config adc1_cfg =
{
    .sar_idx = 0,
    .sequencer = 0,
    .max_channels = IFX_ADC_MAX_CHANNELS,
    .channel_mask = ADC1_CHANNEL_MASK,
    .input = CY_AUTANALOG_SAR_INPUT_GPIO,
    .low_power = false,
    .vref_mv = 1800,
    .channel_map = adc1_channel_map,
};

#ifndef ADC1_CONFIG
#define ADC1_CONFIG                 \
    {                               \
        .cfg = &adc1_cfg,           \
        .name = "adc1",             \
    }
#endif /* ADC1_CONFIG */

#endif


#ifdef __cplusplus
}
#endif

#endif /* __ADC_CONFIG_H__ */
