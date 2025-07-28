/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
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
#ifndef _C_CC_JSON_H_INCLUDED_
#define _C_CC_JSON_H_INCLUDED_

#include "dylib.h"

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
    /**/
    size_t length;
    /**/
    size_t size;

    tchar_t *name;

    union {
        bool_t uni_boolean;
        float64_t uni_float;
        int64_t uni_int;
        _cc_rbtree_t uni_object;
        _cc_json_t **uni_array;
        tchar_t *uni_string;
    } element;

    _cc_rbtree_iterator_t lnk;

};

#define _CC_JSON_RETURN_VALUE(O, T, V)                                                                                 \
    do {                                                                                                               \
        if (O && O->type == T) {                                                                                       \
            return O->element.V;                                                                                        \
        }                                                                                                              \
    } while (0)

/**
 * @brief Create a JSON object
 *
 * @param type JSON Type =_CC_JSON_TYPES_
 * @param keyword The keyword of the object
 *
 * @return  JSON object
 */
_CC_WIDGETS_API(_cc_json_t *)
_cc_json_alloc_object(byte_t type, const tchar_t *keyword);
/**
 * @brief Create a JSON object
 *
 * @param keyword The keyword of the object
 * @param size JSON array size
 *
 * @return  JSON Array
 */
_CC_WIDGETS_API(_cc_json_t*) 
_cc_json_alloc_array(const tchar_t *keyword, size_t size);
/**
 * @brief Add an object to JSON
 *
 * @param ctx JSON object
 * @param j JSON object
 *
 * @return true if successful or false on error.
 */
_CC_WIDGETS_API(bool_t) _cc_json_array_push(_cc_json_t *ctx, _cc_json_t *j);
/**
 * @brief Add an object to JSON
 *
 * @param ctx JSON object
 * @param j JSON object
 * @param replacement Replace with a new one if it is 'true'
 *
 * @return true if successful or false on error.
 */
_CC_WIDGETS_API(bool_t)
_cc_json_object_push(_cc_json_t *ctx, _cc_json_t *j, bool_t replacement);

/**
 * @brief Add an boolen to JSON
 *
 * @param ctx JSON object
 * @param keyword The keyword of the object
 * @param value object value
 *
 * @return JSON object
 */
_CC_WIDGETS_API(_cc_json_t *)
_cc_json_add_boolean(_cc_json_t *ctx, const tchar_t *keyword, bool_t value);

/**
 * @brief Add an number to JSON
 *
 * @param ctx JSON object
 * @param keyword The keyword of the object
 * @param value object value
 *
 * @return JSON object
 */
_CC_WIDGETS_API(_cc_json_t *)
_cc_json_add_number(_cc_json_t *ctx, const tchar_t *keyword, int64_t value);

/**
 * @brief Add an double to JSON
 *
 * @param ctx JSON object
 * @param keyword The keyword of the object
 * @param value object value
 * @param replacement Replace with a new one if it is 'true'
 *
 * @return JSON object
 */
_CC_WIDGETS_API(_cc_json_t *)
_cc_json_add_float(_cc_json_t *ctx, const tchar_t *keyword, float64_t value);

/**
 * @brief Add an string to JSON
 *
 * @param ctx JSON object
 * @param keyword The keyword of the object
 * @param value object value
 * @param replacement Replace with a new one if it is 'true'
 *
 * @return JSON object
 */
_CC_WIDGETS_API(_cc_json_t *)
_cc_json_add_string(_cc_json_t *ctx, const tchar_t *keyword, const tchar_t *value);
/**
 * @brief remove  JSON
 *
 * @param ctx JSON object
 * @param index array index
 *
 * @return true if successful or false on error.
 */
_CC_WIDGETS_API(bool_t) _cc_json_array_remove(_cc_json_t *ctx, const uint32_t index);
/**
 * @brief remove  JSON
 *
 * @param ctx JSON object
 * @param keyword The keyword of the object
 *
 * @return true if successful or false on error.
 */
