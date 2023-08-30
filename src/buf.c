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
#include <cc/alloc.h>
#include <cc/buf.h>
#include <cc/string.h>

/* Skips spaces and comments as many as possible.*/
bool_t _cc_buf_jump_comments(_cc_sbuf_tchar_t *const buffer) {
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
                    if (*_cc_sbuf_offset(buffer) == _T(_CC_LF_)) {
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

bool_t _cc_buf_alloc(_cc_buf_t *ctx, size_t initsize) {
    _cc_assert(ctx != NULL);

    memset(ctx, 0, sizeof(_cc_buf_t));
    ctx->limit = initsize;
    ctx->length = 0;
    ctx->bytes = _CC_CALLOC(byte_t, ctx->limit);

    return true;
}

/**/
_cc_buf_t *_cc_create_buf(size_t initsize) {
    _cc_buf_t *ctx = _CC_MALLOC(_cc_buf_t);
    _cc_buf_alloc(ctx, initsize);
    return ctx;
}

/**/
bool_t _cc_buf_free(_cc_buf_t *ctx) {
    _cc_assert(ctx != NULL);

    if (_cc_likely(ctx->bytes)) {
        _cc_free(ctx->bytes);
        ctx->bytes = NULL;
    }

    ctx->limit = ctx->length = 0;

    return true;
}

/**/
void _cc_destroy_buf(_cc_buf_t **ctx) {
    _cc_assert(ctx != NULL);

    if (_cc_buf_free(*ctx)) {
        _cc_free(*ctx);
    }

    *ctx = NULL;
}

/**/
static bool_t _buf_expand(_cc_buf_t *ctx, size_t size) {
    size_t new_size = (ctx->length + size);
    byte_t *new_data = (byte_t *)_cc_realloc(ctx->bytes, new_size);

    if (_cc_likely(new_data)) {
        ctx->bytes = new_data;
        ctx->limit = new_size;
        return true;
    }

    return false;
}

/**/
bool_t _cc_buf_expand(_cc_buf_t *ctx, size_t size) {
    _cc_assert(ctx != NULL && size > 0);
    if (ctx->limit >= (ctx->length + size)) {
        return true;
    }

    return _buf_expand(ctx, size);
}

/**/
bool_t _cc_buf_write(_cc_buf_t *ctx, const void *data, size_t length) {
    size_t len = 0;
    _cc_assert(ctx != NULL && data != NULL);

    if (_cc_unlikely(length <= 0 || ctx == NULL)) {
        return false;
    }

    len = length + ctx->length;
    if (ctx->limit <= 0x80000000 && len > ctx->limit) {
        if (_buf_expand(ctx, len) == false) {
            return false;
        }
    }

    memcpy((ctx->bytes + ctx->length), data, length);
    ctx->length += length;

    return true;
}

/**/
bool_t _cc_bufA_puts(_cc_buf_t *ctx, const char_t *s) {
    _cc_assert(ctx != NULL && s != NULL);
    return _cc_buf_write(ctx, (const pvoid_t)s, strlen(s) * sizeof(char_t));
}

/**/
bool_t _cc_bufA_putsvf(_cc_buf_t *ctx, const char_t *fmt, va_list arg) {
    int32_t fmt_length, empty_len;

    /* If the first attempt to append fails, resize the buffer appropriately
     * and try again */
    while (1) {
        empty_len = (int32_t)_cc_buf_empty_length(ctx);
        /* Append the new formatted string */
        /* fmt_length is the length of the string required*/
        fmt_length = (int32_t)_vsnprintf((char_t *)(ctx->bytes + ctx->length), empty_len, fmt, arg);

#ifdef __CC_WINDOWS__
        if (fmt_length == -1) {
            fmt_length = _vsnprintf(nullptr, 0, fmt, arg);
        }
#endif
        if (fmt_length > 0) {
            /* SUCCESS */
            if (fmt_length <= empty_len) {
                break;
            }

            if (_buf_expand(ctx, (fmt_length + 32) * sizeof(char_t))) {
                continue;
            }
        }
        _cc_logger_error(_T("_cc_buf_t: length of formatted string changed"));

        return false;
    }

    ctx->length += (fmt_length * sizeof(char_t));
    return true;
}

/* _cc_bufA_putsf() can be used when the there is no known
 * upper bound for the output string. */
bool_t _cc_bufA_putsf(_cc_buf_t *ctx, const char_t *fmt, ...) {
    bool_t result;
    va_list arg;

    _cc_assert(ctx != NULL && fmt != NULL);

    if (_cc_unlikely(NULL == strchr(fmt, '%'))) {
        return _cc_bufA_puts(ctx, fmt);
    }

    va_start(arg, fmt);
    result = _cc_bufA_putsvf(ctx, fmt, arg);
    va_end(arg);

    return result;
}

/**/
bool_t _cc_bufW_puts(_cc_buf_t *ctx, const wchar_t *s) {
    _cc_assert(ctx != NULL && s != NULL);
    return _cc_buf_write(ctx, (const pvoid_t)s, wcslen(s) * sizeof(wchar_t));
}

/**/
bool_t _cc_bufW_putsvf(_cc_buf_t *ctx, const wchar_t *fmt, va_list arg) {
    size_t fmt_length, empty_len;

    /* If the first attempt to append fails, resize the buffer appropriately
     * and try again */
    while (1) {
        empty_len = _cc_buf_empty_length(ctx) / sizeof(wchar_t);
        /* Append the new formatted string */
        /* fmt_length is the length of the string required*/
        fmt_length = _vsnwprintf((wchar_t *)(ctx->bytes + ctx->length), empty_len, fmt, arg);

#ifdef __CC_WINDOWS__
        if (fmt_length == -1) {
            fmt_length = _vsnwprintf(nullptr, 0, fmt, arg);
        }
#endif
        if (fmt_length > 0) {
            /* SUCCESS */
            if (fmt_length <= empty_len) {
                break;
            }

            if (_buf_expand(ctx, (fmt_length + 32) * sizeof(wchar_t))) {
                continue;
            }
        }

        va_end(arg);
        _cc_logger_error(_T("_cc_buf_t: length of formatted string changed"));

        return false;
    }
    ctx->length += (fmt_length * sizeof(wchar_t));
    return true;
}
/* _cc_bufA_putsf() can be used when the there is no known
 * upper bound for the output string. */
bool_t _cc_bufW_putsf(_cc_buf_t *ctx, const wchar_t *fmt, ...) {
    bool_t result;
    va_list arg;

    _cc_assert(ctx != NULL && fmt != NULL);

    if (_cc_unlikely(NULL == wcschr(fmt, L'%'))) {
        return _cc_bufW_puts(ctx, fmt);
    }

    va_start(arg, fmt);
    result = _cc_bufW_putsvf(ctx, fmt, arg);
    va_end(arg);

    return result;
}

_cc_buf_t *_cc_load_buf(const tchar_t *file_name) {
    _cc_buf_t *buf = NULL;
    _cc_file_t *f;
    int64_t fsize;
    size_t byte_read;

    f = _cc_open_file(file_name, _T("rb"));
    if (f == NULL) {
        return NULL;
    }

    fsize = _cc_file_size(f);

    if (_cc_likely(fsize > 0)) {
        buf = _cc_create_buf((size_t)fsize);
        if (_cc_unlikely(buf == NULL)) {
            _cc_file_close(f);
            return NULL;
        }

        while ((byte_read = _cc_file_read(f, buf->bytes + buf->length, sizeof(byte_t), buf->limit - buf->length)) > 0) {
            buf->length += byte_read;
        }
    }
    _cc_file_close(f);

    return buf;
}

bool_t _cc_buf_utf8_to_utf16(_cc_buf_t *ctx, uint32_t offset) {
    _cc_buf_t b;
    int32_t len;

    if (ctx == NULL || ctx->length <= 0 || ctx->length <= offset) {
        return false;
    }

    if (!_cc_buf_alloc(&b, (ctx->length - offset + 1) * sizeof(wchar_t))) {
        return false;
    }

    len = _cc_utf8_to_utf16((const uint8_t *)(ctx->bytes + offset), 
                            (const uint8_t *)(ctx->bytes + ctx->length + 1),
                            (uint16_t *)b.bytes, (uint16_t *)(b.bytes + b.limit), false);

    if (len > 0) {
        _cc_free(ctx->bytes);
        ctx->bytes = b.bytes;
        ctx->limit = b.limit;
        ctx->length = len * sizeof(wchar_t);
        return true;
    }

    _cc_buf_free(&b);
    return false;
}

bool_t _cc_buf_utf16_to_utf8(_cc_buf_t *ctx, uint32_t offset) {
    _cc_buf_t b;
    int32_t len;

    if (ctx == NULL || ctx->length <= 0 || ctx->length <= offset) {
        return false;
    }

    if (!_cc_buf_alloc(&b, (ctx->length - offset + 1) * sizeof(char_t))) {
        return false;
    }

    len = _cc_utf16_to_utf8((const uint16_t *)(ctx->bytes + offset), 
                            (const uint16_t *)(ctx->bytes + ctx->length + 1),
                            (uint8_t *)b.bytes, (uint8_t *)(b.bytes + b.limit), false);

    if (_cc_likely(len > 0)) {
        _cc_free(ctx->bytes);
        ctx->bytes = b.bytes;
        ctx->limit = b.limit;
        ctx->length = len * sizeof(char_t);
        return true;
    }

    _cc_buf_free(&b);
    return false;
}
