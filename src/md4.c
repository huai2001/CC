/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#include <libcc/md4.h>
#include <libcc/string.h>

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
 * MD4 context setup
 */
_CC_API_PUBLIC(void) _cc_md4_init(_cc_md4_t *ctx) {
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
}

#ifndef _CC_MD4_PROCESS_ALT
_CC_API_PUBLIC(void) _cc_md4_process(_cc_md4_t *ctx, const byte_t data[64]) {
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

    A = ctx->state[0];
    B = ctx->state[1];
    C = ctx->state[2];
    D = ctx->state[3];

#define F(x, y, z) ((x & y) | ((~x) & z))
#define P(a, b, c, d, x, s)                                                                                            \
    {                                                                                                                  \
        a += F(b, c, d) + x;                                                                                           \
        a = S(a, s);                                                                                                   \
    }

    P(A, B, C, D, X[0], 3);
    P(D, A, B, C, X[1], 7);
    P(C, D, A, B, X[2], 11);
    P(B, C, D, A, X[3], 19);
    P(A, B, C, D, X[4], 3);
    P(D, A, B, C, X[5], 7);
    P(C, D, A, B, X[6], 11);
    P(B, C, D, A, X[7], 19);
    P(A, B, C, D, X[8], 3);
    P(D, A, B, C, X[9], 7);
    P(C, D, A, B, X[10], 11);
    P(B, C, D, A, X[11], 19);
    P(A, B, C, D, X[12], 3);
    P(D, A, B, C, X[13], 7);
    P(C, D, A, B, X[14], 11);
    P(B, C, D, A, X[15], 19);

#undef P
#undef F

#define F(x, y, z) ((x & y) | (x & z) | (y & z))
#define P(a, b, c, d, x, s)                                                                                            \
    {                                                                                                                  \
        a += F(b, c, d) + x + 0x5A827999;                                                                              \
        a = S(a, s);                                                                                                   \
    }

    P(A, B, C, D, X[0], 3);
    P(D, A, B, C, X[4], 5);
    P(C, D, A, B, X[8], 9);
    P(B, C, D, A, X[12], 13);
    P(A, B, C, D, X[1], 3);
    P(D, A, B, C, X[5], 5);
    P(C, D, A, B, X[9], 9);
    P(B, C, D, A, X[13], 13);
    P(A, B, C, D, X[2], 3);
    P(D, A, B, C, X[6], 5);
    P(C, D, A, B, X[10], 9);
    P(B, C, D, A, X[14], 13);
    P(A, B, C, D, X[3], 3);
    P(D, A, B, C, X[7], 5);
    P(C, D, A, B, X[11], 9);
    P(B, C, D, A, X[15], 13);

#undef P
#undef F

#define F(x, y, z) (x ^ y ^ z)
#define P(a, b, c, d, x, s)                                                                                            \
    {                                                                                                                  \
        a += F(b, c, d) + x + 0x6ED9EBA1;                                                                              \
        a = S(a, s);                                                                                                   \
    }

    P(A, B, C, D, X[0], 3);
    P(D, A, B, C, X[8], 9);
    P(C, D, A, B, X[4], 11);
    P(B, C, D, A, X[12], 15);
    P(A, B, C, D, X[2], 3);
    P(D, A, B, C, X[10], 9);
    P(C, D, A, B, X[6], 11);
    P(B, C, D, A, X[14], 15);
    P(A, B, C, D, X[1], 3);
    P(D, A, B, C, X[9], 9);
    P(C, D, A, B, X[5], 11);
    P(B, C, D, A, X[13], 15);
    P(A, B, C, D, X[3], 3);
    P(D, A, B, C, X[11], 9);
    P(C, D, A, B, X[7], 11);
    P(B, C, D, A, X[15], 15);

#undef F
#undef P

    ctx->state[0] += A;
    ctx->state[1] += B;
    ctx->state[2] += C;
    ctx->state[3] += D;
}
#endif /* !CC_MD4_PROCESS_ALT */

/*
 * MD4 process buffer
 */
_CC_API_PUBLIC(void) _cc_md4_update(_cc_md4_t *ctx, const byte_t *input, size_t ilen) {
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
        memcpy((void *)(ctx->buffer + left), (void *)input, fill);
        _cc_md4_process(ctx, ctx->buffer);
        input += fill;
        ilen -= fill;
        left = 0;
    }

    while (ilen >= 64) {
        _cc_md4_process(ctx, input);
        input += 64;
        ilen -= 64;
    }

    if (ilen > 0) {
        memcpy((void *)(ctx->buffer + left), (void *)input, ilen);
    }
}

static const byte_t md4_padding[64] = {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/*
 * MD4 final digest
 */
_CC_API_PUBLIC(void) _cc_md4_final(_cc_md4_t *ctx, byte_t *output) {
    uint32_t last, padn;
    uint32_t high, low;
    byte_t msglen[8];

    high = (ctx->total[0] >> 29) | (ctx->total[1] << 3);
    low = (ctx->total[0] << 3);

    PUT_UINT32_LE(low, msglen, 0);
    PUT_UINT32_LE(high, msglen, 4);

    last = ctx->total[0] & 0x3F;
    padn = (last < 56) ? (56 - last) : (120 - last);

    _cc_md4_update(ctx, (byte_t *)md4_padding, padn);
    _cc_md4_update(ctx, msglen, 8);

    PUT_UINT32_LE(ctx->state[0], output, 0);
    PUT_UINT32_LE(ctx->state[1], output, 4);
    PUT_UINT32_LE(ctx->state[2], output, 8);
    PUT_UINT32_LE(ctx->state[3], output, 12);
}

/*
    Digests a file.
 */
_CC_API_PUBLIC(bool_t) _cc_md4_fp(FILE *fp, tchar_t *output) {
    byte_t md[_CC_MD4_DIGEST_LENGTH_];
    byte_t buf[1024 * 16];
    size_t i;
    long seek_cur = 0;
    _cc_md4_t c;

    if (fp == nullptr) {
        return false;
    }

    _cc_md4_init(&c);

    seek_cur = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    while ((i = fread(buf, sizeof(byte_t), _cc_countof(buf), fp))) {
        _cc_md4_update(&c, buf, i);
    }

    _cc_md4_final(&c, &(md[0]));

    fseek(fp, seek_cur, SEEK_CUR);

    _cc_bytes2hex(md, _CC_MD4_DIGEST_LENGTH_, output, _CC_MD4_DIGEST_LENGTH_ * 2);

    return true;
}

/*
    Digests a file.
 */
_CC_API_PUBLIC(bool_t) _cc_md4file(const tchar_t *filename, tchar_t *output) {
    FILE *fp = _tfopen(filename, _T("rb"));

    if (fp) {
        _cc_md4_fp(fp, output);
        fclose(fp);
        return true;
    }
    return false;
}

/*
 * output = MD4( input buffer )
 */
_CC_API_PUBLIC(void) _cc_md4(const byte_t *input, size_t length, tchar_t *output) {
    _cc_md4_t c;
    byte_t md[_CC_MD4_DIGEST_LENGTH_];

    _cc_md4_init(&c);
    _cc_md4_update(&c, input, length);
    _cc_md4_final(&c, &(md[0]));

    _cc_bytes2hex(md, _CC_MD4_DIGEST_LENGTH_, output, _CC_MD4_DIGEST_LENGTH_ * 2);
}
