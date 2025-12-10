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
#include <string.h>
#include "tlv.h"

#include "wps_helper_utility.h"

/******************************************************
 *                      Macros
 ******************************************************/

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
 *               Static Function Declarations
 ******************************************************/

static uint32_t tlv_hton32_ptr(uint8_t* in, uint8_t* out);
static uint16_t tlv_hton16_ptr(uint8_t* in, uint8_t* out);

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

tlv8_data_t* tlv_find_tlv8( const uint8_t* message, uint32_t message_length, uint8_t type )
{
    while ( message_length != 0 )
    {
        uint8_t  current_tlv_type   = message[ 0 ];
        uint16_t current_tlv_length = message[ 1 ] + 2;

        /* Check if we've overrun the buffer */
        if ( current_tlv_length > message_length )
        {
            return NULL;
        }

        /* Check if we've found the type we are looking for */
        if ( current_tlv_type == type )
        {
            return (tlv8_data_t*) message;
        }

        /* Skip current TLV */
        message        += current_tlv_length;
        message_length -= current_tlv_length;
    }
    return 0;
}

tlv16_data_t* tlv_find_tlv16( const uint8_t* message, uint32_t message_length, uint16_t type )
{
    uint16_t aligned_type, aligned_length;

    while ( message_length != 0 )
    {
        tlv16_data_t* tlv                = (tlv16_data_t*) message;
        aligned_type = tlv->type;
        uint16_t      current_tlv_type   = htobe16( TLV_READ_16( &aligned_type ) );
        aligned_length = tlv->length;
        uint16_t      current_tlv_length = htobe16( TLV_READ_16( &aligned_length ) ) + 4;

        /* Check if we've overrun the buffer */
        if ( current_tlv_length > message_length )
        {
            return NULL;
        }

        /* Check if we've found the type we are looking for */
        if ( current_tlv_type == type )
        {
            return tlv;
        }

        /* Skip current TLV */
        message        += current_tlv_length;
        message_length -= current_tlv_length;
    }

    return NULL;
}

tlv_result_t tlv_read_value ( uint16_t type, const uint8_t* message, uint16_t message_length, void* value, uint16_t value_size, tlv_data_type_t data_type )
{
    tlv16_data_t* tlv = tlv_find_tlv16( message, message_length, type );
    if (tlv == NULL)
    {
        return TLV_NOT_FOUND;
    }

    switch (data_type)
    {
        case TLV_UINT16:
            tlv_hton16_ptr((uint8_t*) tlv->data, value );
            break;

        case TLV_UINT32:
            tlv_hton32_ptr((uint8_t*) tlv->data, value );
            break;

        default:
            memcpy( value, tlv->data, value_size );
            break;
    }

    return TLV_SUCCESS;
}

uint8_t* tlv_write_value( uint8_t* buffer, uint16_t type, uint16_t length, const void* data, tlv_data_type_t data_type )
{
    uint16_t aligned_type, aligned_length;
    tlv16_data_t* tlv = (tlv16_data_t*) buffer;

    TLV_WRITE_16( &aligned_type, htobe16(type) );
    tlv->type = aligned_type;
    TLV_WRITE_16( &aligned_length, htobe16(length) );
    tlv->length = aligned_length;

    switch (data_type)
    {
        case TLV_UINT16:
            tlv_hton16_ptr((uint8_t*) data, tlv->data);
            break;

        case TLV_UINT32:
            tlv_hton32_ptr((uint8_t*) data, tlv->data);
            break;

        default:
            memcpy(tlv->data, data, length);
            break;
    }
    return buffer + sizeof(tlv16_header_t) + length;
}

uint8_t* tlv_write_header( uint8_t* buffer, uint16_t type, uint16_t length )
{
    uint16_t aligned_type, aligned_length;

    tlv16_header_t* tlv = (tlv16_header_t*)buffer;
    TLV_WRITE_16( &aligned_type, htobe16(type) );
    tlv->type = aligned_type;
    TLV_WRITE_16( &aligned_length, htobe16(length) );
    tlv->length = aligned_length;
    return buffer + sizeof(tlv16_header_t);
}



static uint32_t tlv_hton32_ptr(uint8_t * in, uint8_t * out)
{
    uint32_t temp;
    temp = TLV_READ_32(in);
    temp = htobe32(temp);
    TLV_WRITE_32(out, temp);
    return temp;
}

static uint16_t tlv_hton16_ptr(uint8_t * in, uint8_t * out)
{
    uint16_t temp;
    temp = TLV_READ_16(in);
    temp = htobe16(temp);
    TLV_WRITE_16(out, temp);
    return temp;
}
