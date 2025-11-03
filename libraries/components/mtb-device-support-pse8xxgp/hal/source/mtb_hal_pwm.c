/***************************************************************************//**
* \file mtb_hal_pwm.c
*
* \brief
* Provides a high level interface for interacting with the Infineon PWM. This is
* a wrapper around the lower level PDL API.
*
********************************************************************************
* \copyright
* Copyright 2018-2024 Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation
*
* SPDX-License-Identifier: Apache-2.0
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include <string.h>

#include "mtb_hal_utils.h"
#include "mtb_hal_pwm.h"
#include "mtb_hal_pwm_impl.h"


#if (MTB_HAL_DRIVER_AVAILABLE_PWM)

#if defined(__cplusplus)
extern "C" {
#endif

#define _MTB_HAL_PWM_US_PER_SEC                 (1000000UL)
// Convert a lengh of time in us to cycles, given the current operating frequency
#define _MTB_HAL_PWM_US_TO_CYCLES(time, \
                                  freq) (((uint64_t)(time) * (freq)) / _MTB_HAL_PWM_US_PER_SEC)


//--------------------------------------------------------------------------------------------------
// _mtb_hal_pwm_set_period_and_compare
//--------------------------------------------------------------------------------------------------
static cy_rslt_t _mtb_hal_pwm_set_period_and_compare(mtb_hal_pwm_t* obj, uint32_t period,
                                                     uint32_t compare)
{
    uint32_t new_compare_value = 0;
    #if defined(MTB_HAL_DISABLE_ERR_CHECK)
    CY_ASSERT_AND_RETURN((period >= 1) || (period < (obj->tcpwm.max_count - 1)),
                         MTB_HAL_PWM_RSLT_BAD_ARGUMENT);
    #else
    if ((period < 1) || (period > obj->tcpwm.max_count))
    {
        return MTB_HAL_PWM_RSLT_BAD_ARGUMENT;
    }
    #endif // if defined(MTB_HAL_DISABLE_ERR_CHECK)

    Cy_TCPWM_PWM_SetCompare0(obj->tcpwm.base, obj->tcpwm.cntnum, 0u);
    Cy_TCPWM_PWM_SetPeriod0(obj->tcpwm.base, obj->tcpwm.cntnum, (period - 1u));

    bool is_center_aligned = _mtb_hal_tcpwm_is_center_aligned(obj);

    /* Based on HW datasheet, for left-aligned or right-aligned, CC_match event is generated on
       entering the COUNTER value.
     * For center-aligned, CC_match event is generated on leaving the COUNTER value.
     * We do below adjustment for 100% duty cycle case, that means "period <= compare".
     * Otherwise for 100% duty cycle case, the waveform will go logic low at last clock cycle.
     * In our current HAL design,
     * if (!is_center_aligned) is true, HAL will configure left-aligned PWM.
     */
    if ((!is_center_aligned) && (compare >= period))
    {
        new_compare_value = period;
    }
    else if (compare >= period)
    {
        new_compare_value = period - 1;
    }
    else
    {
        new_compare_value = compare;
    }

    Cy_TCPWM_PWM_SetCompare0(obj->tcpwm.base, obj->tcpwm.cntnum, new_compare_value);
    if (Cy_TCPWM_PWM_GetCounter(obj->tcpwm.base, obj->tcpwm.cntnum) >= new_compare_value)
    {
        Cy_TCPWM_PWM_SetCounter(obj->tcpwm.base, obj->tcpwm.cntnum, 0);
    }

    return CY_RSLT_SUCCESS;
}


/* Macro to convert PWM object to group count address */
#define _MTB_HAL_PWM_OBJ_TO_GRP_CNT(base, group, \
                                    cntnum) \
    ((TCPWM_GRP_CNT_Type*) &(base)->GRP[(group)].CNT[(cntnum)])

//--------------------------------------------------------------------------------------------------
// mtb_hal_pwm_setup
//--------------------------------------------------------------------------------------------------
cy_rslt_t mtb_hal_pwm_setup(mtb_hal_pwm_t* obj, const mtb_hal_pwm_configurator_t* config,
                            const mtb_hal_clock_t* clock)
{
    CY_ASSERT(NULL != obj);
    memset(obj, 0, sizeof(mtb_hal_pwm_t));

    obj->tcpwm.base = config->base;
    obj->tcpwm.cntnum = _MTB_HAL_TCPWM_CNT_NUMBER(config->group, config->cntnum);
    obj->tcpwm.group = _MTB_HAL_TCPWM_GET_GRP(config->group);
    obj->tcpwm.grp_cnt_base = _MTB_HAL_PWM_OBJ_TO_GRP_CNT(config->base, config->group,
                                                          config->cntnum);
    obj->tcpwm.max_count = config->max_count;
    obj->tcpwm.clock = (clock == NULL) ? config->clock : clock;

    return CY_RSLT_SUCCESS;
}


//--------------------------------------------------------------------------------------------------
// mtb_hal_pwm_start
//--------------------------------------------------------------------------------------------------
cy_rslt_t mtb_hal_pwm_start(mtb_hal_pwm_t* obj)
{
    CY_ASSERT(NULL != obj);
    Cy_TCPWM_PWM_Enable(obj->tcpwm.base, obj->tcpwm.cntnum);
    _mtb_hal_tcpwm_trigger_reload(obj);
    return CY_RSLT_SUCCESS;
}


//--------------------------------------------------------------------------------------------------
// mtb_hal_pwm_stop
//--------------------------------------------------------------------------------------------------
cy_rslt_t mtb_hal_pwm_stop(mtb_hal_pwm_t* obj)
{
    CY_ASSERT(NULL != obj);
    _mtb_hal_pwm_disable(obj->tcpwm.base, obj->tcpwm.cntnum);
    return CY_RSLT_SUCCESS;
}


//--------------------------------------------------------------------------------------------------
// mtb_hal_pwm_set_period
//--------------------------------------------------------------------------------------------------
cy_rslt_t mtb_hal_pwm_set_period(mtb_hal_pwm_t* obj, uint32_t period_us, uint32_t pulse_width_us)
{
    CY_ASSERT(NULL != obj);
    CY_ASSERT(NULL != obj->tcpwm.clock->interface->get_frequency_hz);
    cy_rslt_t result = CY_RSLT_SUCCESS;

    obj->tcpwm.clock_hz =
        obj->tcpwm.clock->interface->get_frequency_hz(obj->tcpwm.clock->clock_ref);

    uint32_t period = (uint32_t)_MTB_HAL_PWM_US_TO_CYCLES(period_us, obj->tcpwm.clock_hz);
    uint32_t width = (uint32_t)_MTB_HAL_PWM_US_TO_CYCLES(pulse_width_us, obj->tcpwm.clock_hz);
    result = _mtb_hal_pwm_set_period_and_compare(obj, period, width);

    return result;
}


#if defined(__cplusplus)
}
#endif

#endif /* MTB_HAL_DRIVER_AVAILABLE_PWM */
