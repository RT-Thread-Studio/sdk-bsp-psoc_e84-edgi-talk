/*
 * (c) 2025, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
 */

/**
* @file cy_wcm_internal.h
* @brief This file contains structures and defines needed for encapsulating the parameters of WCM APIs.
*        These structures can be used to encapsulate and send API parameters over multiple interfaces.
*        For example, Virtual Connectivity Manager uses these structures to communicate the API parameters over IPC.
*/

#ifndef CY_WCM_INTERNAL_H
#define CY_WCM_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cy_wcm.h"
#ifdef ENABLE_MULTICORE_CONN_MW
#include "cy_vcm_internal.h"
#endif

/**
 * \addtogroup group_wcm_structures
 * \{
 */
/******************************************************
 *             Structures
 ******************************************************/
/**
 * Structure which encapsulates the parameters of cy_wcm_register_event_callback API.
 */
typedef struct
{
#ifdef ENABLE_MULTICORE_CONN_MW
    cy_vcm_internal_callback_t event_callback;      /**< Callback function registered by the virtual WCM API with VCM to receive WCM events. This function internally calls the application callback. */
#else
    cy_wcm_event_callback_t    event_callback;      /**< WCM event callback function pointer type; if registered, callback is invoked when WHD posts events to WCM */
#endif
} cy_wcm_register_event_callback_params_t;

/**
 * Structure which encapsulates the parameters of cy_wcm_deregister_event_callback API.
 */
typedef struct
{
#ifdef ENABLE_MULTICORE_CONN_MW
    cy_vcm_internal_callback_t event_callback;      /**< VCM event callback function pointer to be de-registered with VCM by the virtual WCM API */
#else
    cy_wcm_event_callback_t    event_callback;      /**< WCM event callback function pointer to be de-registered */
#endif
} cy_wcm_deregister_event_callback_params_t;

/**
 * Structure which encapsulates the parameters of cy_wcm_event_callback_t function.
 */
typedef struct
{
    cy_wcm_event_t             event;               /**< WCM event type. */
    cy_wcm_event_data_t        *event_data;         /**< A pointer to the event data. The event data will be freed once the callback returns from the application */
} cy_wcm_event_callback_params_t;

/** \} group_wcm_structures */

#ifdef __cplusplus
} /* extern C */
#endif

#endif  /* CY_WCM_INTERNAL_H */
