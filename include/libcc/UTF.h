/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#ifndef _C_CC_UTF8_H_INCLUDED_
#define _C_CC_UTF8_H_INCLUDED_

#include "core.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
/*
 Unicode    UTF-8
---+-----------------------+------------------------------------------------------  
 1 | 0000 0000 - 0000 007F |                                              0xxxxxxx  
 2 | 0000 0080 - 0000 07FF |                                     110xxxxx 10xxxxxx  
 3 | 0000 0800 - 0000 FFFF |                            1110xxxx 10xxxxxx 10xxxxxx  
 4 | 0001 0000 - 0010 FFFF |                   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  
 5 | 0020 0000 - 03FF FFFF |          111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
 6 | 0400 0000 - 7FFF FFFF | 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 */
#define _CC_UTF_STRICT_                     0x00
#define _CC_UTF_LENIENT_                    0x01

/* Some fundamental constants */
#define _CC_UNI_REPLACEMENT_CHAR_           (uint32_t)0x0000FFFD
#define _CC_UNI_MAX_BMP_                    (uint32_t)0x0000FFFF
#define _CC_UNI_MAX_UTF16_                  (uint32_t)0x0010FFFF
#define _CC_UNI_MAX_UTF32_                  (uint32_t)0x7FFFFFFF
#define _CC_UNI_MAX_LEGAL_UTF32_            (uint32_t)0x0010FFFF

#define _CC_INVALID_UNICODE_CODEPOINT_      0xFFFD

/**/
_CC_API_PUBLIC(int32_t) _cc_unicode_to_utf8(uint32_t unic, uint8_t *utf8, size_t size);
/**/
_CC_API_PUBLIC(int32_t) _cc_utf8_to_unicode(uint8_t *utf8, size_t len, uint32_t *unic);
/**/
_CC_API_PUBLIC(int32_t)
_cc_unicode_to_utf16(uint32_t unic, uint16_t *utf16, size_t size);

/**/
_CC_API_PUBLIC(int32_t)
_cc_utf8_to_utf16(const uint8_t *, const uint8_t *, uint16_t *, uint16_t *, bool_t);
/**/
_CC_API_PUBLIC(int32_t)
_cc_utf16_to_utf8(const uint16_t *, const uint16_t *, uint8_t *, uint8_t *, bool_t);
/**/
_CC_API_PUBLIC(int32_t)
_cc_utf8_to_utf32(const uint8_t *, const uint8_t *, uint32_t *, uint32_t *, bool_t);
/**/
_CC_API_PUBLIC(int32_t)
_cc_utf32_to_utf8(const uint32_t *, const uint32_t *, uint8_t *, uint8_t *, bool_t);
/**/
_CC_API_PUBLIC(int32_t)
_cc_utf16_to_utf32(const uint16_t *, const uint16_t *, uint32_t *, uint32_t *, bool_t);
/**/
_CC_API_PUBLIC(int32_t)
_cc_utf32_to_utf16(const uint32_t *, const uint32_t *, uint16_t *, uint16_t *, bool_t);
/**/
_CC_API_PUBLIC(int32_t)
_cc_utf8_to_gbk(const uint8_t *, const uint8_t *, uint8_t *, uint8_t *);
/**/
_CC_API_PUBLIC(int32_t)
_cc_gbk_to_utf8(const uint8_t *, const uint8_t *, uint8_t *, uint8_t *);

/*convert u0041 u0042 u0043 u4e2d u6587 to utf8 */
_CC_API_PUBLIC(int32_t)
_cc_convert_utf16_literal_to_utf8(const tchar_t **input, const tchar_t *input_end, tchar_t *output, size_t output_length);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_STRING_H_INCLUDED_*/
