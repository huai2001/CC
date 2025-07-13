#include <libcc/alloc.h>
#include <libcc/endian.h>
#include <libcc/xxtea.h>

#define MX (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^ ((sum ^ y) + (key[(p & 3) ^ e] ^ z))

#define _DELTA_ 0x9e3779b9

_CC_API_PRIVATE(uint32_t *)
xxtea_to_uint_array(const uint8_t *data, size_t len, int input_length, size_t *output_length) {
    uint32_t *out;
    size_t n;
#if !(defined(_CC_BYTEORDER_) && (BYTE_ORDER == _CC_LIL_ENDIAN_))
    size_t i;
#endif

    n = (((len & 3) == 0) ? (len >> 2) : ((len >> 2) + 1));
    out = (uint32_t *)_cc_calloc(n + 1, sizeof(uint32_t));
    if (!out) {
        return nullptr;
    }

    if (input_length) {
        out[n] = (uint32_t)len;
        *output_length = n + 1;
    } else {
        *output_length = n;
    }
#if defined(_CC_BYTEORDER_) && (BYTE_ORDER == _CC_LIL_ENDIAN_)
    memcpy(out, data, len);
#else
    for (i = 0; i < len; ++i) {
        out[i >> 2] |= (uint32_t)data[i] << ((i & 3) << 3);
    }
#endif

    return out;
}

_CC_API_PRIVATE(uint8_t *)
xxtea_to_bytes_array(const uint32_t *data, size_t len, int input_length, size_t *output_length) {
    uint8_t *out;
#if !(defined(_CC_BYTEORDER_) && (_CC_BYTEORDER_ == _CC_LIL_ENDIAN_))
    size_t i;
#endif
    size_t m, n = len << 2;

    if (input_length) {
        m = data[len - 1];
        n -= 4;
        if ((m < n - 3) || (m > n)) {
            return nullptr;
        }
        n = m;
    }

    out = (uint8_t *)_cc_malloc(n + 1);

#if defined(_CC_BYTEORDER_) && (_CC_BYTEORDER_ == _CC_LIL_ENDIAN_)
    memcpy(out, data, n);
#else
    for (i = 0; i < n; ++i) {
        out[i] = (uint8_t)(data[i >> 2] >> ((i & 3) << 3));
    }
#endif

    out[n] = '\0';
    *output_length = n;

    return out;
}

_CC_API_PRIVATE(uint32_t *) xxtea_uint_encrypt(uint32_t *data, size_t len, uint32_t *key) {
    size_t n = len - 1;
    uint32_t z = data[n], y, p, sum, e;
    size_t q;

    if (n < 1) {
        return data;
    }

    q = 6 + 52 / len;
    sum = 0;
    while (0 < q--) {
        sum += _DELTA_;
        e = sum >> 2 & 3;

        for (p = 0; p < n; p++) {
            y = data[p + 1];
            z = data[p] += MX;
        }

        y = data[0];
        z = data[n] += MX;
    }

    return data;
}

_CC_API_PRIVATE(uint32_t *) xxtea_uint_decrypt(uint32_t *data, size_t len, uint32_t *key) {
    size_t n = len - 1;
    uint32_t z, y = data[0], sum, e;
    size_t q, p;
    if (n < 1) {
        return data;
    }
    q = 6 + 52 / len;
    sum = (uint32_t)(q * _DELTA_);

    while (sum != 0) {
        e = sum >> 2 & 3;

        for (p = n; p > 0; p--) {
            z = data[p - 1];
            y = data[p] -= MX;
        }

        z = data[n];
        y = data[0] -= MX;
        sum -= _DELTA_;
    }

    return data;
}

_CC_API_PRIVATE(uint8_t *)
xxtea_bytes_encrypt(const uint8_t *data, size_t len, const uint8_t *key, size_t *output_length) {
    uint8_t *out;
    uint32_t *data_array, *key_array;
    size_t data_len, key_len;

    if (!len) {
        return nullptr;
    }

    data_array = xxtea_to_uint_array(data, len, 1, &data_len);
    if (!data_array) {
        return (uint8_t *)data;
    }

    key_array = xxtea_to_uint_array(key, 16, 0, &key_len);
    if (!key_array) {
        _cc_free(data_array);
        return nullptr;
    }

    out = xxtea_to_bytes_array(xxtea_uint_encrypt(data_array, data_len, key_array), data_len, 0, output_length);

    _cc_free(data_array);
    _cc_free(key_array);

    return out;
}

_CC_API_PRIVATE(uint8_t *)
xxtea_bytes_decrypt(const uint8_t *data, size_t len, const uint8_t *key, size_t *output_length) {
    uint8_t *out;
    uint32_t *data_array, *key_array;
    size_t data_len, key_len;

    if (!len) {
        return nullptr;
    }

    data_array = xxtea_to_uint_array(data, len, 0, &data_len);
    if (!data_array) {
        return (uint8_t *)data;
    }

    key_array = xxtea_to_uint_array(key, 16, 0, &key_len);
    if (!key_array) {
        _cc_free(data_array);
        return nullptr;
    }

    out = xxtea_to_bytes_array(xxtea_uint_decrypt(data_array, data_len, key_array), data_len, 1, output_length);

    _cc_free(data_array);
    _cc_free(key_array);

    return out;
}

_CC_API_PUBLIC(byte_t *) _cc_xxtea_encrypt(const byte_t *data, size_t len, const byte_t *key, size_t *output_length) {
    int32_t i;
    uint8_t fixed[16];
    for (i = 0; i < 16 && (*(key + i) != 0); ++i) {
        fixed[i] = *(key + i);
    }

    for (; i < 16; ++i) {
        fixed[i] = 0;
    }
    return xxtea_bytes_encrypt((const uint8_t *)data, len, fixed, output_length);
}

_CC_API_PUBLIC(byte_t *) _cc_xxtea_decrypt(const byte_t *data, size_t len, const byte_t *key, size_t *output_length) {
    int32_t i;
    uint8_t fixed[16];
    for (i = 0; i < 16 && (*(key + i) != 0); ++i) {
        fixed[i] = *(key + i);
    }

    for (; i < 16; ++i) {
        fixed[i] = 0;
    }
    return xxtea_bytes_decrypt((const uint8_t *)data, len, fixed, output_length);
}
