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
#include <cc/alloc.h>
#include <cc/crc.h>
#include <cc/hashtable.h>
#include <cc/math.h>
#include <cc/string.h>

#define INITIAL_SIZE (16)
#define MAX_CHAIN_LENGTH (8)
#define MAP_USEING -4
#define MAP_MISSING -3 /* No such element */
#define MAP_FULL -2    /* hashtable is full */
#define MAP_OMEM -1    /* Out of Memory */
#define MAP_OK 0       /* OK */

enum _CC_HAMP_TYPES {
    T_EMPTY = 0,
    T_USED
};

/* We need to keep keywords and values */
struct _cc_hashtable_element {
    byte_t flag;
    uint32_t hash_code;
    pvoid_t data;
    _cc_list_iterator_t lnk;
};

/* from kyotocabinet-1.2.76/kchashdb.h */
//static uint32_t fold_hash(uint64_t hash) {
//    return (uint32_t)((((hash & 0xffff000000000000ULL) >> 48) | ((hash &
//    0x0000ffff00000000ULL) >> 16)) ^
//        (((hash & 0x000000000000ffffULL) << 16) | ((hash &
//        0x00000000ffff0000ULL) >> 16)));
//}
/*
 * Hashing function for a string
 */
//static uint32_t hashtable_build_hash(uint32_t keyword) {
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
static int _hashtable_hash(_cc_hashtable_t *ctx, const pvoid_t keyword, uint32_t hash_code) {
    uint32_t curr;
    uint32_t i;
    _cc_hashtable_element_t *element;

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

static _cc_hashtable_element_t *_hashtable_empty_element(_cc_hashtable_element_t *elements, uint32_t slots,
                                                                    uint32_t hash_code) {
    uint32_t curr;
    uint32_t i;
    _cc_hashtable_element_t *element;

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

    return NULL;
}
/*
 * Doubles the size of the hashtable, and rehashes all the elements
 */
static int _hashtable_rehash(_cc_hashtable_t *ctx) {
    _cc_list_iterator_t list;
    _cc_hashtable_element_t *data = ctx->data;
    uint32_t slots = ctx->slots * 2;

    /* Setup the new elements */
    _cc_hashtable_element_t *elements = (_cc_hashtable_element_t *)_cc_calloc(slots, sizeof(_cc_hashtable_element_t));
    if (_cc_unlikely(!elements)) {
        return MAP_OMEM;
    }

    _cc_list_iterator_cleanup(&list);

    /* Rehash the elements */
    _cc_list_iterator_for_each_next(it, &ctx->list, {
        /**/
        _cc_hashtable_element_t *n = _cc_upcast(it, _cc_hashtable_element_t, lnk);
        /* Set the data */
        _cc_hashtable_element_t *element = _hashtable_empty_element(elements, slots, n->hash_code);
        if (element == NULL) {
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
bool_t _cc_hashtable_alloc(_cc_hashtable_t *ctx, uint32_t initial_size, _cc_hashtable_keyword_equals_func_t equals_func,
                           _cc_hashtable_keyword_hash_func_t hash_func) {
    _cc_assert(ctx != NULL);
    _cc_assert(equals_func != NULL);
    _cc_assert(hash_func != NULL);
    if (_cc_unlikely(!ctx || !equals_func || !hash_func)) {
        return false;
    }

    ctx->slots = _max(initial_size, INITIAL_SIZE);

    ctx->data = (_cc_hashtable_element_t *)_cc_calloc(ctx->slots, sizeof(_cc_hashtable_element_t));
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
 * Return an empty hashtable, or NULL on failure.
 */
_cc_hashtable_t *_cc_create_hashtable(uint32_t initial_size, _cc_hashtable_keyword_equals_func_t equals_func,
                                      _cc_hashtable_keyword_hash_func_t hash_func) {
    _cc_hashtable_t *ctx = (_cc_hashtable_t *)_cc_malloc(sizeof(_cc_hashtable_t));

    if (_cc_hashtable_alloc(ctx, initial_size, equals_func, hash_func)) {
        return ctx;
    }

    _cc_safe_free(ctx);
    return NULL;
}

/*
 * Add a pointer to the hashtable with some keyword
 */
bool_t _cc_hashtable_insert(_cc_hashtable_t *ctx, const pvoid_t keyword, const pvoid_t data) {
    _cc_hashtable_element_t *element;
    uint32_t hash_code = ctx->hash_func(keyword);
    uint32_t index = _hashtable_hash(ctx, keyword, hash_code);

    if (MAP_USEING == index) {
        return false;
    }

    while (index == MAP_FULL) {
        if (_hashtable_rehash(ctx) == MAP_OMEM) {
            return false;
        }

        index = _hashtable_hash(ctx, keyword, hash_code);
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
 * Get your pointer out of the hashtable with a keyword
 */
pvoid_t _cc_hashtable_find(_cc_hashtable_t *ctx, const pvoid_t keyword) {
    int32_t i;
    int32_t curr;
    uint32_t hash_code;
    _cc_hashtable_element_t *element;

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
    return NULL;
}

/*
 * Remove an element with that keyword from the map
 */
pvoid_t _cc_hashtable_remove(_cc_hashtable_t *ctx, const pvoid_t keyword) {
    int32_t i;
    int32_t curr;
    uint32_t hash_code;
    _cc_hashtable_element_t *element;
    pvoid_t any;

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
    return NULL;
}

/**
 *  Removes all items.
 */
bool_t _cc_hashtable_cleanup(_cc_hashtable_t *ctx) {
    /* Rehash the elements */
    _cc_list_iterator_for_each_next(it, &ctx->list, {
        _cc_hashtable_element_t *n = _cc_upcast(it, _cc_hashtable_element_t, lnk);
        n->flag = T_EMPTY;
    });
    _cc_list_iterator_cleanup(&ctx->list);
    ctx->count = 0;

    return true;
}

/* free the hashtable */
bool_t _cc_hashtable_free(_cc_hashtable_t *ctx) {
    _cc_assert(ctx != NULL);

    _cc_safe_free(ctx->data);

    return true;
}

/* Deallocate the hashtable */
void _cc_destroy_hashtable(_cc_hashtable_t **ctx) {
    if (_cc_hashtable_free(*ctx)) {
        _cc_free((*ctx));
    }

    (*ctx) = NULL;
}

/**/
pvoid_t _cc_hashtable_value(pvoid_t v) {
    _cc_hashtable_element_t *n = _cc_upcast(v, _cc_hashtable_element_t, lnk);
    _cc_assert(n != NULL);
    if (n && n->flag == T_USED) {
        return n->data;
    }
    return NULL;
}
