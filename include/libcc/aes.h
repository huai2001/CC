/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
 * All rights reserved.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/
#ifndef _C_CC_AES_H_INCLUDED_
#define _C_CC_AES_H_INCLUDED_

#include "generic.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_CIPHER_MODE_CFB_ 1

#define _CC_AES_ENCRYPT_ 1
#define _CC_AES_DECRYPT_ 0

#define _CC_AES_KEY_SIZE_ 16
#define _CC_AES_BLOCK_SIZE_ 16
#define _CC_AES_BUFFER_LEN(l) ((l + (_CC_AES_BLOCK_SIZE_ - 1)) / _CC_AES_BLOCK_SIZE_) * _CC_AES_BLOCK_SIZE_

/**< Invalid key length. */
#define _CC_ERR_AES_INVALID_KEY_LENGTH_ -0x0020
/**< Invalid data input length. */
#define _CC_ERR_AES_INVALID_INPUT_LENGTH_ -0x0022
/**
 * @brief          AES context structure
 *
 * @note           buf is able to hold 32 extra bytes, which can be used:
 *                 - for alignment purposes if VIA padlock is used, and/or
 *                 - to simplify key expansion in the 256-bit case by
 *                 generating an extra round key
 */
typedef struct _cc_aes {
    int nr;           /*!<  number of rounds  */
    uint32_t *rk;     /*!<  AES round keys    */
    uint32_t buf[68]; /*!<  unaligned data    */
} _cc_aes_t;

/**
 * @brief          Initialize AES context
 *
 * @param ctx      AES context to be initialized
 */
_CC_API_PUBLIC(void) _cc_aes_init(_cc_aes_t *ctx);

/**
 * @brief          AES key schedule (encryption)
 *
 * @param ctx      AES context to be initialized
 * @param key      encryption key
 * @param keybits  must be 128, 192 or 256
 *
 * @return         0 if successful, or _CC_ERR_AES_INVALID_KEY_LENGTH_
 */
_CC_API_PUBLIC(int)
_cc_aes_setkey_enc(_cc_aes_t *ctx, const byte_t *key, unsigned int keybits);

/**
 * @brief          AES key schedule (decryption)
 *
 * @param ctx      AES context to be initialized
 * @param key      decryption key
 * @param keybits  must be 128, 192 or 256
 *
 * @return         0 if successful, or _CC_ERR_AES_INVALID_KEY_LENGTH_
 */
_CC_API_PUBLIC(int)
_cc_aes_setkey_dec(_cc_aes_t *ctx, const byte_t *key, unsigned int keybits);

/**
 * @brief          AES key schedule ( encryption/decryption )
 *
 * @param ctx      AES context
 * @param mode     _CC_AES_ENCRYPT_ or _CC_AES_DECRYPT_
 * @param key      encryption/decryption key
 * @param keybits  must be 128, 192 or 256
 */
_CC_API_PUBLIC(int)
_cc_aes_setkey(_cc_aes_t *ctx, int mode, const byte_t *key, uint32_t keybits);

#if defined(_CC_CIPHER_MODE_CBC_)
/**
 * @brief          AES-CBC buffer encryption/decryption
 *                 Length should be a multiple of the block
 *                 size (16 bytes)
 *
 * @note           Upon exit, the content of the IV is updated so that you can
 *                 call the function same function again on the following
 *                 block(s) of data and get the same result as if it was
 *                 encrypted in one call. This allows a "streaming" usage.
 *                 If on the other hand you need to retain the contents of the
 *                 IV, you should either save it manually or use the cipher
 *                 module instead.
 *
 * @param ctx      AES context
 * @param mode     _CC_AES_ENCRYPT_ or _CC_AES_DECRYPT_
 * @param length   length of the input data
 * @param iv       initialization vector (updated after use)
 * @param input    buffer holding the input data
 * @param output   buffer holding the output data
 *
 * @return         0 if successful, or _CC_ERR_AES_INVALID_INPUT_LENGTH_
 */
_CC_API_PUBLIC(int)
_cc_aes_crypt_cbc(_cc_aes_t *ctx, int mode, size_t length, byte_t iv[16], const byte_t *input, byte_t *output);
#endif /* _CC_CIPHER_MODE_CBC_ */

