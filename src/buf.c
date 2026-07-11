#include <libcc/alloc.h>
#include <libcc/buf.h>
#include <libcc/file.h>
#include <libcc/string.h>
#include <libcc/UTF.h>

/* Skips spaces and comments as many as possible.*/
_CC_API_PUBLIC(bool_t) _cc_buf_jump_comment(_cc_sbuf_t *const buffer) {
    register const tchar_t *p = NULL;
    /*if ((buffer == NULL) || (buffer->content == NULL)) {
        return false;
    }*/
    while (_cc_sbuf_access(buffer)) {
        p = _cc_sbuf_offset(buffer);
        /*Whitespace characters.*/
        if (*p <= 32) {
            if (*p == _T(_CC_LF_)) {
                buffer->line++;
            }

            buffer->offset++;
        } else if (*p == _T('/')) {
            p++;
            /*double-slash comments, to end of line.*/
            if (*p == _T('/')) {
                buffer->offset += 2;
                while (_cc_sbuf_access(buffer)) {
                    if (_cc_sbuf_offset_equal(buffer, _T(_CC_LF_))) {
                        buffer->offset++;
                        buffer->line++;
                        break;
                    }
                    buffer->offset++;
                }
                /*multiline comments.*/
            } else if (*p == _T('*')) {
                buffer->offset += 2;
                while (_cc_sbuf_access(buffer)) {
                    p = _cc_sbuf_offset(buffer);
                    if ((*p == _T('*') && *(p + 1) == _T('/'))) {
                        /*skip '*' and '/' */
                        buffer->offset += 2;
                        break;
                    }

                    if (*p == _T(_CC_LF_)) {
                        buffer->line++;
                    }

                    buffer->offset++;
                }
            } else
                break;
        } else
            break;
    }

    return _cc_sbuf_access(buffer);
}

