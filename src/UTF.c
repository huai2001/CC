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

/*convert u0041 u0042 u0043 u4e2d u6587 to utf8 */
_CC_API_PUBLIC(int32_t)
_cc_convert_utf16_literal_to_utf8(const tchar_t **input, const tchar_t *input_end, tchar_t *output,
                                  size_t output_length) {
    uint32_t first_code;
    uint32_t second_code;
    uint32_t codepoint = 0;

    first_code = _cc_hex4(*input);
    if (first_code == 0) {
        return 0;
    }
    /* check that the code is valid */
    if (((first_code >= 0xDC00) && (first_code <= 0xDFFF))) {
        return 0;
    }

    *input += 4;
    /* UTF16 surrogate pair */
    //\ud83c\udf0a
    if ((first_code >= 0xD800) && (first_code <= 0xDBFF)) {
        if ((*input + 6) > input_end) {
            /* input ends unexpectedly */
            return 0;
        }
        if (((*input)[0] != '\\') || ((*input)[1] != 'u')) {
            /* missing second half of the surrogate pair */
            return 0;
        }
        /* get the second utf16 sequence */
        second_code = _cc_hex4(*input + 2);
        /* check that the code is valid */
        if ((second_code < 0xDC00) || (second_code > 0xDFFF)) {
            /* invalid second half of the surrogate pair */
            return 0;
        }

        *input += 6;
        /* calculate the unicode codepoint from the surrogate pair */
        codepoint = 0x10000 + (((first_code & 0x3FF) << 10) | (second_code & 0x3FF));
    } else {
        codepoint = first_code;
    }

    return _cc_unicode_to_utf8(codepoint, (uint8_t *)output, output_length);
}

_CC_API_PUBLIC(int32_t) _cc_unicode_to_utf16(uint32_t unic, uint16_t *utf16, size_t size) {
    _cc_assert(utf16 != nullptr);
    if (_cc_unlikely(utf16 == nullptr)) {
        return 0;
    }

    if (unic <= 0xFFFF) {
        *utf16 = (uint16_t)(unic);
        return 1;
    }

    if (unic <= 0xEFFFF && size >= 2) {
        *utf16++ = (uint16_t)(0xD800 + (unic >> 10) - 0x40);
        *utf16++ = (uint16_t)(0xDC00 + (unic & 0x3FF));
        return 2;
    }

    return 0;
}

_CC_API_PUBLIC(int32_t) _cc_unicode_to_utf8(uint32_t unic, uint8_t *utf8, size_t size) {
    _cc_assert(utf8 != nullptr);
    if (_cc_unlikely(utf8 == nullptr)) {
        return 0;
    }

    if (unic <= 0x0000007F && size > 0) {
        /* U-00000000 - U-0000007F:  0xxxxxxx */
        *utf8 = (unic) & 0x7F;
        return 1;
    } else if (unic >= 0x00000080 && unic <= 0x000007FF && size > 1) {
        /* U-00000080 - U-000007FF:  110xxxxx 10xxxxxx */
        *(utf8++) = ((unic >> 6) & 0x1F) | 0xC0;
        *(utf8++) = (unic & 0x3F) | 0x80;
        return 2;
    } else if (unic >= 0x00000800 && unic <= 0x0000FFFF && size > 2) {
        /* U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx */
        *(utf8++) = ((unic >> 12) & 0x0F) | 0xE0;
        *(utf8++) = ((unic >> 6) & 0x3F) | 0x80;
        *(utf8++) = (unic & 0x3F) | 0x80;
        return 3;
    } else if (unic >= 0x00010000 && unic <= 0x001FFFFF && size > 3) {
        /* U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
        *(utf8++) = ((unic >> 18) & 0x07) | 0xF0;
        *(utf8++) = ((unic >> 12) & 0x3F) | 0x80;
        *(utf8++) = ((unic >> 6) & 0x3F) | 0x80;
        *(utf8++) = (unic & 0x3F) | 0x80;
        return 4;
    } else if (unic >= 0x00200000 && unic <= 0x03FFFFFF && size > 4) {
        /* U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx
         * 10xxxxxx */
        *(utf8++) = ((unic >> 24) & 0x03) | 0xF8;
        *(utf8++) = ((unic >> 18) & 0x3F) | 0x80;
        *(utf8++) = ((unic >> 12) & 0x3F) | 0x80;
        *(utf8++) = ((unic >> 6) & 0x3F) | 0x80;
        *(utf8++) = (unic & 0x3F) | 0x80;
        return 5;
    } else if (unic >= 0x04000000 && unic <= 0x7FFFFFFF && size > 5) {
        /* U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx
         * 10xxxxxx 10xxxxxx */
        *(utf8++) = ((unic >> 30) & 0x01) | 0xFC;
        *(utf8++) = ((unic >> 24) & 0x3F) | 0x80;
        *(utf8++) = ((unic >> 18) & 0x3F) | 0x80;
        *(utf8++) = ((unic >> 12) & 0x3F) | 0x80;
        *(utf8++) = ((unic >> 6) & 0x3F) | 0x80;
        *(utf8++) = (unic & 0x3F) | 0x80;
        return 6;
    }
    return 0;
}

