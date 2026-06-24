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
    _cc_atomic_lock_t lock;
    int32_t size;
    intptr_t* data;

} _cc_ring_t;

/**/
_CC_API_PUBLIC(bool_t) _cc_alloc_ring(_cc_ring_t*, int32_t);
/**/
_CC_API_PUBLIC(bool_t) _cc_free_ring(_cc_ring_t*);
/**/
_CC_API_PUBLIC(bool_t) _cc_ring_push(_cc_ring_t*, intptr_t);
/**/
_CC_API_PUBLIC(intptr_t) _cc_ring_pop(_cc_ring_t*);
/**/
_CC_API_PUBLIC(void) _cc_ring_cleanup(_cc_ring_t*);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_RING_H_INCLUDED_*/
