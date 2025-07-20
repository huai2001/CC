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
#ifndef _C_CC_ARRAY_H_INCLUDED_
#define _C_CC_ARRAY_H_INCLUDED_

#include "list.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**/
#define _cc_array_for_each(__TYPE, __VAL, __KEY, __FIRST, __OP) \
    do {                                                        \
        __TYPE* __VAL;                                          \
        size_t __COUNT = _cc_array_length(__FIRST), __KEY;    \
        for (__KEY = 0; __KEY < __COUNT; __KEY++) {             \
            __VAL = (__TYPE*)((__FIRST)->data[__KEY]);          \
            __OP                                                \
        }                                                       \
    } while (0)

/*
 */
typedef struct _cc_array {
    size_t limit;
    size_t length;
    pvoid_t* data;
} _cc_array_t;

/**/
#define _cc_array_get _cc_array_find

/**/
_CC_API_PUBLIC(_cc_array_t*) _cc_create_array(size_t);
/**/
_CC_API_PUBLIC(void) _cc_destroy_array(_cc_array_t**);
/**/
_CC_API_PUBLIC(bool_t) _cc_array_alloc(_cc_array_t*, size_t);
/**/
_CC_API_PUBLIC(bool_t) _cc_array_realloc(_cc_array_t*, size_t);
/**/
_CC_API_PUBLIC(bool_t) _cc_array_free(_cc_array_t* thiz);
/**/
_CC_API_PUBLIC(bool_t) _cc_array_cleanup(_cc_array_t*);
/**/
_CC_API_PUBLIC(pvoid_t) _cc_array_find(const _cc_array_t*, const size_t);
/**/
_CC_API_PUBLIC(bool_t) _cc_array_set(_cc_array_t*, const size_t, pvoid_t);
/**/
_CC_API_PUBLIC(bool_t) _cc_array_append(_cc_array_t*, const _cc_array_t*);
/**/
_CC_API_PUBLIC(bool_t) _cc_array_insert(_cc_array_t*, const size_t, pvoid_t);
/**/
_CC_API_PUBLIC(pvoid_t) _cc_array_remove(_cc_array_t*, const size_t);
/**/
_CC_API_PUBLIC(size_t) _cc_array_push(_cc_array_t*, pvoid_t);
/**/
_CC_API_PUBLIC(pvoid_t) _cc_array_pop(_cc_array_t*);
/**/
_CC_API_PUBLIC(pvoid_t) _cc_array_begin(const _cc_array_t*);
/**/
_CC_API_PUBLIC(pvoid_t) _cc_array_end(const _cc_array_t*);
/**/
_CC_API_PUBLIC(size_t) _cc_array_length(const _cc_array_t*);
/** @} */
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_ARRAY_H_INCLUDED_*/
