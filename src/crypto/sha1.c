#include <libcc/crypto/sha.h>
#include <libcc/string.h>
#include <libcc/alloc.h>

#ifdef _CC_USE_OPENSS_
#include "openssl/openssl_sha.c"
#else

struct _cc_sha1 {
    uint32_t total[2]; /*!< number of bytes processed  */
    uint32_t state[5]; /*!< intermediate digest state  */
    byte_t buffer[64]; /*!< data block being processed */
};

#if !defined(_CC_SHA1_ALT_)
/*
 * 32-bit integer manipulation macros (big endian)
 */
#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n, b, i)                                                                                         \
    do {                                                                                                               \
        (n) = ((uint32_t)(b)[(i)] << 24) | ((uint32_t)(b)[(i) + 1] << 16) | ((uint32_t)(b)[(i) + 2] << 8) |            \
              ((uint32_t)(b)[(i) + 3]);                                                                                \
    } while(0)
#endif

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n, b, i)                                                                                         \
    do {                                                                                                               \
        (b)[(i)] = (byte_t)((n) >> 24);                                                                                \
        (b)[(i) + 1] = (byte_t)((n) >> 16);                                                                            \
        (b)[(i) + 2] = (byte_t)((n) >> 8);                                                                             \
        (b)[(i) + 3] = (byte_t)((n));                                                                                  \
    } while(0)
#endif

/*
 * SHA-1 context setup
 */
_CC_API_PRIVATE(void) __sha1_init(_cc_hash_t *sha) {
   struct _cc_sha1 *ctx = (struct _cc_sha1 *)sha->handle;
    sha->method = _CC_SHA1_;
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
}

/*
 * SHA-1 context free
 */
_CC_API_PRIVATE(void) __free_sha1(_cc_hash_t *ctx) {
    _cc_free((struct _cc_sha1 *)ctx->handle);
}

