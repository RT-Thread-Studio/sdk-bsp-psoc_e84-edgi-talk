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

/** @file cy_wps_crypto.h
* @brief Helper functions for WPS AES encryption/decryption and SHA hash. These are the specific to the NetXSecure security stack.
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stdio.h"
#include "nx_crypto_sha2.h"
#include "nx_crypto_aes.h"
#include "aes_alt.h"
#include "cy_result.h"
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

typedef union
{
    /* This anonymous structure members should be
     * used only within NetXSecure s/w crypto code
     * and not in h/w crypto code. */
    struct
    {
        NX_CRYPTO_SHA256 ctx;
        unsigned char ipad[64];     /*!< HMAC: inner padding        */
        unsigned char opad[64];     /*!< HMAC: outer padding        */
    };
} cy_sha2_hmac_context;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/


cy_rslt_t cy_sha256( const unsigned char *input, size_t ilen, unsigned char output[32], uint32_t hash_algo_type );
cy_rslt_t cy_aes_cbc_encrypt( const unsigned char *input, uint32_t input_length, unsigned char *output, const unsigned char *key, unsigned int keybits, const unsigned char iv[16] );
cy_rslt_t cy_aes_cbc_decrypt( const unsigned char *input, uint32_t input_length, unsigned char *output, uint32_t* output_length, const unsigned char *key, unsigned int keybits, const unsigned char iv[16] );
void      cy_sha2_hmac_starts(cy_sha2_hmac_context *ctx, const unsigned char *key, uint32_t keylen, int32_t hash_algo_type);
void      cy_sha2_hmac_update(cy_sha2_hmac_context *ctx, const unsigned char *input, uint32_t ilen);
void      cy_sha2_hmac_finish(cy_sha2_hmac_context * ctx, unsigned char output[32]);
void      cy_sha2_hmac(const unsigned char *key, uint32_t keylen, const unsigned char *input, uint32_t ilen, unsigned char output[32], int32_t hash_algo_type);

#ifdef __cplusplus
} /*extern "C" */
#endif
