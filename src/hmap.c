#include <libcc/alloc.h>
#include <libcc/crc.h>
#include <libcc/hmap.h>
#include <libcc/math.h>
#include <libcc/string.h>

#define INITIAL_SIZE (32)
#define MAX_CHAIN_LENGTH (8)
#define MAP_USEING -4
#define MAP_MISSING -3 /* No such cell */
#define MAP_FULL -2    /* hmap is full */
#define MAP_OMEM -1    /* Out of Memory */
#define MAP_OK 0       /* OK */

/* We need to keep keywords and values */
struct _cc_hmap_cell {
    intptr_t hash;
    uintptr_t data;
    _cc_list_t lnk;
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
    _cc_hmap_cell_t *cell;

    *slot = 0;

    /* If full, return immediately */
    if (((float32_t)ctx->count / (float32_t)ctx->limit) > LOAD_FACTOR_THRESHOLD) {
        return MAP_FULL;
    }

    /* find the best index */
    curr = (uint32_t)(hash % (intptr_t)ctx->limit);
    step = (uint32_t)(1 + (hash %  (intptr_t)(ctx->limit - 1)));

    /* Linear probing */
    for (i = 0; i < MAX_CHAIN_LENGTH; i++) {
        cell = &ctx->cells[curr];
        if (cell->data == 0) {
            *slot = curr;
            return MAP_OK;
        }

        if (hash == cell->hash && (ctx->equals_func(cell->data, keyword))) {
            *slot = curr;
            return MAP_USEING;
        }

        curr = (curr + step) % ctx->limit;
    }

    return MAP_FULL;
}

_CC_API_PRIVATE(_cc_hmap_cell_t*) _hmap_empty_cell(_cc_hmap_cell_t *cells, uint32_t limit, intptr_t hash) {
    uint32_t curr,step;
    uint32_t i;
    _cc_hmap_cell_t *cell;

    /* find the best index */
    curr = (uint32_t)(hash % (intptr_t)limit);
    step = (uint32_t)(1 + (hash %  (intptr_t)(limit - 1)));

    /* Linear probing */
    for (i = 0; i < MAX_CHAIN_LENGTH; i++) {
        cell = &cells[curr];
        if (cell->data == 0) {
            return cell;
        }
        curr = (curr + step) % limit;
    }

    return NULL;
}
/*
 * Doubles the size of the hmap, and rehashes all the cells
 */