#if !defined(_CC_SHA1_PROCESS_ALT)
_CC_API_PRIVATE(void) __sha1_process(struct _cc_sha1 *ctx, const byte_t data[64]) {
    uint32_t temp, W[16], A, B, C, D, E;

    GET_UINT32_BE(W[0], data, 0);
    GET_UINT32_BE(W[1], data, 4);
    GET_UINT32_BE(W[2], data, 8);
    GET_UINT32_BE(W[3], data, 12);
    GET_UINT32_BE(W[4], data, 16);
    GET_UINT32_BE(W[5], data, 20);
    GET_UINT32_BE(W[6], data, 24);
    GET_UINT32_BE(W[7], data, 28);
    GET_UINT32_BE(W[8], data, 32);
    GET_UINT32_BE(W[9], data, 36);
    GET_UINT32_BE(W[10], data, 40);
    GET_UINT32_BE(W[11], data, 44);
    GET_UINT32_BE(W[12], data, 48);
    GET_UINT32_BE(W[13], data, 52);
    GET_UINT32_BE(W[14], data, 56);
    GET_UINT32_BE(W[15], data, 60);

#define S(x, n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define R(t)                                                                                                           \
    (temp = W[(t - 3) & 0x0F] ^ W[(t - 8) & 0x0F] ^ W[(t - 14) & 0x0F] ^ W[t & 0x0F], (W[t & 0x0F] = S(temp, 1)))

#define P(a, b, c, d, e, x)                                                                                            \
    do {                                                                                                               \
        e += S(a, 5) + F(b, c, d) + K + x;                                                                             \
        b = S(b, 30);                                                                                                  \
    } while(0)

    A = ctx->state[0];
    B = ctx->state[1];
    C = ctx->state[2];
    D = ctx->state[3];
    E = ctx->state[4];

#define F(x, y, z) (z ^ (x & (y ^ z)))
#define K 0x5A827999

    P(A, B, C, D, E, W[0]);
    P(E, A, B, C, D, W[1]);
    P(D, E, A, B, C, W[2]);
    P(C, D, E, A, B, W[3]);
    P(B, C, D, E, A, W[4]);
    P(A, B, C, D, E, W[5]);
    P(E, A, B, C, D, W[6]);
    P(D, E, A, B, C, W[7]);
    P(C, D, E, A, B, W[8]);
    P(B, C, D, E, A, W[9]);
    P(A, B, C, D, E, W[10]);
    P(E, A, B, C, D, W[11]);
    P(D, E, A, B, C, W[12]);
    P(C, D, E, A, B, W[13]);
    P(B, C, D, E, A, W[14]);
    P(A, B, C, D, E, W[15]);
    P(E, A, B, C, D, R(16));
    P(D, E, A, B, C, R(17));
    P(C, D, E, A, B, R(18));
    P(B, C, D, E, A, R(19));

#undef K
#undef F

#define F(x, y, z) (x ^ y ^ z)
#define K 0x6ED9EBA1

    P(A, B, C, D, E, R(20));
    P(E, A, B, C, D, R(21));
    P(D, E, A, B, C, R(22));
    P(C, D, E, A, B, R(23));
    P(B, C, D, E, A, R(24));
    P(A, B, C, D, E, R(25));
    P(E, A, B, C, D, R(26));
    P(D, E, A, B, C, R(27));
    P(C, D, E, A, B, R(28));
    P(B, C, D, E, A, R(29));
    P(A, B, C, D, E, R(30));
    P(E, A, B, C, D, R(31));
    P(D, E, A, B, C, R(32));
    P(C, D, E, A, B, R(33));
    P(B, C, D, E, A, R(34));
    P(A, B, C, D, E, R(35));
    P(E, A, B, C, D, R(36));
    P(D, E, A, B, C, R(37));
    P(C, D, E, A, B, R(38));
    P(B, C, D, E, A, R(39));

#undef K
#undef F

#define F(x, y, z) ((x & y) | (z & (x | y)))
#define K 0x8F1BBCDC

    P(A, B, C, D, E, R(40));
    P(E, A, B, C, D, R(41));
    P(D, E, A, B, C, R(42));
    P(C, D, E, A, B, R(43));
    P(B, C, D, E, A, R(44));
    P(A, B, C, D, E, R(45));
    P(E, A, B, C, D, R(46));
    P(D, E, A, B, C, R(47));
    P(C, D, E, A, B, R(48));
    P(B, C, D, E, A, R(49));
    P(A, B, C, D, E, R(50));
    P(E, A, B, C, D, R(51));
    P(D, E, A, B, C, R(52));
    P(C, D, E, A, B, R(53));
    P(B, C, D, E, A, R(54));
    P(A, B, C, D, E, R(55));
    P(E, A, B, C, D, R(56));
    P(D, E, A, B, C, R(57));
    P(C, D, E, A, B, R(58));
    P(B, C, D, E, A, R(59));

#undef K
#undef F

#define F(x, y, z) (x ^ y ^ z)
#define K 0xCA62C1D6

    P(A, B, C, D, E, R(60));
    P(E, A, B, C, D, R(61));
    P(D, E, A, B, C, R(62));
    P(C, D, E, A, B, R(63));
    P(B, C, D, E, A, R(64));
    P(A, B, C, D, E, R(65));
    P(E, A, B, C, D, R(66));
    P(D, E, A, B, C, R(67));
    P(C, D, E, A, B, R(68));
    P(B, C, D, E, A, R(69));
    P(A, B, C, D, E, R(70));
    P(E, A, B, C, D, R(71));
    P(D, E, A, B, C, R(72));
    P(C, D, E, A, B, R(73));
    P(B, C, D, E, A, R(74));
    P(A, B, C, D, E, R(75));
    P(E, A, B, C, D, R(76));
    P(D, E, A, B, C, R(77));
    P(C, D, E, A, B, R(78));
    P(B, C, D, E, A, R(79));

#undef K
#undef F

    ctx->state[0] += A;
    ctx->state[1] += B;
    ctx->state[2] += C;
    ctx->state[3] += D;
    ctx->state[4] += E;
}
#endif /* !CC_SHA1_PROCESS_ALT */

/*
 * SHA-1 process buffer
 */
