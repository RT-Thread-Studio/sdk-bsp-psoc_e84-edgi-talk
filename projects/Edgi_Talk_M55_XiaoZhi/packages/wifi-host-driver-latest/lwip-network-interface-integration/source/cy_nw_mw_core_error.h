/*
 * $ Copyright Cypress Semiconductor $
*/

#ifndef CY_NW_MW_CORE_ERROR_H_
#define CY_NW_MW_CORE_ERROR_H_

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "cy_result.h"
#include "cy_result_mw.h"

/**
 * @defgroup generic_lwip_whd_port_defines WiFi middleware specific error codes
 * @ingroup group_utils_enums
 * Wi-Fi middleware core APIs return results of type cy_rslt_t and consist of three parts:
 * - module base
 * - type
 * - error code
 *
 * \par Result Format
 *
   \verbatim
              Module base                   Type    Library-specific error code
      +-----------------------------------+------+------------------------------+
      |CY_RSLT_MODULE_NETWORK_PORT_BASE  | 0x2  |           Error code         |
      +-----------------------------------+------+------------------------------+
                14-bits                    2-bits            16-bits

   Refer to the macro section of this document for library-specific error codes.
   \endverbatim
 *
 * The data structure cy_rslt_t is part of cy_result.h located in <core_lib/include>.
 *
 * Module base: This base is derived from CY_RSLT_MODULE_MIDDLEWARE_BASE (defined in cy_result.h) and is an offset of the CY_RSLT_MODULE_MIDDLEWARE_BASE.
 *              The details of the offset and the middleware base are defined in cy_result_mw.h, that is part of [Github connectivity-utilities] (https://github.com/infineon/connectivity-utilities).
 *              For example, lwIP WHD PORT (CY_NETWORK_PORT) uses CY_RSLT_MODULE_NETWORK_PORT_BASE as the module base.
 *
 * Type: This type is defined in cy_result.h and can be one of CY_RSLT_TYPE_FATAL, CY_RSLT_TYPE_ERROR, CY_RSLT_TYPE_WARNING, or CY_RSLT_TYPE_INFO. AWS library error codes are of type CY_RSLT_TYPE_ERROR.
 *
 * Library-specific error codes: These error codes are library-specific and defined in the macro section.
 *
 * Helper macros used for creating the library-specific result are provided as part of cy_result.h.
 * \{
 */
/** Generic CY NETWORK port base error code */
#define CY_RSLT_NETWORK_PORT_ERR_BASE                     CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_NETWORK_PORT_BASE, 0)

/** CY NETWORK error */
#define CY_RSLT_NETWORK_INTERFACE_EXISTS                      (CY_RSLT_NETWORK_PORT_ERR_BASE + 1)  /**< Denotes that the interface already exists   */
#define CY_RSLT_NETWORK_ERROR_ADDING_INTERFACE                (CY_RSLT_NETWORK_PORT_ERR_BASE + 2)  /**< Denotes that adding the interface failed    */
#define CY_RSLT_NETWORK_ERROR_STARTING_DHCP                   (CY_RSLT_NETWORK_PORT_ERR_BASE + 3)  /**< Denotes a failure to start DHCP      */
#define CY_RSLT_NETWORK_INTERFACE_DOES_NOT_EXIST              (CY_RSLT_NETWORK_PORT_ERR_BASE + 4)  /**< Denotes that the interface does not exist   */
#define CY_RSLT_NETWORK_BAD_ARG                               (CY_RSLT_NETWORK_PORT_ERR_BASE + 6)  /**< Denotes a BAD arg                    */
#define CY_RSLT_NETWORK_SOCKET_ERROR                          (CY_RSLT_NETWORK_PORT_ERR_BASE + 7)  /**< Denotes the lwIP socket error          */
#define CY_RSLT_NETWORK_SOCKET_CREATE_FAIL                    (CY_RSLT_NETWORK_PORT_ERR_BASE + 8)  /**< Denotes the lwIP socket create failure    */
#define CY_RSLT_NETWORK_INVALID_SOCKET                        (CY_RSLT_NETWORK_PORT_ERR_BASE + 9)  /**< Denotes an invalid socket             */
#define CY_RSLT_NETWORK_CORRUPT_BUFFER                        (CY_RSLT_NETWORK_PORT_ERR_BASE + 10) /**< Denotes a corrupt buffer             */
#define CY_RSLT_NETWORK_DHCP_TIMEOUT                          (CY_RSLT_NETWORK_PORT_ERR_BASE + 11) /**< Denotes the DHCP timeout               */
#define CY_RSLT_NETWORK_DHCP_WAIT_TIMEOUT                     (CY_RSLT_NETWORK_PORT_ERR_BASE + 12) /**< Denotes the DHCP wait timeout          */
#define CY_RSLT_NETWORK_DHCP_MUTEX_ERROR                      (CY_RSLT_NETWORK_PORT_ERR_BASE + 13) /**< Denotes the DHCP Mutex error           */
#define CY_RSLT_NETWORK_ERROR_STARTING_INTERNAL_DHCP          (CY_RSLT_NETWORK_PORT_ERR_BASE + 14) /**< Denotes the failure to start the internal DHCP server */
#define CY_RSLT_NETWORK_INTERFACE_NETWORK_NOT_UP              (CY_RSLT_NETWORK_PORT_ERR_BASE + 15) /**< Denotes that the network is not up for the given interface */
#define CY_RSLT_NETWORK_ERROR_REMOVING_INTERFACE              (CY_RSLT_NETWORK_PORT_ERR_BASE + 16) /**< Denotes an error while removing the interface */
#define CY_RSLT_NETWORK_IPV6_INTERFACE_NOT_READY              (CY_RSLT_NETWORK_PORT_ERR_BASE + 17) /**< Denotes that the IPv6 interface is not ready */
#define CY_RSLT_NETWORK_ERROR_GET_MAC_ADDR                    (CY_RSLT_NETWORK_PORT_ERR_BASE + 18) /**< Denotes the failure to read the MAC address */
#define CY_RSLT_NETWORK_ERROR_PING                            (CY_RSLT_NETWORK_PORT_ERR_BASE + 19) /**< Denotes the failure to send a ping request*/
#define CY_RSLT_NETWORK_ERROR_NOMEM                           (CY_RSLT_NETWORK_PORT_ERR_BASE + 20) /**< Denotes the failure to allocate memory*/
#define CY_RSLT_NETWORK_ERROR_RTOS                            (CY_RSLT_NETWORK_PORT_ERR_BASE + 21) /**< Denotes the failure from RTOS calls*/
#define CY_RSLT_NETWORK_NOT_SUPPORTED                         (CY_RSLT_NETWORK_PORT_ERR_BASE + 22) /**< Denotes that the feature is not supported*/
#define CY_RSLT_NETWORK_LINK_NOT_UP                           (CY_RSLT_NETWORK_PORT_ERR_BASE + 23) /**< Denotes that the Link is not up*/
#define CY_RSLT_NETWORK_ERROR_TRNG                            (CY_RSLT_NETWORK_PORT_ERR_BASE + 24) /**< Denotes that the random number generation failed*/
/**
 * \}
 */

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* CY_NW_MW_CORE_ERROR_H_ */
