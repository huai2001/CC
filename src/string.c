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
#include <math.h>
#include <string.h>

#ifndef _CC_UNICODE_
#include <wchar.h>
#endif

#ifdef __CC_WINDOWS__
#include <Windows.h>
#endif

/* this is read-only, so it's ok */
const wchar_t _w_lower_xdigits[] = L"0123456789abcdef";
const wchar_t _w_upper_xdigits[] = L"0123456789ABCDEF";
const char_t _a_lower_xdigits[] = "0123456789abcdef";
const char_t _a_upper_xdigits[] = "0123456789ABCDEF";

#define __CC_TO_BYTE(CH, XX, OP)                                                                                       \
    do {                                                                                                               \
        if (XX <= _T('9')) {                                                                                           \
            CH OP (XX & 0x0F);                                                                                         \
        } else {                                                                                                       \
            CH OP ((XX & 0x0F) + 0x09);                                                                                \
        }                                                                                                              \
    } while (0)
#if 0
/*-----------------------------------------------------------------------------
 * Purpose: Returns the 4 bit nibble for a hex character
 * Input  : c -
 * Output : unsigned char
 *-----------------------------------------------------------------------------*/
_CC_API_PRIVATE(unsigned char) nibble(unsigned char c) {
    if ((c >= '0') && (c <= '9')) {
        return c - '0';
    }

    if ((c >= 'A') && (c <= 'F')) {
        return c - 'A' + 0x0a;
    }

    if ((c >= 'a') && (c <= 'f')) {
        return c - 'a' + 0x0a;
    }

    /* received an invalid character, and no real way to return an error */
    /* _cc_logger_error(_T("nibble invalid hex character '%c' "), c); */
    return 0;
}
#endif
/* parse hexadecimal number */
_CC_API_PUBLIC(uint64_t) _cc_hex16(const tchar_t *input) {
    uint64_t ch = 0;
    int i;

    __CC_TO_BYTE(ch, input[0], =);
    for (i = 1; i < 16; i++) {
        ch <<= 4;
        __CC_TO_BYTE(ch, input[i], +=);
    }

    return ch;
}

_CC_API_PUBLIC(uint32_t) _cc_hex8(const tchar_t *input) {
    uint32_t ch = 0;

    __CC_TO_BYTE(ch, input[0], =);
    ch <<= 4;
    __CC_TO_BYTE(ch, input[1], +=);
    ch <<= 4;
    __CC_TO_BYTE(ch, input[2], +=);
    ch <<= 4;
    __CC_TO_BYTE(ch, input[3], +=);
    ch <<= 4;
    __CC_TO_BYTE(ch, input[4], +=);
    ch <<= 4;
    __CC_TO_BYTE(ch, input[5], +=);
    ch <<= 4;
    __CC_TO_BYTE(ch, input[6], +=);
    ch <<= 4;
    __CC_TO_BYTE(ch, input[7], +=);

    return ch;
}

/* parse hexadecimal number */
_CC_API_PUBLIC(uint16_t) _cc_hex4(const tchar_t *input) {
    uint16_t ch = 0;

    __CC_TO_BYTE(ch, input[0], =);
    ch <<= 4;
    __CC_TO_BYTE(ch, input[1], +=);
    ch <<= 4;
    __CC_TO_BYTE(ch, input[2], +=);
    ch <<= 4;
    __CC_TO_BYTE(ch, input[3], +=);

    return ch;
}

/* parse hexadecimal number */
_CC_API_PUBLIC(uint8_t) _cc_hex2(const tchar_t *input) {
    uint8_t ch = 0;

    __CC_TO_BYTE(ch, input[0], =);
    ch <<= 4;
    __CC_TO_BYTE(ch, input[1], +=);

    return ch;
}

_CC_API_PUBLIC(size_t) _cc_bytes2hex(const byte_t *in, size_t in_len, tchar_t *out, size_t out_max_len) {
    size_t k = 0;
    size_t i = 0;
    byte_t ch = 0;

    _cc_assert(in != NULL && out != NULL);
    out_max_len -= 1;

    while (i < in_len && k < out_max_len) {
        ch = *(in + i++);

        out[k++] = _lower_xdigits[ch / 16];
        out[k++] = _lower_xdigits[ch & 15]; /*ch & 15 == ch % 16*/
    }

    out[k] = 0;

    return k;
}

/* ascii to bytes*/
_CC_API_PUBLIC(size_t) _cc_hex2bytes(const tchar_t *in, size_t in_len, byte_t *out, size_t out_max_len) {
    byte_t ch = 0;
    size_t i = 0, k = 0;

    _cc_assert(in != NULL && out != NULL);

    if (_cc_unlikely(!in || !out)) {
        return 0;
    }

    if (_cc_unlikely(in_len > (out_max_len * 2))) {
        return 0;
    }

    while (i < in_len && k <= out_max_len) {
        __CC_TO_BYTE(ch, in[i], =);
        ch <<= 4;
        __CC_TO_BYTE(ch, in[i + 1], +=);
        out[k++] = ch;

        i += 2;
    }

    return k;
}

#undef __CC_TO_BYTE

/* Returns a string converted to lower case */
_CC_API_PUBLIC(char_t*) _cc_to_lowerA(char_t *s) {
    char_t *_t = s;
    _cc_assert(s != NULL);

    for (; *_t && *_t != _T('\0'); _t++) {
        *_t = tolower(*_t);
    }
    return s;
}

