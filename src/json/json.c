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
#include "json.c.h"
#include <cc/math.h>

static const tchar_t* char2escape[256] = {
    _T("\\u0000"), _T("\\u0001"), _T("\\u0002"), _T("\\u0003"), _T("\\u0004"), _T("\\u0005"),
    _T("\\u0006"), _T("\\u0007"), _T("\\b"),     _T("\\t"),     _T("\\n"),     _T("\\u000b"),
    _T("\\f"),     _T("\\r"),     _T("\\u000e"), _T("\\u000f"), _T("\\u0010"), _T("\\u0011"),
    _T("\\u0012"), _T("\\u0013"), _T("\\u0014"), _T("\\u0015"), _T("\\u0016"), _T("\\u0017"),
    _T("\\u0018"), _T("\\u0019"), _T("\\u001a"), _T("\\u001b"), _T("\\u001c"), _T("\\u001d"),
    _T("\\u001e"), _T("\\u001f"), NULL,          NULL,          _T("\\\""),    NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          /*_T("\\/")*/ NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          _T("\\\\"),    NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          _T("\\u007f"), NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,          NULL,          NULL,
    NULL,          NULL,          NULL,          NULL,
};

_CC_API_PRIVATE(void) _json_free_object_rb_node(_cc_rbtree_iterator_t *node);
_CC_API_PRIVATE(void) _json_free_node(_cc_json_t *item);

int32_t _json_push_object(_cc_rbtree_iterator_t *left, _cc_rbtree_iterator_t *right) {
    _cc_json_t *_left = _cc_upcast(left, _cc_json_t, node);
    _cc_json_t *_right = _cc_upcast(right, _cc_json_t, node);

    return _tcscmp(_right->name, _left->name);
}

int32_t _json_get_object(_cc_rbtree_iterator_t *left, pvoid_t args) {
    _cc_json_t *_left = _cc_upcast(left, _cc_json_t, node);

    return _tcscmp((const tchar_t *)args, _left->name);
}

_CC_API_PUBLIC(_cc_json_t*) _cc_json_array_find(const _cc_json_t *ctx, uint32_t index) {
    _cc_assert(ctx != NULL);

    if (ctx->type == _CC_JSON_ARRAY_) {
        return (_cc_json_t *)_cc_array_find(&ctx->object.uni_array, index);
    }

    return NULL;
}

_CC_API_PUBLIC(_cc_json_t*) _cc_json_object_find(const _cc_json_t *ctx, const tchar_t *keyword) {
    _cc_rbtree_iterator_t *node;
    _cc_assert(ctx != NULL);

    if (ctx->type == _CC_JSON_OBJECT_) {
        node = _cc_rbtree_get(&ctx->object.uni_object, (pvoid_t)keyword, _json_get_object);
        if (node) {
            return _cc_upcast(node, _cc_json_t, node);
        }
    }

    return NULL;
}

_CC_API_PRIVATE(void) _json_free_node(_cc_json_t *item) {
    switch (item->type) {
    case _CC_JSON_OBJECT_:
        _cc_rbtree_destroy(&item->object.uni_object, _json_free_object_rb_node);
        break;
    case _CC_JSON_STRING_:
        _cc_free(item->object.uni_string);
        break;
    case _CC_JSON_ARRAY_:
        _destroy_json_array(item);
        break;
    }

    _cc_free(item);
}

_CC_API_PRIVATE(void) _json_free_object_rb_node(_cc_rbtree_iterator_t *node) {
    _cc_json_t *item = _cc_upcast(node, _cc_json_t, node);
    _cc_safe_free(item->name);
    _json_free_node(item);
}

void _destroy_json_array(_cc_json_t *root) {
    uint32_t i;
    uint32_t length;

    length = root->object.uni_array.length;

    for (i = 0; i < length; i++) {
        _cc_json_t *item = (_cc_json_t *)root->object.uni_array.data[i];
        if (item) {
            _json_free_node(item);
        }
    }
    _cc_array_free(&root->object.uni_array);
}

void _destroy_json_object(_cc_json_t *root) {
    _cc_rbtree_destroy(&root->object.uni_object, _json_free_object_rb_node);
}

