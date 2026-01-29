#ifndef _C_CC_ARRAY_H_INCLUDED_
#define _C_CC_ARRAY_H_INCLUDED_

#include "list.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**/
#define _cc_array_for_each(__TYPE, __VAL, __KEY, __FIRST, __OP)     \
    do {                                                            \
        __TYPE* __VAL;                                              \
        size_t __COUNT = _cc_array_length(__FIRST), __KEY;          \
        for (__KEY = 0; __KEY < __COUNT; __KEY++) {                 \
            __VAL = ((__TYPE*)*((uintptr_t*)(__FIRST) + __KEY));    \
            __OP                                                    \
        }                                                           \
    } while (0)

/**/
typedef uintptr_t _cc_array_t;

#define _cc_array_value(CTX,INDEX)  (*((uintptr_t*)(CTX) + (INDEX)));
#define _cc_array_at(CTX,INDEX)  (*((uintptr_t*)(CTX) + (INDEX)));

/**/
_CC_API_PUBLIC(_cc_array_t) _cc_alloc_array(size_t);
/**/
_CC_API_PUBLIC(_cc_array_t) _cc_realloc_array(_cc_array_t, size_t);
/**/
_CC_API_PUBLIC(void) _cc_free_array(_cc_array_t);
/**/
_CC_API_PUBLIC(bool_t) _cc_array_clear(_cc_array_t);
/**/
_CC_API_PUBLIC(uintptr_t) _cc_array_get(const _cc_array_t, const size_t);
/**/
_CC_API_PUBLIC(bool_t) _cc_array_set(_cc_array_t, const size_t, uintptr_t);
/**/
_CC_API_PUBLIC(void) _cc_array_append(_cc_array_t*, const _cc_array_t);
/**/
_CC_API_PUBLIC(uintptr_t) _cc_array_remove(_cc_array_t, const size_t);
/**/
_CC_API_PUBLIC(size_t) _cc_array_push(_cc_array_t*, uintptr_t);
/**/
_CC_API_PUBLIC(uintptr_t) _cc_array_pop(_cc_array_t);
/**/
_CC_API_PUBLIC(uintptr_t*) _cc_array_begin(const _cc_array_t);
/**/
_CC_API_PUBLIC(uintptr_t*) _cc_array_end(const _cc_array_t);
/**/
_CC_API_PUBLIC(size_t) _cc_array_length(const _cc_array_t);
/**/
_CC_API_PUBLIC(size_t) _cc_array_available(const _cc_array_t);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_ARRAY_H_INCLUDED_*/
