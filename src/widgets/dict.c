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
#include <cc/widgets/dict.h>

_CC_API_PRIVATE(_cc_dict_node_t*) _alloc_dict_node(_cc_dict_node_t *ctx, tchar_t *name, tchar_t *value) {
    if (ctx == NULL) {
        ctx = (_cc_dict_node_t *)_cc_malloc(sizeof(_cc_dict_node_t));
        ctx->name = NULL;
        ctx->value = NULL;
    }

    if (name) {
        if (ctx->name) {
            _cc_free(ctx->name);
        }
        ctx->name = name;
    }

    if (value) {
        if (ctx->value) {
            _cc_free(ctx->value);
        }
        ctx->value = value;
    }

    // printf("%s = %s\n",ctx->name,ctx->value);
    return ctx;
}

_CC_API_PRIVATE(tchar_t*) _dict_read(_cc_sbuf_tchar_t *const buffer, const tchar_t delimiter) {
    const tchar_t *start = NULL;
    const tchar_t *ended = NULL;
    const tchar_t *p = NULL;
    size_t alloc_length = 0;
    size_t skipped_bytes = 0;
    int32_t convert_bytes = 0;

    tchar_t *output = NULL;
    tchar_t *cps = NULL;

    if (!_cc_buf_jump_comments(buffer)) {
        return NULL;
    }

    ended = start = _cc_sbuf_offset(buffer);

    while (ended && ((size_t)(ended - buffer->content) < buffer->length) && (*ended) != delimiter) {
        if (*ended == '\\') {
            if ((size_t)((ended + 1) - buffer->content) >= buffer->length) {
                goto READ_VALUE_FAIL;
            }
            skipped_bytes++;
            ended++;
        }
        ended++;
    }

    if (ended && (((size_t)(ended - buffer->content) > buffer->length) || (*ended) != delimiter)) {
        goto READ_VALUE_FAIL;
    }

    /*remove trailing whitespace*/
    p = ended - 1;
    while (start < p && _cc_isspace(*p)) {
        p--;
    }
    p++;

    /* This is at most how much we need for the output */
    alloc_length = sizeof(tchar_t) * ((size_t)(p - start) - skipped_bytes + 1);
    cps = output = (tchar_t *)_cc_malloc(alloc_length);

    while (start < p) {
        if (*start != _T('\\')) {
            if (*start == '%' && (start + 3) < p && isxdigit((int)*(start + 1)) && isxdigit((int)*(start + 2))) {
                *cps++ = (tchar_t)_cc_hex2(start + 1);
                start += 3;
            } else {
                *cps++ = *start++;
            }
            continue;
        }

        start++;
        switch (*start) {
        case _T('b'):
            *cps++ = _T('\b');
            start++;
            break;
        case _T('f'):
            *cps++ = _T('\f');
            start++;
            break;
        case _T('n'):
            *cps++ = _T('\n');
            start++;
            break;
        case _T('r'):
            *cps++ = _T('\r');
            start++;
            break;
        case _T('t'):
            *cps++ = _T('\t');
            start++;
            break;
        case _T('\"'):
        case _T('\\'):
        case _T('/'): {
            *cps++ = *start++;
        } break;
        /* UTF-16 literal */
        case _T('u'): {
            /* failed to convert UTF16-literal to UTF-8 */
            start++;
            convert_bytes = _cc_convert_utf16_literal_to_utf8(&start, p, cps, alloc_length);
            if (_cc_unlikely(convert_bytes == 0)) {
                goto READ_VALUE_FAIL;
            }
            cps += convert_bytes;
            break;
        }
        default:
            goto READ_VALUE_FAIL;
            break;
        }
    }
    /* zero terminate the output */
    *cps = 0;

    buffer->offset = (size_t)(ended - buffer->content);
    buffer->offset++;

    return output;

READ_VALUE_FAIL:
    return NULL;
}

_CC_API_PRIVATE(bool_t) _dict_decoding(_cc_dict_t *ctx, _cc_sbuf_tchar_t *const buffer, const tchar_t delimiter) {
    tchar_t *name = NULL;
    tchar_t *value = NULL;

    do {
        name = _dict_read(buffer, _T('='));
        if (name == NULL || !(_cc_sbuf_access(buffer) && (*_cc_sbuf_offset_at(buffer, -1) == _T('=')))) {
            break;
        }
        value = _dict_read(buffer, delimiter);
        if (value == NULL) {
            _cc_free(name);
            break;
        }
        _cc_dict_insert(ctx, name, value);
    } while (_cc_sbuf_access(buffer) && (*_cc_sbuf_offset_at(buffer, -1) == delimiter));

    return true;
}

