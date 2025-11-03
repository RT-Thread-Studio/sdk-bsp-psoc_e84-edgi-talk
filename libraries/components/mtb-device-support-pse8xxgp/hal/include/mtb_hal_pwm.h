/***************************************************************************//**
* \file mtb_hal_pwm.h
*
* \brief
* Provides a high level interface for interacting with the Infineon PWM.
* This interface abstracts out the chip specific details. If any chip specific
* functionality is necessary, or performance is critical the low level functions
* can be used directly.
*
********************************************************************************
* \copyright
* Copyright 2018-2022 Cypress Semiconductor Corporation (an Infineon company) or
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

/**
 * \addtogroup group_hal_pwm PWM (Pulse Width Modulator)
 * \ingroup group_hal
 * \{
 * High level interface for interacting with the pulse width modulator (PWM) hardware resource
 *
 * The PWM driver can be used to generate periodic digital waveforms with configurable frequency and
 * duty cycle.
 * The driver allows assigning the PWM output and an optional inverted output to supplied pins.
 * The driver supports interrupt generation on PWM terminal count and compare events.
 *
 * \section section_pwm_features Features
 * * Configurable pin assignment for the PWM output
 * * Optional complementary (inverted) PWM output to a second pin
 * * Configurable dead time between normal and inverted PWM outputs
 * * Configurable alignment: left, right or center
 * * Continuous or One-shot operation
 * * Option to instantiate and use a new clock or use pre-allocated clock for clock input
 * * Configurable interrupt and callback assignment on PWM events: terminal count, compare match or
 * combination of both
 *
 * \section section_pwm_quickstart Quick Start
 *
 * See \ref subsection_pwm_snippet_1 for a code snippet that generates a signal with the specified
 * frequency and duty cycle on the specified pin.
 *
 * \section section_pwm_snippets Code snippets
 *
 * \subsection subsection_pwm_snippet_1 Snippet 1: Simple PWM initialization and output to pin
 * The clock parameter <b>clk</b> is optional and need not be provided (NULL), to generate and use
 * an available clock resource with a default frequency. <br>
 * The clock frequency and the duty cycle is set using \ref mtb_hal_pwm_set_period. <br>
 * \ref mtb_hal_pwm_start starts the PWM output on the pin.
 *
 * \snippet hal_pwm.c snippet_mtb_hal_pwm_init
 *
 *
 * \subsection subsection_pwm_snippet_2 Snippet 2: Starting and stopping the PWM output
 * \ref mtb_hal_pwm_start and \ref mtb_hal_pwm_stop functions can be used after PWM initialization
 * to start and stop the PWM output.
 *
 * \snippet hal_pwm.c snippet_mtb_hal_pwm_start_stop
 *
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "cy_result.h"
#include "cy_utils.h"
#include "mtb_hal_hw_types.h"

#if defined(MTB_HAL_DRIVER_AVAILABLE_PWM)

#if defined(__cplusplus)
extern "C" {
#endif

/** \addtogroup group_hal_results_pwm PWM HAL Results
 *  PWM specific return codes
 *  \ingroup group_hal_results
 *  \{ *//**
 */

/** Bad argument */
#define MTB_HAL_PWM_RSLT_BAD_ARGUMENT                     \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, MTB_HAL_RSLT_MODULE_PWM, 0))
/** The requested operation is not supported on the current hardware */
#define MTB_HAL_PWM_RSLT_ERR_UNSUPPORTED                  \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, MTB_HAL_RSLT_MODULE_PWM, 1))

/**
 * \}
 */

/**
 * Sets up a HAL instance to use the specified hardware resource. This hardware
 * resource must have already been configured via the PDL.
 *
 * @param[out] obj   The HAL driver instance object. The caller must allocate the
 *                   memory for this object, but the HAL will initialize its contents
 * @param[in] config The configurator-generated HAL config structure for this
 *                   peripheral instance
 * @param[in] clock  The HAL clock object that is connected to this peripheral instance
 * @return the status of the HAL setup
 */
cy_rslt_t mtb_hal_pwm_setup(mtb_hal_pwm_t* obj, const mtb_hal_pwm_configurator_t* config,
                            const mtb_hal_clock_t* clock);

/** Set the number of microseconds for the PWM period & pulse width
 *
 * @param[in] obj            The PWM object
 * @param[in] period_us      The period in microseconds
 * @param[in] pulse_width_us The pulse width in microseconds
 * @return                   The status of the period request
 */
cy_rslt_t mtb_hal_pwm_set_period(mtb_hal_pwm_t* obj, uint32_t period_us, uint32_t pulse_width_us);

/** Starts the PWM generation and outputs on <b>pin</b> and <b>compl_pin</b>.
 *
 * @param[in] obj           The PWM object
 * @return                  The status of the start request
 */
cy_rslt_t mtb_hal_pwm_start(mtb_hal_pwm_t* obj);

/** Stops the PWM generation and outputs on <b>pin</b> and <b>compl_pin</b>.
 *
 * @param[in] obj          The PWM object
 * @return                 The status of the stop request
 */
cy_rslt_t mtb_hal_pwm_stop(mtb_hal_pwm_t* obj);


#if defined(__cplusplus)
}
#endif

#ifdef MTB_HAL_PWM_IMPL_HEADER
#include MTB_HAL_PWM_IMPL_HEADER
#endif /* MTB_HAL_PWM_IMPL_HEADER */

#endif //defined(MTB_HAL_DRIVER_AVAILABLE_PWM)

/** \} group_hal_pwm */
