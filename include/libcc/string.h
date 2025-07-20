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
#ifndef _C_CC_STRING_H_INCLUDED_
#define _C_CC_STRING_H_INCLUDED_

#include <stdio.h>
#include <string.h>
#include "UTF.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t length;
    char_t* data;
} _cc_AString_t;

typedef struct {
    size_t length;
    wchar_t* data;
} _cc_WString_t;

#define _cc_String(_str) { sizeof(_str) - 1, _str }

/*for porting from GCC compilers*/
#ifndef _CC_MSVC_
    #include <stdarg.h>
    #include <wchar.h>
    #define _snprintf snprintf
    #define _vsnprintf vsnprintf
    #define _snwprintf swprintf
    #define _vsnwprintf vswprintf

#ifndef _WSTRING_DEFINED
    #define _stricmp strcasecmp
    #define _wcsicmp wcscasecmp

    #define stricmp strcasecmp
    #define wcsicmp wcscasecmp

    #define _strnicmp strncasecmp
    #define _wcsnicmp wcsncasecmp
#endif

    #define _atoi64 atoll
    #define _wtoi64 wtoll
#else
    #define strcasecmp _stricmp
    #define wcscasecmp _wcsicmp

    #define strncasecmp _strnicmp
    #define wcsncasecmp _wcsnicmp

    #define atoll _atoi64
    #define wtoll _wtoi64
#endif /* ndef _CC_MSVC_ */

extern const wchar_t _w_lower_xdigits[];
extern const wchar_t _w_upper_xdigits[];
extern const char_t _a_lower_xdigits[];
extern const char_t _a_upper_xdigits[];

#ifdef _CC_UNICODE_
    #define _lower_xdigits _w_lower_xdigits
    #define _upper_xdigits _w_upper_xdigits
    #define _cc_trim_copy _cc_trimW_copy

    #define _cc_split _cc_splitW
    typedef _cc_WString_t _cc_String_t;
#else
    #define _lower_xdigits _a_lower_xdigits
    #define _upper_xdigits _a_upper_xdigits
    #define _cc_trim_copy _cc_trimA_copy
    #define _cc_split _cc_splitA
    typedef _cc_AString_t _cc_String_t;
#endif

#define _cc_first_index_of(FIRST, LAST, FN) do {\
    while ((FIRST) < (LAST) && (FN)) {\
        (FIRST)++;\
    }\
} while(0)

#define _cc_last_index_of(FIRST, LAST, FN) do {\
    (LAST)--;\
    while(FIRST > (LAST) && (FN)) {\
        (LAST)--;\
    }\
    (LAST)++;\
} while(0)

_CC_FORCE_INLINE_ int _cc_char2hex(int ch) {
    return (ch <= _T('9')) ? (ch & 0x0F) : ((ch & 0x0F) + 0x09);
}

/* parse hexadecimal number */
_CC_API_PUBLIC(uint8_t) _cc_hex2(const tchar_t *);
/* parse hexadecimal number */
_CC_API_PUBLIC(uint16_t) _cc_hex4(const tchar_t *);
/* parse hexadecimal number */
_CC_API_PUBLIC(uint32_t) _cc_hex8(const tchar_t *);
/* parse hexadecimal number */
_CC_API_PUBLIC(uint64_t) _cc_hex16(const tchar_t *);

/* byte to hex string*/
_CC_API_PUBLIC(size_t) _cc_bytes2hex(const byte_t *, size_t, tchar_t *, size_t);
/* hex string to byte*/
_CC_API_PUBLIC(size_t) _cc_hex2bytes(const tchar_t *, size_t, byte_t *, size_t);
/**/
_CC_API_PUBLIC(int32_t)
_cc_splitA(_cc_AString_t *dst, int32_t count, char_t *src, int32_t(separator)(char_t *, int32_t));
/**/
_CC_API_PUBLIC(int32_t)
_cc_splitW(_cc_WString_t *dst, int32_t count, wchar_t *src, int32_t(separator)(wchar_t *, int32_t));
/**/
_CC_API_PUBLIC(tchar_t *) _cc_substr(tchar_t *, const tchar_t *, uint32_t, int32_t);
/**/
_CC_API_PUBLIC(size_t) 
_cc_trimW_copy(wchar_t *dst, size_t dst_capacity,  const wchar_t *src, size_t src_len);
/**/
_CC_API_PUBLIC(size_t)
_cc_trimA_copy(char_t *dst, size_t dst_capacity,  const char_t *src, size_t src_len);

typedef enum _CC_NUMBER_TYPES_ {
    _CC_NUMBER_INT_ = 1,
    _CC_NUMBER_FLOAT_
} _CC_NUMBER_TYPES_;

typedef struct _cc_number {
    _CC_NUMBER_TYPES_ vt;
    union {
        int64_t uni_int;
        float64_t uni_float;
    } v;
} _cc_number_t;

/* Parse the input text to generate a number, and populate the result into item.
 */
_CC_API_PUBLIC(const tchar_t *) _cc_to_number(const tchar_t *s, _cc_number_t *item);

/* buf points to the END of the buffer
_CC_FORCE_INLINE_ char_t *_cc_long2buf(char_t *buf, long num) {
    *buf = '\0';

    do {
        *--buf = (char_t) (num % 10) + '0';
        num /= 10;
    } while (num > 0);

    if (num < 0) {
        *--buf = '-';
    }
    return buf;
}
*/

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_STRING_H_INCLUDED_*/
