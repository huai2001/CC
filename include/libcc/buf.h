
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
#define _cc_sbuf_can_read(BUFFER, SIZE) (((BUFFER)->offset + (SIZE)) <= (BUFFER)->length)
/**
 * @brief check if the BUFFER can be accessed
 */
#define _cc_sbuf_access(BUFFER) ((BUFFER)->offset < (BUFFER)->length)
/**
 * @brief check if the BUFFER can be accessed at the given index (starting with
 * 0)
 */
#define _cc_sbuf_access_offset(BUFFER, OFFSET) (((BUFFER)->offset + (OFFSET)) < (BUFFER)->length)

/**
 * @brief get a pointer to the BUFFER at the position
 */
#define _cc_sbuf_offset(BUFFER) ((BUFFER)->content + (BUFFER)->offset)
#define _cc_sbuf_offset_at(BUFFER, INDEX) ((BUFFER)->content + ((BUFFER)->offset + (INDEX)))

#define _cc_sbuf_offset_unequal(BUFFER,X) (*(_cc_sbuf_offset(BUFFER)) != (X))
#define _cc_sbuf_offset_equal(BUFFER,X) (*(_cc_sbuf_offset(BUFFER)) == (X))

#define _cc_sbuf_if_offset(BUFFER, FN) do {\
    while (_cc_sbuf_access(BUFFER) && (FN)) {\
        (BUFFER)->offset++;\
    }\
} while(0)

/**/
_CC_API_PUBLIC(bool_t) _cc_buf_jump_comment(_cc_sbuf_t* const buffer);

/**/
typedef struct _cc_buf {
    size_t limit;   // !< capacity of 'bytes' (terminating 0 byte doesn't count here)
    size_t length;  // !< number of bytes in 'bytes'
    byte_t* bytes;  // !< pointer to internal memory
} _cc_buf_t;

#define _cc_buf_bytes(buffer) ((buffer)->bytes)
#define _cc_buf_length(buffer) ((buffer)->length)
#define _cc_buf_cleanup(buffer) ((buffer)->length = 0)

/* @brief Return bytes remaining in the buffer */
#define _cc_buf_remaining(buffer) ((buffer)->limit - (buffer)->length)

_CC_FORCE_INLINE_ void _cc_dump(const byte_t *bytes, size_t length) {
    size_t i;
    for (i = 0; i < length; i++) {
        _ftprintf(stdout, _T("%02X"), bytes[i] & 0xff);
    }
    putc('\n',stdout);
}

#define _cc_buf_dump(_x) (_cc_dump((_x)->bytes,(_x)->length))

/**/
_CC_FORCE_INLINE_ void _cc_buf_reset(_cc_buf_t *buf) {
    buf->length = 0;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_buf_from_file(_cc_buf_t* buf,const tchar_t* file_name);
/**/
_CC_API_PUBLIC(bool_t) _cc_alloc_buf(_cc_buf_t* ctx, size_t initial);
/**/
_CC_API_PUBLIC(bool_t) _cc_free_buf(_cc_buf_t* ctx);
/**/
_CC_API_PUBLIC(const tchar_t*) _cc_buf_stringify(_cc_buf_t *ctx, size_t *length);
/**/
_CC_API_PUBLIC(bool_t) _cc_buf_expand(_cc_buf_t* ctx, size_t size);
/**/
_CC_API_PUBLIC(bool_t) _cc_buf_expand_factor(_cc_buf_t *ctx, float32_t factor);
/**/
_CC_API_PUBLIC(bool_t) _cc_buf_append(_cc_buf_t* ctx, const void* data, size_t size);
/**/
_CC_API_PUBLIC(bool_t) _cc_bufA_puts(_cc_buf_t* ctx, const char_t* s);
/**/
_CC_API_PUBLIC(bool_t) _cc_bufA_appendvf(_cc_buf_t* ctx, const char_t* fmt, va_list arg);
/**/
_CC_API_PUBLIC(bool_t) _cc_bufA_appendf(_cc_buf_t* ctx, const char_t* fmt, ...);
/**/
_CC_API_PUBLIC(bool_t) _cc_bufW_puts(_cc_buf_t* ctx, const wchar_t* s);
/**/
_CC_API_PUBLIC(bool_t) _cc_bufW_appendvf(_cc_buf_t* ctx, const wchar_t* fmt, va_list arg);
/**/
_CC_API_PUBLIC(bool_t) _cc_bufW_appendf(_cc_buf_t* ctx, const wchar_t* fmt, ...);
/**/
_CC_API_PUBLIC(bool_t) _cc_buf_utf8_to_utf16(_cc_buf_t *ctx, size_t offset);
/**/
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
