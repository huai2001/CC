#include <libcc/alloc.h>
#include <libcc/http.h>

_CC_API_PUBLIC(_cc_http_header_t*) _cc_http_header_alloc(void) {
    _cc_http_header_t *m = (_cc_http_header_t *)_cc_malloc(sizeof(_cc_http_header_t));
    m->keyword = NULL;
    m->value = NULL;
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
    _cc_rbtree_iterator_t **node = &(ctx->rb_node), *parent = NULL;
    while (*node) {
        _cc_http_header_t *m = _cc_upcast(*node, _cc_http_header_t, lnk);
        int32_t result = _tcsicmp(data->keyword, m->keyword);
        if (result == 0) {
            if (m->value) {
                _cc_sds_free(m->value);
            }
            m->value = data->value;
            data->value = NULL;
            _cc_http_header_free(data);
            return true;
        }
        parent = *node;
        node = (result < 0) ? &(parent->left) : &(parent->right);
    }
    _cc_rbtree_insert(ctx, &data->lnk, parent, node);
    return true;
}

_CC_API_PUBLIC(const _cc_http_header_t*) _cc_http_header_find(_cc_rbtree_t *ctx, const tchar_t *keyword) {
    _cc_rbtree_iterator_t *node = ctx->rb_node;

    while (node) {
        _cc_http_header_t *m = _cc_upcast(node, _cc_http_header_t, lnk);
        int32_t result = _tcsicmp(keyword, m->keyword);
        if (result == 0) {
            return m;
        }
        node = (result < 0) ? node->left : node->right;
    }
    return NULL;
}

static void _http_header_free(_cc_rbtree_iterator_t *node) {
    _cc_http_header_free(_cc_upcast(node, _cc_http_header_t, lnk));
}

_CC_API_PUBLIC(void) _cc_http_header_free_all(_cc_rbtree_t *ctx) {
    _cc_assert(ctx != NULL);
    _cc_rbtree_free_all(ctx, _http_header_free);
}

_CC_API_PUBLIC(bool_t) _cc_http_header_line(_cc_rbtree_t *headers, tchar_t *line, int length) {
    _cc_http_header_t *m;
    tchar_t *v;
    tchar_t *n = _tmemchr(line, _T(':'), length);
    tchar_t *v_end = line + length;

    if (n == NULL) {
        return false;
    }

    /* Skip ':' */
    v = n + 1;
    /* Trim trailing spaces from keyword */
    do {
        n--;
    } while (_CC_ISSPACE(*n) && n > line);

    /* Trim trailing spaces from value */
    do {
        v_end--;
    } while (_CC_ISSPACE(*v_end) && v_end > v);

    /* Skip leading spaces from value */
    while (_CC_ISSPACE(*v) && v < v_end) {
        v++;
    }

    if (v > v_end) {
        return false;
    }

    m = (_cc_http_header_t*)_cc_malloc(sizeof(_cc_http_header_t));
    m->keyword = _cc_sds_alloc(line, n - line + 1);
    m->value = _cc_sds_alloc(v, v_end - v + 1);

    return _cc_http_header_push(headers, m);
}

/**/
_CC_API_PUBLIC(int) _cc_http_header_parser(_cc_http_header_fn_t fn, pvoid_t *arg, byte_t *bytes, int32_t *length) {
    byte_t *n;
    byte_t *start = (byte_t*)bytes;
#ifdef _CC_UNICODE_
    int32_t i;
    wchar_t buf[1024 * 4];
#endif
    int result = _CC_HTTP_STATE_HEADER_;
    while (true) {
        n = memchr(start, '\n', *length - (start - bytes));
        if (n == NULL) {
            break;
        }

        if (*(n - 1) == '\r') {
            if ((size_t)(n - start) > 4096) {
                _cc_logger_error("size of header is too bigger");
                return _CC_HTTP_ERROR_TOOLARGE_;
            }
            /*If we received just a CR LF on a line, the headers are finished*/
            if ((n - 1) == start) {
                start = n + 1;
                result = _CC_HTTP_STATE_PAYLOAD_;
                break;
            }
#ifdef _CC_UNICODE_
            i = _cc_utf8_to_utf16((const uint8_t *)start, (const uint8_t *)n, (uint16_t *)buf, (uint16_t *)&buf[_cc_countof(buf)]);
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
