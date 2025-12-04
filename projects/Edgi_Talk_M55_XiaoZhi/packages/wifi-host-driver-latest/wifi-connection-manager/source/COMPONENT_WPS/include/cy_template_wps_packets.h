/*
 * $ Copyright Cypress Semiconductor $
 */

/**
* @file cy_template_wps_packets.h
* @brief Cypress WPS templates
*/
#pragma once

#include "cy_wps_structures.h"
#include "tlv.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

typedef struct
{
    tlv16_header_t header;
    uint8_t        vendor_extension_id[3];
    tlv8_uint8_t   version2;
} template_vendor_extension_t;

typedef struct
{
    tlv16_uint8_t                version;
    tlv16_uint8_t                request_type;
    tlv16_uint16_t               config_methods;
    tlv16_header_t               uuid;
    cy_wps_uuid_t                uuid_data;
    tlv16_header_t               primary_device;
    cy_wps_primary_device_type_t primary_device_data;
    tlv16_uint8_t                rf_band;
    tlv16_uint16_t               assoc_state;
    tlv16_uint16_t               config_error;
    tlv16_uint16_t               pwd_id;
} template_wps_probe_ie_t;

typedef struct
{
    tlv16_uint8_t                version;
    tlv16_uint8_t                request_type;
    tlv16_uint16_t               config_methods;
    tlv16_header_t               uuid;
    cy_wps_uuid_t                uuid_data;
    tlv16_header_t               primary_device;
    cy_wps_primary_device_type_t primary_device_data;
    tlv16_uint8_t                rf_band;
    tlv16_uint16_t               assoc_state;
    tlv16_uint16_t               config_error;
    tlv16_uint16_t               pwd_id;
    tlv16_header_t               manufacturer;
    tlv16_header_t               model_name;
    tlv16_header_t               model_number;
    tlv16_header_t               device_name;
    tlv16_header_t               vendor_ext;
    cy_wps_m1_vendor_ext_t       vendor_ext_data;
} template_wps2_probe_ie_t;

typedef struct
{
    tlv16_uint8_t                version;
    tlv16_uint8_t                wps_setup_state;
    tlv16_uint16_t               password_id;
    tlv16_uint8_t                selected_registrar;
    tlv16_uint16_t               selected_registrar_config_methods;
    tlv16_uint8_t                response_type;
    tlv16_header_t               uuid;
    cy_wps_uuid_t                uuid_data;
    tlv16_header_t               primary_device;
    cy_wps_primary_device_type_t primary_device_data;
    tlv16_uint16_t               config_methods;
    tlv16_header_t               manufacturer;
    tlv16_header_t               model_name;
    tlv16_header_t               model_number;
    tlv16_header_t               device_name;
    tlv16_header_t               serial_number;
} template_wps_probe_response_ie_t;

typedef struct
{
    template_wps_probe_response_ie_t wps1;
    tlv16_header_t                   vendor_ext;
    cy_vendor_ext_t                  vendor_ext_data;
} template_wps2_probe_response_ie_t;

typedef struct
{
    tlv16_uint8_t     version;
    tlv16_uint8_t     request_type;
    tlv16_header_t    vendor_ext;
    cy_vendor_ext_t   vendor_ext_data;
} template_wps2_assoc_request_ie_t;

typedef struct
{
    tlv16_uint8_t  version;
    tlv16_uint8_t  configuration_state;
    tlv16_uint8_t  selected_registrar;
    tlv16_uint16_t password_id;
    tlv16_uint16_t selected_registrar_config_methods;
} template_wps_beacon_ie_t;

typedef struct
{
    template_wps_beacon_ie_t wps1;
    tlv16_header_t           vendor_ext;
    cy_vendor_ext_t          vendor_ext_data;
} template_wps2_beacon_ie_t;

typedef struct
{
    template_wps_beacon_ie_t        wps1;
    tlv16_header_t                  primary_device_type;
    cy_wps_primary_device_type_t    primary_device_data;
    tlv16_header_t                  vendor_ext;
    cy_vendor_ext_t                 vendor_ext_data;
    tlv16_header_t                  device_name;
} template_wps2_p2p_beacon_ie_t;

typedef struct
{
    tlv16_uint8_t  version;
    tlv16_uint8_t  response_type;
} template_wps_assoc_response_ie_t;

typedef struct
{
    cy_ether_header_t        ethernet;
    cy_eapol_header_t        eapol;
    cy_eap_header_t          eap_header;
    cy_eap_expanded_header_t eap_expanded_header;
    tlv16_uint8_t            version;
    tlv16_uint8_t            msg_type;
    tlv16_header_t           enrollee_nonce;
    cy_wps_nonce_t           enrollee_nonce_data;
    tlv16_header_t           registrar_nonce;
    cy_wps_nonce_t           registrar_nonce_data;
} template_basic_wps_packet_t;

#pragma pack()

#ifdef __cplusplus
} /*extern "C" */
#endif
