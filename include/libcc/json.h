#ifndef _C_CC_JSON_H_INCLUDED_
#define _C_CC_JSON_H_INCLUDED_

#include "sds.h"
#include "buf.h"
#include "rbtree.h"
#include "array.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
/*
 * JSON Types:
 */
enum _CC_JSON_TYPES_ {
    _CC_JSON_NULL_ = 0,
    _CC_JSON_BOOLEAN_,
    _CC_JSON_FLOAT_,
    _CC_JSON_INT_,
    _CC_JSON_OBJECT_,
    _CC_JSON_ARRAY_,
    _CC_JSON_STRING_
};

#define _CC_JSON_NUMBER_ _CC_JSON_INT_

typedef struct _cc_json _cc_json_t;

/* The JSON structre */
struct _cc_json {
    /* The type of the ctx, as above. */
    byte_t type;
    _cc_sds_t name;
    union {
        bool_t uni_boolean;
        float64_t uni_float;
        int64_t uni_int;
        _cc_rbtree_t uni_object;
        _cc_array_t uni_array;
        _cc_sds_t uni_string;
    } element;
    _cc_rbtree_iterator_t lnk;
};

/**/
_CC_API_PUBLIC(_cc_json_t *) _cc_json_alloc_object(byte_t type, const tchar_t *keyword);
/**/
_CC_API_PUBLIC(_cc_json_t*) _cc_json_alloc_array(const tchar_t *keyword, size_t size);

/**/
_CC_API_PUBLIC(bool_t) _cc_json_array_push(_cc_json_t *ctx, _cc_json_t *j);
/**/
_CC_API_PUBLIC(bool_t) _cc_json_object_push(_cc_json_t *ctx, _cc_json_t *j, bool_t replacement);

/**/
_CC_API_PUBLIC(_cc_json_t *) _cc_json_add_boolean(_cc_json_t *ctx, const tchar_t *keyword, bool_t value);
/**/
_CC_API_PUBLIC(_cc_json_t *) _cc_json_add_number(_cc_json_t *ctx, const tchar_t *keyword, int64_t value);
/**/
_CC_API_PUBLIC(_cc_json_t *) _cc_json_add_float(_cc_json_t *ctx, const tchar_t *keyword, float64_t value);
/**/
_CC_API_PUBLIC(_cc_json_t *) _cc_json_add_string(_cc_json_t *ctx, const tchar_t *keyword, const tchar_t *value);
/**/
_CC_API_PUBLIC(_cc_json_t*) _cc_json_add_sds(_cc_json_t *ctx, const tchar_t *keyword, const _cc_sds_t value);

/**/
_CC_API_PUBLIC(bool_t) _cc_json_array_remove(_cc_json_t *ctx, const uint32_t index);
/**/
_CC_API_PUBLIC(bool_t) _cc_json_object_remove(_cc_json_t *ctx, const tchar_t *keyword);

/**/
_CC_API_PUBLIC(void) _cc_json_dump(const _cc_json_t *ctx, _cc_buf_t* buf);

/**/
_CC_API_PUBLIC(_cc_json_t *) _cc_json_from_file(const tchar_t *file);
/**/
_CC_API_PUBLIC(_cc_json_t *) _cc_json_parse(const tchar_t *src, size_t length);
/**/
_CC_API_PUBLIC(_cc_json_t *) _cc_josn_parser(_cc_sbuf_t *const buffer);

/**/
_CC_API_PUBLIC(void) _cc_free_json(_cc_json_t *ctx);

/**/
_CC_API_PUBLIC(const tchar_t *) _cc_json_error(void);

/**/
_CC_API_PUBLIC(_cc_json_t *) _cc_json_object_find(const _cc_json_t *ctx, const tchar_t *keyword);
/**/
_CC_API_PUBLIC(_cc_json_t *) _cc_json_array_find(const _cc_json_t *ctx, uint32_t index);

/**/
_CC_FORCE_INLINE_ int64_t _cc_json_number(const _cc_json_t *ctx) {
    if (!ctx) {
        return 0;
    }
    switch (ctx->type) {
    case _CC_JSON_BOOLEAN_:
        return ctx->element.uni_boolean ? 1 : 0;
    case _CC_JSON_INT_:
        return (int64_t)ctx->element.uni_int;
    case _CC_JSON_FLOAT_:
        return (int64_t)ctx->element.uni_float;
    case _CC_JSON_STRING_:
        return (int64_t)_ttoi64(ctx->element.uni_string);
    }
    return 0;
}

