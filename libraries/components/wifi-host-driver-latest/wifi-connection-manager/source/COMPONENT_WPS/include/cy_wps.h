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
* @file cy_wps.h
* @brief Cypress WPS file for WPS initialization and handshake
*/
#pragma once

#include "cy_wps_structures.h"
#include "cy_chip_constants.h"
#include "whd_types.h"
#include "cyabs_rtos_impl.h"
#include "whd_wlioctl.h"
#include "whd_debug.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/
#define DSSS_PARAMETER_SET_LENGTH (1)

#define HT_CAPABILITIES_IE_LENGTH (26)
#define HT_OPERATION_IE_LENGTH    (22)

#define RTOS_HIGHEST_PRIORITY            (CY_RTOS_PRIORITY_MAX)
#define RTOS_DEFAULT_THREAD_PRIORITY     (RTOS_HIGHEST_PRIORITY - 4)
#define RTOS_HIGHER_PRIORTIY_THAN(x)     (x < RTOS_HIGHEST_PRIORITY ? x+1 : RTOS_HIGHEST_PRIORITY)
#define RTOS_LOWER_PRIORTIY_THAN(x)      (x > RTOS_LOWEST_PRIORITY ? x-1 : RTOS_LOWEST_PRIORITY)



/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

extern cy_rslt_t cy_wps_init( cy_wps_agent_t* workspace, const cy_wps_device_detail_t* details, cy_wps_agent_type_t type, whd_interface_t interface );
extern cy_rslt_t cy_wps_get_result( cy_wps_agent_t* workspace );
extern cy_rslt_t cy_wps_deinit( cy_wps_agent_t* workspace );
extern cy_rslt_t cy_wps_start( cy_wps_agent_t* workspace, cy_wps_mode_t mode, const char* password, cy_wps_credential_t* credentials, uint16_t* credential_length );
extern cy_rslt_t cy_p2p_wps_start( cy_wps_agent_t* workspace );
extern cy_rslt_t cy_wps_restart( cy_wps_agent_t* workspace );
extern cy_rslt_t cy_wps_reset_registrar( cy_wps_agent_t* workspace, whd_mac_t* mac );
extern cy_rslt_t cy_wps_wait_till_complete( cy_wps_agent_t* workspace );
extern cy_rslt_t cy_wps_abort( cy_wps_agent_t* workspace );
extern cy_rslt_t cy_wps_management_set_event_handler( cy_wps_agent_t* workspace, bool enable );
extern cy_rslt_t cy_wps_set_directed_wps_target( cy_wps_agent_t* workspace, cy_wps_ap_t* ap, uint32_t maximum_join_attempts );


int              cy_wps_get_stored_credential_count( cy_wps_agent_t* workspace );
extern void      cy_wps_thread_main( cy_thread_arg_t arg );
extern cy_rslt_t cy_wps_internal_init( cy_wps_agent_t* workspace, uint32_t interface, cy_wps_mode_t mode, const char* password, cy_wps_credential_t* credentials, uint16_t* credential_length );

#ifdef __cplusplus
} /*extern "C" */
#endif
