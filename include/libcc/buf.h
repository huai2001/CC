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

#ifndef _C_CC_BUFFER_H_INCLUDED_
#define _C_CC_BUFFER_H_INCLUDED_

#include "types.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cc_sbuf_char {
    size_t offset;
    size_t length;
    size_t line;
    size_t depth;
    const char_t* content;
} _cc_sbuf_char_t;

typedef struct _cc_sbuf_wchar {
    size_t offset;
    size_t length;
    size_t line;
    size_t depth;
    const wchar_t* content;
} _cc_sbuf_wchar_t;

#ifndef _CC_UNICODE_
typedef _cc_sbuf_char_t _cc_sbuf_t;
#else
typedef _cc_sbuf_wchar_t _cc_sbuf_t;
#endif

/**
 * @brief check if the given size is left to read in a given parse buffer
 * (starting with 1)
 */
#define _cc_sbuf_can_read(BUFFER, SIZE) \
    (((BUFFER)->offset + (SIZE)) <= (BUFFER)->length)
/**
 * @brief check if the BUFFER can be accessed
 */
#define _cc_sbuf_access(BUFFER) ((BUFFER)->offset < (BUFFER)->length)
/**
 * @brief check if the BUFFER can be accessed at the given index (starting with
 * 0)
 */
#define _cc_sbuf_access_offset(BUFFER, OFFSET) \
    (((BUFFER)->offset + (OFFSET)) < (BUFFER)->length)

/**
 * @brief get a pointer to the BUFFER at the position
 */
#define _cc_sbuf_offset(BUFFER) ((BUFFER)->content + (BUFFER)->offset)
#define _cc_sbuf_offset_at(BUFFER, INDEX) ((BUFFER)->content + (BUFFER)->offset + (INDEX))

#define _cc_sbuf_offset_unequal(BUFFER,X) (*((BUFFER)->content + (BUFFER)->offset) != (X))
#define _cc_sbuf_offset_equal(BUFFER,X) (*((BUFFER)->content + (BUFFER)->offset) == (X))

#define _cc_sbuf_if_offset(BUFFER, FN) do {\
    while (_cc_sbuf_access(BUFFER) && (FN)) {\
        (BUFFER)->offset++;\
    }\
} while(0)
/**
 * @brief Skips spaces and comments as many as possible.
 *
 * @param buffer _cc_sbuf_t
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_buf_jump_comment(_cc_sbuf_t* const buffer);

/**
 * @brief buf structure
 */
typedef struct _cc_buf {
    size_t limit;
    size_t length;
    byte_t* bytes;
} _cc_buf_t;

#define _cc_buf_bytes(buffer) ((buffer)->bytes)
#define _cc_buf_length(buffer) ((buffer)->length)
#define _cc_buf_cleanup(buffer) ((buffer)->length = 0)
/**
 * @brief Return bytes remaining in the buffer
 */
#define _cc_buf_empty_length(buffer) ((buffer)->limit - (buffer)->length - 1)

_CC_FORCE_INLINE_ void _cc_dump(const byte_t *bytes, size_t length) {
    size_t i;
    for (i = 0; i < length; i++) {
        _ftprintf(stdout, _T("%02X,"), bytes[i] & 0xff);
    }
    putc('\n',stdout);
}

#define _cc_buf_dump(_x) (_cc_dump((_x)->bytes,(_x)->length))

/**/
_CC_FORCE_INLINE_ void _cc_buf_reset(_cc_buf_t *buf) {
    buf->length = 0;
}

/**
 * @brief Create buf
 *
 * @param file_name file path
 *
 * @return _cc_buf_t structure
 */
_CC_API_PUBLIC(_cc_buf_t*) _cc_buf_from_file(const tchar_t* file_name);
/**
 * @brief Initialize buf
 *
 * @param ctx _cc_buf_t structure
 * @param initial Size of buffer
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_buf_alloc(_cc_buf_t* ctx, size_t initial);
/**
 * @brief free buf
 *
 * @param ctx _cc_buf_t structure
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_buf_free(_cc_buf_t* ctx);
/**
 * @brief Create buf
 *
 * @param initial Size of buffer
 *
 * @return _cc_buf_t structure
 */
_CC_API_PUBLIC(_cc_buf_t*) _cc_create_buf(size_t initial);
/**
 * @brief Destroy buf
 *
 * @param ctx _cc_buf_t structure
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(void) _cc_destroy_buf(_cc_buf_t** ctx);
/**/
_CC_API_PUBLIC(const tchar_t*) _cc_buf_stringify(_cc_buf_t *ctx, size_t *length);
/**
 * @brief Expand buf
 *
 * @param ctx _cc_buf_t structure
 * @param size Size of buffer
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_buf_expand(_cc_buf_t* ctx, size_t size);
/**
 * @brief Written buf
 *
 * @param ctx _cc_buf_t structure
 * @param data Written data
 * @param size Size of Written data
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_buf_append(_cc_buf_t* ctx, const void* data, size_t size);
/**
 * @brief Written the string to buf
 *
 * @param ctx _cc_buf_t structure
 * @param s String buffer
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_bufA_puts(_cc_buf_t* ctx, const char_t* s);
/**
 * @brief Written the foramt string to buf
 *
 * @param ctx _cc_buf_t structure
 * @param fmt Format string buffer
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_bufA_appendvf(_cc_buf_t* ctx, const char_t* fmt, va_list arg);
/**
 * @brief Written the foramt string to buf
 *
 * @param ctx _cc_buf_t structure
 * @param fmt Format string buffer
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_bufA_appendf(_cc_buf_t* ctx, const char_t* fmt, ...);
/**
 * @brief Written the string to buf
 *
 * @param ctx _cc_buf_t structure
 * @param s string buffer
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_bufW_puts(_cc_buf_t* ctx, const wchar_t* s);
/**
 * @brief Written the foramt string to buf
 *
 * @param ctx _cc_buf_t structure
 * @param fmt Format string buffer
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_bufW_appendvf(_cc_buf_t* ctx, const wchar_t* fmt, va_list arg);
/**
 * @brief Written the foramt string to buf
 *
 * @param ctx _cc_buf_t structure
 * @param fmt Format string buffer
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_bufW_appendf(_cc_buf_t* ctx, const wchar_t* fmt, ...);
/**
 * @brief Transform coding
 *
 * @param ctx _cc_buf_t structure
 * @param offset buffer start offset
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_buf_utf8_to_utf16(_cc_buf_t *ctx, size_t offset);
/**
 * @brief Transform coding
 *
 * @param ctx _cc_buf_t structure
 * @param offset buffer start offset
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_buf_utf16_to_utf8(_cc_buf_t *ctx, size_t offset);

#ifdef _CC_UNICODE_
#define _cc_buf_puts _cc_bufW_puts
#define _cc_buf_appendf _cc_bufW_appendf
#define _cc_buf_appendvf _cc_bufW_appendvf
#else
#define _cc_buf_puts _cc_bufA_puts
#define _cc_buf_appendf _cc_bufA_appendf
#define _cc_buf_appendvf _cc_bufA_appendvf
#endif

/** @} */
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_BUFFER_H_INCLUDED_*/