/**/
_CC_FORCE_INLINE_ float64_t _cc_json_float(const _cc_json_t *ctx) {
    if (!ctx) {
        return 0;
    }

    switch (ctx->type) {
    case _CC_JSON_BOOLEAN_:
        return ctx->element.uni_boolean ? 1.0f : 0.0f;
    case _CC_JSON_INT_:
        return (float64_t)ctx->element.uni_int;
    case _CC_JSON_FLOAT_:
        return ctx->element.uni_float;
    case _CC_JSON_STRING_:
        return _ttof(ctx->element.uni_string);
    }
    return 0;
}

/**/
_CC_FORCE_INLINE_ const _cc_sds_t _cc_json_string(const _cc_json_t *ctx) {
    if (ctx && ctx->type == _CC_JSON_STRING_) {
        return ctx->element.uni_string;
    }
    return nullptr;
}

/**/
_CC_FORCE_INLINE_ const _cc_rbtree_t* _cc_json_object(const _cc_json_t *ctx) {
    if (ctx && ctx->type == _CC_JSON_OBJECT_) {
        return &ctx->element.uni_object;
    }
    return nullptr;
}

/**/
_CC_FORCE_INLINE_ _cc_array_t _cc_json_array(const _cc_json_t *ctx) {
    if (ctx && ctx->type == _CC_JSON_ARRAY_) {
        return ctx->element.uni_array;
    }
    return -1;
}

/**/
_CC_FORCE_INLINE_ bool_t _cc_json_boolean(const _cc_json_t *ctx) {
    if (ctx && ctx->type == _CC_JSON_BOOLEAN_) {
        return ctx->element.uni_boolean;
    }
    return false;
}

/**/
_CC_FORCE_INLINE_ int64_t _cc_json_object_find_number(const _cc_json_t *ctx, const tchar_t *keyword) {
    return _cc_json_number(_cc_json_object_find(ctx, keyword));
}

/**/
_CC_FORCE_INLINE_ float64_t _cc_json_object_find_float(const _cc_json_t *ctx, const tchar_t *keyword) {
    return _cc_json_float(_cc_json_object_find(ctx, keyword));
}

/**/
_CC_FORCE_INLINE_ const _cc_sds_t _cc_json_object_find_string(const _cc_json_t *ctx, const tchar_t *keyword) {
    return _cc_json_string(_cc_json_object_find(ctx, keyword));
}

/**/
_CC_FORCE_INLINE_ _cc_array_t _cc_json_object_find_array(const _cc_json_t *ctx, const tchar_t *keyword) {
    return _cc_json_array(_cc_json_object_find(ctx, keyword));
}

/**/
_CC_FORCE_INLINE_ const _cc_rbtree_t *_cc_json_object_find_object(const _cc_json_t *ctx, const tchar_t *keyword) {
    return _cc_json_object(_cc_json_object_find(ctx, keyword));
}

/**/
_CC_FORCE_INLINE_ bool_t _cc_json_object_find_boolean(const _cc_json_t *ctx, const tchar_t *keyword) {
    return _cc_json_boolean(_cc_json_object_find(ctx, keyword));
}

/**/
_CC_FORCE_INLINE_ int64_t _cc_json_array_find_number(const _cc_json_t *ctx, const uint32_t index) {
    return _cc_json_number(_cc_json_array_find(ctx, index));
}

/**/
_CC_FORCE_INLINE_ float64_t _cc_json_array_find_float(const _cc_json_t *ctx, const uint32_t index) {
    return _cc_json_float(_cc_json_array_find(ctx, index));
}

/**/
_CC_FORCE_INLINE_ const _cc_sds_t _cc_json_array_find_string(const _cc_json_t *ctx, const uint32_t index) {
    return _cc_json_string(_cc_json_array_find(ctx, index));
}

/**/
_CC_FORCE_INLINE_ const _cc_rbtree_t* _cc_json_array_find_object(const _cc_json_t *ctx, const uint32_t index) {
    return _cc_json_object(_cc_json_array_find(ctx, index));
}

/**/
_CC_FORCE_INLINE_ _cc_array_t _cc_json_array_find_array(const _cc_json_t *ctx, const uint32_t index) {
    return _cc_json_array(_cc_json_array_find(ctx, index));
}

/**/
_CC_FORCE_INLINE_ bool_t _cc_json_array_find_boolean(const _cc_json_t *ctx, const uint32_t index) {
    return _cc_json_boolean(_cc_json_array_find(ctx, index));
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_JSON_H_INCLUDED_*/