_CC_API_PRIVATE(void) __sha1_update(_cc_hash_t *sha, const byte_t *input, size_t length) {
    struct _cc_sha1 *ctx = (struct _cc_sha1 *)sha->handle;
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
        __sha1_process(ctx, ctx->buffer);
        input += fill;
        length -= fill;
        left = 0;
    }

    while (length >= 64) {
        __sha1_process(ctx, input);
        input += 64;
        length -= 64;
    }

    if (length > 0) {
        memcpy((void *)(ctx->buffer + left), input, length);
    }
}

static const byte_t sha1_padding[64] = {0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/*
 * SHA-1 final digest
 */
_CC_API_PRIVATE(void) __sha1_final(_cc_hash_t *sha, byte_t *digest, int32_t *digest_length) {
    struct _cc_sha1 *ctx = (struct _cc_sha1 *)sha->handle;
    uint32_t last, padn;
    uint32_t high, low;
    byte_t msglen[8];

    high = (ctx->total[0] >> 29) | (ctx->total[1] << 3);
    low = (ctx->total[0] << 3);

    PUT_UINT32_BE(high, msglen, 0);
    PUT_UINT32_BE(low, msglen, 4);

    last = ctx->total[0] & 0x3F;
    padn = (last < 56) ? (56 - last) : (120 - last);

    __sha1_update(sha, sha1_padding, padn);
    __sha1_update(sha, msglen, 8);

    PUT_UINT32_BE(ctx->state[0], digest, 0);
    PUT_UINT32_BE(ctx->state[1], digest, 4);
    PUT_UINT32_BE(ctx->state[2], digest, 8);
    PUT_UINT32_BE(ctx->state[3], digest, 12);
    PUT_UINT32_BE(ctx->state[4], digest, 16);

    if (digest_length) {
        *digest_length = _CC_SHA1_DIGEST_LENGTH_;
    }
}
#endif /* !CC_SHA1_ALT */

_CC_API_PUBLIC(void) _cc_sha1_init(_cc_hash_t *sha) {
    sha->handle = (uintptr_t)_cc_malloc(sizeof(struct _cc_sha1));
    sha->init = __sha1_init;
    sha->update = __sha1_update;
    sha->final = __sha1_final;
    sha->free = __free_sha1;

    __sha1_init(sha);
}

#endif /* _CC_USE_OPENSS_ */

/*
    Digests a file.
 */
_CC_API_PUBLIC(bool_t) _cc_sha1_fp(FILE *fp, tchar_t *output) {
    byte_t results[_CC_SHA1_DIGEST_LENGTH_];
    byte_t buf[1024 * 16];
    size_t i;
    long seek_cur = 0;
    _cc_hash_t c;

    if (fp == NULL) {
        return false;
    }

    _cc_sha1_init(&c);

    seek_cur = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    while ((i = fread(buf, sizeof(byte_t), _cc_countof(buf), fp))) {
        c.update(&c, buf, i);
    }

    c.final(&c, results, NULL);
    c.free(&c);

    fseek(fp, seek_cur, SEEK_SET);

    _cc_bytes2hex(results, _CC_SHA1_DIGEST_LENGTH_, output, _CC_SHA1_DIGEST_LENGTH_ * 2);

    return true;
}
/*
    Digests a file.
 */
_CC_API_PUBLIC(bool_t) _cc_sha1_from_file(const tchar_t *file, tchar_t *output) {
    FILE *fp = _tfopen(file, _T("rb"));

    if (fp) {
        _cc_sha1_fp(fp, output);
        fclose(fp);
        return true;
    }
    return false;
}

/*
 * output = SHA-1( input buffer )
 */
_CC_API_PUBLIC(void) _cc_sha1(const byte_t *input, size_t length, tchar_t *output) {
    _cc_hash_t c;
    byte_t results[_CC_SHA1_DIGEST_LENGTH_];

    _cc_sha1_init(&c);

    c.update(&c, input, length);
    c.final(&c, results, NULL);
    c.free(&c);

    _cc_bytes2hex(results, _CC_SHA1_DIGEST_LENGTH_, output, _CC_SHA1_DIGEST_LENGTH_ * 2);
}