_CC_API_PUBLIC(bool_t) _cc_alloc_buf(_cc_buf_t *ctx, size_t initial) {
    _cc_assert(ctx != NULL);

    memset(ctx, 0, sizeof(_cc_buf_t));
    ctx->limit = initial;
    ctx->length = 0;
    ctx->bytes = (byte_t*)_cc_calloc(ctx->limit,sizeof(byte_t));

    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_free_buf(_cc_buf_t *ctx) {
    _cc_assert(ctx != NULL);

    if (_cc_likely(ctx->bytes)) {
        _cc_free(ctx->bytes);
        ctx->bytes = NULL;
    }

    ctx->limit = ctx->length = 0;

    return true;
}

/**/
_CC_API_PUBLIC(const tchar_t*) _cc_buf_stringify(_cc_buf_t *ctx, size_t *length) {
    if (length != NULL) {
        *length = ctx->length;
    }
    ctx->bytes[ctx->length] = 0;
    return (const tchar_t*)ctx->bytes;
}

/**/
_CC_API_PRIVATE(bool_t) _buf_expand(_cc_buf_t *ctx, size_t size) {
    byte_t *data = (byte_t *)_cc_realloc(ctx->bytes, size);
    if (_cc_likely(data)) {
        ctx->bytes = data;
        ctx->limit = size;
        return true;
    }

    return false;
}

/**/
_CC_FORCE_INLINE_ size_t _factor_length(size_t limit) {
    return limit + (size_t)((double)(limit * 0.76));
}

/**/
_CC_API_PUBLIC(bool_t) _cc_buf_expand_factor(_cc_buf_t *ctx, float32_t factor) {
    size_t expand_length = ctx->limit + (size_t)((double)(ctx->limit * factor));
    return _buf_expand(ctx, expand_length);
}

/**/
_CC_API_PUBLIC(bool_t) _cc_buf_expand(_cc_buf_t *ctx, size_t size) {
    _cc_assert(ctx != NULL && size > 0);
    if (ctx->limit >= (ctx->length + size)) {
        return true;
    }

    return _buf_expand(ctx, (ctx->length + size));
}

/**/
_CC_API_PUBLIC(bool_t) _cc_buf_append(_cc_buf_t *ctx, const void *data, size_t length) {
    size_t expand_length = 0;
    _cc_assert(ctx != NULL && data != NULL);

    if (_cc_unlikely(length <= 0 || ctx == NULL)) {
        return false;
    }

    expand_length = length + ctx->length;
    if (ctx->limit <= 0x80000000 && expand_length >= ctx->limit) {
        expand_length = length + _factor_length(ctx->limit);
        if (_buf_expand(ctx, expand_length) == false) {
            return false;
        }
    }

    memcpy((ctx->bytes + ctx->length), data, length);
    ctx->length += length;

    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_buf_putchar(_cc_buf_t* ctx, const tchar_t data) {
    size_t expand_length = 0;
    _cc_assert(ctx != NULL);

    if (_cc_unlikely(ctx == NULL)) {
        return false;
    }

    expand_length = ctx->length + sizeof(tchar_t);
    if (ctx->limit <= 0x80000000 && expand_length >= ctx->limit) {
        expand_length = _factor_length(ctx->limit);
        if (_buf_expand(ctx, expand_length) == false) {
            return false;
        }
    }

    //memcpy((ctx->bytes + ctx->length), &data, sizeof(tchar_t));
    *((tchar_t*)(ctx->bytes + ctx->length)) = data;
    ctx->length += sizeof(tchar_t);
    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_bufA_puts(_cc_buf_t *ctx, const char_t *s) {
    _cc_assert(ctx != NULL && s != NULL);
    return _cc_buf_append(ctx, (const pvoid_t)s, strlen(s) * sizeof(char_t));
}

/**/
_CC_API_PUBLIC(bool_t) _cc_bufA_appendvf(_cc_buf_t *ctx, const char_t *fmt, va_list arg) {
    int fmt_length, remaining, cnt = 0;
    size_t expand_length;
    va_list arg_copy;

    remaining = (int)_cc_buf_remaining(ctx);
    if (remaining <= 0) {
        if (!_buf_expand(ctx, _factor_length(ctx->limit))) {
            return false;
        }
        remaining = (int)_cc_buf_remaining(ctx);
    }
    /* If the first attempt to append fails, resize the buffer appropriately
     * and try again */
_ABUF_TRY_AGAIN:
    /* Append the new formatted string */
    /* fmt_length is the length of the string required*/
    va_copy(arg_copy, arg);
    fmt_length = (int)_vsnprintf((char_t *)(ctx->bytes + ctx->length), remaining, fmt, arg_copy);
    va_end(arg_copy);
#ifdef __CC_WINDOWS__
    if (fmt_length == -1) {
        va_copy(arg_copy, arg);
        fmt_length = (int)_vscprintf(fmt, arg_copy);
        va_end(arg_copy);
    }
#endif
    if (fmt_length < 0) {
        _cc_logger_error("_cc_buf_appendvf: Invalid parameters or out of memory (%d)", fmt_length);
        return false;
    } else if (fmt_length < remaining) {
        /* SUCCESS */
        ctx->length += fmt_length;
        return true;
    } else if (cnt >= 3) {
        _cc_assert(false);
        _cc_logger_error("_cc_buf_appendvf: too many attempts (%d)", cnt);
        return false;
    }
    /* FAILURE */
    expand_length = (size_t)(fmt_length + 128) + _factor_length(ctx->limit);
    if (_buf_expand(ctx, expand_length)) {
        remaining = (int)_cc_buf_remaining(ctx);
        cnt++;
        goto _ABUF_TRY_AGAIN;
    }
    _cc_logger_error("_cc_buf_appendvf: out of memory (%d)", expand_length);
    return false;
}

/* _cc_bufA_appendf() can be used when the there is no known
 * upper bound for the output string. */
_CC_API_PUBLIC(bool_t) _cc_bufA_appendf(_cc_buf_t *ctx, const char_t *fmt, ...) {
    bool_t result;
    va_list arg;

    _cc_assert(ctx != NULL && fmt != NULL);

    //if (NULL == strchr(fmt, '%')) {
    //    return _cc_bufA_puts(ctx, fmt);
    //}

    va_start(arg, fmt);
    result = _cc_bufA_appendvf(ctx, fmt, arg);
    va_end(arg);

    return result;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_bufW_puts(_cc_buf_t *ctx, const wchar_t *s) {
    _cc_assert(ctx != NULL && s != NULL);
    return _cc_buf_append(ctx, (const pvoid_t)s, wcslen(s) * sizeof(wchar_t));
}

/**/
_CC_API_PUBLIC(bool_t) _cc_bufW_appendvf(_cc_buf_t *ctx, const wchar_t *fmt, va_list arg) {
    int fmt_length, remaining, cnt = 0;
    size_t expand_length;
    va_list arg_copy;
    
    remaining = (int)(_cc_buf_remaining(ctx) / sizeof(wchar_t));
    if (remaining <= 0) {
        expand_length = _factor_length(ctx->limit);
        if (!_buf_expand(ctx, expand_length)) {
            return false;
        }
        remaining = (int)(_cc_buf_remaining(ctx) / sizeof(wchar_t));
    }
    
    /* If the first attempt to append fails, resize the buffer appropriately
     * and try again */
_WBUF_TRY_AGAIN:
    /* Append the new formatted string */
    /* fmt_length is the length of the string required*/
    va_copy(arg_copy, arg);
    fmt_length = (int)_vsnwprintf((wchar_t *)(ctx->bytes + ctx->length), remaining, fmt, arg_copy);
    va_end(arg_copy);

#ifdef __CC_WINDOWS__
    if (fmt_length == -1) {
        va_copy(arg_copy, arg);
        fmt_length = (int)_vscwprintf(fmt, arg_copy);
        va_end(arg_copy);
    }
#endif
    if (fmt_length < 0) {
        _cc_logger_error("_cc_buf_appendvf: Invalid parameters or out of memory (%d)", fmt_length);
        return false;
    } else if (fmt_length < remaining) {
        /* SUCCESS */
        ctx->length += (fmt_length * sizeof(wchar_t));
        return true;
    } else if (cnt >= 3) {
        _cc_assert(false);
        _cc_logger_error("_cc_buf_appendvf: too many attempts (%d)", cnt); 
        return false;
    }

    /* FAILURE */
    expand_length = (size_t)(fmt_length + 128) * sizeof(wchar_t);
    expand_length = expand_length + _factor_length(ctx->limit);
    if (_buf_expand(ctx, expand_length)) {
        remaining = (int)(_cc_buf_remaining(ctx) / sizeof(wchar_t));
        cnt++;
        goto _WBUF_TRY_AGAIN;
    }

    _cc_logger_error("_cc_buf_appendvf: out of memory (%d)", expand_length);
    return false;
}
/* _cc_bufA_appendf() can be used when the there is no known
 * upper bound for the output string. */
_CC_API_PUBLIC(bool_t) _cc_bufW_appendf(_cc_buf_t *ctx, const wchar_t *fmt, ...) {
    bool_t result;
    va_list arg;

    _cc_assert(ctx != NULL && fmt != NULL);

    //if (NULL == wcschr(fmt, L'%')) {
    //    return _cc_bufW_puts(ctx, fmt);
    //}

    va_start(arg, fmt);
    result = _cc_bufW_appendvf(ctx, fmt, arg);
    va_end(arg);

    return result;
}

_CC_API_PUBLIC(bool_t) _cc_buf_from_file(_cc_buf_t* buf,const tchar_t *file_name) {
    _cc_file_t *f;
    size_t file_size;
    size_t r;

    f = _cc_open_file(file_name, _T("rb"));
    if (f == NULL) {
        return false;
    }

    file_size = (size_t)_cc_file_size(f);

    if (_cc_likely(file_size > 0)) {
        _cc_alloc_buf(buf, file_size);

        r = _cc_file_read(f, buf->bytes, sizeof(byte_t), 3);
        if (r < 0) {
            _cc_free_buf(buf);
            return false;
        }
        buf->length = r;

        /*----BOM----
        EF BB BF = UTF-8
        FE FF 00 = UTF-16, big-endian
        FF FE    = UTF-16, little-endian
        */

        /*UTF8 BOM */
        if (buf->bytes[0] == 0xEF && buf->bytes[1] == 0xBB && buf->bytes[2] == 0xBF) {
            buf->length = 0;
        }

        while ((r = _cc_file_read(f, buf->bytes + buf->length, 
                sizeof(byte_t), buf->limit - buf->length)) > 0) {
            buf->length += r;
        }

#ifdef _CC_UNICODE_
        _cc_buf_utf8_to_utf16(buf, 0);
#endif
    }
    _cc_file_close(f);

    return true;
}

_CC_API_PUBLIC(bool_t) _cc_buf_utf8_to_utf16(_cc_buf_t *ctx, size_t offset) {
    _cc_buf_t b;
    size_t length;

    if (ctx == NULL || ctx->length <= 0 || ctx->length <= offset) {
        return false;
    }

    if (!_cc_alloc_buf(&b, (ctx->length - offset + 1) * sizeof(wchar_t))) {
        return false;
    }

    length = _cc_utf8_to_utf16((const uint8_t *)(ctx->bytes + offset), 
                            (const uint8_t *)(ctx->bytes + ctx->length + 1),
                            (uint16_t *)b.bytes, (uint16_t *)(b.bytes + b.limit));

    if (length > 0) {
        _cc_free(ctx->bytes);
        ctx->bytes = b.bytes;
        ctx->limit = b.limit;
        ctx->length = length * sizeof(wchar_t);
        return true;
    }

    _cc_free_buf(&b);
    return false;
}

_CC_API_PUBLIC(bool_t) _cc_buf_utf16_to_utf8(_cc_buf_t *ctx, size_t offset) {
    _cc_buf_t b;
    size_t length;

    if (ctx == NULL || ctx->length <= 0 || ctx->length <= offset) {
        return false;
    }

    if (!_cc_alloc_buf(&b, (ctx->length - offset + 1) * sizeof(char_t))) {
        return false;
    }

    length = _cc_utf16_to_utf8((const uint16_t *)(ctx->bytes + offset), 
                            (const uint16_t *)(ctx->bytes + ctx->length + 1),
                            (uint8_t *)b.bytes, (uint8_t *)(b.bytes + b.limit));

    if (_cc_likely(length > 0)) {
        _cc_free(ctx->bytes);
        ctx->bytes = b.bytes;
        ctx->limit = b.limit;
        ctx->length = length * sizeof(char_t);
        return true;
    }

    _cc_free_buf(&b);
    return false;
}
