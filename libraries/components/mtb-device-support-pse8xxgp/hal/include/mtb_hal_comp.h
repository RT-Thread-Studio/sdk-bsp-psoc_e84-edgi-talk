/***************************************************************************//**
* \file mtb_hal_comp.h
*
* Provides a high level interface for interacting with the Infineon Analog Comparator.
* This interface abstracts out the chip specific details.
* If any chip specific functionality is required the low level functions can
* be used directly.
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
 * \addtogroup group_hal_comp COMP (Analog Comparator)
 * \ingroup group_hal
 * \{
 * High level interface for interacting with an analog Comparator.
 *
 * \section mtb_hal_comp_features Features
 * The analog comparator measures one input voltage from the non-inverting pin against
 * a second voltage provided on the inverting pin. The result of this comparison can
 * be used in two ways:
 * - Output to pin or other on-chip destination (configured via configurator)
 * - Read state via firmware (see @ref mtb_hal_comp_read)
 *
 * These abilities can be used in any combination.
 *
 * \section mtb_hal_comp_quickstart Quickstart
 * Call \ref mtb_hal_comp_setup to initialize the HAL for a peripheral instance. The peripheral
 * must]
 * have already been initialized and configured using the PDL.
 *
 * Use \ref mtb_hal_comp_read to read the comparator state from firmware.
 *
 * \section subsection_comp_snippets Code Snippets:
 * \note Error checking is omitted for clarity
 * \section subsection_comp_snippet_1 Snippet 1: Comparator read value
 * The following snippet reads the current comparator value into a variable
 * \snippet hal_comp.c snippet_mtb_hal_comp_read
 *
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "mtb_hal_gpio.h"
#include "cy_result.h"
#include "mtb_hal_hw_types.h"

#if defined(MTB_HAL_DRIVER_AVAILABLE_COMP)

#if defined(__cplusplus)
extern "C" {
#endif

/** \addtogroup group_hal_results_comp Comparator HAL Results
 *  Comparator specific return codes
 *  \ingroup group_hal_results
 *  \{ *//**
 */

/** Bad argument */
#define MTB_HAL_COMP_RSLT_ERR_BAD_ARGUMENT                  \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, MTB_HAL_RSLT_MODULE_COMP, 0))
/** Not supported */
#define MTB_HAL_COMP_RSLT_ERR_NOT_SUPPORTED                  \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, MTB_HAL_RSLT_MODULE_COMP, 1))

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
 * @return the status of the HAL setup
 */
cy_rslt_t mtb_hal_comp_setup(mtb_hal_comp_t* obj, const mtb_hal_comp_configurator_t* config);

/** Reads the Comparator state.
 *
 * @param[in]  obj    Comparator object
 * @return The Comparator state. True if the non-inverting pin voltage is greater than the
 * inverting pin voltage, false otherwise.
 */
bool mtb_hal_comp_read(mtb_hal_comp_t* obj);

/** Sets the reference voltage value
 *
 * @param[in] obj            Comparator object
 * @param[in] ref_mv         Reference voltage in millivolts
 * @return The status of the set reference request
 */
cy_rslt_t mtb_hal_comp_set_ref(mtb_hal_comp_t* obj, uint16_t ref_mv);

#if defined(__cplusplus)
}
#endif

#ifdef MTB_HAL_COMP_IMPL_HEADER
#include MTB_HAL_COMP_IMPL_HEADER
#endif /* MTB_HAL_COMP_IMPL_HEADER */

#endif // defined(MTB_HAL_DRIVER_AVAILABLE_COMP)

/** \} group_hal_comp */
