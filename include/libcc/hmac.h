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
#ifndef _C_CC_HMAC_H_INCLUDED_
#define _C_CC_HMAC_H_INCLUDED_

#include "md5.h"
#include "sha.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif


enum {
    _CC_HMAC_MD5_,
    _CC_HMAC_SHA1_,
    _CC_HMAC_SHA224_,
    _CC_HMAC_SHA256_,
    _CC_HMAC_SHA384_,
    _CC_HMAC_SHA512_,
};

typedef struct _cc_hmac _cc_hmac_t;
_cc_hmac_t* _cc_hmac_alloc(byte_t type);

_CC_API_PUBLIC(void) _cc_hmac_init(_cc_hmac_t *ctx, const byte_t *key, size_t length);
_CC_API_PUBLIC(void) _cc_hmac_update(_cc_hmac_t *ctx, const byte_t *input, size_t length);
_CC_API_PUBLIC(int) _cc_hmac_final(_cc_hmac_t *c, byte_t *output, int output_length);

_CC_API_PUBLIC(int) _cc_hmac(byte_t type, const byte_t *input, size_t ilen, const byte_t *key, size_t key_length, tchar_t *output);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_HMAC_H_INCLUDED_*/
