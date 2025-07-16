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
#include "json.c.h"

#define _JSON_ARRAY_SIZE_ 32

void _json_array_alloc(_cc_json_t* ctx, size_t size) {
    ctx->size = _cc_aligned_alloc_opt(size, _JSON_ARRAY_SIZE_);
    ctx->length = 0;
    ctx->element.uni_array = (_cc_json_t**)_cc_calloc(ctx->size, sizeof(_cc_json_t*));
}

/**/
_CC_API_PUBLIC(_cc_json_t*) _cc_json_alloc_array(const tchar_t *keyword, size_t size) {
    _cc_json_t *ctx = _cc_json_alloc_object(_CC_JSON_ARRAY_, keyword);
    _json_array_alloc(ctx, size);
    return ctx;
}

/**/
_CC_API_PUBLIC(bool_t) _json_array_realloc(_cc_json_t *ctx, size_t size) {
    _cc_json_t **data;

    _cc_assert(ctx != nullptr);
    if (size <= ctx->size) {
        return true;
    }

    size = _cc_aligned_alloc_opt(size, _JSON_ARRAY_SIZE_);
    data = (_cc_json_t **)_cc_realloc(ctx->element.uni_array, sizeof(_cc_json_t*) * size);
    bzero(&data[ctx->size], (size - ctx->size) * sizeof(_cc_json_t *));
    ctx->element.uni_array = data;
    ctx->size = size;

    return true;
}

_CC_API_PUBLIC(size_t) _json_array_push(_cc_json_t *ctx, _cc_json_t *data) {
    _cc_assert(ctx != nullptr && data != nullptr);
    size_t index;

    index = ctx->length;
    /*if not enough space,expand first*/
    if (ctx->size <= 0x80000000 && index >= ctx->size) {
        _json_array_realloc(ctx, index + 1);
    }

    if (index >= ctx->size) {
        _cc_logger_error(_T("Array insert: index out of range [%d] with size %d"), index, ctx->size);
        return -1;
    }

    ctx->element.uni_array[ctx->length++] = data;
    return index;
}

_CC_API_PRIVATE(_cc_json_t*) _json_array_remove(_cc_json_t *ctx, const size_t index) {
    size_t move_count;
    _cc_json_t* data;
    if (_cc_unlikely(ctx == nullptr || ctx->element.uni_array == nullptr || ctx->size <= index)) {
        return nullptr;
    }
    
    data = ctx->element.uni_array[index];
    move_count = ctx->length - index - 1;
    
    if (move_count > 0) {
        memmove(&ctx->element.uni_array[index], &ctx->element.uni_array[index + 1], move_count * sizeof(_cc_json_t*));
    }
    
    ctx->length--;
    return data;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_json_array_remove(_cc_json_t *ctx, const uint32_t index) {
    _cc_json_t *item;
    if (ctx->type != _CC_JSON_ARRAY_) {
        return false;
    }

    item = (_cc_json_t *)_json_array_remove(ctx, index);
    if (item == nullptr) {
        return false;
    }
    _json_free_node(item);

    return true;
}
/**/
_CC_API_PUBLIC(bool_t) _cc_json_array_push(_cc_json_t *ctx, _cc_json_t *data) {
    if (ctx->type != _CC_JSON_ARRAY_) {
        return false;
    }
    return _json_array_push(ctx, data) != -1;
}