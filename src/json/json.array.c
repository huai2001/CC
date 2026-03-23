#include "json.c.h"

#define _JSON_ARRAY_SIZE_ 64


void _json_array_alloc(_cc_json_t* ctx, size_t capacity) {
    ctx->type = _CC_JSON_ARRAY_;
    ctx->element.uni_array = _cc_alloc_array(capacity);
}

void _free_json_array(_cc_json_t* ctx) {
    uintptr_t arr = (uintptr_t)ctx->element.uni_array;
    size_t i;
    size_t length = _cc_array_length(arr);

    for (i = 0; i < length; i++) {
        _cc_json_t *v = ((_cc_json_t*)*((uintptr_t*)(arr) + i));
        _json_free_node(v);
    }
    _cc_free_array(arr);
}

/**/
_CC_API_PUBLIC(_cc_json_t*) _cc_json_alloc_array(const tchar_t *keyword, size_t capacity) {
    _cc_json_t *item = (_cc_json_t *)_cc_malloc(sizeof(_cc_json_t));
    memset(item, 0, sizeof(_cc_json_t));
    item->type = _CC_JSON_ARRAY_;
    item->element.uni_array = _cc_alloc_array(capacity);
    if (keyword) {
        item->name = _cc_sds_alloc(keyword,_tcslen(keyword));
    } else {
        item->name = NULL;
    }

    return item;
}

size_t _json_array_push(_cc_json_t *ctx, _cc_json_t *data) {
    return _cc_array_push(&ctx->element.uni_array, (uintptr_t)data);
}

/**/
_CC_API_PUBLIC(bool_t) _cc_json_array_remove(_cc_json_t *ctx, const uint32_t index) {
    uintptr_t r;
    if (ctx->type != _CC_JSON_ARRAY_) {
        return false;
    }

    r = _cc_array_remove(ctx->element.uni_array, index);
    if (r == -1) {
        return false;
    }

    _json_free_node((_cc_json_t *)(uintptr_t*)r);
    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_json_array_push(_cc_json_t *ctx, _cc_json_t *data) {
    if (ctx->type != _CC_JSON_ARRAY_) {
        return false;
    }
    return _json_array_push(ctx, data) != -1;
}