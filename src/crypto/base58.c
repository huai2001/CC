#include <libcc/alloc.h>
#include <libcc/crypto/base58.h>

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
    byte_t *out_ptr = (byte_t *)output;
    size_t idx = 0;
    size_t total = 0;
    size_t leading_zeros = 0;
    size_t i, j;
	size_t max_output;

    if (_cc_unlikely(input == NULL || output == NULL || length == 0)) {
        return 0;
    }

    /* Count leading zeros (Bitcoin style) */
    while (leading_zeros < length && input[leading_zeros] == 0) {
        if (total >= output_length) {
            return 0;
        }
        output[total++] = base58_table[0];
        leading_zeros++;
    }

    input += leading_zeros;
    length -= leading_zeros;

    /* Maximum possible output length: ceil(length * log(256) / log(58)) */
    max_output = (length * 137 / 100) + leading_zeros;
    if (output_length < max_output + 1) {
        return 0;
    }

    /* Encoding - process each input byte */
    for (i = 0; i < length; i++) {
        unsigned int carry = input[i];
        
        for (j = 0; j < idx; j++) {
            carry += (unsigned int)out_ptr[j] << 8;
            out_ptr[j] = (byte_t)(carry % 58);
            carry /= 58;
        }
        
        while (carry > 0) {
            out_ptr[idx++] = (byte_t)(carry % 58);
            carry /= 58;
        }
    }

    /* Apply alphabet and reverse the result */
    total = idx / 2;
    for (i = 0; i < total; i++) {
        byte_t temp = base58_table[out_ptr[i]];
        out_ptr[i] = base58_table[out_ptr[idx - i - 1]];
        out_ptr[idx - i - 1] = temp;
    }
    if (idx & 1) {
        out_ptr[total] = base58_table[out_ptr[total]];
    }

    /* Move encoded data after leading zeros */
    if (leading_zeros > 0 && idx > 0) {
        memmove(output + leading_zeros, out_ptr, idx);
    }

    total = leading_zeros + idx;
    output[total] = _T('\0');
    return total;
}

/* {{{ */
_CC_API_PUBLIC(size_t) _cc_base58_decode(const tchar_t *input, size_t length, byte_t *output, size_t output_length) {
    byte_t *out_ptr = output;
    size_t idx = 0;
    size_t total = 0;
    size_t leading_ones = 0;
    size_t i, j;
	size_t max_output;

    if (_cc_unlikely(input == NULL || output == NULL)) {
        return 0;
    }

    /* Count leading ones (map to leading zeros) */
    while (leading_ones < length && input[leading_ones] == _T('1')) {
        if (total >= output_length) {
            return 0;
        }
        out_ptr[total++] = 0;
        leading_ones++;
    }

    input += leading_ones;
    length -= leading_ones;

    /* Maximum possible output length: ceil(length * log(58) / log(256)) */
    max_output = (length * 733 / 1000) + leading_ones;
    if (output_length < max_output) {
        return 0;
    }

    /* Decoding - process each input character */
    for (i = 0; i < length; i++) {
        unsigned int carry = base58_alphabet_table[(unsigned char)input[i]];
        
        if (carry == 0xFF) {
            return 0; /* Invalid character */
        }
        
        for (j = 0; j < idx; j++) {
            carry += (unsigned int)out_ptr[leading_ones + j] * 58;
            out_ptr[leading_ones + j] = (byte_t)(carry & 0xFF);
            carry >>= 8;
        }
        
        while (carry > 0) {
            if (leading_ones + idx >= output_length) {
                return 0;
            }
            out_ptr[leading_ones + idx++] = (byte_t)(carry & 0xFF);
            carry >>= 8;
        }
    }

    /* Reverse the result */
    total = idx / 2;
    for (i = 0; i < total; i++) {
        byte_t temp = out_ptr[leading_ones + i];
        out_ptr[leading_ones + i] = out_ptr[leading_ones + idx - i - 1];
        out_ptr[leading_ones + idx - i - 1] = temp;
    }

    total = leading_ones + idx;
    output[total] = _T('\0');
    return total;
}
