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

enum _CC_HAMP_TYPES {
    T_EMPTY = 0,
    T_USED
};

/* We need to keep keywords and values */
struct _cc_hmap_element {
    byte_t flag;
    uint32_t hash_code;
    intptr_t data;
    _cc_list_iterator_t lnk;
};

/* from kyotocabinet-1.2.76/kchashdb.h */
//_CC_API_PRIVATE(uint32_t) fold_hash(uint64_t hash) {
//    return (uint32_t)((((hash & 0xffff000000000000ULL) >> 48) | ((hash &
//    0x0000ffff00000000ULL) >> 16)) ^
//        (((hash & 0x000000000000ffffULL) << 16) | ((hash &
//        0x00000000ffff0000ULL) >> 16)));
//}
/*
 * Hashing function for a string
 */
//_CC_API_PRIVATE(uint32_t) hmap_build_hash(uint32_t keyword) {
//    /* Robert Jenkins' 32 bit Mix Function */
//    keyword += (keyword << 12);
//    keyword ^= (keyword >> 22);
//    keyword += (keyword << 4);
//    keyword ^= (keyword >> 9);
//    keyword += (keyword << 10);
//    keyword ^= (keyword >> 2);
//    keyword += (keyword << 7);
//    keyword ^= (keyword >> 12);
//
//    /* Knuth's Multiplicative Method */
//    keyword = (keyword >> 3) * 2654435761UL;
//
//    return keyword;
//}

/*
 * Return the integer of the location in data
 * to store the point to the item, or MAP_FULL.
 */
_CC_API_PRIVATE(int) _hmap_hash(_cc_hmap_t *ctx, const intptr_t keyword, uint32_t hash_code) {
    uint32_t curr;
    uint32_t i;
    _cc_hmap_element_t *element;

    /* If full, return immediately */
    if (ctx->count >= (ctx->slots / 2)) {
        return MAP_FULL;
    }

    /* find the best index */
    curr = hash_code % ctx->slots;

    /* Linear probing */
    for (i = 0; i < MAX_CHAIN_LENGTH; i++) {
        element = &ctx->data[curr];
        if (element->flag == T_EMPTY) {
            return curr;
        }

        if (hash_code == element->hash_code && (ctx->equals_func(element->data, keyword))) {
            return MAP_USEING;
        }

        curr = (curr + 1) % ctx->slots;
    }

    return MAP_FULL;
}

_CC_API_PRIVATE(_cc_hmap_element_t*) _hmap_empty_element(_cc_hmap_element_t *elements, uint32_t slots, uint32_t hash_code) {
    uint32_t curr;
    uint32_t i;
    _cc_hmap_element_t *element;

    /* find the best index */
    curr = hash_code % slots;

    /* Linear probing */
    for (i = 0; i < MAX_CHAIN_LENGTH; i++) {
        element = &elements[curr];
        if (element->flag == T_EMPTY) {
            return element;
        }
        curr = (curr + 1) % slots;
    }

    return nullptr;
}
/*
 * Doubles the size of the hmap, and rehashes all the elements
 */
