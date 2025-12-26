#include <libcc/crypto/sha.h>
#include <libcc/string.h>
#include <libcc/alloc.h>

#ifdef _CC_USE_OPENSSL_
#include "openssl/openssl_sha256.c"
#else
struct _cc_sha256 {
    uint32_t total[2]; /*!< number of bytes processed  */
    uint32_t state[8]; /*!< intermediate digest state  */
    byte_t buffer[64]; /*!< data block being processed */
};
#if !defined(_CC_SHA256_ALT_)
/*
 * 32-bit integer manipulation macros (big endian)
 */
#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n, b, i)                                                                                         \
    do {                                                                                                               \
        (n) = ((uint32_t)(b)[(i)] << 24) | ((uint32_t)(b)[(i) + 1] << 16) | ((uint32_t)(b)[(i) + 2] << 8) |            \
              ((uint32_t)(b)[(i) + 3]);                                                                                \
    } while (0)
#endif

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n, b, i)                                                                                         \
    do {                                                                                                               \
        (b)[(i)] = (byte_t)((n) >> 24);                                                                                \
        (b)[(i) + 1] = (byte_t)((n) >> 16);                                                                            \
        (b)[(i) + 2] = (byte_t)((n) >> 8);                                                                             \
        (b)[(i) + 3] = (byte_t)((n));                                                                                  \
    } while (0)
#endif

/*
 * SHA-256 context setup
 */
_CC_API_PRIVATE(void) __sha256_init(_cc_hash_t *sha) {
    struct _cc_sha256 *ctx = (struct _cc_sha256 *)sha->handle;
    sha->method = _CC_SHA256_;
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    /* SHA-256 */
    ctx->state[0] = 0x6A09E667;
    ctx->state[1] = 0xBB67AE85;
    ctx->state[2] = 0x3C6EF372;
    ctx->state[3] = 0xA54FF53A;
    ctx->state[4] = 0x510E527F;
    ctx->state[5] = 0x9B05688C;
    ctx->state[6] = 0x1F83D9AB;
    ctx->state[7] = 0x5BE0CD19;
}
/*
 * SHA-256 context setup
 */
_CC_API_PRIVATE(void) __sha224_init(_cc_hash_t *sha) {
    struct _cc_sha256 *ctx = (struct _cc_sha256 *)sha->handle;
    sha->method = _CC_SHA224_;
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    /* SHA-224 */
    ctx->state[0] = 0xC1059ED8;
    ctx->state[1] = 0x367CD507;
    ctx->state[2] = 0x3070DD17;
    ctx->state[3] = 0xF70E5939;
    ctx->state[4] = 0xFFC00B31;
    ctx->state[5] = 0x68581511;
    ctx->state[6] = 0x64F98FA7;
    ctx->state[7] = 0xBEFA4FA4;
}

_CC_API_PRIVATE(void) __free_sha256(_cc_hash_t *sha) {
    if (sha->handle) {
        _cc_free((struct _cc_sha256 *)sha->handle);
    }
}

#if !defined(_CC_SHA256_PROCESS_ALT)
static const uint32_t K[] = {
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
};

#define SHR(x, n) ((x & 0xFFFFFFFF) >> n)
#define ROTR(x, n) (SHR(x, n) | (x << (32 - n)))

#define S0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ SHR(x, 3))
#define S1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ SHR(x, 10))

#define S2(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define S3(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))

#define F0(x, y, z) ((x & y) | (z & (x | y)))
#define F1(x, y, z) (z ^ (x & (y ^ z)))

#define R(t) (W[t] = S1(W[t - 2]) + W[t - 7] + S0(W[t - 15]) + W[t - 16])

#define P(a, b, c, d, e, f, g, h, x, K)                                                                                \
    do {                                                                                                               \
        temp1 = h + S3(e) + F1(e, f, g) + K + x;                                                                       \
        temp2 = S2(a) + F0(a, b, c);                                                                                   \
        d += temp1;                                                                                                    \
        h = temp1 + temp2;                                                                                             \
    } while (0)

