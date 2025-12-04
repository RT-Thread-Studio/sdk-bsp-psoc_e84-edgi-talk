/*
 * $ Copyright Cypress Semiconductor $
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
