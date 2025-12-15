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
* @file cy_eapol.h
* @brief EAPOL handling for receiving EAPOL data from WHD
*/
#pragma once

#include "whd_types.h"
#include "whd.h"
#include "whd_buffer_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define CY_EAP_MTU_SIZE                 ( 1020 )
#define CY_ETHERNET_ADDRESS_LENGTH      ( 6 )
/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef enum
{
    CY_EAP_CODE_REQUEST  = 1,
    CY_EAP_CODE_RESPONSE = 2,
    CY_EAP_CODE_SUCCESS  = 3,
    CY_EAP_CODE_FAILURE  = 4
} cy_eap_code_t;

/*
 * EAP Request and Response data begins with one octet Type. Success and
 * Failure do not have additional data.
 */
typedef enum {
    CY_EAP_TYPE_NONE         = 0,
    CY_EAP_TYPE_IDENTITY     = 1   /* RFC 3748 */,
    CY_EAP_TYPE_NOTIFICATION = 2   /* RFC 3748 */,
    CY_EAP_TYPE_NAK          = 3   /* Response only, RFC 3748 */,
    CY_EAP_TYPE_MD5          = 4,  /* RFC 3748 */
    CY_EAP_TYPE_OTP          = 5   /* RFC 3748 */,
    CY_EAP_TYPE_GTC          = 6,  /* RFC 3748 */
    CY_EAP_TYPE_TLS          = 13  /* RFC 2716 */,
    CY_EAP_TYPE_LEAP         = 17  /* Cisco proprietary */,
    CY_EAP_TYPE_SIM          = 18  /* draft-haverinen-pppext-eap-sim-12.txt */,
    CY_EAP_TYPE_TTLS         = 21  /* draft-ietf-pppext-eap-ttls-02.txt */,
    CY_EAP_TYPE_AKA          = 23  /* draft-arkko-pppext-eap-aka-12.txt */,
    CY_EAP_TYPE_PEAP         = 25  /* draft-josefsson-pppext-eap-tls-eap-06.txt */,
    CY_EAP_TYPE_MSCHAPV2     = 26  /* draft-kamath-pppext-eap-mschapv2-00.txt */,
    CY_EAP_TYPE_TLV          = 33  /* draft-josefsson-pppext-eap-tls-eap-07.txt */,
    CY_EAP_TYPE_FAST         = 43  /* draft-cam-winget-eap-fast-00.txt */,
    CY_EAP_TYPE_PAX          = 46, /* draft-clancy-eap-pax-04.txt */
    CY_EAP_TYPE_EXPANDED_NAK = 253 /* RFC 3748 */,
    CY_EAP_TYPE_WPS          = 254 /* Wireless Simple Config */,
    CY_EAP_TYPE_PSK          = 255 /* EXPERIMENTAL - type not yet allocated draft-bersani-eap-psk-09 */
} cy_eap_type_t;

/**
 * EAPOL types
 */

typedef enum
{
    CY_EAP_PACKET                   = 0,
    CY_EAPOL_START                  = 1,
    CY_EAPOL_LOGOFF                 = 2,
    CY_EAPOL_KEY                    = 3,
    CY_EAPOL_ENCAPSULATED_ASF_ALERT = 4
} cy_eapol_packet_type_t;


/******************************************************
 *                    Structures
 ******************************************************/

#pragma pack(1)

typedef struct
{
    uint8_t* data;
    uint16_t length;
    uint32_t packet_mask;
} cy_ie_t;

typedef struct
{
    uint8_t   ether_dhost[CY_ETHERNET_ADDRESS_LENGTH];
    uint8_t   ether_shost[CY_ETHERNET_ADDRESS_LENGTH];
    uint16_t  ether_type;
} cy_ether_header_t;

typedef struct
{
    uint8_t  version;
    uint8_t  type;
    uint16_t length;
} cy_eapol_header_t;

typedef struct
{
    cy_ether_header_t  ethernet;
    cy_eapol_header_t  eapol;
} cy_eapol_packet_header_t;

typedef struct
{
    cy_ether_header_t  ethernet;
    cy_eapol_header_t  eapol;
    uint8_t         data[1];
} cy_eapol_packet_t;

typedef struct
{
    uint8_t  code;
    uint8_t  id;
    uint16_t length;
    uint8_t  type;
} cy_eap_header_t;

typedef struct
{
    cy_ether_header_t  ethernet;
    cy_eapol_header_t  eapol;
    cy_eap_header_t    eap;
    uint8_t         data[1];
} cy_eap_packet_t;

typedef struct
{
    uint8_t  vendor_id[3];
    uint32_t vendor_type;
    uint8_t  op_code;
    uint8_t  flags;
} cy_eap_expanded_header_t;

typedef struct
{
    uint8_t  flags;
} cy_eap_tls_header_t;

typedef struct
{
    cy_ether_header_t        ethernet;
    cy_eapol_header_t        eapol;
    cy_eap_header_t          eap;
    cy_eap_tls_header_t      eap_tls;
    uint8_t               data[1]; // Data starts with a length of TLS data field or TLS data depending on the flags field
} cy_eap_tls_packet_t;

#pragma pack()

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
extern uint8_t*     cy_eapol_get_eapol_data( whd_buffer_t packet, whd_interface_t interface );
extern uint16_t     cy_get_eapol_packet_size( whd_buffer_t packet, whd_interface_t interface );

#ifdef __cplusplus
} /*extern "C" */
#endif
