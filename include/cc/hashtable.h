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
#ifndef _C_CC_HASH_TABLE_H_INCLUDED_
#define _C_CC_HASH_TABLE_H_INCLUDED_

#include "list.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**/
#define _cc_hashtable_for_each(__TYPE, __VAL, __FIRST, __OP)       \
    do {                                                           \
        _cc_list_iterator_t* __NEXT = _cc_list_iterator_first( \
            (_cc_list_iterator_t*)&(__FIRST)->list);             \
        __TYPE* __VAL = NULL;                                      \
        while (NULL != __NEXT) {                                   \
            __VAL = (__TYPE*)_cc_hashtable_value(__NEXT);          \
            __NEXT = __NEXT->next;                                 \
            __OP                                                   \
        }                                                          \
    } while (0)

/**/
typedef bool_t (*_cc_hashtable_keyword_equals_func_t)(const pvoid_t custom, const pvoid_t keyword);
/**/
typedef uint32_t (*_cc_hashtable_keyword_hash_func_t)(const pvoid_t keyword);
/**/
typedef struct _cc_hashtable_element _cc_hashtable_element_t;

/* A hashtable has some maximum size and current size,
 * as well as the data to hold. */
typedef struct _cc_hashtable {
    uint32_t slots;
    uint32_t count;
    _cc_hashtable_element_t *data;
    _cc_list_iterator_t list;

    _cc_hashtable_keyword_equals_func_t equals_func;
    _cc_hashtable_keyword_hash_func_t hash_func;
} _cc_hashtable_t;

/**/
_CC_FORCE_INLINE_ _cc_list_iterator_t *_cc_hashtable_link(_cc_hashtable_t *ctx) {
    _cc_assert(ctx != NULL);
    return &ctx->list;
}
/**
 *
 * @param ctx hashtable context
 *
 * @return Return the length of the hashtable
 */
_CC_FORCE_INLINE_ uint32_t _cc_hashtable_length(_cc_hashtable_t *ctx) {
    _cc_assert(ctx != NULL);
    if (ctx == NULL) {
        return 0;
    }

    return ctx->count;
}
/**
 * @brief Initialize a new empty hashtable.
 *
 * @param ctx hashtable context
 * @param initial_size buffer count(!= 0. Creates a new memory buffer)
 * @param equals_func keywork equals callback function
 * @param hash_func keywork hash code callback function
 *
 * @return a new empty hashtable
 */
_CC_API(bool_t)
_cc_hashtable_alloc(_cc_hashtable_t *ctx, uint32_t initial_size, _cc_hashtable_keyword_equals_func_t equals_func,
                    _cc_hashtable_keyword_hash_func_t hash_func);
/**
 * @brief Free a hashtable.
 *
 * @param ctx hashtable context
 */
_CC_API(bool_t) _cc_hashtable_free(_cc_hashtable_t *ctx);
/**
 * @brief Creates a new empty hashtable.
 *
 * @param initial_size buffer count(!= 0. Creates a new memory buffer)
 * @param equals_func keywork equals callback function
 * @param hash_func keywork hash code callback function
 *
 * @return a new empty hashtable
 */
_CC_API(_cc_hashtable_t *)
_cc_create_hashtable(uint32_t initial_size, _cc_hashtable_keyword_equals_func_t equals_func,
                     _cc_hashtable_keyword_hash_func_t hash_func);
/**
 * @brief destroy a hashtable.
 *
 * @param ctx hashtable context
 */
_CC_API(void) _cc_destroy_hashtable(_cc_hashtable_t **ctx);
/**/
_CC_API(pvoid_t) _cc_hashtable_value(pvoid_t);
/**
 * @brief Removes all items.
 */
_CC_API(bool_t) _cc_hashtable_cleanup(_cc_hashtable_t *);
/**/
_CC_API(bool_t)
_cc_hashtable_insert(_cc_hashtable_t *, const pvoid_t keyword, const pvoid_t custom);
/**/
_CC_API(pvoid_t) _cc_hashtable_find(_cc_hashtable_t *, const pvoid_t keyword);
/**/
_CC_API(pvoid_t) _cc_hashtable_remove(_cc_hashtable_t *, const pvoid_t keyword);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#endif /*_C_CC_HASH_TABLE_H_INCLUDED_*/
