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
#include <cc/md5.h>
#include <cc/string.h>

/*
 * 32-bit integer manipulation macros (little endian)
 */
#ifndef GET_UINT32_LE
#define GET_UINT32_LE(n, b, i)                                                                                         \
    {                                                                                                                  \
        (n) = ((uint32_t)(b)[(i)]) | ((uint32_t)(b)[(i) + 1] << 8) | ((uint32_t)(b)[(i) + 2] << 16) |                  \
              ((uint32_t)(b)[(i) + 3] << 24);                                                                          \
    }
#endif

#ifndef PUT_UINT32_LE
#define PUT_UINT32_LE(n, b, i)                                                                                         \
    {                                                                                                                  \
        (b)[(i)] = (byte_t)(((n)) & 0xFF);                                                                             \
        (b)[(i) + 1] = (byte_t)(((n) >> 8) & 0xFF);                                                                    \
        (b)[(i) + 2] = (byte_t)(((n) >> 16) & 0xFF);                                                                   \
        (b)[(i) + 3] = (byte_t)(((n) >> 24) & 0xFF);                                                                   \
    }
#endif

/*
 * MD5 context setup
 */
_CC_API_PUBLIC(void) _cc_md5_init(_cc_md5_t *ctx) {
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
}

#if !defined(_CC_MD5_PROCESS_ALT)
_CC_API_PUBLIC(void) _cc_md5_process(_cc_md5_t *ctx, const byte_t data[64]) {
    uint32_t X[16], A, B, C, D;

    GET_UINT32_LE(X[0], data, 0);
    GET_UINT32_LE(X[1], data, 4);
    GET_UINT32_LE(X[2], data, 8);
    GET_UINT32_LE(X[3], data, 12);
    GET_UINT32_LE(X[4], data, 16);
    GET_UINT32_LE(X[5], data, 20);
    GET_UINT32_LE(X[6], data, 24);
    GET_UINT32_LE(X[7], data, 28);
    GET_UINT32_LE(X[8], data, 32);
    GET_UINT32_LE(X[9], data, 36);
    GET_UINT32_LE(X[10], data, 40);
    GET_UINT32_LE(X[11], data, 44);
    GET_UINT32_LE(X[12], data, 48);
    GET_UINT32_LE(X[13], data, 52);
    GET_UINT32_LE(X[14], data, 56);
    GET_UINT32_LE(X[15], data, 60);

#define S(x, n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define P(a, b, c, d, k, s, t)                                                                                         \
    {                                                                                                                  \
        a += F(b, c, d) + X[k] + t;                                                                                    \
        a = S(a, s) + b;                                                                                               \
    }

    A = ctx->state[0];
    B = ctx->state[1];
    C = ctx->state[2];
    D = ctx->state[3];

#define F(x, y, z) (z ^ (x & (y ^ z)))

    P(A, B, C, D, 0, 7, 0xD76AA478);
    P(D, A, B, C, 1, 12, 0xE8C7B756);
    P(C, D, A, B, 2, 17, 0x242070DB);
    P(B, C, D, A, 3, 22, 0xC1BDCEEE);
    P(A, B, C, D, 4, 7, 0xF57C0FAF);
    P(D, A, B, C, 5, 12, 0x4787C62A);
    P(C, D, A, B, 6, 17, 0xA8304613);
    P(B, C, D, A, 7, 22, 0xFD469501);
    P(A, B, C, D, 8, 7, 0x698098D8);
    P(D, A, B, C, 9, 12, 0x8B44F7AF);
    P(C, D, A, B, 10, 17, 0xFFFF5BB1);
    P(B, C, D, A, 11, 22, 0x895CD7BE);
    P(A, B, C, D, 12, 7, 0x6B901122);
    P(D, A, B, C, 13, 12, 0xFD987193);
    P(C, D, A, B, 14, 17, 0xA679438E);
    P(B, C, D, A, 15, 22, 0x49B40821);

#undef F

#define F(x, y, z) (y ^ (z & (x ^ y)))

    P(A, B, C, D, 1, 5, 0xF61E2562);
    P(D, A, B, C, 6, 9, 0xC040B340);
    P(C, D, A, B, 11, 14, 0x265E5A51);
    P(B, C, D, A, 0, 20, 0xE9B6C7AA);
    P(A, B, C, D, 5, 5, 0xD62F105D);
    P(D, A, B, C, 10, 9, 0x02441453);
    P(C, D, A, B, 15, 14, 0xD8A1E681);
    P(B, C, D, A, 4, 20, 0xE7D3FBC8);
    P(A, B, C, D, 9, 5, 0x21E1CDE6);
    P(D, A, B, C, 14, 9, 0xC33707D6);
    P(C, D, A, B, 3, 14, 0xF4D50D87);
    P(B, C, D, A, 8, 20, 0x455A14ED);
    P(A, B, C, D, 13, 5, 0xA9E3E905);
    P(D, A, B, C, 2, 9, 0xFCEFA3F8);
    P(C, D, A, B, 7, 14, 0x676F02D9);
    P(B, C, D, A, 12, 20, 0x8D2A4C8A);

#undef F

#define F(x, y, z) (x ^ y ^ z)

    P(A, B, C, D, 5, 4, 0xFFFA3942);
    P(D, A, B, C, 8, 11, 0x8771F681);
    P(C, D, A, B, 11, 16, 0x6D9D6122);
    P(B, C, D, A, 14, 23, 0xFDE5380C);
    P(A, B, C, D, 1, 4, 0xA4BEEA44);
    P(D, A, B, C, 4, 11, 0x4BDECFA9);
    P(C, D, A, B, 7, 16, 0xF6BB4B60);
    P(B, C, D, A, 10, 23, 0xBEBFBC70);
    P(A, B, C, D, 13, 4, 0x289B7EC6);
    P(D, A, B, C, 0, 11, 0xEAA127FA);
    P(C, D, A, B, 3, 16, 0xD4EF3085);
    P(B, C, D, A, 6, 23, 0x04881D05);
    P(A, B, C, D, 9, 4, 0xD9D4D039);
    P(D, A, B, C, 12, 11, 0xE6DB99E5);
    P(C, D, A, B, 15, 16, 0x1FA27CF8);
    P(B, C, D, A, 2, 23, 0xC4AC5665);

#undef F

#define F(x, y, z) (y ^ (x | ~z))

    P(A, B, C, D, 0, 6, 0xF4292244);
    P(D, A, B, C, 7, 10, 0x432AFF97);
    P(C, D, A, B, 14, 15, 0xAB9423A7);
    P(B, C, D, A, 5, 21, 0xFC93A039);
    P(A, B, C, D, 12, 6, 0x655B59C3);
    P(D, A, B, C, 3, 10, 0x8F0CCC92);
    P(C, D, A, B, 10, 15, 0xFFEFF47D);
    P(B, C, D, A, 1, 21, 0x85845DD1);
    P(A, B, C, D, 8, 6, 0x6FA87E4F);
    P(D, A, B, C, 15, 10, 0xFE2CE6E0);
    P(C, D, A, B, 6, 15, 0xA3014314);
    P(B, C, D, A, 13, 21, 0x4E0811A1);
    P(A, B, C, D, 4, 6, 0xF7537E82);
    P(D, A, B, C, 11, 10, 0xBD3AF235);
    P(C, D, A, B, 2, 15, 0x2AD7D2BB);
    P(B, C, D, A, 9, 21, 0xEB86D391);

#undef F

    ctx->state[0] += A;
    ctx->state[1] += B;
    ctx->state[2] += C;
    ctx->state[3] += D;
}
#endif /* !CC_MD5_PROCESS_ALT */