_CC_API_PUBLIC(int32_t) _cc_utf8_to_unicode(uint8_t *utf8, size_t len, uint32_t *unic) {
    const uint8_t *u = utf8;

    if (*u <= 0x7f) {
        *unic = (uint32_t)*u;
        return 1;
    }

    if (*u <= 0xdf && len > 1) {
        *unic = ((*u & 0x1f) << 6) | (*(u + 1) & 0x3f);
        return 2;
    }

    if (*u <= 0xef && len > 2) {
        *unic = ((*u & 0x0f) << 12 | (*(u + 1) & 0x3f) << 6 | (*(u + 2) & 0x3f));
        return 3;
    }

    if (*u <= 0xf7 && len > 3) {
        *unic = ((*u & 0x07) << 18 | (*(u + 1) & 0x3f) << 12 | (*(u + 2) & 0x3f) << 6 | (*(u + 3) & 0x3f));
        return 4;
    }

    if (*u <= 0xfb && len > 4) {
        *unic = ((*u & 0x03) << 24 | (*(u + 1) & 0x3f) << 18 | (*(u + 2) & 0x3f) << 12 | (*(u + 3) & 0x3f) << 6 |
                 (*(u + 4) & 0x3f));
        return 5;
    }

    if (*u <= 0xfd && len > 5) {
        *unic = ((*u & 0x01) << 30 | (*(u + 1) & 0x3f) << 24 | (*(u + 2) & 0x3f) << 18 | (*(u + 3) & 0x3f) << 12 |
                 (*(u + 4) & 0x3f) << 6 | (*(u + 5) & 0x3f));
        return 6;
    }

    return 0;
}

/* --------------------------------------------------------------------- */
_CC_API_PUBLIC(int32_t)
_cc_utf32_to_utf16(const uint32_t *source_start, const uint32_t *source_end, uint16_t *target_start,
                   uint16_t *target_end, bool_t flags) {
    int32_t len = 0;
    uint32_t *utf32 = (uint32_t *)source_start;
    uint16_t *utf16 = (uint16_t *)target_start;

    while (utf32 < source_end && utf16 < target_end) {
        len = _cc_unicode_to_utf16(*utf32++, utf16, target_end - utf16);
        if (len == 0) {
            break;
        }
        utf16 += len;
    }
    *utf16 = 0;
    return (int32_t)(utf16 - target_start);
}

/* --------------------------------------------------------------------- */
_CC_API_PUBLIC(int32_t)
_cc_utf16_to_utf32(const uint16_t *source_start, const uint16_t *source_end, uint32_t *target_start,
                   uint32_t *target_end, bool_t flags) {
    return 0;
}

/* --------------------------------------------------------------------- */
_CC_API_PUBLIC(int32_t)
_cc_utf16_to_utf8(const uint16_t *source_start, const uint16_t *source_end, uint8_t *target_start, uint8_t *target_end,
                  bool_t flags) {
    int32_t len = 0;
    uint16_t *utf16 = (uint16_t *)source_start;
    uint8_t *utf8 = (uint8_t *)target_start;

    while (utf16 < source_end && utf8 < target_end) {
        len = _cc_unicode_to_utf8((*utf16++), utf8, target_end - utf8);
        if (len == 0) {
            break;
        }
        utf8 += len;
    }
    *utf8 = 0;
    return (int32_t)(utf8 - target_start);
}

