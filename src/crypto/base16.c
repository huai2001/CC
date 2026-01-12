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
    const byte_t *in_end = input + length;
    tchar_t *out_ptr = output;

    if (_cc_unlikely(input == NULL || output == NULL || length == 0)) {
        return 0;
    }

    /* Check output buffer size: length * 2 + 1 for null terminator */
    if (_cc_unlikely((length * 2 + 1) > output_length)) {
        return 0;
    }

    while (input < in_end) {
        *out_ptr++ = base16_table[(*input >> 4) & 0x0F];
        *out_ptr++ = base16_table[*input++ & 0x0F];
    }

    *out_ptr = _T('\0');
    return (size_t)(out_ptr - output);
}

/* {{{ */
_CC_API_PUBLIC(size_t) _cc_base16_decode(const tchar_t *input, size_t length, byte_t *output, size_t output_length) {
    const tchar_t *in_end = input + length;
    byte_t *out_end = output + output_length;
    byte_t *out_ptr = output;

    if (_cc_unlikely(input == NULL || output == NULL)) {
        return 0;
    }

    /* Check output buffer size */
    if (_cc_unlikely((length + 1) / 2 > output_length)) {
        return 0;
    }

    while (input + 1 < in_end && out_ptr < out_end) {
        int high = base16_reverse_table[(byte_t)*input++ & 0x7F];
        int low = base16_reverse_table[(byte_t)*input++ & 0x7F];
        *out_ptr++ = (byte_t)((high << 4) | low);
    }

    *out_ptr = _T('\0');
    return (size_t)(out_ptr - output);
}
