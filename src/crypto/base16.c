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
#include <libcc/crypto/base16.h>

/* {{{ base16 tables */
static const tchar_t base16_table[] = {
    _T('0'), _T('1'), _T('2'), _T('3'), _T('4'), _T('5'), _T('6'), _T('7'),
    _T('8'), _T('9'), _T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F')};

static const short base16_reverse_table[256] = {
    0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1,  2,  3,  4,  5,  6,  7, 8, 9, 0, 0, 0, 0, 0, 0, /* 0 - 9 */
    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* A - F */
    0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* a - f */
    0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0,
};
/* }}} */

/* {{{ */
_CC_API_PUBLIC(size_t) _cc_base16_encode(const byte_t *input, size_t length, tchar_t *output, size_t output_length) {
    const byte_t *current = input;
    tchar_t *p = output;

    if (_cc_unlikely(length < 0 || output == nullptr)) {
        return 0;
    }

    if (_cc_unlikely((length * 2) > output_length)) {
        return -1;
    }

    while (length > 0) {
        *p++ = base16_table[(*current >> 4) & 0x0F];
        *p++ = base16_table[*current & 0x0F];

        current++;
        /* we just handle 1 octets of data */
        length--;
    }

    *p = 0;

    return (size_t)(p - output);
}

/* {{{ */
_CC_API_PUBLIC(size_t) _cc_base16_decode(const tchar_t *input, size_t length, byte_t *output, size_t output_length) {
    size_t i = 0;
    byte_t *p = output;
    /* this sucks for threaded environments */
    if (_cc_unlikely(output == nullptr)) {
        return 0;
    }

    if (_cc_unlikely((length / 2) > output_length)) {
        return -1;
    }

    /* run through the whole string, converting as we go */
    for (i = 0; i < length; i += 2) {
        *p = (base16_reverse_table[*input++ & 0x7F] << 4);
        *p++ |= (base16_reverse_table[*input++ & 0x7F]);
    }

    *p = 0;

    return (size_t)(p - output);
}
