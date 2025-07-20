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
#ifndef _C_CC_MATH_H_INCLUDED_
#define _C_CC_MATH_H_INCLUDED_

#include "endian.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
/*
    _cc_round(2.4,  10)  = 2
    _cc_round(2.5,  10)  = 3

    _cc_round(2.12, 100) = 2.1
    _cc_round(2.19, 100) = 2.2
*/
_CC_FORCE_INLINE_ float32_t _cc_roundf(float32_t num, float32_t num_digits) {
    return ((num * num_digits) + 5.0f) / num_digits;
}

/**/
_CC_FORCE_INLINE_ float64_t _cc_round(float64_t num, float64_t num_digits) {
    return ((num * num_digits) + 5.0) / num_digits;
}

#define _cc_byte_align(x, c) ((x + (c - 1)) & ~((c - 1)))

/* The min and max functions missing in C.  As usual, be careful not to */
/* write things like MIN( a++, b++ ) to avoid side effects.             */

#ifndef _min
#define _min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef _min3
#define _min3(a, b, c) ((a) < (b) ? _min(a, c) : _min(b, c))
#endif
/**/
#ifndef _max
#define _max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef _max3
#define _max3(a, b, c) ((a) < (b) ? _max(b, c) : _max(a, c))
#endif
/**/
#ifndef _abs
#define _abs(a) ((a) < 0 ? -(a) : (a))
#endif


_CC_FORCE_INLINE_ int8_t _cc_min_int8(int8_t a, int8_t b) {
    return _min(a, b);
}

_CC_FORCE_INLINE_ int16_t _cc_min_int16(int16_t a, int16_t b) {
    return _min(a, b);
}

_CC_FORCE_INLINE_ int32_t _cc_min_int32(int32_t a, int32_t b) {
    return _min(a, b);
}

_CC_FORCE_INLINE_ int64_t _cc_min_int64(int64_t a, int64_t b) {
    return _min(a, b);
}

_CC_FORCE_INLINE_ float32_t _cc_min_float32(float32_t a, float32_t b) {
    return _min(a, b);
}

_CC_FORCE_INLINE_ float64_t _cc_min_float64(float64_t a, float64_t b) {
    return _min(a, b);
}

_CC_FORCE_INLINE_ int8_t _cc_max_int8(int8_t a, int8_t b) {
    return _max(a, b);
}

_CC_FORCE_INLINE_ int16_t _cc_max_int16(int16_t a, int16_t b) {
    return _max(a, b);
}

_CC_FORCE_INLINE_ int32_t _cc_max_int32(int32_t a, int32_t b) {
    return _max(a, b);
}

_CC_FORCE_INLINE_ int64_t _cc_max_int64(int64_t a, int64_t b) {
    return _max(a, b);
}

_CC_FORCE_INLINE_ float32_t _cc_max_float32(float32_t a, float32_t b) {
    return _max(a, b);
}

_CC_FORCE_INLINE_ float64_t _cc_max_float64(float64_t a, float64_t b) {
    return _max(a, b);
}
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_MATH_H_INCLUDED_ */
