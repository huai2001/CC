/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#ifndef _C_CC_RAND_H_INCLUDED_
#define _C_CC_RAND_H_INCLUDED_

#include <stdlib.h>
#include "types.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cc_prd {
    float64_t C;
    int32_t NMax;
} _cc_prd_t;

/**/
_CC_API_PUBLIC(void) _cc_srand(uint64_t);
/**/
_CC_API_PUBLIC(int32_t) _cc_rand(int32_t);
/**/
_CC_API_PUBLIC(int32_t) _cc_rand_bits(void);
/**/
_CC_API_PUBLIC(float32_t) _cc_randf(void);
/**/
_CC_API_PUBLIC(int32_t) _cc_rand_bits_r(uint64_t *state);
/**/
_CC_API_PUBLIC(int32_t) _cc_rand_r(uint64_t *state, int32_t n);
/**/
_CC_API_PUBLIC(float32_t) _cc_randf_r(uint64_t *state);
/**/
_CC_API_PUBLIC(void) _cc_random_bytes(byte_t *buf, size_t nbytes);

/**/
_CC_API_PUBLIC(uint32_t) _cc_random32(uint32_t, uint32_t);
/**/
_CC_API_PUBLIC(uint64_t) _cc_random64(uint64_t, uint64_t);
/**/
_CC_API_PUBLIC(float32_t) _cc_randomf32(float32_t, float32_t);
/**/
_CC_API_PUBLIC(float64_t) _cc_randomf64(float64_t, float64_t);

#define _CC_RANDOM_MAX_ 2147483647L

/**/
_CC_API_PUBLIC(void) _cc_calculate_prd(_cc_prd_t* prd, float64_t p);
/**/
_CC_API_PUBLIC(int32_t) _cc_get_probability(_cc_prd_t* prd, int T);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_RAND_H_INCLUDED_*/
