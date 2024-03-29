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
#include <cc/math.h>
#include <cc/ring.h>
#include <cc/string.h>

#define _CC_RING_EXPAND_ 1

/**/
_CC_API_PUBLIC(bool_t) _cc_ring_alloc(_cc_ring_t *ctx, int32_t slot_size) {
    _cc_assert(ctx != NULL);

    ctx->size = _max(slot_size, 10);
    ctx->data = (pvoid_t)_cc_calloc(ctx->size, sizeof(pvoid_t));
    if (_cc_unlikely(ctx->data == NULL)) {
        return false;
    }
    ctx->r = 0;
    ctx->w = 0;
    _cc_spin_lock_init(&ctx->lock);

    return true;
}

/**/
_CC_API_PUBLIC(_cc_ring_t*) _cc_create_ring(int32_t slot_size) {
    _cc_ring_t *ctx = (_cc_ring_t *)_cc_malloc(sizeof(_cc_ring_t));
    if (_cc_likely(_cc_ring_alloc(ctx, slot_size))) {
        return ctx;
    }

    _cc_safe_free(ctx);
    return NULL;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_ring_free(_cc_ring_t *ctx) {
    _cc_assert(ctx != NULL);
    _cc_safe_free(ctx->data);
    return true;
}

/**/
_CC_API_PUBLIC(void) _cc_destroy_ring(_cc_ring_t **ctx) {
    _cc_assert(ctx != NULL);

    if (_cc_ring_free(*ctx)) {
        _cc_free((*ctx));
    }

    (*ctx) = NULL;
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
        _cc_spin_unlock(&ctx->lock);
        return false;
    }

    ctx->data[ctx->w] = data;
    ctx->w = w;

    _cc_spin_unlock(&ctx->lock);
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

    _cc_spin_unlock(&ctx->lock);
    return data;
}