/* --------------------------------------------------------------------- */
_CC_API_PUBLIC(int32_t)
_cc_utf32_to_utf8(const uint32_t *source_start, const uint32_t *source_end, uint8_t *target_start, uint8_t *target_end,
                  bool_t flags) {
    int32_t len = 0;
    uint32_t *utf32 = (uint32_t *)source_start;
    uint8_t *utf8 = (uint8_t *)target_start;

    while (utf32 < source_end && utf8 < target_end) {
        len = _cc_unicode_to_utf8(*utf32++, utf8, target_start - utf8);
        if (len == 0) {
            break;
        }
        utf8 += len;
    }
    *utf8 = 0;
    return (int32_t)(utf8 - target_start);
}

/* --------------------------------------------------------------------- */
_CC_API_PUBLIC(int32_t)
_cc_utf8_to_utf32(const uint8_t *source_start, const uint8_t *source_end, uint32_t *target_start, uint32_t *target_end,
                  bool_t flags) {
    int32_t len = 0;
    uint32_t *utf32 = target_start;
    uint8_t *utf8 = (uint8_t *)source_start;

    while (utf8 < source_end && (utf32 + 1) < target_end) {
        len = _cc_utf8_to_unicode(utf8, source_end - utf8, utf32++);
        if (len == 0) {
            break;
        }
        utf8 += len;
    }
    *utf32 = 0;
    return (int32_t)(utf32 - target_start);
}

/* --------------------------------------------------------------------- */
_CC_API_PUBLIC(int32_t)
_cc_utf8_to_utf16(const uint8_t *source_start, const uint8_t *source_end, uint16_t *target_start, uint16_t *target_end,
                  bool_t flags) {
    int32_t len = 0;
    uint16_t *utf16 = (uint16_t *)target_start;
    uint8_t *utf8 = (uint8_t *)source_start;

    while (utf8 < source_end && (utf16 + 1) < target_end) {
        len = _cc_utf8_to_unicode(utf8, source_end - utf8, (uint32_t *)utf16);
        if (len == 0) {
            break;
        }
        utf8 += len;
        utf16++;
    }

    *utf16 = 0;

    return (int32_t)(utf16 - target_start);
}
#if 0
static uint32_t StepUTF8(const char **_str, const size_t slen) {
    /*
     * From rfc3629, the UTF-8 spec:
     *  https://www.ietf.org/rfc/rfc3629.txt
     *
     *   Char. number range  |        UTF-8 octet sequence
     *      (hexadecimal)    |              (binary)
     *   --------------------+---------------------------------------------
     *   0000 0000-0000 007F | 0xxxxxxx
     *   0000 0080-0000 07FF | 110xxxxx 10xxxxxx
     *   0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
     *   0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
     */

    const uint8_t *str = (const uint8_t *) *_str;
    const uint32_t octet = (uint32_t) (slen ? *str : 0);

    if (octet == 0) {  // nullptr terminator, end of string.
        return 0;  // don't advance `*_str`.
    } else if ((octet & 0x80) == 0) {  // 0xxxxxxx: one byte codepoint.
        (*_str)++;
        return octet;
    } else if (((octet & 0xE0) == 0xC0) && (slen >= 2)) {  // 110xxxxx 10xxxxxx: two byte codepoint.
        const uint8_t str1 = str[1];
        if ((str1 & 0xC0) == 0x80) {  // If trailing bytes aren't 10xxxxxx, sequence is bogus.
            const uint32_t result = ((octet & 0x1F) << 6) | (str1 & 0x3F);
            if (result >= 0x0080) {  // rfc3629 says you can't use overlong sequences for smaller values.
                *_str += 2;
                return result;
            }
        }
    } else if (((octet & 0xF0) == 0xE0) && (slen >= 3)) {  // 1110xxxx 10xxxxxx 10xxxxxx: three byte codepoint.
        const uint8_t str1 = str[1];
        const uint8_t str2 = str[2];
        if (((str1 & 0xC0) == 0x80) && ((str2 & 0xC0) == 0x80)) {  // If trailing bytes aren't 10xxxxxx, sequence is bogus.
            const uint32_t octet2 = ((uint32_t) (str1 & 0x3F)) << 6;
            const uint32_t octet3 = ((uint32_t) (str2 & 0x3F));
            const uint32_t result = ((octet & 0x0F) << 12) | octet2 | octet3;
            if (result >= 0x800) {  // rfc3629 says you can't use overlong sequences for smaller values.
                if ((result < 0xD800) || (result > 0xDFFF)) {  // UTF-16 surrogate values are illegal in UTF-8.
                    *_str += 3;
                    return result;
                }
            }
        }
    } else if (((octet & 0xF8) == 0xF0) && (slen >= 4)) {  // 11110xxxx 10xxxxxx 10xxxxxx 10xxxxxx: four byte codepoint.
        const uint8_t str1 = str[1];
        const uint8_t str2 = str[2];
        const uint8_t str3 = str[3];
        if (((str1 & 0xC0) == 0x80) && ((str2 & 0xC0) == 0x80) && ((str3 & 0xC0) == 0x80)) {  // If trailing bytes aren't 10xxxxxx, sequence is bogus.
            const uint32_t octet2 = ((uint32_t) (str1 & 0x1F)) << 12;
            const uint32_t octet3 = ((uint32_t) (str2 & 0x3F)) << 6;
            const uint32_t octet4 = ((uint32_t) (str3 & 0x3F));
            const uint32_t result = ((octet & 0x07) << 18) | octet2 | octet3 | octet4;
            if (result >= 0x10000) {  // rfc3629 says you can't use overlong sequences for smaller values.
                *_str += 4;
                return result;
            }
        }
    }

    // bogus byte, skip ahead, return a REPLACEMENT CHARACTER.
    (*_str)++;
    return _CC_INVALID_UNICODE_CODEPOINT_;
}

