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
#include <libcc/widgets/map.h>
#include <libcc/UTF.h>

_CC_API_PUBLIC(_cc_map_element_t*) _cc_map_element_alloc(byte_t type) {
    _cc_map_element_t *m = (_cc_map_element_t *)_cc_malloc(sizeof(_cc_map_element_t));
    m->name = nullptr;
    m->element.uni_string = nullptr;
    m->type = type;
    return m;
}

_CC_API_PUBLIC(void) _cc_map_element_free(_cc_map_element_t *m) {
    _cc_free((pvoid_t)m->name);

    if (m->element.uni_string && m->type == _CC_MAP_STRING_) {
        _cc_free((pvoid_t)m->element.uni_string);
    }

    _cc_free(m);
}

_CC_API_PRIVATE(tchar_t*) _map_read(_cc_sbuf_t *const buffer, const tchar_t delimiter) {
    const tchar_t *start = nullptr;
    const tchar_t *ended = nullptr;
    const tchar_t *p = nullptr;
    size_t alloc_length = 0;
    size_t skipped_bytes = 0;
    int32_t convert_bytes = 0;

    tchar_t *output = nullptr;
    tchar_t *cps = nullptr;

    if (!_cc_buf_jump_comment(buffer)) {
        return nullptr;
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
    return nullptr;
}

_CC_API_PRIVATE(bool_t) _map_decoding(_cc_map_t *ctx, _cc_sbuf_t *const buffer, const tchar_t delimiter) {
    _cc_map_element_t *m;

    do {
        m = _cc_map_element_alloc(_CC_MAP_STRING_);
        m->name = _map_read(buffer, _T('='));
        if (m->name == nullptr || !(_cc_sbuf_access(buffer) && (*_cc_sbuf_offset_at(buffer, -1) == _T('=')))) {
            _cc_map_element_free(m);
            break;
        }
        m->element.uni_string = _map_read(buffer, delimiter);
        if (m->element.uni_string == nullptr) {
            _cc_map_element_free(m);
            break;
        }
        _cc_map_push(ctx, m);
    } while (_cc_sbuf_access(buffer) && (*_cc_sbuf_offset_at(buffer, -1) == delimiter));

    return true;
}

_CC_API_PUBLIC(bool_t) _cc_map_split(_cc_map_t *ctx, const tchar_t *src, const tchar_t delimiter) {
    _cc_sbuf_t buffer;

    buffer.content = src;
    buffer.length = _tcslen(src);
    buffer.offset = 0;
    buffer.line = 1;

    return _map_decoding(ctx, &buffer, delimiter);
}

static void _traverse_map_join(_cc_rbtree_iterator_t *node, pvoid_t args, const tchar_t delimiter) {
    _cc_buf_t *buf = (_cc_buf_t *)args;
    _cc_map_element_t *m;

    if (node->left) {
        _traverse_map_join(node->left, args, delimiter);
    }

    if (node->right) {
        _traverse_map_join(node->right, args, delimiter);
    }

    m = _cc_upcast(node, _cc_map_element_t, lnk);
    switch(m->type) {
    case _CC_MAP_STRING_:
        _cc_buf_appendf(buf, _T("%s=%s%c"), m->name, m->element.uni_string, delimiter);
        break;
    case _CC_MAP_INT_:
        _cc_buf_appendf(buf, _T("%s=%lld%c"), m->name, m->element.uni_int, delimiter);
        break;
    case _CC_MAP_FLOAT_:
        _cc_buf_appendf(buf, _T("%s=%lld%c"), m->name, m->element.uni_float, delimiter);
        break;
    }
}

_CC_API_PUBLIC(_cc_buf_t*) _cc_map_join(_cc_map_t *ctx, const tchar_t delimiter) {
    tchar_t *s;
    size_t len;
    _cc_buf_t *buf;

    buf = _cc_create_buf(1024);
    if (buf == nullptr) {
        return nullptr;
    }

    if (ctx->rb_node) {
        _traverse_map_join(ctx->rb_node, buf, delimiter);
    }

    s = (tchar_t *)_cc_buf_bytes(buf);
    len = _cc_buf_length(buf);

    if (*(s + len - 1) == delimiter) {
        *(s + len - 1) = 0;
    }

    return buf;
}

_CC_API_PUBLIC(bool_t) _cc_map_push(_cc_map_t *ctx, _cc_map_element_t *data) {
    int32_t result = 0;
    _cc_map_element_t *m = nullptr;

    _cc_rbtree_iterator_t **node = &(ctx->rb_node), *parent = nullptr;
    while (*node) {
        m = _cc_upcast(*node, _cc_map_element_t, lnk);
        result = _tcsicmp(data->name, m->name);

        parent = *node;

        if (result < 0) {
            node = &((*node)->left);
        } else if (result > 0) {
            node = &((*node)->right);
        } else {
            _cc_map_element_free(data);
            return false;
        }
    }
    _cc_rbtree_insert(ctx, &data->lnk, parent, node);
    return true;
}

_CC_API_PUBLIC(const _cc_map_element_t*) _cc_map_find(_cc_map_t *ctx, const tchar_t *name) {
    int32_t result = 0;
    _cc_map_element_t *m;
    _cc_rbtree_iterator_t *node = ctx->rb_node;

    while (node) {
        m = _cc_upcast(node, _cc_map_element_t, lnk);
        result = _tcsicmp(name, m->name);
        if (result < 0) {
            node = node->left;
        } else if (result > 0) {
            node = node->right;
        } else {
            return m;
        }
    }
    return nullptr;
}

static void _map_free(_cc_rbtree_iterator_t *node) {
    _cc_map_element_free(_cc_upcast(node, _cc_map_element_t, lnk));
}

_CC_API_PUBLIC(void) _cc_map_free(_cc_map_t *ctx) {
    _cc_assert(ctx != nullptr);
    _cc_rbtree_destroy(ctx, _map_free);
}
