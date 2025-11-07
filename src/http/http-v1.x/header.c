#include <libcc/alloc.h>
#include <libcc/http.h>

_CC_API_PUBLIC(_cc_http_header_t*) _cc_http_header_alloc(void) {
    _cc_http_header_t *m = (_cc_http_header_t *)_cc_malloc(sizeof(_cc_http_header_t));
    m->keyword = nullptr;
    m->value = nullptr;
    return m;
}

_CC_API_PUBLIC(void) _cc_http_header_free(_cc_http_header_t *m) {
    if (m->keyword) {
        _cc_sds_free(m->keyword);
    }
    if (m->value) {
        _cc_sds_free(m->value);
    }
    _cc_free(m);
}

_CC_API_PUBLIC(bool_t) _cc_http_header_push(_cc_rbtree_t *ctx, _cc_http_header_t *data) {
    int32_t result = 0;
    _cc_http_header_t *m = nullptr;

    _cc_rbtree_iterator_t **node = &(ctx->rb_node), *parent = nullptr;
    while (*node) {
        m = _cc_upcast(*node, _cc_http_header_t, lnk);
        result = _tcsicmp(data->keyword, m->keyword);

        parent = *node;

        if (result < 0) {
            node = &((*node)->left);
        } else if (result > 0) {
            node = &((*node)->right);
        } else {
            _cc_http_header_free(data);
            return false;
        }
    }
    _cc_rbtree_insert(ctx, &data->lnk, parent, node);
    return true;
}

_CC_API_PUBLIC(const _cc_http_header_t*) _cc_http_header_find(_cc_rbtree_t *ctx, const tchar_t *keyword) {
    int32_t result = 0;
    _cc_http_header_t *m;
    _cc_rbtree_iterator_t *node = ctx->rb_node;

    while (node) {
        m = _cc_upcast(node, _cc_http_header_t, lnk);
        result = _tcsicmp(keyword, m->keyword);
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

static void _http_header_free(_cc_rbtree_iterator_t *node) {
    _cc_http_header_free(_cc_upcast(node, _cc_http_header_t, lnk));
}

_CC_API_PUBLIC(void) _cc_http_header_destroy(_cc_rbtree_t *ctx) {
    _cc_assert(ctx != nullptr);
    _cc_rbtree_destroy(ctx, _http_header_free);
}

_CC_API_PUBLIC(bool_t) _cc_http_header_line(_cc_rbtree_t *headers, tchar_t *line, int length) {
    int first = 0, last = 0, i = 0;
    _cc_http_header_t *m = (_cc_http_header_t*)_cc_malloc(sizeof(_cc_http_header_t));

    /* Find the first non-space letter */
    _cc_first_index_of(first, length, _cc_isspace(line[first]));
    last = first;
    
    if (line[last] == ':') {
        last++;
    }

    _cc_first_index_of(last, length, line[last] != ':');
    if (line[last] != ':') {
        _cc_free(m);
        return false;
    }

    i = last;
    /*Find the last non-space letter*/
    _cc_last_index_of(first, i, _cc_isspace(line[i]));
    if (i == first) {
        _cc_free(m);
        return false;
    }
    
    m->keyword = _cc_sds_alloc(&line[first], i);
    last += 1;
    /* Find the first non-space letter */
    _cc_first_index_of(last, length, _cc_isspace(line[last]));
    /*Find the last non-space letter*/
    _cc_last_index_of(last, length, _cc_isspace(line[length]));
    if (last == length) {
        _cc_http_header_free(m);
        return false;
    }
    m->value = _cc_sds_alloc(&line[last], length - last);
    return _cc_http_header_push(headers, m);
}

/**/
_CC_API_PUBLIC(int) _cc_http_header_parser(_cc_http_header_fn_t fn, pvoid_t *arg, byte_t *bytes, int32_t *length) {
    byte_t *n;
    byte_t *start = (byte_t*)bytes;
#ifdef _CC_UNICODE_
    int32_t i;
    wchar_t buf[1024];
#endif
    int result = _CC_HTTP_STATUS_HEADER_;
    while (true) {
        n = memchr(start, '\n', *length - (start - bytes));
        if (n == nullptr) {
            break;
        }

        if (*(n - 1) == '\r') {
            if ((size_t)(n - start) > 4096) {
                _cc_logger_error(_T("size of header is too bigger"));
                return _CC_HTTP_ERROR_TOOLARGE_;
            }
            /*If we received just a CR LF on a line, the headers are finished*/
            if ((n - 1) == start) {
                start = n + 1;
                result = _CC_HTTP_STATUS_PAYLOAD_;
                break;
            }
#ifdef _CC_UNICODE_
            i = _cc_utf8_to_utf16((const uint8_t *)start, (const uint8_t *)n, 
                                  (uint16_t *)buf, (uint16_t *)&buf[_cc_countof(buf)]);
            if (!fn(arg, (wchar_t*)buf, i - 1)) {
#else
            if (!fn(arg, (tchar_t*)start, (int32_t)(n - start - 1))) {
#endif
                return _CC_HTTP_ERROR_BADREQUEST_;
            }
            start = n + 1;
        } else {
            return _CC_HTTP_ERROR_BADREQUEST_;
        }
    }

    if (start == bytes) {
        return result;
    }

    *length -= (int32_t)(start - bytes);
    if (*length > 0) {
        memmove(bytes, start, *length);
    }
    return result;
}