_CC_WIDGETS_API(bool_t) _cc_json_object_remove(_cc_json_t *ctx, const tchar_t *keyword);
/**
 * @brief Print JSON Object to buffer
 *
 * @param ctx JSON object
 *
 * @return Buf context
 */
_CC_WIDGETS_API(_cc_buf_t *) _cc_json_dump(_cc_json_t *jctx);

/**
 * @brief Open JSON file
 *
 * @param file_name JSON file path
 *
 * @return JSON object
 */
_CC_WIDGETS_API(_cc_json_t *) _cc_json_from_file(const tchar_t *file_name);

/**
 * @brief Parse JSON string
 *
 * @param src JSON string
 *
 * @return JSON object
 */
_CC_WIDGETS_API(_cc_json_t *) _cc_json_parse(const tchar_t *src, size_t length);
/**
 * @brief Parse JSON
 *
 * @return JSON object
 */
_CC_WIDGETS_API(_cc_json_t *) _cc_josn_parser(_cc_sbuf_t *const buffer);
/**
 * @brief Destroy JSON object
 *
 * @param ctx _cc_json_t structure
 *
 */
_CC_WIDGETS_API(void) _cc_destroy_json(_cc_json_t **ctx);
/**
 * @brief Get JSON Parse error message
 *
 * @return JSON error message string
 */
_CC_WIDGETS_API(const tchar_t *) _cc_json_error(void);
/**
 * @brief Find JSON Object
 *
 * @param ctx JSON Root
 * @param keyword Keywords for JSON objects
 *
 * @return _cc_json_t structure
 */
_CC_WIDGETS_API(_cc_json_t *)
_cc_json_object_find(const _cc_json_t *ctx, const tchar_t *keyword);
/**
 * @brief Find JSON Array
 *
 * @param ctx JSON Root
 * @param index Index for JSON Array
 *
 * @return _cc_json_t structure
 */
_CC_WIDGETS_API(_cc_json_t *)
_cc_json_array_find(const _cc_json_t *ctx, uint32_t index);
/**
 * @brief Get JSON Number Value
 *
 * @param ctx JSON Object
 *
 * @return Number
 */
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

/**
 * @brief Get JSON Double Value
 *
 * @param ctx JSON Object
 *
 * @return double
 */
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

/**
 * @brief Get JSON String Value
 *
 * @param ctx JSON Object
 *
 * @return String
 */
_CC_FORCE_INLINE_ const tchar_t *_cc_json_string(const _cc_json_t *ctx) {
    _CC_JSON_RETURN_VALUE(ctx, _CC_JSON_STRING_, uni_string);
    return nullptr;
}

/**
 * @brief Get JSON Object
 *
 * @param ctx JSON Object
 *
 * @return _cc_rbtree_t
 */
_CC_FORCE_INLINE_ const _cc_rbtree_t *_cc_json_object(const _cc_json_t *ctx) {
    if (ctx && ctx->type == _CC_JSON_OBJECT_) {
        return &ctx->element.uni_object;
    }
    return nullptr;
}
/**
 * @brief Get JSON Boolen
 *
 * @param ctx JSON Object
 *
 * @return Boolen
 */
_CC_FORCE_INLINE_ bool_t _cc_json_boolean(const _cc_json_t *ctx) {
    _CC_JSON_RETURN_VALUE(ctx, _CC_JSON_BOOLEAN_, uni_boolean);
    return false;
}
/**
 * @brief Find JSON Number Value
 *
 * @param ctx JSON Object
 * @param keyword The keyword of the object
 *
 * @return Number Value
 */
_CC_FORCE_INLINE_ int64_t _cc_json_object_find_number(const _cc_json_t *ctx, const tchar_t *keyword) {
    return _cc_json_number(_cc_json_object_find(ctx, keyword));
}

/**
 * @brief Find JSON float value
 *
 * @param ctx JSON Object
 * @param keyword The keyword of the object
 *
 * @return float value
 */
