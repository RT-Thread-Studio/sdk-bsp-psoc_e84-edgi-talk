/***************************************************************************//**
* \file mtb_hal_comp_mxs22lpcomp_v1.h
*
* \brief
* Provides common API declarations of the mxs22lpcomp_v1 driver
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

#pragma once

#include "mtb_hal_comp.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
*       Defines
*******************************************************************************/

/*******************************************************************************
*       Functions
*******************************************************************************/

//--------------------------------------------------------------------------------------------------
// _mtb_hal_comp_lpcomp_setup
//--------------------------------------------------------------------------------------------------
__STATIC_INLINE cy_rslt_t _mtb_hal_comp_lpcomp_setup(mtb_hal_comp_t* obj,
                                                     const mtb_hal_comp_configurator_t* config)
{
    CY_ASSERT(obj != NULL);
    CY_ASSERT(config != NULL);

    obj->channel_lpcomp = (1u == config->channel_num) ? CY_LPCOMP_CHANNEL_1 : CY_LPCOMP_CHANNEL_0;
    obj->base_lpcomp = (config->lpcomp_base == NULL) ? LPCOMP : config->lpcomp_base;
    return CY_RSLT_SUCCESS;
}


//--------------------------------------------------------------------------------------------------
// _mtb_hal_comp_lpcomp_read
//--------------------------------------------------------------------------------------------------
__STATIC_INLINE bool _mtb_hal_comp_lpcomp_read(mtb_hal_comp_t* obj)
{
    CY_ASSERT(obj != NULL);
    return (0UL != Cy_LPComp_GetCompare(obj->base_lpcomp, obj->channel_lpcomp));
}


//--------------------------------------------------------------------------------------------------
// _mtb_hal_comp_lpcomp_set_ref
//--------------------------------------------------------------------------------------------------
__STATIC_INLINE cy_rslt_t _mtb_hal_comp_lpcomp_set_ref(mtb_hal_comp_t* obj, uint16_t ref_mv)
{
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(ref_mv);
    return MTB_HAL_COMP_RSLT_ERR_NOT_SUPPORTED;
}


#if defined(__cplusplus)
}
#endif