_CC_API_PUBLIC(bool_t) _cc_destroy_json(_cc_json_t **item) {
    if (_cc_unlikely(item == NULL || *item == NULL)) {
        return false;
    }

    if ((*item)->object.uni_object.rb_node) {
        switch ((*item)->type) {
        case _CC_JSON_OBJECT_:
            _cc_rbtree_destroy(&(*item)->object.uni_object, _json_free_object_rb_node);
            break;
        case _CC_JSON_ARRAY_:
            _destroy_json_array(*item);
            break;
        }
    }

    _cc_free(*item);
    *item = NULL;

    return true;
}

_CC_API_PUBLIC(_cc_json_t*) _cc_json_alloc_object(byte_t type, const tchar_t *keyword) {
    _cc_json_t *item = (_cc_json_t *)_cc_malloc(sizeof(_cc_json_t));
    bzero(item, sizeof(_cc_json_t));
    item->type = type;
    item->object.uni_object.rb_node = NULL;
    if (keyword) {
        item->name = _cc_tcsdup(keyword);
    } else {
        item->name = NULL;
    }

    return item;
}

/**/
_CC_API_PUBLIC(_cc_json_t*) _cc_json_add_boolean(_cc_json_t *ctx, const tchar_t *keyword, bool_t value, bool_t replacement) {
    _cc_json_t *item = _cc_json_alloc_object(_CC_JSON_BOOLEAN_, keyword);
    if (_cc_unlikely(item == NULL)) {
        return NULL;
    }

    item->object.uni_boolean = value;

    if (!_cc_json_object_append(ctx, item, replacement)) {
        _cc_free(item->name);
        _cc_free(item);
        return NULL;
    }

    return item;
}

/**/
_CC_API_PUBLIC(_cc_json_t*) _cc_json_add_number(_cc_json_t *ctx, const tchar_t *keyword, int64_t value, bool_t replacement) {
    _cc_json_t *item = _cc_json_alloc_object(_CC_JSON_INT_, keyword);
    if (_cc_unlikely(item == NULL)) {
        return NULL;
    }

    item->object.uni_int = value;

    if (!_cc_json_object_append(ctx, item, replacement)) {
        _cc_free(item->name);
        _cc_free(item);
        return NULL;
    }

    return item;
}

/**/
_CC_API_PUBLIC(_cc_json_t*) _cc_json_add_float(_cc_json_t *ctx, const tchar_t *keyword, float64_t value, bool_t replacement) {
    _cc_json_t *item = _cc_json_alloc_object(_CC_JSON_FLOAT_, keyword);
    if (_cc_unlikely(item == NULL)) {
        return NULL;
    }

    item->object.uni_float = value;

    if (!_cc_json_object_append(ctx, item, replacement)) {
        _cc_free(item->name);
        _cc_free(item);
        return NULL;
    }

    return item;
}

