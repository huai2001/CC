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
#include <libcc/alloc.h>
#include <libcc/crc.h>
#include <libcc/hmap.h>
#include <libcc/math.h>
#include <libcc/string.h>

#define INITIAL_SIZE (32)
#define MAX_CHAIN_LENGTH (8)
#define MAP_USEING -4
#define MAP_MISSING -3 /* No such element */
#define MAP_FULL -2    /* hmap is full */
#define MAP_OMEM -1    /* Out of Memory */
#define MAP_OK 0       /* OK */

/* We need to keep keywords and values */
struct _cc_hmap_element {
    intptr_t hash;
    uintptr_t data;
    _cc_list_iterator_t lnk;
};
#if 0
/* from kyotocabinet-1.2.76/kchashdb.h */
_CC_API_PRIVATE(uint32_t) fold_hash(uint64_t hash) {
   return (uint32_t)((((hash & 0xffff000000000000ULL) >> 48) | 
                      ((hash & 0x0000ffff00000000ULL) >> 16)) ^ (((hash & 0x000000000000ffffULL) << 16) | 
                      ((hash & 0x00000000ffff0000ULL) >> 16)));
}
/*
 * Hashing function for a string
 */
_CC_API_PRIVATE(uint32_t) hmap_build_hash(uint32_t keyword) {
    /* Robert Jenkins' 32 bit Mix Function */
    keyword += (keyword << 12);
    keyword ^= (keyword >> 22);
    keyword += (keyword << 4);
    keyword ^= (keyword >> 9);
    keyword += (keyword << 10);
    keyword ^= (keyword >> 2);
    keyword += (keyword << 7);
    keyword ^= (keyword >> 12);

    /* Knuth's Multiplicative Method */
    keyword = (keyword >> 3) * 2654435761UL;

    return keyword;
}
#endif
/*
 * Return the integer of the location in data
 * to store the point to the item, or MAP_FULL.
 */

#define LOAD_FACTOR_THRESHOLD 0.7f
_CC_API_PRIVATE(int) _hmap_hash(_cc_hmap_t *ctx, uint32_t *slot, const uintptr_t keyword, intptr_t hash) {
    uint32_t curr,step;
    uint32_t i;
    _cc_hmap_element_t *element;

    *slot = 0;

    /* If full, return immediately */
    if (((float32_t)ctx->count / (float32_t)ctx->limit) > LOAD_FACTOR_THRESHOLD) {
        return MAP_FULL;
    }

    /* find the best index */
    curr = hash % ctx->limit;
    step = 1 + (hash % (ctx->limit - 1));

    /* Linear probing */
    for (i = 0; i < MAX_CHAIN_LENGTH; i++) {
        element = &ctx->slots[curr];
        if (element->data == 0) {
            *slot = curr;
            return MAP_OK;
        }

        if (hash == element->hash && (ctx->equals_func(element->data, keyword))) {
            *slot = curr;
            return MAP_USEING;
        }

        curr = (curr + step) % ctx->limit;
    }

    return MAP_FULL;
}

_CC_API_PRIVATE(_cc_hmap_element_t*) _hmap_empty_element(_cc_hmap_element_t *slots, uint32_t limit, intptr_t hash) {
    uint32_t curr,step;
    uint32_t i;
    _cc_hmap_element_t *element;

    /* find the best index */
    curr = hash % limit;
    step = 1 + (hash % (limit - 1));
    /* Linear probing */
    for (i = 0; i < MAX_CHAIN_LENGTH; i++) {
        element = &slots[curr];
        if (element->data == 0) {
            return element;
        }
        curr = (curr + step) % limit;
    }

    return nullptr;
}
/*
 * Doubles the size of the hmap, and rehashes all the elements
 */