/*
 * MD5 process buffer
 */
_CC_API_PUBLIC(void) _cc_md5_update(_cc_md5_t *ctx, const byte_t *input, size_t ilen) {
    size_t fill;
    uint32_t left;

    if (ilen == 0) {
        return;
    }

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += (uint32_t)ilen;
    ctx->total[0] &= 0xFFFFFFFF;

    if (ctx->total[0] < (uint32_t)ilen) {
        ctx->total[1]++;
    }

    if (left && ilen >= fill) {
        memcpy((void *)(ctx->buffer + left), input, fill);
        _cc_md5_process(ctx, ctx->buffer);
        input += fill;
        ilen -= fill;
        left = 0;
    }

    while (ilen >= 64) {
        _cc_md5_process(ctx, input);
        input += 64;
        ilen -= 64;
    }

    if (ilen > 0) {
        memcpy((void *)(ctx->buffer + left), input, ilen);
    }
}

static const byte_t md5_padding[64] = {0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                       0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                       0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/*
 * MD5 final digest
 */
_CC_API_PUBLIC(void) _cc_md5_final(_cc_md5_t *ctx, byte_t *output) {
    uint32_t last, padn;
    uint32_t high, low;
    byte_t msglen[8];

    high = (ctx->total[0] >> 29) | (ctx->total[1] << 3);
    low = (ctx->total[0] << 3);

    PUT_UINT32_LE(low, msglen, 0);
    PUT_UINT32_LE(high, msglen, 4);

    last = ctx->total[0] & 0x3F;
    padn = (last < 56) ? (56 - last) : (120 - last);

    _cc_md5_update(ctx, md5_padding, padn);
    _cc_md5_update(ctx, msglen, 8);

    PUT_UINT32_LE(ctx->state[0], output, 0);
    PUT_UINT32_LE(ctx->state[1], output, 4);
    PUT_UINT32_LE(ctx->state[2], output, 8);
    PUT_UINT32_LE(ctx->state[3], output, 12);
}

/*
    Digests a file.
 */
_CC_API_PUBLIC(bool_t) _cc_md5_fp(FILE *fp, tchar_t *output) {
    byte_t md[_CC_MD5_DIGEST_LENGTH_];
    byte_t buf[1024 * 16];
    size_t i;
    long seek_cur = 0;
    _cc_md5_t c;

    if (fp == NULL) {
        return false;
    }

    _cc_md5_init(&c);

    seek_cur = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    while ((i = fread(buf, sizeof(byte_t), _cc_countof(buf), fp))) {
        _cc_md5_update(&c, buf, i);
    }

    _cc_md5_final(&c, &(md[0]));

    fseek(fp, seek_cur, SEEK_CUR);

    _cc_bytes2hex(md, _CC_MD5_DIGEST_LENGTH_, output, _CC_MD5_DIGEST_LENGTH_ * 2);

    return true;
}

/*
    Digests a file.
 */
_CC_API_PUBLIC(bool_t) _cc_md5file(const tchar_t *filename, tchar_t *output) {
    FILE *fp = _tfopen(filename, _T("rb"));

    if (fp) {
        _cc_md5_fp(fp, output);
        fclose(fp);
        return true;
    }
    return false;
}

/*
 * output = MD5( input buffer )
 */
_CC_API_PUBLIC(void) _cc_md5(const byte_t *input, uint32_t length, tchar_t *output) {
    _cc_md5_t c;
    byte_t md[_CC_MD5_DIGEST_LENGTH_];

    _cc_md5_init(&c);
    _cc_md5_update(&c, input, length);
    _cc_md5_final(&c, &(md[0]));

    _cc_bytes2hex(md, _CC_MD5_DIGEST_LENGTH_, output, _CC_MD5_DIGEST_LENGTH_ * 2);
}
