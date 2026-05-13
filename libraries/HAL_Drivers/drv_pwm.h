/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-07-13     Rbb666       first version
 */

#ifndef __PWM_CONFIG_H__
#define __PWM_CONFIG_H__

#include <rtthread.h>
#include <board.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IFX_PWM_DEVICE(_cfg, _hal_cfg, _name) \
{                                              \
    .tcpwm_pwm_config = &(_cfg),               \
    .hal_cfg          = &(_hal_cfg),           \
    .name             = (_name),               \
},

#ifdef BSP_USING_PWM5
#define IFX_PWM_DEVICE_ITEM_PWM5 \
    IFX_PWM_DEVICE(tcpwm_0_group_0_cnt_5_config, tcpwm_0_group_0_cnt_5_hal_config, "pwm5")
#else
#define IFX_PWM_DEVICE_ITEM_PWM5
#endif

#ifdef BSP_USING_PWM6
#define IFX_PWM_DEVICE_ITEM_PWM6 \
    IFX_PWM_DEVICE(tcpwm_0_group_0_cnt_6_config, tcpwm_0_group_0_cnt_6_hal_config, "pwm6")
#else
#define IFX_PWM_DEVICE_ITEM_PWM6
#endif

#ifdef BSP_USING_PWM18
#define IFX_PWM_DEVICE_ITEM_PWM18 \
    IFX_PWM_DEVICE(tcpwm_0_group_1_cnt_9_config, tcpwm_0_group_1_cnt_9_hal_config, "pwm18")
#else
#define IFX_PWM_DEVICE_ITEM_PWM18
#endif

#ifdef BSP_USING_PWM13
#define IFX_PWM_DEVICE_ITEM_PWM13 \
    IFX_PWM_DEVICE(tcpwm_0_group_1_cnt_13_config, tcpwm_0_group_1_cnt_13_hal_config, "pwm13")
#else
#define IFX_PWM_DEVICE_ITEM_PWM13
#endif

#define IFX_PWM_DEVICE_LIST \
    IFX_PWM_DEVICE_ITEM_PWM5 \
    IFX_PWM_DEVICE_ITEM_PWM6 \
    IFX_PWM_DEVICE_ITEM_PWM18 \
    IFX_PWM_DEVICE_ITEM_PWM13

#ifdef __cplusplus
}
#endif

#endif /* __PWM_CONFIG_H__ */
