#include <libcc/alloc.h>
#include <libcc/math.h>
#include <libcc/ring.h>
#include <libcc/string.h>

#define _CC_RING_EXPAND_ 1

/**/
_CC_API_PUBLIC(bool_t) _cc_alloc_ring(_cc_ring_t *ctx, int32_t slot_size) {
    _cc_assert(ctx != NULL);

    ctx->size = _max(slot_size, 10);
    ctx->data = (pvoid_t)_cc_calloc(ctx->size, sizeof(pvoid_t));
    if (_cc_unlikely(ctx->data == NULL)) {
        return false;
    }
    ctx->r = 0;
    ctx->w = 0;
    _cc_lock_init(&ctx->lock);

    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_free_ring(_cc_ring_t *ctx) {
    _cc_assert(ctx != NULL);
    _cc_if_free(ctx->data);
    return true;
}

/**/
_CC_API_PUBLIC(void) _cc_ring_cleanup(_cc_ring_t *ctx) {
    _cc_assert(ctx != NULL);
    ctx->r = 0;
    ctx->w = 0;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_ring_empty(_cc_ring_t *ctx) {
    return ctx->r == ctx->w;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_ring_push(_cc_ring_t *ctx, pvoid_t data) {
    uint32_t w;
    _cc_assert(ctx != NULL);

    _cc_spin_lock(&ctx->lock);
    w = (ctx->w + 1) % ctx->size;

    if (w == ctx->r) {
        _cc_unlock(&ctx->lock);
        return false;
    }

    ctx->data[ctx->w] = data;
    ctx->w = w;

    _cc_unlock(&ctx->lock);
    return true;
}

/**/
_CC_API_PUBLIC(pvoid_t) _cc_ring_pop(_cc_ring_t *ctx) {
    uint32_t r;
    pvoid_t data;
    _cc_assert(ctx != NULL);

    if (ctx->r == ctx->w) {
        return NULL;
    }

    _cc_spin_lock(&ctx->lock);
    r = (ctx->r + 1) % ctx->size;

    data = ctx->data[ctx->r];
    ctx->r = r;

    _cc_unlock(&ctx->lock);
    return data;
}
