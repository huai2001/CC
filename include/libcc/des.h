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
#ifndef _C_CC_DES_H_INCLUDED_
#define _C_CC_DES_H_INCLUDED_

#include "generic.h"

#define _CC_DES_ENCRYPT_ 1
#define _CC_DES_DECRYPT_ 0

/**< The data input has an invalid length. */
#define _CC_ERR_DES_INVALID_INPUT_LENGTH_ -0x0032

#define _CC_DES_KEY_SIZE_ 8

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief          DES context structure
 */
typedef struct _cc_des {
    uint32_t sk[32]; /*!<  DES subkeys       */
} _cc_des_t;

/**
 * @brief          Triple-DES context structure
 */
typedef struct _cc_des3 {
    uint32_t sk[96]; /*!<  3DES subkeys      */
} _cc_des3_t;

/**
 * @brief          Initialize DES context
 *
 * @param ctx      DES context to be initialized
 */
_CC_API_PUBLIC(void) _cc_des_init(_cc_des_t *ctx);

/**
 * @brief          Initialize Triple-DES context
 *
 * @param ctx      DES3 context to be initialized
 */
_CC_API_PUBLIC(void) _cc_des3_init(_cc_des3_t *ctx);

/**
 * @brief          Set key parity on the given key to odd.
 *
 *                 DES keys are 56 bits long, but each byte is padded with
 *                 a parity bit to allow verification.
 *
 * @param key      8-byte secret key
 */
_CC_API_PUBLIC(void) _cc_des_key_set_parity(byte_t key[_CC_DES_KEY_SIZE_]);

/**
 * @brief          Check that key parity on the given key is odd.
 *
 *                 DES keys are 56 bits long, but each byte is padded with
 *                 a parity bit to allow verification.
 *
 * @param key      8-byte secret key
 *
 * @return         true is parity was ok, false if parity was not correct.
 */
_CC_API_PUBLIC(bool_t)
_cc_des_key_check_key_parity(const byte_t key[_CC_DES_KEY_SIZE_]);

/**
 * @brief          Check that key is not a weak or semi-weak DES key
 *
 * @param key      8-byte secret key
 *
 * @return         false if no weak key was found, true if a weak key was
 * identified.
 */
_CC_API_PUBLIC(bool_t) _cc_des_key_check_weak(const byte_t key[_CC_DES_KEY_SIZE_]);

/**
 * @brief          DES key schedule (56-bit, encryption)
 *
 * @param ctx      DES context to be initialized
 * @param key      8-byte secret key
 *
 */
_CC_API_PUBLIC(void)
_cc_des_setkey_enc(_cc_des_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_]);

/**
 * @brief          DES key schedule (56-bit, decryption)
 *
 * @param ctx      DES context to be initialized
 * @param key      8-byte secret key
 *
 */
_CC_API_PUBLIC(void)
_cc_des_setkey_dec(_cc_des_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_]);

/**
 * @brief          Triple-DES key schedule (112-bit, encryption)
 *
 * @param ctx      3DES context to be initialized
 * @param key      16-byte secret key
 *
 */
_CC_API_PUBLIC(void)
_cc_des3_set2key_enc(_cc_des3_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_ * 2]);

/**
 * @brief          Triple-DES key schedule (112-bit, decryption)
 *
 * @param ctx      3DES context to be initialized
 * @param key      16-byte secret key
 *
 */
_CC_API_PUBLIC(void)
_cc_des3_set2key_dec(_cc_des3_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_ * 2]);

/**
 * @brief          Triple-DES key schedule (168-bit, encryption)
 *
 * @param ctx      3DES context to be initialized
 * @param key      24-byte secret key
 *
 */
_CC_API_PUBLIC(void)
_cc_des3_set3key_enc(_cc_des3_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_ * 3]);

/**
 * @brief          Triple-DES key schedule (168-bit, decryption)
 *
 * @param ctx      3DES context to be initialized
 * @param key      24-byte secret key
 *
 */
_CC_API_PUBLIC(void)
_cc_des3_set3key_dec(_cc_des3_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_ * 3]);

/**
 * @brief          DES-ECB block encryption/decryption
 *
 * @param ctx      DES context
 * @param input    64-bit input block
 * @param output   64-bit output block
 *
 */
_CC_API_PUBLIC(void)
_cc_des_crypt_ecb(_cc_des_t *ctx, const byte_t input[8], byte_t output[8]);

#if defined(_CC_CIPHER_MODE_CBC_)
/**
 * @brief          DES-CBC buffer encryption/decryption
 *
 * @note           Upon exit, the content of the IV is updated so that you can
 *                 call the function same function again on the following
 *                 block(s) of data and get the same result as if it was
 *                 encrypted in one call. This allows a "streaming" usage.
 *                 If on the other hand you need to retain the contents of the
 *                 IV, you should either save it manually or use the cipher
 *                 module instead.
 *
 * @param ctx      DES context
 * @param mode     _CC_DES_ENCRYPT_ or _CC_DES_DECRYPT_
 * @param length   length of the input data
 * @param iv       initialization vector (updated after use)
 * @param input    buffer holding the input data
 * @param output   buffer holding the output data
 */
_CC_API_PUBLIC(int)
_cc_des_crypt_cbc(_cc_des_t *ctx, int mode, size_t length, byte_t iv[8], const byte_t *input, byte_t *output);
#endif /* _CC_CIPHER_MODE_CBC_ */

/**
 * @brief          3DES-ECB block encryption/decryption
 *
 * @param ctx      3DES context
 * @param input    64-bit input block
 * @param output   64-bit output block
 *
 */
_CC_API_PUBLIC(void)
_cc_des3_crypt_ecb(_cc_des3_t *ctx, const byte_t input[8], byte_t output[8]);

#if defined(_CC_CIPHER_MODE_CBC_)
/**
 * @brief          3DES-CBC buffer encryption/decryption
 *
 * @note           Upon exit, the content of the IV is updated so that you can
 *                 call the function same function again on the following
 *                 block(s) of data and get the same result as if it was
 *                 encrypted in one call. This allows a "streaming" usage.
 *                 If on the other hand you need to retain the contents of the
 *                 IV, you should either save it manually or use the cipher
 *                 module instead.
 *
 * @param ctx      3DES context
 * @param mode     _CC_DES_ENCRYPT_ or _CC_DES_DECRYPT_
 * @param length   length of the input data
 * @param iv       initialization vector (updated after use)
 * @param input    buffer holding the input data
 * @param output   buffer holding the output data
 *
 * @return         0 if successful, or _CC_ERR_DES_INVALID_INPUT_LENGTH_
 */
_CC_API_PUBLIC(int)
_cc_des3_crypt_cbc(_cc_des3_t *ctx, int mode, size_t length, byte_t iv[8], const byte_t *input, byte_t *output);
#endif /* _CC_CIPHER_MODE_CBC_ */

/**
 * @brief          Internal function for key expansion.
 *                 (Only exposed to allow overriding it,
 *                 see _CC_DES_SETKEY_ALT)
 *
 * @param SK       Round keys
 * @param key      Base key
 */
_CC_API_PUBLIC(void)
_cc_des_setkey(uint32_t SK[32], const byte_t key[_CC_DES_KEY_SIZE_]);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_DES_H_INCLUDED_*/
