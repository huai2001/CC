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
#include <cc/array.h>

#define _CC_MAX_ARRAY_EXPAND_ 64

_CC_API_PRIVATE(uint32_t) _static_array_expand_size(uint32_t number) {
    if (number & (_CC_MAX_ARRAY_EXPAND_ - 1)) {
        number = (uint32_t)(number / _CC_MAX_ARRAY_EXPAND_) + 1;
    } else {
        number = (uint32_t)(number / _CC_MAX_ARRAY_EXPAND_);
    }
    return number * _CC_MAX_ARRAY_EXPAND_;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_array_alloc(_cc_array_t *ctx, uint32_t initsize) {
    _cc_assert(ctx != NULL);

    if (_cc_unlikely(initsize == 0)) {
        return false;
    }

    memset(ctx, 0, sizeof(_cc_array_t));
    ctx->size = initsize;
    ctx->length = 0;
    ctx->data = _CC_CALLOC(pvoid_t, ctx->size);
    return true;
}

/**/
_CC_API_PUBLIC(_cc_array_t*) _cc_create_array(uint32_t initsize) {
    _cc_array_t *ctx = _CC_MALLOC(_cc_array_t);
    if (_cc_array_alloc(ctx, initsize)) {
        return ctx;
    }

    _cc_safe_free(ctx);
    return NULL;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_array_expand(_cc_array_t *ctx, uint32_t slot_size) {
    pvoid_t *data;

    _cc_assert(ctx != NULL);
    if (slot_size <= ctx->size) {
        return true;
    }

    slot_size = _static_array_expand_size(slot_size);
    data = (pvoid_t *)_cc_realloc(ctx->data, sizeof(pvoid_t) * slot_size);
    bzero(&data[ctx->size], (slot_size - ctx->size) * sizeof(pvoid_t));
    ctx->data = data;
    ctx->size = slot_size;

    return true;
}

_CC_API_PUBLIC(pvoid_t) _cc_array_find(const _cc_array_t *ctx, const uint32_t index) {
    _cc_assert(ctx != NULL);

    if (_cc_unlikely(ctx->size <= index)) {
        _cc_logger_error(_T("Array find: index out of range [%d] with size %d"), index, ctx->size);
        return NULL;
    }

    return ctx->data[index];
}

_CC_API_PUBLIC(uint32_t) _cc_array_push(_cc_array_t *ctx, void *data) {
    uint32_t index = 0;
    _cc_assert(ctx != NULL);

    index = ctx->length;

    if (_cc_array_insert(ctx, index, data)) {
        return index;
    }

    return -1;
}

_CC_API_PUBLIC(void *) _cc_array_pop(_cc_array_t *ctx) {
    _cc_assert(ctx != NULL);
    return _cc_array_remove(ctx, ctx->length - 1);
}

_CC_API_PUBLIC(bool_t) _cc_array_append(_cc_array_t *ctx, const _cc_array_t *append) {
    uint32_t size = 0;
    _cc_assert(ctx != NULL && append != NULL);

    size = ctx->length + append->length;
    /*if not enough space,expand first*/
    if (ctx->size <= 0x80000000 && size > ctx->size) {
        _cc_array_expand(ctx, size);
    }

    memcpy(ctx->data[ctx->length], append->data[0], append->length * sizeof(pvoid_t));

    ctx->length = size;

    return true;
}

_CC_API_PUBLIC(bool_t) _cc_array_insert(_cc_array_t *ctx, const uint32_t index, pvoid_t data) {
    _cc_assert(ctx != NULL && data != NULL);

    if (_cc_unlikely(data == NULL)) {
        return false;
    }

    /*if not enough space,expand first*/
    if (ctx->size <= 0x80000000 && index >= ctx->size) {
        _cc_array_expand(ctx, index + 1);
    }

    if (index >= ctx->size) {
        _cc_logger_error(_T("Array insert: index out of range [%d] with size %d"), index, ctx->size);
        return false;
    }

    if (_cc_likely(index >= ctx->length)) {
        ctx->length++;
    }

    ctx->data[index] = data;

    return true;
}

_CC_API_PUBLIC(bool_t) _cc_array_set(_cc_array_t *ctx, const uint32_t index, pvoid_t data) {
    _cc_assert(ctx != NULL);

    if (_cc_unlikely(index >= ctx->size)) {
        _cc_logger_error(_T("Array insert: index out of range [%d] with size %d"), index, ctx->size);
        return false;
    }

    if (ctx->data[index] && data == NULL) {
        ctx->length--;
    }

    ctx->data[index] = data;
    return true;
}

_CC_API_PUBLIC(pvoid_t) _cc_array_remove(_cc_array_t *ctx, const uint32_t index) {
    uint32_t i = 0;
    uint32_t length = 0;
    pvoid_t data = NULL;

    _cc_assert(ctx != NULL && ctx->size > index);
    if (_cc_unlikely(ctx->size <= index)) {
        return NULL;
    }

    data = ctx->data[index];
    length = ctx->length - 1;

    for (i = index; i < length; i++) {
        ctx->data[i] = ctx->data[i + 1];
    }

    ctx->length = length;

    return data;
}

_CC_API_PUBLIC(bool_t) _cc_array_free(_cc_array_t *ctx) {
    _cc_assert(ctx != NULL);

    _cc_safe_free(ctx->data);
    ctx->length = 0;
    ctx->size = 0;

    return true;
}

_CC_API_PUBLIC(void) _cc_destroy_array(_cc_array_t **ctx) {
    if (_cc_array_free(*ctx)) {
        _cc_free(*ctx);
    }

    *ctx = NULL;
}

_CC_API_PUBLIC(bool_t) _cc_array_cleanup(_cc_array_t *ctx) {
    _cc_assert(ctx != NULL);
    bzero(ctx->data, sizeof(pvoid_t) * ctx->size);
    ctx->length = 0;

    return true;
}

_CC_API_PUBLIC(uint32_t) _cc_array_length(const _cc_array_t *ctx) {
    _cc_assert(ctx != NULL);

    return ctx->length;
}

_CC_API_PUBLIC(pvoid_t) _cc_array_begin(const _cc_array_t *ctx) {
    _cc_assert(ctx != NULL);

    return ctx->data[0];
}

_CC_API_PUBLIC(pvoid_t) _cc_array_end(const _cc_array_t *ctx) {
    _cc_assert(ctx != NULL);

    return ctx->data[ctx->size - 1];
}
