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
#ifndef _C_CC_MD2_H_INCLUDED_
#define _C_CC_MD2_H_INCLUDED_

#include "core.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_MD2_DIGEST_LENGTH_ 16
/**
 * @brief          MD2 context structure
 */
typedef struct _cc_md2 {
    byte_t cksum[16];  /*!< checksum of the data block */
    byte_t state[48];  /*!< intermediate digest state  */
    byte_t buffer[16]; /*!< data block being processed */
    size_t left;       /*!< amount of data in buffer   */
} _cc_md2_t;

/**
 * @brief          MD2 context setup
 *
 * @param ctx      context to be initialized
 */
_CC_API_PUBLIC(void) _cc_md2_init(_cc_md2_t* ctx);
/**
 * @brief          MD2 process buffer
 *
 * @param ctx      MD2 context
 * @param input    buffer holding the  data
 * @param ilen     length of the input data
 */
_CC_API_PUBLIC(void) _cc_md2_update(_cc_md2_t* ctx, const byte_t* input, size_t ilen);
/**
 * @brief          MD2 final digest
 *
 * @param ctx      MD2 context
 * @param output   MD2 checksum result
 */
_CC_API_PUBLIC(void) _cc_md2_final(_cc_md2_t* ctx, byte_t* output);

/**
 * @brief          Digests a file.
 *
 * @param fp       FILE handle
 * @param output   MD2 checksum result
 */
_CC_API_PUBLIC(bool_t) _cc_md2_fp(FILE* fp, tchar_t* output);
/**
 * @brief         Digests a file.
 *
 * @param filename       FILE handle
 * @param output   MD2 checksum result
 */
_CC_API_PUBLIC(bool_t) _cc_md2file(const tchar_t* filename, tchar_t* output);
/**
 * @brief          Output = MD2( input buffer )
 *
 * @param input    buffer holding the  data
 * @param ilen     length of the input data
 * @param output   MD2 checksum result
 */
_CC_API_PUBLIC(void) _cc_md2(const byte_t* input, size_t ilen, tchar_t* output);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_MD2_H_INCLUDED_*/
