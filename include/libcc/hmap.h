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
#ifndef _C_CC_HASH_TABLE_H_INCLUDED_
#define _C_CC_HASH_TABLE_H_INCLUDED_

#include "list.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**/
#define _cc_hmap_for_each(__TYPE, __VAL, __FIRST, __OP)                 \
    do {                                                                \
        _cc_list_iterator_t* __NEXT = _cc_list_iterator_first(          \
            (_cc_list_iterator_t*)&(__FIRST)->list);                    \
        __TYPE* __VAL = nullptr;                                        \
        while (nullptr != __NEXT) {                                     \
            __VAL = (__TYPE*)_cc_hmap_value(__NEXT);                    \
            __NEXT = __NEXT->next;                                      \
            __OP                                                        \
        }                                                               \
    } while (0)

/**/
typedef bool_t (*_cc_hmap_keyword_equals_func_t)(const uintptr_t custom, const uintptr_t keyword);
/**/
typedef intptr_t (*_cc_hmap_keyword_hash_func_t)(const uintptr_t keyword);
/**/
typedef struct _cc_hmap_element _cc_hmap_element_t;

/* A hmap has some maximum size and current size,
 * as well as the data to hold. */
typedef struct _cc_hmap {
    uint32_t limit;
    uint32_t count;
    _cc_hmap_element_t *slots;
    _cc_list_iterator_t list;

    _cc_hmap_keyword_equals_func_t equals_func;
    _cc_hmap_keyword_hash_func_t hash_func;
} _cc_hmap_t;

/**/
_CC_FORCE_INLINE_ _cc_list_iterator_t *_cc_hmap_link(_cc_hmap_t *ctx) {
    _cc_assert(ctx != nullptr);
    return &ctx->list;
}
/**
 *
 * @param ctx hmap context
 *
 * @return Return the length of the hmap
 */
_CC_FORCE_INLINE_ uint32_t _cc_hmap_length(_cc_hmap_t *ctx) {
    _cc_assert(ctx != nullptr);
    if (ctx == nullptr) {
        return 0;
    }

    return ctx->count;
}
/**
 * @brief Initialize a new empty hmap.
 *
 * @param ctx hmap context
 * @param capacity buffer count(!= 0. Creates a new memory buffer)
 * @param equals_func keywork equals callback function
 * @param hash_func keywork hash code callback function
 *
 * @return a new empty hmap
 */
_CC_API_PUBLIC(bool_t)
_cc_alloc_hmap(_cc_hmap_t *ctx, uint32_t capacity,
    _cc_hmap_keyword_equals_func_t equals_func, _cc_hmap_keyword_hash_func_t hash_func);
/**
 * @brief Free a hmap.
 *
 * @param ctx hmap context
 */
_CC_API_PUBLIC(bool_t) _cc_free_hmap(_cc_hmap_t *ctx);
/**/
_CC_API_PUBLIC(uintptr_t) _cc_hmap_value(_cc_list_iterator_t*);
/**
 * @brief Removes all items.
 */
_CC_API_PUBLIC(bool_t) _cc_hmap_cleanup(_cc_hmap_t *);
/**/
_CC_API_PUBLIC(bool_t) _cc_hmap_push(_cc_hmap_t *, const uintptr_t keyword, const uintptr_t custom);
/**/
_CC_API_PUBLIC(uintptr_t) _cc_hmap_find(_cc_hmap_t *, const uintptr_t keyword);
/**/
_CC_API_PUBLIC(uintptr_t) _cc_hmap_pop(_cc_hmap_t *, const uintptr_t keyword);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#endif /*_C_CC_HASH_TABLE_H_INCLUDED_*/