_CC_FORCE_INLINE_ float64_t _cc_json_object_find_float(const _cc_json_t *ctx, const tchar_t *keyword) {
    return _cc_json_float(_cc_json_object_find(ctx, keyword));
}

/**
 * @brief Find JSON string value
 *
 * @param ctx JSON Object
 * @param keyword The keyword of the object
 *
 * @return string value
 */
_CC_FORCE_INLINE_ const tchar_t *_cc_json_object_find_string(const _cc_json_t *ctx, const tchar_t *keyword) {
    return _cc_json_string(_cc_json_object_find(ctx, keyword));
}

/**
 * @brief Find the JSON array of the JSON object
 *
 * @param ctx JSON Object
 * @param keyword The keyword of the object
 *
 * @return JSON Array
 */
_CC_FORCE_INLINE_ const _cc_json_t *_cc_json_object_find_array(const _cc_json_t *ctx, const tchar_t *keyword) {
    return _cc_json_object_find(ctx, keyword);
}

/**
 * @brief Find the JSON Object of the JSON object
 *
 * @param ctx JSON Object
 * @param keyword The keyword of the object
 *
 * @return Object
 */
_CC_FORCE_INLINE_ const _cc_rbtree_t *_cc_json_object_find_object(const _cc_json_t *ctx, const tchar_t *keyword) {
    return _cc_json_object(_cc_json_object_find(ctx, keyword));
}

/**
 * @brief Find the boolen of the JSON object
 *
 * @param ctx JSON Object
 * @param keyword The keyword of the object
 *
 * @return boolen
 */
_CC_FORCE_INLINE_ bool_t _cc_json_object_find_boolean(const _cc_json_t *ctx, const tchar_t *keyword) {
    return _cc_json_boolean(_cc_json_object_find(ctx, keyword));
}

/**
 * @brief Find the number of the JSON array
 *
 * @param ctx JSON Object
 * @param index The Index of the Array
 *
 * @return number
 */
_CC_FORCE_INLINE_ int64_t _cc_json_array_find_number(const _cc_json_t *ctx, const uint32_t index) {
    return _cc_json_number(_cc_json_array_find(ctx, index));
}

/**
 * @brief Find the double of the JSON array
 *
 * @param ctx JSON Object
 * @param index The Index of the Array
 *
 * @return double
 */
_CC_FORCE_INLINE_ float64_t _cc_json_array_find_float(const _cc_json_t *ctx, const uint32_t index) {
    return _cc_json_float(_cc_json_array_find(ctx, index));
}

/**
 * @brief Find the string of the JSON array
 *
 * @param ctx JSON Object
 * @param index The Index of the Array
 *
 * @return string
 */
_CC_FORCE_INLINE_ const tchar_t *_cc_json_array_find_string(const _cc_json_t *ctx, const uint32_t index) {
    return _cc_json_string(_cc_json_array_find(ctx, index));
}

/**
 * @brief Find the JSON object of the JSON array
 *
 * @param 1 JSON Object
 * @param 2 The Index of the Array
 *
 * @return JSON object
 */
_CC_FORCE_INLINE_ const _cc_rbtree_t *_cc_json_array_find_object(const _cc_json_t *ctx, const uint32_t index) {
    return _cc_json_object(_cc_json_array_find(ctx, index));
}

/**
 * @brief Find the JSON array of the JSON array
 *
 * @param ctx JSON Object
 * @param index The Index of the Array
 *
 * @return JSON array
 */
_CC_FORCE_INLINE_ const _cc_json_t *_cc_json_array_find_array(const _cc_json_t *ctx, const uint32_t index) {
    return _cc_json_array_find(ctx, index);
}

/**
 * @brief Find the boolen array of the JSON array
 *
 * @param ctx JSON Object
 * @param index The Index of the Array
 *
 * @return boolen array
 */
_CC_FORCE_INLINE_ bool_t _cc_json_array_find_boolean(const _cc_json_t *ctx, const uint32_t index) {
    return _cc_json_boolean(_cc_json_array_find(ctx, index));
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_JSON_H_INCLUDED_*/
