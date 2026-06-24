#include <libcc/alloc.h>
#include <libcc/math.h>
#include <libcc/ring.h>
#include <libcc/string.h>

/**/
_CC_API_PUBLIC(bool_t) _cc_alloc_ring(_cc_ring_t *ctx, int32_t slot_size) {
    _cc_assert(ctx != NULL);

    ctx->size = _max(slot_size, 10);
    ctx->data = (intptr_t*)_cc_calloc(ctx->size, sizeof(intptr_t));
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
_CC_API_PUBLIC(bool_t) _cc_ring_push(_cc_ring_t *ctx, intptr_t data) {
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
_CC_API_PUBLIC(intptr_t) _cc_ring_pop(_cc_ring_t *ctx) {
    intptr_t data;
    _cc_assert(ctx != NULL);

    if (ctx->r == ctx->w) {
        return 0;
    }

    _cc_spin_lock(&ctx->lock);
    data = ctx->data[ctx->r];
    ctx->r = (ctx->r + 1) % ctx->size;
    _cc_unlock(&ctx->lock);

    return data;
}
