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


/** @file
 * Functions to read/write Type-Length-Value (TLV) data used in many common protocols.
 */

#ifndef INCLUDED_TLV_H
#define INCLUDED_TLV_H

#ifdef COMPONENT_CM0P
#define WICED_HOST_REQUIRES_ALIGNED_MEMORY_ACCESS
#endif /* COMPONENT_CM0P */

#include <stdint.h>

/******************************************************
 *                      Macros
 ******************************************************/
/**
 *  Define macros to assist operation on host MCUs that require aligned memory access
 */
#ifndef WICED_HOST_REQUIRES_ALIGNED_MEMORY_ACCESS

#define TLV_WRITE_16( pointer, value )      (*((uint16_t*)pointer) = value)
#define TLV_WRITE_32( pointer, value )      (*((uint32_t*)pointer) = value)
#define TLV_READ_16( pointer )              *((uint16_t*)pointer)
#define TLV_READ_32( pointer )              *((uint32_t*)pointer)

#else /* WICED_HOST_REQUIRES_ALIGNED_MEMORY_ACCESS */

#define WICED_MEMCPY( destination, source, size )   mem_byte_cpy( destination, source, size )

#define TLV_WRITE_16( pointer, value )      do { ((uint8_t*)pointer)[0] = (uint8_t)value; ((uint8_t*)pointer)[1]=(uint8_t)(value>>8); } while(0)
#define TLV_WRITE_32( pointer, value )      do { ((uint8_t*)pointer)[0] = (uint8_t)value; ((uint8_t*)pointer)[1]=(uint8_t)(value>>8); ((uint8_t*)pointer)[2]=(uint8_t)(value>>16); ((uint8_t*)pointer)[3]=(uint8_t)(value>>24); } while(0)
#define TLV_READ_16( pointer )              (((uint8_t*)pointer)[0] + (((uint8_t*)pointer)[1] << 8))
#define TLV_READ_32( pointer )              (((uint8_t*)pointer)[0] + ((((uint8_t*)pointer)[1] << 8)) + (((uint8_t*)pointer)[2] << 16) + (((uint8_t*)pointer)[3] << 24))

#endif /* WICED_HOST_REQUIRES_ALIGNED_MEMORY_ACCESS */

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    TLV_UINT8     = 1,
    TLV_UINT16    = 2,
    TLV_UINT32    = 3,
    TLV_CHAR_PTR  = 4,
    TLV_UINT8_PTR = 5,
} tlv_data_type_t;

typedef enum
{
    TLV_SUCCESS,
    TLV_NOT_FOUND
} tlv_result_t;


/******************************************************
 *                 Type Definitions
 ******************************************************/

#pragma pack(1)

/* Structures for TLVs with 16-bit type and 16-bit length */
typedef struct
{
    uint16_t type;
    uint16_t length;
} tlv16_header_t;

typedef struct
{
    uint16_t type;
    uint16_t length;
    uint8_t data;
} tlv16_uint8_t;

typedef struct
{
    uint16_t type;
    uint16_t length;
    uint16_t data;
} tlv16_uint16_t;

typedef struct
{
    uint16_t type;
    uint16_t length;
    uint32_t data;
} tlv16_uint32_t;

typedef struct
{
    uint16_t type;
    uint16_t length;
    uint8_t data[1];
} tlv16_data_t;

/* Structures for TLVs with 8-bit type and 8-bit length */
typedef struct
{
    uint8_t type;
    uint8_t length;
} tlv8_header_t;

typedef struct
{
    uint8_t type;
    uint8_t length;
    uint8_t data;
} tlv8_uint8_t;

typedef struct
{
    uint8_t type;
    uint8_t length;
    uint16_t data;
} tlv8_uint16_t;

typedef struct
{
    uint8_t type;
    uint8_t length;
    uint32_t data;
} tlv8_uint32_t;

typedef struct
{
    uint8_t type;
    uint8_t length;
    uint8_t data[1];
} tlv8_data_t;

#pragma pack()

#ifdef __cplusplus
extern "C" {
#endif

/** Finds a particular type from a buffer of TLV data.
 *
 * Returns a pointer to the first TLV which has a type that matches the given type.
 * The pointer points to the 'type' field.
 * This is the 8-bit-Type & 8-bit-Lenght version
 *
 * @param[in] message        : A pointer to the source TLV data to be searched
 * @param[in] message_length : The length of the source TLV data to be searched
 * @param[in] type           : The TLV type to search for
 *
 * @return Pointer to start of first matching TLV, or NULL if not found
 */
tlv8_data_t*  tlv_find_tlv8    ( const uint8_t* message, uint32_t message_length, uint8_t type );

/** Finds a particular type from a buffer of TLV data.
 *
 * Returns a pointer to the first TLV which has a type that matches the given type.
 * The pointer points to the 'type' field.
 * This is the 16-bit-Type & 16-bit-Lenght version
 *
 * @param[in] message        : A pointer to the source TLV data to be searched
 * @param[in] message_length : The length of the source TLV data to be searched
 * @param[in] type           : The TLV type to search for
 *
 * @return Pointer to start of first matching TLV, or NULL if not found
 */
tlv16_data_t* tlv_find_tlv16   ( const uint8_t* message, uint32_t message_length, uint16_t type );
uint8_t*      tlv_write_header ( uint8_t* buffer, uint16_t type, uint16_t length );
tlv_result_t  tlv_read_value   ( uint16_t type, const uint8_t* message, uint16_t message_length, void* value, uint16_t value_size, tlv_data_type_t data_type );
uint8_t*      tlv_write_value  ( uint8_t* buffer, uint16_t type, uint16_t length, const void* data, tlv_data_type_t data_type );

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* INCLUDED_TLV_H */
