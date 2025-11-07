#include <libcc/alloc.h>
#include <libcc/string.h>
#include <libcc/crypto/hmac.h>

#define MAX_BLOCKLEN 128

#ifdef _CC_USE_OPENSSL_
#include "openssl/openssl_hmac.c"
#else
#define MAX_HASHLEN 64

typedef struct _cc_hmac {
    _cc_hash_t hash;
    byte_t *key;
    size_t key_length;
    size_t block_length;
}_cc_hmac_t;

_CC_API_PRIVATE(void) _hmac_init_block(_cc_hmac_t *ctx, byte_t *block, byte_t cp) {
    int i;
    for (i = 0; i < ctx->key_length; i++) {
        block[i] = *(ctx->key + i) ^ cp;
    }
    for (i = ctx->key_length; i < ctx->block_length; i++) {
        block[i] = cp;
    }
}

_CC_API_PRIVATE(void) __hmac_update(_cc_hash_t *ctx, const byte_t *input, size_t length) {
    _cc_hmac_t *hmac_ctx = (_cc_hmac_t *)ctx->handle;
    hmac_ctx->hash.update(&hmac_ctx->hash, input, length);
}

_CC_API_PRIVATE(void) __hmac_final(_cc_hash_t *ctx, byte_t *digest, int32_t *digest_length) {
    byte_t block[MAX_BLOCKLEN];
    _cc_hmac_t *hmac_ctx = (_cc_hmac_t *)ctx->handle;

    hmac_ctx->hash.final(&hmac_ctx->hash, digest, digest_length);
    hmac_ctx->hash.init(&hmac_ctx->hash);

    _hmac_init_block(hmac_ctx, block, 0x5C);

    hmac_ctx->hash.update(&hmac_ctx->hash, block, hmac_ctx->block_length);
    hmac_ctx->hash.update(&hmac_ctx->hash, digest, *digest_length);
    hmac_ctx->hash.final(&hmac_ctx->hash, digest, digest_length);
}

_CC_API_PRIVATE(void) __free_hmac(_cc_hash_t *ctx) {
    if (ctx) {
        _cc_hmac_t *hmac_ctx = (_cc_hmac_t *)ctx->handle;
        hmac_ctx->hash.free(&hmac_ctx->hash);
        if (hmac_ctx->key) {
            _cc_free(hmac_ctx->key);
        }
        _cc_free(hmac_ctx);
    }
}

_CC_API_PUBLIC(void) _cc_hmac_init(_cc_hash_t *ctx, byte_t method, const byte_t *key, size_t key_length) {
    _cc_hmac_t *hmac_ctx = (_cc_hmac_t *)_cc_malloc(sizeof(_cc_hmac_t));
    byte_t digest[MAX_BLOCKLEN];
    byte_t block[MAX_BLOCKLEN];
    int32_t digest_length = MAX_BLOCKLEN;
    
    ctx->handle = (uintptr_t)hmac_ctx;

    switch (method) {
        case _CC_SHA1_:
            hmac_ctx->block_length = 64;
            _cc_sha1_init(&hmac_ctx->hash);
            break;
        case _CC_SHA256_:
            hmac_ctx->block_length = 64;
            _cc_sha256_init(&hmac_ctx->hash);
            break;
        case _CC_SHA224_:
            hmac_ctx->block_length = 64;
            _cc_sha224_init(&hmac_ctx->hash);
            break;
        case _CC_SHA384_:
            hmac_ctx->block_length = 128;
            _cc_sha384_init(&hmac_ctx->hash);
            break;
        case _CC_SHA512_:
            hmac_ctx->block_length = 128;
            _cc_sha512_init(&hmac_ctx->hash);
            break;
        case _CC_MD5_:
            hmac_ctx->block_length = 64;
            _cc_md5_hash_init(&hmac_ctx->hash);
            break;
    }
    
    ctx->method = method;

    if (key && key_length) {
        if (key_length > hmac_ctx->block_length) {
            hmac_ctx->hash.init(&hmac_ctx->hash);
            hmac_ctx->hash.update(&hmac_ctx->hash, key, key_length);
            hmac_ctx->hash.final(&hmac_ctx->hash, digest, &digest_length);
            key_length = digest_length;
            key = digest;
        }
        hmac_ctx->key = (byte_t*)_cc_malloc(key_length);
        memcpy(hmac_ctx->key, key, key_length);
        hmac_ctx->key_length = key_length;
    }

    _hmac_init_block(hmac_ctx, block, 0x36);

    hmac_ctx->hash.init(&hmac_ctx->hash);
    hmac_ctx->hash.update(&hmac_ctx->hash, block, hmac_ctx->block_length);

    ctx->update = __hmac_update;
    ctx->final = __hmac_final;
    ctx->free = __free_hmac;
}

#endif /* _CC_USE_OPENSSL_ */
/*
 *
 */
_CC_API_PUBLIC(int) _cc_hmac(byte_t type, const byte_t *input, size_t length, const byte_t *key, size_t key_length, tchar_t *output) {
    byte_t digest[MAX_BLOCKLEN];
    _cc_hash_t hmac;
    int32_t digest_length = MAX_BLOCKLEN;

    _cc_hmac_init(&hmac, type, key, key_length);
    hmac.update(&hmac, input, length);
    hmac.final(&hmac, digest, &digest_length);
    hmac.free(&hmac);

    return (int)_cc_bytes2hex(digest, digest_length, output, digest_length * 2);
}