uint32_t StepUTF8(const char **pstr, size_t *pslen) {
    if (!pslen) {
        return StepUTF8(pstr, 4);  // 4 == max codepoint size.
    }
    const char *origstr = *pstr;
    const uint32_t result = StepUTF8(pstr, *pslen);
    *pslen -= (size_t) (*pstr - origstr);
    return result;
}

uint32_t StepBackUTF8(const char *start, const char **pstr) {
    if (!pstr || *pstr <= start) {
        return 0;
    }

    // Step back over the previous UTF-8 character
    const char *str = *pstr;
    do {
        if (str == start) {
            break;
        }
        --str;
    } while ((*str & 0xC0) == 0x80);

    size_t length = (*pstr - str);
    *pstr = str;
    return StepUTF8(&str, length);
}

#if (CC_SIZEOF_WCHAR_T == 2)
static uint32_t StepUTF16(const uint16_t **_str, const size_t slen) {
    const uint16_t *str = *_str;
    uint32_t cp = (uint32_t) *(str++);
    if (cp == 0) {
        return 0;  // don't advance string pointer.
    } else if ((cp >= 0xDC00) && (cp <= 0xDFFF)) {
        cp = _CC_INVALID_UNICODE_CODEPOINT_;  // Orphaned second half of surrogate pair
    } else if ((cp >= 0xD800) && (cp <= 0xDBFF)) {  // start of surrogate pair!
        const uint32_t pair = (uint32_t) *str;
        if ((pair == 0) || ((pair < 0xDC00) || (pair > 0xDFFF))) {
            cp = _CC_INVALID_UNICODE_CODEPOINT_;
        } else {
            str++;  // eat the other surrogate.
            cp = 0x10000 + (((cp - 0xD800) << 10) | (pair - 0xDC00));
        }
    }

    *_str = str;
    return (cp > 0x10FFFF) ? _CC_INVALID_UNICODE_CODEPOINT_ : cp;
}
#elif (CC_SIZEOF_WCHAR_T == 4)
static uint32_t StepUTF32(const uint32_t **_str, const size_t slen) {
    if (!slen) {
        return 0;
    }

    const uint32_t *str = *_str;
    const uint32_t cp = *str;
    if (cp == 0) {
        return 0;  // don't advance string pointer.
    }

    (*_str)++;
    return (cp > 0x10FFFF) ? _CC_INVALID_UNICODE_CODEPOINT_ : cp;
}
#endif

#endif