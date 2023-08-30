/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
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
#ifndef _C_CC_RING_H_INCLUDED_
#define _C_CC_RING_H_INCLUDED_

#include "atomic.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**/
typedef struct _cc_ring {
    volatile uint32_t r;
    volatile uint32_t w;
    _cc_spinlock_t lock;
    int32_t size;
    pvoid_t* data;

} _cc_ring_t;

/**/
_CC_API(bool_t) _cc_ring_alloc(_cc_ring_t*, int32_t initsize);
/**/
_CC_API(bool_t) _cc_ring_free(_cc_ring_t*);
/**/
_CC_API(_cc_ring_t*) _cc_create_ring(int32_t);
/**/
_CC_API(void) _cc_destroy_ring(_cc_ring_t**);
/**/
_CC_API(bool_t) _cc_ring_push(_cc_ring_t*, pvoid_t data);
/**/
_CC_API(pvoid_t) _cc_ring_pop(_cc_ring_t*);
/**/
_CC_API(void) _cc_ring_cleanup(_cc_ring_t*);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_RING_H_INCLUDED_*/
