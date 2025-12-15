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

/*******************************************************************************
 * @file
 * Header for the natural numbers library
 *******************************************************************************
 */

#ifndef INCLUDED_NN_H
#define INCLUDED_NN_H

#include <stdint.h>

#include "cy_wps_common.h"
#include "cy_wps_structures.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t len;
    uint32_t num[1];
} NN_t;

void     NN_Clr          ( NN_t* number );
uint32_t NN_Add          ( NN_t* result, const NN_t*x, const NN_t*y );
uint32_t NN_Sub          ( NN_t* result, const NN_t*x, const NN_t*y );
void     NN_Mul          ( NN_t* result, const NN_t*x, const NN_t*y );
void     NN_AddMod       ( NN_t* result, const NN_t*x, const NN_t*y, const NN_t*modulus );
void     NN_SubMod       ( NN_t* result, const NN_t*x, const NN_t*y, const NN_t*modulus );
void     NN_MulMod       ( NN_t* result, const NN_t*x, const NN_t*y, const NN_t*modulus );
void     NN_ExpMod       ( NN_t* result, NN_t*x, NN_t*modulus, NN_t*e, NN_t*w );
void     NN_MulModMont   ( NN_t* result, const NN_t*x, const NN_t*y, const NN_t*m, uint32_t t );
void     NN_ExpModMont   ( NN_t* result, NN_t*x, NN_t*m, NN_t*e, NN_t*w );
uint32_t NN_EmTick       ( const NN_t* mod );
void     NN_ErModEm      ( NN_t* result, const NN_t*m );
uint64_t NN_Mul32x32u64  ( uint32_t a, uint32_t b );
void     wps_NN_set      ( cy_wps_NN_t* m, const uint8_t* buffer);
void     wps_NN_get      ( const cy_wps_NN_t* m, uint8_t* buffer);

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* ifndef INCLUDED_NN_H */

