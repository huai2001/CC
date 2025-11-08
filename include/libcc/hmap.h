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
typedef struct _cc_hmap_cell _cc_hmap_cell_t;

/* A hmap has some maximum size and current size,
 * as well as the data to hold. */
typedef struct _cc_hmap {
    uint32_t limit;
    uint32_t count;
    _cc_hmap_cell_t *cells;
    _cc_list_iterator_t list;

    _cc_hmap_keyword_equals_func_t equals_func;
    _cc_hmap_keyword_hash_func_t hash_func;
} _cc_hmap_t;

/**/
_CC_FORCE_INLINE_ _cc_list_iterator_t *_cc_hmap_link(_cc_hmap_t *ctx) {
    _cc_assert(ctx != nullptr);
    return &ctx->list;
}

/**/
_CC_FORCE_INLINE_ uint32_t _cc_hmap_length(_cc_hmap_t *ctx) {
    _cc_assert(ctx != nullptr);
    if (ctx == nullptr) {
        return 0;
    }
    return ctx->count;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_alloc_hmap(_cc_hmap_t *ctx, uint32_t capacity, _cc_hmap_keyword_equals_func_t equals_func, _cc_hmap_keyword_hash_func_t hash_func);
/**/
_CC_API_PUBLIC(bool_t) _cc_free_hmap(_cc_hmap_t *ctx);
/**/
_CC_API_PUBLIC(uintptr_t) _cc_hmap_value(_cc_list_iterator_t*);
/**/
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
