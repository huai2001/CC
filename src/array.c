/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#include <libcc/array.h>

#define _CC_MAX_ARRAY_EXPAND_ 32

/**/
_CC_API_PUBLIC(bool_t) _cc_array_alloc(_cc_array_t *ctx, size_t capacity) {
    _cc_assert(ctx != nullptr);

    ctx->limit = _cc_aligned_alloc_opt(capacity, 64);
    ctx->length = 0;
    ctx->data = _CC_CALLOC(pvoid_t, ctx->limit);
    return true;
}

/**/
_CC_API_PUBLIC(_cc_array_t*) _cc_create_array(size_t capacity) {
    _cc_array_t *ctx = _CC_MALLOC(_cc_array_t);
    if (_cc_array_alloc(ctx, capacity)) {
        return ctx;
    }

    _cc_safe_free(ctx);
    return nullptr;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_array_realloc(_cc_array_t *ctx, size_t capacity) {
    pvoid_t *data;

    _cc_assert(ctx != nullptr);
    if (capacity <= ctx->limit) {
        return true;
    }

    capacity = _cc_aligned_alloc_opt(capacity, 64);
    data = (pvoid_t *)_cc_realloc(ctx->data, sizeof(pvoid_t) * capacity);
    bzero(&data[ctx->limit], (capacity - ctx->limit) * sizeof(pvoid_t));
    ctx->data = data;
    ctx->limit = capacity;

    return true;
}

_CC_API_PUBLIC(pvoid_t) _cc_array_find(const _cc_array_t *ctx, const size_t index) {
    _cc_assert(ctx != nullptr);

    if (_cc_unlikely(ctx->limit <= index)) {
        _cc_logger_error(_T("Array find: index out of range [%d] with size %d"), index, ctx->limit);
        return nullptr;
    }

    return ctx->data[index];
}

_CC_API_PUBLIC(size_t) _cc_array_push(_cc_array_t *ctx, pvoid_t data) {
    _cc_assert(ctx != nullptr && data != nullptr);
    size_t index;
    if (_cc_unlikely(data == nullptr)) {
        return -1;
    }
    
    index = ctx->length;
    /*if not enough space,expand first*/
    if (ctx->limit <= 0x80000000 && index >= ctx->limit) {
        _cc_array_realloc(ctx, index + 1);
    }

    if (index >= ctx->limit) {
        _cc_logger_error(_T("Array insert: index out of range [%d] with capacity %d"), index, ctx->limit);
        return -1;
    }

    ctx->data[ctx->length++] = data;
    return index;
}

_CC_API_PUBLIC(pvoid_t) _cc_array_pop(_cc_array_t *ctx) {
    _cc_assert(ctx != nullptr);
    return _cc_array_remove(ctx, ctx->length - 1);
}

_CC_API_PUBLIC(bool_t) _cc_array_append(_cc_array_t *ctx, const _cc_array_t *append) {
    size_t capacity = 0;
    _cc_assert(ctx != nullptr && append != nullptr);

    capacity = ctx->length + append->length;
    /*if not enough space,expand first*/
    if (ctx->limit <= 0x80000000 && capacity > ctx->limit) {
        _cc_array_realloc(ctx, capacity);
    }

    memcpy(ctx->data[ctx->length], append->data[0], append->length * sizeof(pvoid_t));

    ctx->length = capacity;

    return true;
}

_CC_API_PUBLIC(bool_t) _cc_array_insert(_cc_array_t *ctx, const size_t index, pvoid_t data) {
    _cc_assert(ctx != nullptr && data != nullptr);

    if (_cc_unlikely(data == nullptr)) {
        return -1;
    }

    if (index >= ctx->limit) {
        _cc_logger_error(_T("Array push: index out of range [%d] with capacity %d"), index, ctx->limit);
        return -1;
    }

    ctx->data[index] = data;
    return true;
}

_CC_API_PUBLIC(bool_t) _cc_array_set(_cc_array_t *ctx, const size_t index, pvoid_t data) {
    _cc_assert(ctx != nullptr);

    if (_cc_unlikely(index >= ctx->limit)) {
        _cc_logger_error(_T("Array insert: index out of range [%d] with size %d"), index, ctx->limit);
        return false;
    }

    if (ctx->data[index] && data == nullptr) {
        ctx->length--;
    }

    ctx->data[index] = data;
    return true;
}

_CC_API_PUBLIC(pvoid_t) _cc_array_remove(_cc_array_t *ctx, const size_t index) {
    size_t move_count;
    pvoid_t data;
    if (_cc_unlikely(ctx == nullptr || ctx->data == nullptr || ctx->limit <= index)) {
        return nullptr;
    }
    
    data = ctx->data[index];
    move_count = ctx->length - index - 1;
    
    if (move_count > 0) {
        memmove(&ctx->data[index], &ctx->data[index + 1], move_count * sizeof(pvoid_t));
    }
    
    ctx->length--;
    return data;
}

_CC_API_PUBLIC(bool_t) _cc_array_free(_cc_array_t *ctx) {
    _cc_assert(ctx != nullptr);

    _cc_safe_free(ctx->data);
    ctx->length = 0;
    ctx->limit = 0;

    return true;
}

_CC_API_PUBLIC(void) _cc_destroy_array(_cc_array_t **ctx) {
    if (_cc_array_free(*ctx)) {
        _cc_free(*ctx);
    }

    *ctx = nullptr;
}

_CC_API_PUBLIC(bool_t) _cc_array_cleanup(_cc_array_t *ctx) {
    _cc_assert(ctx != nullptr);
    bzero(ctx->data, sizeof(pvoid_t) * ctx->limit);
    ctx->length = 0;

    return true;
}

_CC_API_PUBLIC(size_t) _cc_array_length(const _cc_array_t *ctx) {
    _cc_assert(ctx != nullptr);

    return ctx->length;
}

_CC_API_PUBLIC(pvoid_t) _cc_array_begin(const _cc_array_t *ctx) {
    _cc_assert(ctx != nullptr);

    return ctx->data[0];
}

_CC_API_PUBLIC(pvoid_t) _cc_array_end(const _cc_array_t *ctx) {
    _cc_assert(ctx != nullptr);

    return ctx->data[ctx->limit - 1];
}
