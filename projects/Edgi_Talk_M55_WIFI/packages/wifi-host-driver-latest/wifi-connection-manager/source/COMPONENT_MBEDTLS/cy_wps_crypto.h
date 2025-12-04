/*
 * $ Copyright Cypress Semiconductor $
 */

/** @file cy_wps_crypto.h
* @brief Helper functions for WPS AES encryption/decryption and SHA hash. These are the specific to the Mbed TLS security stack.
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stdio.h"
#include "mbedtls/sha256.h"
#include "mbedtls/aes.h"
#include "whd_types.h"
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
     * used only within Mbed TLS s/w crypto code
     * and not in h/w crypto code. */
    struct
    {
        mbedtls_sha256_context ctx;

        unsigned char ipad[64];     /*!< HMAC: inner padding        */
        unsigned char opad[64];     /*!< HMAC: outer padding        */
    };
    /* This anonymous structure member should be
     * used only within h/w crypto code.*/
    struct
    {
        void *sha2_hmac_hw_ctx;
    };
} cy_sha2_hmac_context;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/


cy_rslt_t cy_sha256(const unsigned char *input, size_t ilen, unsigned char output[32], int is224);
cy_rslt_t cy_aes_cbc_encrypt(const unsigned char *input, uint32_t input_length, unsigned char *output, const unsigned char *key, unsigned int keybits, const unsigned char iv[16]);
cy_rslt_t cy_aes_cbc_decrypt(const unsigned char *input, uint32_t input_length, unsigned char *output, uint32_t* output_length, const unsigned char *key, unsigned int keybits, const unsigned char iv[16]);
void      cy_sha2_hmac_starts(cy_sha2_hmac_context *ctx, const unsigned char *key, uint32_t keylen, int32_t is224);
void      cy_sha2_hmac_update(cy_sha2_hmac_context *ctx, const unsigned char *input, uint32_t ilen);
void      cy_sha2_hmac_finish(cy_sha2_hmac_context * ctx, unsigned char output[32]);
void      cy_sha2_hmac(const unsigned char *key, uint32_t keylen, const unsigned char *input, uint32_t ilen, unsigned char output[32], int32_t is224);

#ifdef __cplusplus
} /*extern "C" */
#endif