_CC_API_PRIVATE(void) __sha256_process(struct _cc_sha256 *ctx, const byte_t *data) {
    uint32_t temp1, temp2, W[64];
    uint32_t A[8];
    unsigned int i;

    for (i = 0; i < 8; i++) {
        A[i] = ctx->state[i];
    }

#if defined(_CC_SHA256_SMALLER)
    for (i = 0; i < 64; i++) {
        if (i < 16) {
            GET_UINT32_BE(W[i], data, 4 * i);
        } else {
            R(i);
        }

        P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[i], K[i]);

        temp1 = A[7];
        A[7] = A[6];
        A[6] = A[5];
        A[5] = A[4];
        A[4] = A[3];
        A[3] = A[2];
        A[2] = A[1];
        A[1] = A[0];
        A[0] = temp1;
    }
#else  /* _CC_SHA256_SMALLER */
    for (i = 0; i < 16; i++) {
        GET_UINT32_BE(W[i], data, 4 * i);
    }

    for (i = 0; i < 16; i += 8) {
        P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], W[i + 0], K[i + 0]);
        P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], W[i + 1], K[i + 1]);
        P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], W[i + 2], K[i + 2]);
        P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], W[i + 3], K[i + 3]);
        P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], W[i + 4], K[i + 4]);
        P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], W[i + 5], K[i + 5]);
        P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], W[i + 6], K[i + 6]);
        P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], W[i + 7], K[i + 7]);
    }

    for (i = 16; i < 64; i += 8) {
        P(A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], R(i + 0), K[i + 0]);
        P(A[7], A[0], A[1], A[2], A[3], A[4], A[5], A[6], R(i + 1), K[i + 1]);
        P(A[6], A[7], A[0], A[1], A[2], A[3], A[4], A[5], R(i + 2), K[i + 2]);
        P(A[5], A[6], A[7], A[0], A[1], A[2], A[3], A[4], R(i + 3), K[i + 3]);
        P(A[4], A[5], A[6], A[7], A[0], A[1], A[2], A[3], R(i + 4), K[i + 4]);
        P(A[3], A[4], A[5], A[6], A[7], A[0], A[1], A[2], R(i + 5), K[i + 5]);
        P(A[2], A[3], A[4], A[5], A[6], A[7], A[0], A[1], R(i + 6), K[i + 6]);
        P(A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[0], R(i + 7), K[i + 7]);
    }
#endif /* _CC_SHA256_SMALLER */

    for (i = 0; i < 8; i++) {
        ctx->state[i] += A[i];
    }
}
#endif /* !CC_SHA256_PROCESS_ALT */

/*
 * SHA-256 process buffer
 */
_CC_API_PRIVATE(void) __sha256_update(_cc_hash_t *sha, const byte_t *input, size_t length) {
    struct _cc_sha256 *ctx = (struct _cc_sha256 *)sha->handle;
    size_t fill;
    uint32_t left;

    if (length == 0) {
        return;
    }

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += (uint32_t)length;
    ctx->total[0] &= 0xFFFFFFFF;

    if (ctx->total[0] < (uint32_t)length) {
        ctx->total[1]++;
    }

    if (left && length >= fill) {
        memcpy((void *)(ctx->buffer + left), input, fill);
        __sha256_process(ctx, ctx->buffer);
        input += fill;
        length -= fill;
        left = 0;
    }

    while (length >= 64) {
        __sha256_process(ctx, input);
        input += 64;
        length -= 64;
    }

    if (length > 0) {
        memcpy((void *)(ctx->buffer + left), input, length);
    }
}