/* Returns a string converted to upper case */
_CC_API_PUBLIC(char_t*) _cc_to_upperA(char_t *s) {
    char_t *_t = s;
    _cc_assert(_t != NULL);

    for (; *_t && *_t != _T('\0'); _t++) {
        *_t = toupper(*_t);
    }
    return s;
}

/* Returns a string converted to lower case */
_CC_API_PUBLIC(wchar_t*) _cc_to_lowerW(wchar_t *s) {
    wchar_t *_t = s;
    _cc_assert(s != NULL);

    for (; *_t && *_t != _T('\0'); _t++) {
        *_t = towlower(*_t);
    }
    return s;
}

/* Returns a string converted to upper case */
_CC_API_PUBLIC(wchar_t*) _cc_to_upperW(wchar_t *s) {
    wchar_t *_t = s;
    _cc_assert(_t != NULL);

    for (; *_t && *_t != _T('\0'); _t++) {
        *_t = towupper(*_t);
    }
    return s;
}

_CC_API_PUBLIC(int32_t) _cc_splitA(_cc_strA_t *dst, int32_t count, char_t *src, int32_t(separator_fn)(char_t *, int32_t)) {
    int32_t i = 0;
    _cc_strA_t *r;
    char_t *p;
    char_t *tmp;
    if (!src || !dst) {
        return 0;
    }

    tmp = p = src;
    while (*p) {
        int32_t rc = separator_fn(p, i);
        if (rc > 0) {
            r = &dst[i++];
            r->data = tmp;
            r->length = (size_t)(p - tmp);

            *p = 0;
            p += rc;
            tmp = p;

            if (i >= count) {
                break;
            }
            continue;
        } else if (rc < 0) {
            *p = 0;
            break;
        }
        p++;
    }

    if (tmp != p && count > i) {
        r = &dst[i++];
        r->data = tmp;
        r->length = (size_t)(p - tmp);
    }
    return i;
}

_CC_API_PUBLIC(int32_t) _cc_splitW(_cc_strW_t *dst, int32_t count, wchar_t *src, int32_t(separator_fn)(wchar_t *, int32_t)) {
    int32_t i = 0;
    _cc_strW_t *r;
    wchar_t *p;
    wchar_t *tmp;
    if (!src || !dst) {
        return 0;
    }

    tmp = p = src;
    while (*p) {
        int32_t rc = separator_fn(p, i);
        if (rc) {
            r = &dst[i++];
            r->data = tmp;
            r->length = (size_t)(p - tmp);

            *p = 0;
            p += rc;
            tmp = p;

            if (i >= count) {
                break;
            }
            continue;
        }
        p++;
    }

    if (tmp != p && count > i) {
        r = &dst[i++];
        r->data = tmp;
        r->length = (size_t)(p - tmp);
    }
    return i;
}

#undef _is_trim

/**/
_CC_API_PUBLIC(tchar_t*) _cc_substr(tchar_t *s1, const tchar_t *s2, uint32_t started, int32_t ended) {
    uint32_t len = 0;
    _cc_assert(s1 != NULL && s2 != NULL);

    if (!s1 || !s2) {
        return NULL;
    }

    len = (uint32_t)_tcslen(s1);
    /* */
    ended = (int32_t)((ended <= 0) ? ((len - started) + ended) : ended);
    /* */
    if (len < started || len < (started + ended)) {
        return NULL;
    }

    _tcsncpy(s1, (s2 + started), ended);
    s1[ended + 1] = _T('\0');

    return s1;
}

/* get the decimal point character of the current locale */
_CC_API_PRIVATE(tchar_t) get_decimal_point(void) {
#ifdef _CC_ENABLE_LOCALES
    struct lconv *lconv = localeconv();
    return (tchar_t)lconv->decimal_point[0];
#else
    return _T('.');
#endif
}

/* Parse the input text to generate a number, and populate the result into item.
 */
_CC_API_PUBLIC(const tchar_t*) _cc_to_number(const tchar_t *s, _cc_number_t *item) {
    float64_t n = 0, sign = 1, scale = 0;
    int32_t subscale = 0, signsubscale = 1;
    tchar_t decimal_point = get_decimal_point();

    /* Has sign? */
    if (*s == _T('-')) {
        sign = -1;
        s++;
    } else if (*s == _T('+')) {
        s++;
    }

    /* is zero */
    while (*s == _T('0')) {
        s++;
    }

    /* Number? */
    while (_CC_ISDIGIT(*s)) {
        n = (n * 10.0) + (*s++ - _T('0'));
    }

    /* Fractional part? */
    if (*s == decimal_point && _cc_isdigit(*(s + 1))) {
        s++;
        do {
            n = (n * 10.0) + (*s++ - _T('0'));
            scale--;
        } while (_CC_ISDIGIT(*s)); /* Number? */
    }

    /* Exponent? */
    if (*s == _T('e') || *s == _T('E')) {
        s++;
        if (*s == _T('+')) {
            s++;
        } else if (*s == _T('-')) {
            signsubscale = -1;
            s++; /* With sign? */
        }

        /* Number? */
        while (_CC_ISDIGIT(*s)) {
            subscale = (subscale * 10) + (*s++ - _T('0'));
        }
    }

    /* number = +/- number.fraction * 10^+/- exponent */
    n = sign * n * pow(10.0, (scale + subscale * signsubscale));

    if (scale == 0) {
        item->vt = _CC_NUMBER_INT_;
        item->v.uni_int = (int64_t)n;
    } else {
        item->vt = _CC_NUMBER_FLOAT_;
        item->v.uni_float = (float64_t)n;
    }

    return s;
}