/**/
_CC_API_PUBLIC(_cc_json_t*) _cc_json_add_string(_cc_json_t *ctx, const tchar_t *keyword, const tchar_t *value, bool_t replacement) {
    _cc_json_t *item = _cc_json_alloc_object(_CC_JSON_STRING_, keyword);
    if (_cc_unlikely(item == NULL)) {
        return NULL;
    }

    item->object.uni_string = _cc_tcsdup(value);

    if (!_cc_json_object_append(ctx, item, replacement)) {
        _cc_free(item->object.uni_string);
        _cc_free(item->name);
        _cc_free(item);
        return NULL;
    }

    return item;
}
/**/
_CC_API_PUBLIC(bool_t) _cc_json_object_remove(_cc_json_t *ctx, const tchar_t *keyword) {
    _cc_rbtree_iterator_t *node;
    if (ctx->type != _CC_JSON_OBJECT_) {
        return false;
    }

    node = _cc_rbtree_get(&ctx->object.uni_object, (pvoid_t)keyword, _json_get_object);
    if (node == NULL) {
        return false;
    }

    _cc_rbtree_erase(&ctx->object.uni_object, node);
    _json_free_object_rb_node(node);
    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_json_array_remove(_cc_json_t *ctx, const uint32_t index) {
    _cc_json_t *item;
    if (ctx->type != _CC_JSON_ARRAY_) {
        return false;
    }

    item = (_cc_json_t *)_cc_array_remove(&ctx->object.uni_array, index);
    if (item == NULL) {
        return false;
    }
    _json_free_node(item);

    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_json_object_append(_cc_json_t *ctx, _cc_json_t *item, bool_t replacement) {
    _cc_rbtree_t *root;
    int32_t result;
    _cc_rbtree_iterator_t **node;
    _cc_rbtree_iterator_t *parent = NULL;

    if (_cc_unlikely(ctx == NULL || item == NULL)) {
        return false;
    }

    if (_cc_unlikely(ctx == item)) {
        return true;
    }

    if (ctx->type == _CC_JSON_ARRAY_) {
        return _cc_array_push(&ctx->object.uni_array, item);
    }
    if (ctx->type != _CC_JSON_OBJECT_) {
        return false;
    }

    root = &ctx->object.uni_object;
    node = &(root->rb_node);

    while (*node) {
        result = _json_push_object(*node, &item->node);

        parent = *node;

        if (result < 0) {
            node = &((*node)->left);
        } else if (result > 0) {
            node = &((*node)->right);
        } else {
            if (replacement) {
                _cc_rbtree_replace_node(root, parent, &item->node);
                /*free old json object*/
                _json_free_object_rb_node(parent);
                return true;
            }
            return false;
        }
    }

    _cc_rbtree_insert(root, &item->node, parent, node);
    return true;
}
/**/
_CC_API_PRIVATE(bool_t) _buf_char_put(_cc_buf_t *ctx, const tchar_t data) {
    return _cc_buf_write(ctx, (pvoid_t)&data, sizeof(tchar_t));
}
/**/
_CC_API_PRIVATE(void) _cc_print_json_array(_cc_json_t *item, _cc_buf_t *buf, int32_t depth);
/**/
_CC_API_PRIVATE(void) _cc_print_json_object(_cc_json_t *item, _cc_buf_t *buf, int32_t depth);

_CC_API_PRIVATE(void) _cc_print_json_string(const tchar_t *output, _cc_buf_t *buf) {
    const tchar_t *p = output;
    size_t len;
    /* numbers of additional characters needed for escaping */
    size_t escape_characters = 0;

    /* empty string */
    if (output == NULL) {
        _cc_buf_putts(buf, _T("\"\""));
        return;
    }

    /* set "flag" to 1 if something needs to be escaped */
    for (len = 0; *p; p++, len++) {
        if (((int32_t)*p) < 256 && char2escape[(byte_t)*p] != NULL) {
            escape_characters++;
        }
    }

    _buf_char_put(buf, _T('"'));
    /* no characters have to be escaped */
    if (escape_characters == 0) {
        _cc_buf_write(buf, (const pvoid_t)output, sizeof(tchar_t) * len);
    } else {
        /* set "flag" to 1 if something needs to be escaped */
        for (p = output; *p; p++) {
            const tchar_t *pp = char2escape[(uchar_t)(*p)];
            if (pp && ((int32_t)*pp) < 256) {
                _cc_buf_putts(buf, pp);
            } else {
                _buf_char_put(buf, *p);
            }
        }
    }
    _buf_char_put(buf, _T('"'));
}

/**/
_CC_API_PRIVATE(void) _cc_print_json_value(_cc_json_t *item, _cc_buf_t *buf, int32_t depth) {
    switch (item->type) {
    case _CC_JSON_NULL_: {
        _cc_buf_write(buf, (const pvoid_t) _T("null"), 4);
    } break;
    case _CC_JSON_BOOLEAN_: {
        if (item->object.uni_boolean) {
            _cc_buf_write(buf, (const pvoid_t) _T("true"), 4);
        } else {
            _cc_buf_write(buf, (const pvoid_t) _T("false"), 5);
        }
    } break;
    case _CC_JSON_FLOAT_: {
        int length = 0;
        double test = 0;
        /* temporary buffer to print the number into */
        tchar_t number_buffer[26];

        /* This checks for NaN and Infinity */
        if ((item->object.uni_float * 0) != 0) {
            length = _stprintf(number_buffer, _T("null"));
        } else {
            /* Try 15 decimal places of precision to avoid nonsignificant
             * nonzero digits */
            length = _stprintf(number_buffer, _T("%1.15g"), item->object.uni_float);

            /* Check whether the original double can be recovered */
            if ((_stscanf(number_buffer, _T("%lg"), &test) != 1) || (test != item->object.uni_float)) {
                /* If not, print with 17 decimal places of precision */
                length = _stprintf(number_buffer, _T("%1.17g"), item->object.uni_float);
            }
        }

        /* sprintf failed or buffer overrun occured */
        if ((length < 0) || (length > (int)(sizeof(number_buffer) - 1))) {
            _stprintf(number_buffer, _T("null"));
        }

        number_buffer[length - 1] = 0;
        _cc_buf_write(buf, number_buffer, sizeof(tchar_t) * (length - 1));
    } break;
    case _CC_JSON_INT_: {
        _cc_buf_puttsf(buf, _T("%lld"), (long long)item->object.uni_int);
    } break;
    case _CC_JSON_OBJECT_: {
        _cc_print_json_object(item, buf, depth + 1);
    } break;
    case _CC_JSON_ARRAY_: {
        _cc_print_json_array(item, buf, depth);
    } break;
    case _CC_JSON_STRING_: {
        _cc_print_json_string(item->object.uni_string, buf);
    } break;
    default:
        break;
    }
}

/**/
_CC_API_PRIVATE(void) _cc_print_json_array(_cc_json_t *root, _cc_buf_t *buf, int32_t depth) {
    uint32_t i, length;

    _buf_char_put(buf, _JSON_ARRAY_START_);
    length = root->object.uni_array.length;

    for (i = 0; i < length; i++) {
        _cc_print_json_value((_cc_json_t *)root->object.uni_array.data[i], buf, depth);
        _buf_char_put(buf, _JSON_NEXT_TOKEN_);
    }

    if (length > 0) {
        buf->length -= 1;
    }

    _buf_char_put(buf, _JSON_ARRAY_END_);
}

/**/
_CC_API_PRIVATE(void) _cc_print_json_object(_cc_json_t *root, _cc_buf_t *buf, int32_t depth) {
    _cc_json_t *item = NULL;
    _cc_rbtree_iterator_t *next, *head;
    tchar_t depth_buf[1024];

    int32_t i;
    _buf_char_put(buf, _JSON_OBJECT_START_);
    _buf_char_put(buf, _CC_LF_);

    depth &= 1023;

    head = next = _cc_rbtree_first(&root->object.uni_object);
    while (next) {
        item = _cc_upcast(next, _cc_json_t, node);

        for (i = 0; i < depth; i++) {
            depth_buf[i] = _T('\t');
        }

        depth_buf[i] = 0;

        _cc_buf_write(buf, depth_buf, sizeof(tchar_t) * depth);

        _cc_print_json_string(item->name, buf);
        _cc_buf_write(buf, _T(": "), 2 * sizeof(tchar_t));

        _cc_print_json_value(item, buf, depth);

        _buf_char_put(buf, _JSON_NEXT_TOKEN_);
        _buf_char_put(buf, _CC_LF_);

        next = _cc_rbtree_next(next);
    }

    if (head) {
        buf->length -= 2;
        _buf_char_put(buf, _CC_LF_);
    }

    depth -= 1;
    for (i = 0; i < depth; i++) {
        depth_buf[i] = _T('\t');
    }
    depth_buf[depth++] = _JSON_OBJECT_END_;
    depth_buf[depth] = 0;
    _cc_buf_write(buf, depth_buf, sizeof(tchar_t) * depth);
}

_CC_API_PUBLIC(_cc_buf_t*) _cc_print_json(_cc_json_t *item) {
    _cc_buf_t *buf;

    if (item == NULL) {
        return NULL;
    }

    buf = _cc_create_buf(16*1024);
    if (_cc_likely(buf)) {
        _cc_print_json_value(item, buf, 0);
    }
    return buf;
}
