#include <libcc/alloc.h>
#include <libcc/crypto/base32.h>

/* {{{ base32 tables */
//ABCDEFGHIJKLMNOPQRSTUVWXYZ234567
static const tchar_t base32_table[] = {
    _T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F'), _T('G'), _T('H'), _T('I'), _T('J'), _T('K'),
    _T('L'), _T('M'), _T('N'), _T('O'), _T('P'), _T('Q'), _T('R'), _T('S'), _T('T'), _T('U'), _T('V'),
    _T('W'), _T('X'), _T('Y'), _T('Z'), _T('2'), _T('3'), _T('4'), _T('5'), _T('6'), _T('7'), _T('\0')};

static const short base32_reverse_table[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 26, 27, 28, 29, 30, 31, -1, -1, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};
/* }}} */

/* {{{ */
_CC_API_PUBLIC(size_t) _cc_base32_encode(const byte_t *input, size_t length, tchar_t *output, size_t output_length) {
    const byte_t *in_end = input + length;
    tchar_t *out_ptr = output;
    uint32_t buffer = 0;
    uint32_t bits = 0;
    size_t pad_count;
	size_t required_length;

    if (_cc_unlikely(input == NULL || output == NULL || length == 0)) {
        return 0;
    }

    /* Ensure output buffer is large enough: ceil(length/5) * 8 + 1 */
    required_length = ((length + 4) / 5) * 8 + 1;
    if (output_length < required_length) {
        return 0;
    }

    /* Process 5 bytes at a time for efficiency */
    while (input < in_end) {
        buffer = (buffer << 8) | *input++;
        bits += 8;

        while (bits >= 5) {
            *out_ptr++ = base32_table[(buffer >> (bits - 5)) & 0x1F];
            bits -= 5;
        }
    }

    /* Handle remaining bits */
    if (bits > 0) {
        *out_ptr++ = base32_table[(buffer << (5 - bits)) & 0x1F];
    }

    /* Add padding to make length a multiple of 8 */
    pad_count = (8 - ((size_t)(out_ptr - output) & 7)) & 7;
    while (pad_count--) {
        *out_ptr++ = _T('=');
    }

    *out_ptr = _T('\0');
    return (size_t)(out_ptr - output);
}

/* {{{ */
_CC_API_PUBLIC(size_t) _cc_base32_decode(const tchar_t *input, size_t length, byte_t *output, size_t output_length) {
    const tchar_t *in_ptr = input;
    const tchar_t *in_end = input + length;
    byte_t *out_ptr = output;
    byte_t *out_end = output + output_length;
    uint32_t buffer = 0;
    uint32_t bits = 0;
	size_t required_length;
    int v;

    if (_cc_unlikely(output == NULL || input == NULL)) {
        return 0;
    }

    /* Calculate required output length: ceil(length/8) * 5 */
    required_length = _CC_BASE32_DE_LEN(length);
    if (output_length < required_length) {
        return 0;
    }

    while (in_ptr < in_end && *in_ptr != _T('\0')) {
        /* Stop at padding character */
        if (*in_ptr == _T('=')) {
            break;
        }

        v = base32_reverse_table[(unsigned char)*in_ptr++];
        
        if (v >= 0) {
            buffer = (buffer << 5) | v;
            bits += 5;
            
            if (bits >= 8 && out_ptr < out_end) {
                *out_ptr++ = (byte_t)((buffer >> (bits - 8)) & 0xFF);
                bits -= 8;
            }
        }
    }

    return (size_t)(out_ptr - output);
}