_CC_API_PRIVATE(int) _hmap_rehash(_cc_hmap_t *ctx, int times) {
    _cc_list_iterator_t list;
    _cc_list_iterator_t *it;
    _cc_hmap_element_t *slots = ctx->slots;
    uint32_t limit = ctx->limit * times;

    /* Setup the new elements */
    _cc_hmap_element_t *elements = (_cc_hmap_element_t *)_cc_malloc(limit * sizeof(_cc_hmap_element_t));
    bzero(elements,sizeof(_cc_hmap_element_t) * limit);

    _cc_list_iterator_cleanup(&list);

    /* Rehash the elements */
    _cc_list_iterator_for(it, &ctx->list) {
        /**/
        _cc_hmap_element_t *n = _cc_upcast(it, _cc_hmap_element_t, lnk);
        /* Set the data */
        _cc_hmap_element_t *element = _hmap_empty_element(elements, limit, n->hash);
        if (element == nullptr) {
            _cc_free(elements);
            return MAP_FULL;
        }

        element->hash = n->hash;
        element->data = n->data;

        _cc_list_iterator_push(&list, &element->lnk);
        ctx->count++;
    };

    /* Update the array */
    ctx->slots = elements;
    /* Update the size */
    ctx->limit = limit;

    ctx->list = list;
    _cc_free(slots);

    return MAP_OK;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_alloc_hmap(_cc_hmap_t *ctx, uint32_t capacity,
    _cc_hmap_keyword_equals_func_t equals_func, _cc_hmap_keyword_hash_func_t hash_func) {
    _cc_assert(ctx != nullptr);
    ctx->limit = (int32_t)_cc_aligned_alloc_opt(capacity, INITIAL_SIZE);

    ctx->slots = (_cc_hmap_element_t *)_cc_malloc(ctx->limit * sizeof(_cc_hmap_element_t));
    bzero(ctx->slots, sizeof(_cc_hmap_element_t) * ctx->limit);

    /*clear link*/
    _cc_list_iterator_cleanup(&ctx->list);

    ctx->equals_func = equals_func;
    ctx->hash_func = hash_func;

    return true;
}

/*
 * Add a pointer to the hmap with some keyword
 */
_CC_API_PUBLIC(bool_t) _cc_hmap_push(_cc_hmap_t *ctx, const uintptr_t keyword, const uintptr_t data) {
    _cc_hmap_element_t *element;
    uint32_t index;
    intptr_t hash = ctx->hash_func(keyword);
    int flag = _hmap_hash(ctx, &index, keyword, hash);
    int times = 2;
    if (MAP_USEING == flag) {
        return false;
    }

    while (flag == MAP_FULL) {
        if (_hmap_rehash(ctx, times++) == MAP_FULL) {
            continue;
        }

        flag = _hmap_hash(ctx, &index, keyword, hash);
        if (flag == MAP_USEING) {
            return false;
        }
    }

    /* Set the data */
    element = &ctx->slots[index];
    element->hash = hash;
    element->data = data;

    /*push link*/
    _cc_list_iterator_push_back(&ctx->list, &(element->lnk));
    ctx->count++;
    return true;
}

/*
 * Get your pointer out of the hmap with a keyword
 */
_CC_API_PUBLIC(uintptr_t) _cc_hmap_find(_cc_hmap_t *ctx, const uintptr_t keyword) {
    uint32_t i;
    uint32_t curr,step;
    intptr_t hash;
    _cc_hmap_element_t *element;

    /* Find data location */
    hash = ctx->hash_func(keyword);
    curr = hash % ctx->limit;
    step = 1 + (hash % (ctx->limit - 1));

    /* Linear probing, if necessary */
    for (i = 0; i < MAX_CHAIN_LENGTH; i++) {
        element = &ctx->slots[curr];
        if (element->data && element->hash == hash && (ctx->equals_func(element->data, keyword))) {
            return element->data;
        }

        curr = (curr + step) % ctx->limit;
    }
    /* Not found */
    return 0;
}

/*
 * Remove an element with that keyword from the map
 */
_CC_API_PUBLIC(uintptr_t) _cc_hmap_pop(_cc_hmap_t *ctx, const uintptr_t keyword) {
    uint32_t i;
    uint32_t curr,step;
    intptr_t hash;
    uintptr_t any;
    _cc_hmap_element_t *element;

    /* Find keyword */
    hash = ctx->hash_func(keyword);
    curr = hash % ctx->limit;
    step = 1 + (hash % (ctx->limit - 1));

    /* Linear probing, if necessary */
    for (i = 0; i < MAX_CHAIN_LENGTH; i++) {
        element = &ctx->slots[curr];
        any = element->data;
        if (any && element->hash == hash && (ctx->equals_func(any, keyword))) {
            /*remove link*/
            _cc_list_iterator_remove(&element->lnk);
            ctx->count--;
            /* Blank out the fields */
            element->data = 0;
            return any;
        }
        curr = (curr + step) % ctx->limit;
    }
    /* Data not found */
    return 0;
}

/**
 *  Removes all items.
 */
_CC_API_PUBLIC(bool_t) _cc_hmap_cleanup(_cc_hmap_t *ctx) {
    /* Rehash the elements */
    _cc_list_iterator_for_each_next(it, &ctx->list, {
        _cc_hmap_element_t *n = _cc_upcast(it, _cc_hmap_element_t, lnk);
        n->data = 0;
    });
    _cc_list_iterator_cleanup(&ctx->list);
    ctx->count = 0;

    return true;
}

/* free the hmap */
_CC_API_PUBLIC(bool_t) _cc_free_hmap(_cc_hmap_t *ctx) {
    _cc_assert(ctx != nullptr);

    _cc_safe_free(ctx->slots);

    return true;
}

/**/
_CC_API_PUBLIC(uintptr_t) _cc_hmap_value(_cc_list_iterator_t *v) {
    _cc_hmap_element_t *n = _cc_upcast(v, _cc_hmap_element_t, lnk);
    _cc_assert(n != nullptr);
    return n->data;
}