#if defined(_CC_CIPHER_MODE_CFB_)
/**
 * @brief          AES-CFB128 buffer encryption/decryption.
 *
 * Note: Due to the nature of CFB you should use the same key schedule for
 * both encryption and decryption. So a context initialized with
 * _cc_aes_setkey_enc() for both _CC_AES_ENCRYPT_ and _CC_AES_DECRYPT_.
 *
 * @note           Upon exit, the content of the IV is updated so that you can
 *                 call the function same function again on the following
 *                 block(s) of data and get the same result as if it was
 *                 encrypted in one call. This allows a "streaming" usage.
 *                 If on the other hand you need to retain the contents of the
 *                 IV, you should either save it manually or use the cipher
 *                 module instead.
 *
 * @param ctx      AES context
 * @param mode     _CC_AES_ENCRYPT_ or _CC_AES_DECRYPT_
 * @param input    buffer holding the input data
 * @param length   length of the input data
 * @param iv_off   offset in IV (updated after use)
 * @param iv       initialization vector (updated after use)
 * @param output   buffer holding the output data
 *
 * @return         0 if successful
 */
_CC_API_PUBLIC(int)
_cc_aes_crypt_cfb128(_cc_aes_t *ctx, int mode, const byte_t *input, size_t length, size_t *iv_off, byte_t iv[16],
                     byte_t *output);

/**
 * @brief          AES-CFB8 buffer encryption/decryption.
 *
 * Note: Due to the nature of CFB you should use the same key schedule for
 * both encryption and decryption. So a context initialized with
 * _cc_aes_setkey_enc() for both _CC_AES_ENCRYPT_ and _CC_AES_DECRYPT_.
 *
 * @note           Upon exit, the content of the IV is updated so that you can
 *                 call the function same function again on the following
 *                 block(s) of data and get the same result as if it was
 *                 encrypted in one call. This allows a "streaming" usage.
 *                 If on the other hand you need to retain the contents of the
 *                 IV, you should either save it manually or use the cipher
 *                 module instead.
 *
 * @param ctx      AES context
 * @param mode     _CC_AES_ENCRYPT_ or _CC_AES_DECRYPT_
 * @param length   length of the input data
 * @param iv       initialization vector (updated after use)
 * @param input    buffer holding the input data
 * @param output   buffer holding the output data
 *
 * @return         0 if successful
 */
_CC_API_PUBLIC(int)
_cc_aes_crypt_cfb8(_cc_aes_t *ctx, int mode, const byte_t *input, size_t length, byte_t iv[16], byte_t *output);
#endif /*_CC_CIPHER_MODE_CFB_ */

#if defined(_CC_CIPHER_MODE_CTR_)
/**
 * @brief AES-CTR buffer encryption/decryption
 *
 * Warning: You have to keep the maximum use of your counter in mind!
 *
 * Note: Due to the nature of CTR you should use the same key schedule for
 * both encryption and decryption. So a context initialized with
 * _cc_aes_setkey_enc() for both _CC_AES_ENCRYPT_ and _CC_AES_DECRYPT_.
 *
 * @param ctx           AES context
 * @param input         The input data stream
 * @param length        The length of the data
 * @param nc_off        The offset in the current stream_block (for resuming
 *                      within current cipher stream). The offset pointer to
 *                      should be 0 at the start of a stream.
 * @param nonce_counter The 128-bit nonce and counter.
 * @param stream_block  The saved stream-block for resuming. Is overwritten
 *                      by the function.
 * @param output        The output data stream
 *
 * @return         0 if successful
 */
_CC_API_PUBLIC(int)
_cc_aes_crypt_ctr(_cc_aes_t *ctx, const byte_t *input, size_t length, size_t *nc_off, byte_t nonce_counter[16],
                  byte_t stream_block[16], byte_t *output);
#endif /* _CC_CIPHER_MODE_CTR_ */

/**
 * @brief           Deprecated internal AES block encryption function
 *                  without return value.
 *
 * @param ctx       AES context
 * @param input     Plaintext block
 * @param output    Output (ciphertext) block
 */
_CC_API_PUBLIC(void)
_cc_aes_encrypt(_cc_aes_t *ctx, const byte_t input[16], byte_t output[16]);

/**
 * @brief           Deprecated internal AES block decryption function
 *                  without return value.
 *
 * @param ctx       AES context
 * @param input     Ciphertext block
 * @param output    Output (plaintext) block
 */
_CC_API_PUBLIC(void)
_cc_aes_decrypt(_cc_aes_t *ctx, const byte_t input[16], byte_t output[16]);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_AES_H_INCLUDED_*/
