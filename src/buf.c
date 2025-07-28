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
#include <libcc/alloc.h>
#include <libcc/buf.h>
#include <libcc/string.h>
#include <libcc/UTF.h>

/* Skips spaces and comments as many as possible.*/
_CC_API_PUBLIC(bool_t) _cc_buf_jump_comment(_cc_sbuf_t *const buffer) {
    register const tchar_t *p = nullptr;
    /*if ((buffer == nullptr) || (buffer->content == nullptr)) {
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

_CC_API_PUBLIC(bool_t) _cc_buf_alloc(_cc_buf_t *ctx, size_t initial) {
    _cc_assert(ctx != nullptr);

    memset(ctx, 0, sizeof(_cc_buf_t));
    ctx->limit = _cc_aligned_alloc_opt(initial, 64);
    ctx->length = 0;
    ctx->bytes = _CC_CALLOC(byte_t, ctx->limit);

    return true;
}

/**/
_CC_API_PUBLIC(_cc_buf_t *) _cc_create_buf(size_t initial) {
    _cc_buf_t *ctx = _CC_MALLOC(_cc_buf_t);
    _cc_buf_alloc(ctx, initial);
    return ctx;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_buf_free(_cc_buf_t *ctx) {
    _cc_assert(ctx != nullptr);

    if (_cc_likely(ctx->bytes)) {
        _cc_free(ctx->bytes);
        ctx->bytes = nullptr;
    }

    ctx->limit = ctx->length = 0;

    return true;
}

/**/
_CC_API_PUBLIC(void) _cc_destroy_buf(_cc_buf_t **ctx) {
    _cc_assert(ctx != nullptr);

    if (_cc_buf_free(*ctx)) {
        _cc_free(*ctx);
    }

    *ctx = nullptr;
}

/**/
_CC_API_PUBLIC(const tchar_t*) _cc_buf_stringify(_cc_buf_t *ctx, size_t *length) {
    if (length != nullptr) {
        *length = ctx->length + 1;
    }
    ctx->bytes[ctx->length] = 0;
    return (const tchar_t*)ctx->bytes;
}

/**/
_CC_API_PRIVATE(bool_t) _buf_expand(_cc_buf_t *ctx, size_t size) {
    size_t new_size = _cc_aligned_alloc_opt(ctx->length + size, 64);
    byte_t *new_data = (byte_t *)_cc_realloc(ctx->bytes, new_size);

    if (_cc_likely(new_data)) {
        ctx->bytes = new_data;
        ctx->limit = new_size;
        return true;
    }

    return false;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_buf_expand(_cc_buf_t *ctx, size_t size) {
    _cc_assert(ctx != nullptr && size > 0);
    if (ctx->limit >= (ctx->length + size)) {
        return true;
    }

    return _buf_expand(ctx, size);
}

/**/
_CC_API_PUBLIC(bool_t) _cc_buf_append(_cc_buf_t *ctx, const void *data, size_t length) {
    size_t len = 0;
    _cc_assert(ctx != nullptr && data != nullptr);

    if (_cc_unlikely(length <= 0 || ctx == nullptr)) {
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
_CC_API_PUBLIC(bool_t) _cc_bufA_puts(_cc_buf_t *ctx, const char_t *s) {
    _cc_assert(ctx != nullptr && s != nullptr);
    return _cc_buf_append(ctx, (const pvoid_t)s, strlen(s) * sizeof(char_t));
}

/**/
_CC_API_PUBLIC(bool_t) _cc_bufA_appendvf(_cc_buf_t *ctx, const char_t *fmt, va_list arg) {
    int32_t fmt_length, free_length;

    /* If the first attempt to append fails, resize the buffer appropriately
     * and try again */
    while (1) {
        free_length = (int32_t)_cc_buf_remaining(ctx);
        /* Append the new formatted string */
        /* fmt_length is the length of the string required*/
        fmt_length = (int32_t)_vsnprintf((char_t *)(ctx->bytes + ctx->length), free_length, fmt, arg);

#ifdef __CC_WINDOWS__
        if (fmt_length == -1) {
            fmt_length = _vsnprintf(nullptr, 0, fmt, arg);
        }
#endif
        if (fmt_length > 0) {
            /* SUCCESS */
            if (fmt_length <= free_length) {
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

/* _cc_bufA_appendf() can be used when the there is no known
 * upper bound for the output string. */
_CC_API_PUBLIC(bool_t) _cc_bufA_appendf(_cc_buf_t *ctx, const char_t *fmt, ...) {
    bool_t result;
    va_list arg;

    _cc_assert(ctx != nullptr && fmt != nullptr);

    //if (nullptr == strchr(fmt, '%')) {
    //    return _cc_bufA_puts(ctx, fmt);
    //}

    va_start(arg, fmt);
    result = _cc_bufA_appendvf(ctx, fmt, arg);
    va_end(arg);

    return result;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_bufW_puts(_cc_buf_t *ctx, const wchar_t *s) {
    _cc_assert(ctx != nullptr && s != nullptr);
    return _cc_buf_append(ctx, (const pvoid_t)s, wcslen(s) * sizeof(wchar_t));
}

/**/
_CC_API_PUBLIC(bool_t) _cc_bufW_appendvf(_cc_buf_t *ctx, const wchar_t *fmt, va_list arg) {
    size_t fmt_length, free_length;

    /* If the first attempt to append fails, resize the buffer appropriately
     * and try again */
    while (1) {
        free_length = _cc_buf_remaining(ctx) / sizeof(wchar_t);
        /* Append the new formatted string */
        /* fmt_length is the length of the string required*/
        fmt_length = _vsnwprintf((wchar_t *)(ctx->bytes + ctx->length), free_length, fmt, arg);

#ifdef __CC_WINDOWS__
        if (fmt_length == -1) {
            fmt_length = _vsnwprintf(nullptr, 0, fmt, arg);
        }
#endif
        if (fmt_length > 0) {
            /* SUCCESS */
            if (fmt_length <= free_length) {
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
/* _cc_bufA_appendf() can be used when the there is no known
 * upper bound for the output string. */
_CC_API_PUBLIC(bool_t) _cc_bufW_appendf(_cc_buf_t *ctx, const wchar_t *fmt, ...) {
    bool_t result;
    va_list arg;

    _cc_assert(ctx != nullptr && fmt != nullptr);

    //if (nullptr == wcschr(fmt, L'%')) {
    //    return _cc_bufW_puts(ctx, fmt);
    //}

    va_start(arg, fmt);
    result = _cc_bufW_appendvf(ctx, fmt, arg);
    va_end(arg);

    return result;
}

_CC_API_PUBLIC(_cc_buf_t*) _cc_buf_from_file(const tchar_t *file_name) {
    _cc_buf_t *buf = nullptr;
    _cc_file_t *f;
    size_t file_size;
    size_t byte_read;

    f = _cc_open_file(file_name, _T("rb"));
    if (f == nullptr) {
        return nullptr;
    }

    file_size = (size_t)_cc_file_size(f);

    if (_cc_likely(file_size > 0)) {
        buf = _cc_create_buf(file_size);
        while ((byte_read = _cc_file_read(f, buf->bytes + buf->length, 
                                sizeof(byte_t), buf->limit - buf->length)) > 0) {
            buf->length += byte_read;
        }
    }
    _cc_file_close(f);

    return buf;
}

_CC_API_PUBLIC(bool_t) _cc_buf_utf8_to_utf16(_cc_buf_t *ctx, size_t offset) {
    _cc_buf_t b;
    size_t length;

    if (ctx == nullptr || ctx->length <= 0 || ctx->length <= offset) {
        return false;
    }

    if (!_cc_buf_alloc(&b, (ctx->length - offset + 1) * sizeof(wchar_t))) {
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

    _cc_buf_free(&b);
    return false;
}

_CC_API_PUBLIC(bool_t) _cc_buf_utf16_to_utf8(_cc_buf_t *ctx, size_t offset) {
    _cc_buf_t b;
    size_t length;

    if (ctx == nullptr || ctx->length <= 0 || ctx->length <= offset) {
        return false;
    }

    if (!_cc_buf_alloc(&b, (ctx->length - offset + 1) * sizeof(char_t))) {
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

    _cc_buf_free(&b);
    return false;
}