_CC_API_PUBLIC(bool_t) _cc_dict_split(_cc_dict_t *ctx, const tchar_t *src, const tchar_t delimiter) {
    _cc_sbuf_tchar_t buffer;

    buffer.content = src;
    buffer.length = _tcslen(src);
    buffer.offset = 0;
    buffer.line = 1;

    return _dict_decoding(ctx, &buffer, delimiter);
}

_CC_API_PRIVATE(void) _traverse_dict_join(_cc_rbtree_iterator_t *node, pvoid_t args, const tchar_t delimiter) {
    _cc_buf_t *buf;
    _cc_dict_node_t *item;

    if (node->left) {
        _traverse_dict_join(node->left, args, delimiter);
    }

    if (node->right) {
        _traverse_dict_join(node->right, args, delimiter);
    }

    item = _cc_upcast(node, _cc_dict_node_t, node);
    buf = (_cc_buf_t *)args;

    _cc_buf_puttsf(buf, _T("%s=%s%c"), item->name, item->value, delimiter);
}

_CC_API_PUBLIC(_cc_buf_t*) _cc_dict_join(_cc_dict_t *ctx, const tchar_t delimiter) {
    tchar_t *s;
    size_t len;
    _cc_buf_t *buf;

    buf = _cc_create_buf(1024);
    if (buf == NULL) {
        return NULL;
    }

    if (ctx->rb_node) {
        _traverse_dict_join(ctx->rb_node, buf, delimiter);
    }

    s = (tchar_t *)_cc_buf_bytes(buf);
    len = _cc_buf_length(buf);

    if (*(s + len - 1) == delimiter) {
        *(s + len - 1) = 0;
    }

    return buf;
}

_CC_API_PUBLIC(bool_t) _cc_dict_insert(_cc_dict_t *ctx, tchar_t *name, tchar_t *value) {
    int32_t result = 0;
    _cc_dict_node_t *item = NULL;
    _cc_rbtree_iterator_t **node = &(ctx->rb_node), *parent = NULL;
    while (*node) {
        item = _cc_upcast(*node, _cc_dict_node_t, node);
        result = _tcsicmp(name, item->name);

        parent = *node;

        if (result < 0) {
            node = &((*node)->left);
        } else if (result > 0) {
            node = &((*node)->right);
        } else {
            // printf("%s = %s\n",item->name,item->value);
            _alloc_dict_node(item, NULL, value);
            _cc_free(name);
            return true;
        }
    }

    item = _alloc_dict_node(NULL, name, value);
    if (item == NULL) {
        _cc_free((pvoid_t)name);
        _cc_free((pvoid_t)value);
        return false;
    }

    _cc_rbtree_insert(ctx, &item->node, parent, node);
    return true;
}

_CC_API_PUBLIC(const tchar_t*) _cc_dict_find(_cc_dict_t *ctx, const tchar_t *name) {
    int32_t result = 0;
    _cc_dict_node_t *item = NULL;
    _cc_rbtree_iterator_t *node = ctx->rb_node;

    while (node) {
        item = _cc_upcast(node, _cc_dict_node_t, node);
        result = _tcsicmp(name, item->name);
        if (result < 0) {
            node = node->left;
        } else if (result > 0) {
            node = node->right;
        } else {
            return item->value;
        }
    }
    return NULL;
}

_CC_API_PRIVATE(void) _dict_free(_cc_rbtree_iterator_t *node) {
    _cc_dict_node_t *item = _cc_upcast(node, _cc_dict_node_t, node);
    _cc_free((pvoid_t)item->name);
    _cc_free((pvoid_t)item->value);
    _cc_free(item);
}

_CC_API_PUBLIC(bool_t) _cc_dict_free(_cc_dict_t *ctx) {
    if (ctx == NULL) {
        return false;
    }

    _cc_rbtree_destroy(ctx, _dict_free);
    return true;
}
