/*
 * $ Copyright Cypress Semiconductor $
*/

#pragma once

#include "whd_wifi_api.h"
#include "cy_result.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
* \addtogroup group_lwip_network_interface_integration lwip-network-interface-integration
* \{
*
* \defgroup group_wifimwcore_eapol_functions EAPOL Functions
*/

/**
* \addtogroup group_wifimwcore_eapol_functions
* \{
* * Provides functions that the application or library can use to register and deregister.
* * These APIs are generally used by the enterprise security library.
*/

/**
 * EAPOL packet handler function pointer type; on receiving the EAPOL data, the WHD will send the data to the Wi-Fi Middleware Core.
 * The buffer should be freed by the EAPOL handler.
 *
 * @param[in] whd_iface  WHD interface.
 * @param[in] buffer     Buffer received from the WHD.
 *
 */
typedef void (*cy_wifimwcore_eapol_packet_handler_t)(whd_interface_t whd_iface, whd_buffer_t buffer);

/**
 *
 * This API allows registering the callback functions to receive EAPOL packets
 * from the WHD. If the callback is registered, and the received packet is an EAPOL packet,
 * it will be redirected to the registered callback. Passing "NULL"
 * as the handler will deregister the previously registered callback.
 *
 * @param[in] eapol_packet_handler : Callback function to be invoked when EAPOL packets are received from the WHD.
 *
 * @return CY_RSLT_SUCCESS if the registration was successful; returns \ref generic_lwip_whd_port_defines otherwise.
 *
 */
cy_rslt_t cy_wifimwcore_eapol_register_receive_handler(cy_wifimwcore_eapol_packet_handler_t eapol_packet_handler);

/** \} group_wifimwcore_eapol_functions */
#ifdef __cplusplus
}
#endif
/** \} group_lwip_network_interface_integration */
