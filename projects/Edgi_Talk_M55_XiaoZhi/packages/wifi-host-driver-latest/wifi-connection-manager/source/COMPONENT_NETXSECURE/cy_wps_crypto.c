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

/** @file cy_wps_crypto.c
* @brief AES and SHA hash related functions required for WPS. Implementation here is specific to NetXSecure library.
*/

#include <string.h>
#include <stdarg.h>

#include "cy_wps_crypto.h"
/******************************************************
 *                      Macros
 ******************************************************/
#define NETXSECURE_AES_BLOCK_SZ  16

/******************************************************
 *               Variable Definitions
 ******************************************************/
extern NX_CRYPTO_METHOD crypto_method_aes_cbc_256;
/******************************************************
 *               Function Definitions
 ******************************************************/
cy_rslt_t cy_sha256( const unsigned char *input, 
                     size_t ilen, 
                     unsigned char output[32], 
                     uint32_t hash_algo_type )
{
    NX_CRYPTO_SHA256 ctx;
    UCHAR *input_ptr = (UCHAR *)input;

    memset( &ctx, 0, sizeof( NX_CRYPTO_SHA256 ) );

    _nx_crypto_sha256_initialize(&ctx, hash_algo_type);

    _nx_crypto_sha256_update(&ctx, input_ptr, ilen);

    _nx_crypto_sha256_digest_calculate(&ctx, output, hash_algo_type);

    return CY_RSLT_SUCCESS;
}

cy_rslt_t cy_aes_cbc_encrypt( const unsigned char *input, 
                              uint32_t input_length, 
                              unsigned char *output, 
                              const unsigned char *key, 
                              unsigned int keybits, 
                              const unsigned char iv[16] )
{
    void *handler = NULL;
    NX_CRYPTO_AES aes_ctx;
    ULONG meta_data_size = sizeof(aes_ctx);
    int32_t remaining = input_length & 0x0f;
    uint32_t rounded_length = input_length & 0xfffffff0;
    unsigned char iv_copy[16];
    uint8_t tmp[NETXSECURE_AES_BLOCK_SZ];
    uint8_t j;
    uint32_t result;

    if ( input_length == 0 )
    {
        return -1;
    }
    
    memcpy( iv_copy, iv, sizeof(iv_copy) );
    
    result = crypto_method_aes_cbc_256.nx_crypto_init(&crypto_method_aes_cbc_256,
                                                      (UCHAR *)key,
                                                      keybits,
                                                      &handler,
                                                      &aes_ctx,
                                                      meta_data_size);
    if(result != NX_CRYPTO_SUCCESS)
    {
        return -1;
    }

    result = crypto_method_aes_cbc_256.nx_crypto_operation(NX_CRYPTO_ENCRYPT,
                                                           handler,
                                                           &crypto_method_aes_cbc_256,
                                                           (UCHAR *)key,
                                                           keybits,
                                                           (UCHAR *)input,
                                                           rounded_length,
                                                           iv_copy,
                                                           output,
                                                           0,
                                                           &aes_ctx,
                                                           meta_data_size,
                                                           NX_NULL, NX_NULL);
    if(result != NX_CRYPTO_SUCCESS)
    {
        return -1;
    }

    for(uint8_t x=0 ; x<16 ; x++)
    {
        iv_copy[x] = output[rounded_length-16 + x];
    }

    for (j = 0; j < remaining; j++)
    {
        tmp[j] = (uint8_t)(input[(uint8_t)rounded_length + j] ^ iv_copy[j]);
    }

    for (j = remaining; j < NETXSECURE_AES_BLOCK_SZ; j++)
    {
        tmp[j] = (uint8_t)(NETXSECURE_AES_BLOCK_SZ - remaining) ^  iv_copy[j];
    }

    _nx_crypto_aes_encrypt(&aes_ctx, tmp, output+rounded_length, NETXSECURE_AES_BLOCK_SZ);

    return CY_RSLT_SUCCESS;
}