_CC_API_PRIVATE(int) _hmap_rehash(_cc_hmap_t *ctx, float32_t factor) {
    _cc_list_t list;
    _cc_list_t *it;
    _cc_hmap_cell_t *cells_bak = ctx->cells;
    uint32_t limit = (uint32_t)(ctx->limit * factor);

    /* Setup the new cells */
    _cc_hmap_cell_t *cells = (_cc_hmap_cell_t *)_cc_calloc(limit, sizeof(_cc_hmap_cell_t));

    _cc_list_cleanup(&list);

    /* Rehash the cells */
    _cc_list_for(it, &ctx->list) {
        /**/
        _cc_hmap_cell_t *n = _cc_upcast(it, _cc_hmap_cell_t, lnk);
        /* Set the data */
        _cc_hmap_cell_t *cell = _hmap_empty_cell(cells, limit, n->hash);
        if (cell == NULL) {
            _cc_free(cells);
            return MAP_FULL;
        }

        cell->hash = n->hash;
        cell->data = n->data;

        _cc_list_push(&list, &cell->lnk);
        ctx->count++;
    };

    /* Update the array */
    ctx->cells = cells;
    /* Update the size */
    ctx->limit = limit;
    
    /*copy*/
    ctx->list = list;
    ctx->list.prev->prev = &ctx->list;
    ctx->list.next->next = &ctx->list;
    _cc_free(cells_bak);

    return MAP_OK;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_alloc_hmap(_cc_hmap_t *ctx, uint32_t capacity,
    _cc_hmap_keyword_equals_func_t equals_func, _cc_hmap_keyword_hash_func_t hash_func) {
    _cc_assert(ctx != NULL);
    ctx->limit = (int32_t)_cc_aligned_alloc_opt(capacity, INITIAL_SIZE);

    ctx->cells = (_cc_hmap_cell_t *)_cc_malloc(ctx->limit * sizeof(_cc_hmap_cell_t));
    memset(ctx->cells, 0, sizeof(_cc_hmap_cell_t) * ctx->limit);

    /*clear link*/
    _cc_list_cleanup(&ctx->list);

    ctx->equals_func = equals_func;
    ctx->hash_func = hash_func;

    return true;
}

/*
 * Add a pointer to the hmap with some keyword
 */
_CC_API_PUBLIC(bool_t) _cc_hmap_push(_cc_hmap_t *ctx, const uintptr_t keyword, const uintptr_t data) {
    _cc_hmap_cell_t *cell;
    uint32_t index;
    intptr_t hash = ctx->hash_func(keyword);
    int flag = _hmap_hash(ctx, &index, keyword, hash);
    int times = 2;
    if (MAP_USEING == flag) {
        return false;
    }

    while (flag == MAP_FULL && times++ < 10) {

        if (_hmap_rehash(ctx, 0.72f) == MAP_FULL) {
            continue;
        }

        flag = _hmap_hash(ctx, &index, keyword, hash);
        if (flag == MAP_USEING) {
            return false;
        }
    }

    /* Set the data */
    cell = &ctx->cells[index];
    cell->hash = hash;
    cell->data = data;

    /*push link*/
    _cc_list_push_back(&ctx->list, &(cell->lnk));
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
    _cc_hmap_cell_t *cell;

    /* Find data location */
    hash = ctx->hash_func(keyword);
    curr = (uint32_t)(hash % (intptr_t)ctx->limit);
    step = (uint32_t)(1 + (hash %  (intptr_t)(ctx->limit - 1)));
    
    /* Linear probing, if necessary */
    for (i = 0; i < MAX_CHAIN_LENGTH; i++) {
        cell = &ctx->cells[curr];
        if (cell->data && cell->hash == hash && (ctx->equals_func(cell->data, keyword))) {
            return cell->data;
        }

        curr = (curr + step) % ctx->limit;
    }
    /* Not found */
    return 0;
}

/*
 * Remove an cell with that keyword from the map
 */
_CC_API_PUBLIC(uintptr_t) _cc_hmap_pop(_cc_hmap_t *ctx, const uintptr_t keyword) {
    uint32_t i;
    uint32_t curr,step;
    intptr_t hash;
    uintptr_t any;
    _cc_hmap_cell_t *cell;

    /* Find keyword */
    hash = ctx->hash_func(keyword);
    curr = (uint32_t)(hash % (intptr_t)ctx->limit);
    step = (uint32_t)(1 + (hash %  (intptr_t)(ctx->limit - 1)));

    /* Linear probing, if necessary */
    for (i = 0; i < MAX_CHAIN_LENGTH; i++) {
        cell = &ctx->cells[curr];
        any = cell->data;
        if (any && cell->hash == hash && (ctx->equals_func(any, keyword))) {
            /*remove link*/
            _cc_list_remove(&cell->lnk);
            ctx->count--;
            /* Blank out the fields */
            cell->data = 0;
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
    /* Rehash the cells */
    _cc_list_for_each(it, &ctx->list, {
        _cc_hmap_cell_t *n = _cc_upcast(it, _cc_hmap_cell_t, lnk);
        n->data = 0;
    });
    _cc_list_cleanup(&ctx->list);
    ctx->count = 0;

    return true;
}

/* free the hmap */
_CC_API_PUBLIC(bool_t) _cc_free_hmap(_cc_hmap_t *ctx) {
    _cc_assert(ctx != NULL);

    _cc_if_free(ctx->cells);

    return true;
}

/**/
_CC_API_PUBLIC(uintptr_t) _cc_hmap_value(_cc_list_t *v) {
    _cc_hmap_cell_t *n = _cc_upcast(v, _cc_hmap_cell_t, lnk);
    _cc_assert(n != NULL);
    return n->data;
}