_CC_API_PRIVATE(int) _hmap_rehash(_cc_hmap_t *ctx) {
    _cc_list_iterator_t list;
    _cc_hmap_element_t *data = ctx->data;
    uint32_t slots = ctx->slots * 2;

    /* Setup the new elements */
    _cc_hmap_element_t *elements = (_cc_hmap_element_t *)_cc_calloc(slots, sizeof(_cc_hmap_element_t));
    if (_cc_unlikely(!elements)) {
        return MAP_OMEM;
    }

    _cc_list_iterator_cleanup(&list);

    /* Rehash the elements */
    _cc_list_iterator_for_each_next(it, &ctx->list, {
        /**/
        _cc_hmap_element_t *n = _cc_upcast(it, _cc_hmap_element_t, lnk);
        /* Set the data */
        _cc_hmap_element_t *element = _hmap_empty_element(elements, slots, n->hash_code);
        if (element == nullptr) {
            _cc_free(elements);
            return MAP_FULL;
        }

        element->hash_code = n->hash_code;
        element->data = n->data;
        element->flag = T_USED;

        _cc_list_iterator_push(&list, &element->lnk);
        ctx->count++;
    });

    /* Update the array */
    ctx->data = elements;
    /* Update the size */
    ctx->slots = slots;

    ctx->list = list;
    _cc_free(data);

    return MAP_OK;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_alloc_hmap(_cc_hmap_t *ctx, uint32_t initial_size,
    _cc_hmap_keyword_equals_func_t equals_func, _cc_hmap_keyword_hash_func_t hash_func) {
    _cc_assert(ctx != nullptr);
    ctx->slots = (int32_t)_cc_aligned_alloc_opt(initial_size, INITIAL_SIZE);

    ctx->data = (_cc_hmap_element_t *)_cc_calloc(ctx->slots, sizeof(_cc_hmap_element_t));
    if (_cc_unlikely(!ctx->data)) {
        return false;
    };

    /*clear link*/
    _cc_list_iterator_cleanup(&ctx->list);

    ctx->equals_func = equals_func;
    ctx->hash_func = hash_func;

    return true;
}

/*
 * Add a pointer to the hmap with some keyword
 */
_CC_API_PUBLIC(bool_t) _cc_hmap_push(_cc_hmap_t *ctx, const intptr_t keyword, const intptr_t data) {
    _cc_hmap_element_t *element;
    uint32_t hash_code = ctx->hash_func(keyword);
    uint32_t index = _hmap_hash(ctx, keyword, hash_code);

    if (MAP_USEING == index) {
        return false;
    }

    while (index == MAP_FULL) {
        if (_hmap_rehash(ctx) == MAP_OMEM) {
            return false;
        }

        index = _hmap_hash(ctx, keyword, hash_code);
        if (index == MAP_USEING) {
            return false;
        }
    }

    /* Set the data */
    element = &ctx->data[index];
    element->hash_code = hash_code;
    element->data = data;
    element->flag = T_USED;

    /*push link*/
    _cc_list_iterator_push_back(&ctx->list, &(element->lnk));
    ctx->count++;
    return true;
}

/*
 * Get your pointer out of the hmap with a keyword
 */
_CC_API_PUBLIC(intptr_t) _cc_hmap_find(_cc_hmap_t *ctx, const intptr_t keyword) {
    int32_t i;
    int32_t curr;
    uint32_t hash_code;
    _cc_hmap_element_t *element;

    /* Find data location */
    hash_code = ctx->hash_func(keyword);
    curr = hash_code % ctx->slots;

    /* Linear probing, if necessary */
    for (i = 0; i < MAX_CHAIN_LENGTH; i++) {
        element = &ctx->data[curr];
        if (element->flag == T_USED && element->hash_code == hash_code && (ctx->equals_func(element->data, keyword))) {
            return element->data;
        }

        curr = (curr + 1) % ctx->slots;
    }

    /* Not found */
    return 0;
}

/*
 * Remove an element with that keyword from the map
 */
_CC_API_PUBLIC(intptr_t) _cc_hmap_pop(_cc_hmap_t *ctx, const intptr_t keyword) {
    int32_t i;
    int32_t curr;
    uint32_t hash_code;
    _cc_hmap_element_t *element;
    intptr_t any;

    /* Find keyword */
    hash_code = ctx->hash_func(keyword);
    curr = hash_code % ctx->slots;

    /* Linear probing, if necessary */
    for (i = 0; i < MAX_CHAIN_LENGTH; i++) {
        element = &ctx->data[curr];
        if (element->flag == T_USED && hash_code == element->hash_code && (ctx->equals_func(element->data, keyword))) {
            any = element->data;
            /*remove link*/
            _cc_list_iterator_remove(&element->lnk);
            ctx->count--;
            /* Blank out the fields */
            element->flag = T_EMPTY;
            return any;
        }
        curr = (curr + 1) % ctx->slots;
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
        n->flag = T_EMPTY;
    });
    _cc_list_iterator_cleanup(&ctx->list);
    ctx->count = 0;

    return true;
}

/* free the hmap */
_CC_API_PUBLIC(bool_t) _cc_free_hmap(_cc_hmap_t *ctx) {
    _cc_assert(ctx != nullptr);

    _cc_safe_free(ctx->data);

    return true;
}

/**/
_CC_API_PUBLIC(intptr_t) _cc_hmap_value(intptr_t v) {
    _cc_hmap_element_t *n = _cc_upcast(v, _cc_hmap_element_t, lnk);
    _cc_assert(n != nullptr);
    if (n && n->flag == T_USED) {
        return n->data;
    }
    return 0;
}