cy_rslt_t cy_aes_cbc_decrypt( const unsigned char *input, 
                              uint32_t input_length, 
                              unsigned char *output, 
                              uint32_t* output_length, 
                              const unsigned char *key, 
                              unsigned int keybits, 
                              const unsigned char iv[16] )
{
    void *handler = NULL;
    NX_CRYPTO_AES aes_ctx;
    ULONG meta_data_size = sizeof(aes_ctx);
    int32_t remaining = input_length & 0x0f;
    uint32_t rounded_length = input_length & 0xfffffff0;
    unsigned char iv_copy[16];
    uint32_t result;

    if ( input_length == 0 )
    {
        return -1;
    }

    if ( remaining != 0 )
    {
        return -1;
    }
    
    memcpy( iv_copy, iv, sizeof(iv_copy) );
    
    result = crypto_method_aes_cbc_256.nx_crypto_init(&crypto_method_aes_cbc_256,
                                                      (UCHAR *)key,
                                                      keybits,
                                                      &handler,
                                                      &aes_ctx,
                                                      meta_data_size);
    if(result != NX_CRYPTO_SUCCESS)
    {
        return -1;
    }

    result = crypto_method_aes_cbc_256.nx_crypto_operation(NX_CRYPTO_DECRYPT,
                                                           handler,
                                                           &crypto_method_aes_cbc_256,
                                                           (UCHAR *)key,
                                                           keybits,
                                                           (UCHAR*)input,
                                                           rounded_length,
                                                           iv_copy,
                                                           output,
                                                           rounded_length,
                                                           &aes_ctx,
                                                           meta_data_size,
                                                           NX_NULL, NX_NULL);

    if(result != NX_CRYPTO_SUCCESS)
    {
        return -1;
    }

    *output_length = (uint32_t)(input_length - output[input_length - 1]);

    return CY_RSLT_SUCCESS;
}

void cy_sha2_hmac_starts( cy_sha2_hmac_context *ctx, 
                          const unsigned char *key, 
                          uint32_t keylen,
                          int32_t hash_algo_type )
{
    uint32_t i;
    unsigned char sum[32];

    if (keylen > 64) {
        cy_sha256(key, keylen, sum, hash_algo_type);
        keylen = (hash_algo_type) ? 28 : 32;
        key = sum;
    }

    memset(ctx->ipad, 0x36, 64);
    memset(ctx->opad, 0x5C, 64);

    for (i = 0; i < keylen; i++) {
        ctx->ipad[i] = (unsigned char)(ctx->ipad[i] ^ key[i]);
        ctx->opad[i] = (unsigned char)(ctx->opad[i] ^ key[i]);
    }

    memset( &(ctx->ctx), 0, sizeof( NX_CRYPTO_SHA256 ) );
    _nx_crypto_sha256_initialize(&(ctx->ctx), hash_algo_type);
    _nx_crypto_sha256_update(&(ctx->ctx), ctx->ipad, 64);

    memset(sum, 0, sizeof(sum));
}

/*
 * SHA-256 HMAC process buffer
 */
void cy_sha2_hmac_update( cy_sha2_hmac_context *ctx, 
                          const unsigned char *input, 
                          uint32_t ilen )
{
    _nx_crypto_sha256_update(&(ctx->ctx), (UCHAR *)input, ilen);
}

/*
 * SHA-256 HMAC final digest
 */
void cy_sha2_hmac_finish( cy_sha2_hmac_context * ctx, 
                          unsigned char output[32] )
{
    int32_t       hash_algo_type;
    uint32_t      hlen;
    unsigned char tmpbuf[32];

    /*
     * Initializing hash_algo_type to NX_CRYPTO_HASH_SHA256 in case of netxsecure to pick 256 bit values
     */
    hash_algo_type = NX_CRYPTO_HASH_SHA256;

    hlen = 32;

    _nx_crypto_sha256_digest_calculate(&(ctx->ctx), tmpbuf, hash_algo_type);
    _nx_crypto_sha256_initialize(&ctx->ctx, hash_algo_type);
    _nx_crypto_sha256_update(&ctx->ctx, ctx->opad, 64);
    _nx_crypto_sha256_update(&ctx->ctx, tmpbuf, hlen);
    _nx_crypto_sha256_digest_calculate(&ctx->ctx, output, hash_algo_type);

    memset(tmpbuf, 0, sizeof(tmpbuf));
}

/*
 * Output = HMAC-SHA-256( hmac key, input buffer )
 */
void cy_sha2_hmac( const unsigned char *key, 
                   uint32_t keylen,
                   const unsigned char *input, 
                   uint32_t ilen,
                   unsigned char output[32], 
                   int32_t hash_algo_type )
{
    cy_sha2_hmac_context ctx;

    cy_sha2_hmac_starts(&ctx, key, keylen, hash_algo_type);
    cy_sha2_hmac_update(&ctx, input, ilen);
    cy_sha2_hmac_finish(&ctx, output);

    memset(&ctx, 0, sizeof(cy_sha2_hmac_context));
}
