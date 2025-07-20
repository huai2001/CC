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
#include <libcc/base58.h>

/* {{{ base58 tables */
//"123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
static const tchar_t base58_table[] = {
    _T('1'), _T('2'), _T('3'), _T('4'), _T('5'), _T('6'), _T('7'), _T('8'), _T('9'),
    _T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F'), _T('G'), _T('H'), _T('J'), _T('K'),
    _T('L'), _T('M'), _T('N'), _T('P'), _T('Q'), _T('R'), _T('S'), _T('T'), _T('U'), _T('V'),
    _T('W'), _T('X'), _T('Y'), _T('Z'), _T('a'), _T('b'), _T('c'), _T('d'), _T('e'), _T('f'), _T('g'),
    _T('h'), _T('i'), _T('j'), _T('k'), _T('m'), _T('n'), _T('o'), _T('p'), _T('q'), _T('r'),
    _T('s'), _T('t'), _T('u'), _T('v'), _T('w'), _T('x'), _T('y'), _T('z'), _T('\0')};

static const uint8_t base58_alphabet_table[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0xFF, 0x11, 0x12, 0x13, 0x14, 0x15, 0xFF,
    0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0xFF, 0x2C, 0x2D, 0x2E,
    0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

/* }}} */

/* {{{ */
_CC_API_PUBLIC(size_t) _cc_base58_encode(const byte_t *input, size_t length, tchar_t *output, size_t output_length) {
    size_t total = 0;
    size_t idx = 0;
    size_t c_idx;
    size_t i, j;

    if (_cc_unlikely(length < 0 || output == nullptr)) {
        return 0;
    }
#if 0
    // leading zeroes
    for (i = 0; i < length && !input[i]; ++i) {
        if (total == output_length) {
            return 0;
        }
        output[total++] = base58_table[0];
    }

    input += total;
    length -= total;
    output += total;
#endif
    // encoding
    for (i = 0; i < length; ++i) {
        unsigned int carry = (byte_t)input[i];
        for (j = 0; j < idx; ++j) {
            carry += (unsigned int)output[j] << 8;
            output[j] = (byte_t)(carry % 58);
            carry /= 58;
        }
        while (carry > 0) {
            if (total == output_length) {
                return 0;
            }
            total++;
            output[idx++] = (byte_t)(carry % 58);
            carry /= 58;
        }
    }

    // apply alphabet and reverse
    c_idx = idx >> 1;
    for (i = 0; i < c_idx; ++i) {
        byte_t s = base58_table[(byte_t)output[i]];
        output[i] = base58_table[(byte_t)output[idx - (i + 1)]];
        output[idx - (i + 1)] = s;
    }

    if ((idx & 1)) {
        output[c_idx] = base58_table[(byte_t)output[c_idx]];
    }

    output[total] = 0;
    return total;
}

/* {{{ */
_CC_API_PUBLIC(size_t) _cc_base58_decode(const tchar_t *input, size_t length, byte_t *output, size_t output_length) {
    size_t total = 0;
    size_t idx = 0;
    size_t c_idx;
    size_t i,j;

    // leading ones
    if (_cc_unlikely(output == nullptr)) {
        return 0;
    }
#if 0
    for (i = 0; i < length && input[i] == _T('1'); ++i) {
        if (total == output_length) {
            return -1;
        }
        output[total++] = 0;
    }
    input += total;
    length -= total;
    output += total;
#endif
    // decoding
    for (i = 0; i < length; ++i) {
        unsigned int carry = (unsigned int)base58_alphabet_table[(byte_t)input[i]];
        if (carry == UINT_MAX) {
            return 0;
        }
        for (j = 0; j < idx; j++) {
            carry += (byte_t)output[j] * 58;
            output[j] = (byte_t)(carry & 0xff);
            carry >>= 8;
        }
        while (carry > 0) {
            if (total == output_length) {
                return 0;
            }
            total++;
            output[idx++] = (byte_t)(carry & 0xff);
            carry >>= 8;
        }
    }

    // apply simple reverse
    c_idx = idx >> 1;
    for (i = 0; i < c_idx; ++i) {
        byte_t s = output[i];
        output[i] = output[idx - (i + 1)];
        output[idx - (i + 1)] = s;
    }
    
    output[total] = 0;
    return total;
}