static const byte_t sha256_padding[64] = {0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                          0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                          0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/*
 * SHA-256 final digest
 */
_CC_API_PRIVATE(void) __sha256_final(_cc_hash_t *sha, byte_t *digest, int32_t *digest_length) {
    struct _cc_sha256 *ctx = (struct _cc_sha256 *)sha->handle;
    uint32_t last, padn;
    uint32_t high, low;
    byte_t msglen[8];

    high = (ctx->total[0] >> 29) | (ctx->total[1] << 3);
    low = (ctx->total[0] << 3);

    PUT_UINT32_BE(high, msglen, 0);
    PUT_UINT32_BE(low, msglen, 4);

    last = ctx->total[0] & 0x3F;
    padn = (last < 56) ? (56 - last) : (120 - last);

    __sha256_update(sha, sha256_padding, padn);
    __sha256_update(sha, msglen, 8);

    PUT_UINT32_BE(ctx->state[0], digest, 0);
    PUT_UINT32_BE(ctx->state[1], digest, 4);
    PUT_UINT32_BE(ctx->state[2], digest, 8);
    PUT_UINT32_BE(ctx->state[3], digest, 12);
    PUT_UINT32_BE(ctx->state[4], digest, 16);
    PUT_UINT32_BE(ctx->state[5], digest, 20);
    PUT_UINT32_BE(ctx->state[6], digest, 24);

    if (sha->method == _CC_SHA256_) {
        PUT_UINT32_BE(ctx->state[7], digest, 28);
    }
    
    if (digest_length) {
        *digest_length = sha->method == _CC_SHA256_ ? _CC_SHA256_DIGEST_LENGTH_ : _CC_SHA224_DIGEST_LENGTH_;
    }
}
#endif /* !_cc_SHA256_ALT */

_CC_API_PUBLIC(void) _cc_sha224_init(_cc_hash_t *sha) {
    sha->handle = (uintptr_t)_cc_malloc(sizeof(struct _cc_sha256));
    sha->init = __sha224_init;
    sha->update = __sha256_update;
    sha->final = __sha256_final;
    sha->free = __free_sha256;

    __sha224_init(sha);
}

_CC_API_PUBLIC(void) _cc_sha256_init(_cc_hash_t *sha) {
    sha->handle = (uintptr_t)_cc_malloc(sizeof(struct _cc_sha256));
    sha->init = __sha256_init;
    sha->update = __sha256_update;
    sha->final = __sha256_final;
    sha->free = __free_sha256;

    __sha256_init(sha);
}

#endif /* _CC_USE_OPENSSL_ */
/*
 * output = SHA-256( input buffer )
 */
_CC_API_PUBLIC(void) _cc_sha256(const byte_t *input, size_t length, tchar_t *output, bool_t is224) {
    _cc_hash_t c;
    byte_t results[_CC_SHA256_DIGEST_LENGTH_];
    int32_t digest_length = _CC_SHA256_DIGEST_LENGTH_;

    if (is224)  {
        _cc_sha224_init(&c);
    } else {
        _cc_sha256_init(&c);
    }

    c.update(&c, input, length);
    c.final(&c, results, &digest_length);
    c.free(&c);

    _cc_bytes2hex(results, digest_length, output, digest_length * 2);
}

/*
    Digests a file.
 */
_CC_API_PUBLIC(bool_t) _cc_sha256_fp(FILE *fp, tchar_t *output, bool_t is224) {
    byte_t results[_CC_SHA256_DIGEST_LENGTH_];
    byte_t buf[1024 * 16];
    size_t i;
    long seek_cur = 0;
    _cc_hash_t c;
    int32_t digest_length = _CC_SHA256_DIGEST_LENGTH_;

    if (fp == NULL) {
        return false;
    }

    if (is224)  {
        _cc_sha224_init(&c);
    } else {
        _cc_sha256_init(&c);
    }

    seek_cur = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    while ((i = fread(buf, sizeof(byte_t), _cc_countof(buf), fp))) {
        c.update(&c, buf, i);
    }

    c.final(&c, results, &digest_length);
    c.free(&c);

    fseek(fp, seek_cur, SEEK_SET);
    _cc_bytes2hex(results, digest_length, output, digest_length * 2);

    return true;
}
/*
    Digests a file.
 */
_CC_API_PUBLIC(bool_t) _cc_sha256_from_file(const tchar_t *file, tchar_t *output, bool_t is224) {
    FILE *fp = _tfopen(file, _T("rb"));

    if (fp) {
        _cc_sha256_fp(fp, output, is224);
        fclose(fp);
        return true;
    }
    return false;
}
