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
#include <cc/base64.h>

/** @var base64_chars
 *   A 64 character alphabet.
 *
 *   A 64-character subset of international Alphabet IA5, enabling
 *   6 bits to be represented per printable character.  (The proposed
 *   subset of characters is represented identically in IA5 and ASCII.)
 *   The character "=" signifies a special processing function used for
 *   padding within the printable encoding procedure.
 *
 *
 Value Encoding  Value Encoding  Value Encoding  Value Encoding
   0      A        17     R        34     i        51     z
   1      B        18     S        35     j        52     0
   2      C        19     T        36     k        53     1
   3      D        0      U        37     l        54     2
   4      E        21     V        38     m        55     3
   5      F        22     W        39     n        56     4
   6      G        23     X        40     o        57     5
   7      H        24     Y        41     p        58     6
   8      I        25     Z        42     q        59     7
   9      J        26     a        43     r        60     8
   10     K        27     b        44     s        61     9
   11     L        28     c        45     t        62     +
   12     M        29     d        46     u        63     /
   13     N        30     e        47     v
   14     O        31     f        48     w        (pad)  =
   15     P        32     g        49     x
   16     Q        33     h        50     y
 */
/* {{{ base64 tables */
static const tchar_t base64_table[] = {
    _T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F'), _T('G'), _T('H'),
    _T('I'), _T('J'), _T('K'), _T('L'), _T('M'), _T('N'), _T('O'), _T('P'),
    _T('Q'), _T('R'), _T('S'), _T('T'), _T('U'), _T('V'), _T('W'), _T('X'),
    _T('Y'), _T('Z'), _T('a'), _T('b'), _T('c'), _T('d'), _T('e'), _T('f'),
    _T('g'), _T('h'), _T('i'), _T('j'), _T('k'), _T('l'), _T('m'), _T('n'),
    _T('o'), _T('p'), _T('q'), _T('r'), _T('s'), _T('t'), _T('u'), _T('v'),
    _T('w'), _T('x'), _T('y'), _T('z'), _T('0'), _T('1'), _T('2'), _T('3'),
    _T('4'), _T('5'), _T('6'), _T('7'), _T('8'), _T('9'), _T('+'), _T('/'),
    _T('\0')};

static const tchar_t base64_pad = _T('=');

static const short base64_reverse_table[256] = {
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -2, -2, -1, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, 62, -2, -2, -2, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, -2, -2, -2, -2, -2, -2, -2, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2,
    -2, -2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -2, -2};
/* }}} */

/* {{{ */
int32_t _cc_base64_encode(const byte_t *input, int32_t length, tchar_t *output, int32_t output_length) {
    const byte_t *current = input;
    tchar_t *p = output;

    if (_cc_unlikely(length < 0 || output == NULL)) {
        return 0;
    }

    while (length > 2 && output_length >= 4) { /* keep going until we have less than 24 bits */
        *p++ = base64_table[current[0] >> 2];
        *p++ = base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
        *p++ = base64_table[((current[1] & 0x0f) << 2) + (current[2] >> 6)];
        *p++ = base64_table[current[2] & 0x3f];

        current += 3;
        /* we just handle 3 octets of data */
        length -= 3;
        output_length -= 4;
    }

    /* now deal with the tail end of things */
    if (length != 0 && output_length >= 4) {
        *p++ = base64_table[current[0] >> 2];
        if (length > 1) {
            *p++ = base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
            *p++ = base64_table[(current[1] & 0x0f) << 2];
            *p++ = base64_pad;
        } else {
            *p++ = base64_table[(current[0] & 0x03) << 4];
            *p++ = base64_pad;
            *p++ = base64_pad;
        }
    }

    *p = 0;

    return (int32_t)(p - output);
}

/* {{{ */
int32_t _cc_base64_decode(const tchar_t *input, int32_t length, byte_t *output, int32_t output_length) {
    const tchar_t *current = input;
    int32_t ch, i = 0, j = 0, k;
    /* this sucks for threaded environments */
    if (_cc_unlikely(output == NULL)) {
        return 0;
    }

    /* run through the whole string, converting as we go */
    while ((ch = *current++) != 0 && length-- > 0) {
        if (_cc_unlikely(ch == base64_pad)) {
            /*i % 4 = i & 3*/
            if (*current != base64_pad && ((i & 3) == 1)) {
                if ((i & 3) != 1) {
                    while (_cc_isspace(*(++current))) {
                        continue;
                    }
                    if (*current == 0) {
                        continue;
                    }
                }
                return 0;
            }
            continue;
        }

        if (ch > 256) {
            return 0;
        }

        ch = base64_reverse_table[ch];
        /* a space or some other separator character, we simply skip over */
        if (ch < 0 || ch == -1) {
            continue;
        } else if (ch == -2) {
            return 0;
        }
        /*i % 4 == i & 3*/
        switch (i & 3) {
        case 0:
            output[j] = ch << 2;
            break;
        case 1:
            output[j++] |= ch >> 4;
            output[j] = (ch & 0x0f) << 4;
            break;
        case 2:
            output[j++] |= ch >> 2;
            output[j] = (ch & 0x03) << 6;
            break;
        case 3:
            output[j++] |= ch;
            break;
        }
        i++;
        if (j > output_length) {
            return 0;
        }
    }

    k = j;

    /* mop things up if we ended on a boundary */
    if (ch == base64_pad) {
        /*i % 4 == i & 3*/
        switch (i & 3) {
        case 1:
            return 0;
        case 2:
            k++;
        case 3:
            output[k] = 0;
        }
    }

    output[j] = 0;

    return j;
}
