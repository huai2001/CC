#include <openssl/hmac.h>
#include <openssl/evp.h>

struct _hmac {
    EVP_MAC *mac;
    EVP_MAC_CTX *ctx;
    byte_t *key;
    size_t key_length;
};

/*
 * HMAC context setup
 */
_CC_API_PRIVATE(void) __hmac_md5_init(_cc_hash_t *ctx) {
    struct _hmac *hmac = (struct _hmac *)ctx->handle;
    OSSL_PARAM params[] = {{"digest", OSSL_PARAM_UTF8_STRING, "MD5", 0}, {NULL, 0, NULL, 0}};
    EVP_MAC_init(hmac->ctx, hmac->key, hmac->key_length, params);
}
_CC_API_PRIVATE(void) __hmac_sha1_init(_cc_hash_t *ctx) {
    struct _hmac *hmac = (struct _hmac *)ctx->handle;
    OSSL_PARAM params[] = {{"digest", OSSL_PARAM_UTF8_STRING, "SHA1", 0}, {NULL, 0, NULL, 0}};
    EVP_MAC_init(hmac->ctx, hmac->key, hmac->key_length, params);
}

_CC_API_PRIVATE(void) __hmac_sha256_init(_cc_hash_t *ctx) {
    struct _hmac *hmac = (struct _hmac *)ctx->handle;
    OSSL_PARAM params[] = {{"digest", OSSL_PARAM_UTF8_STRING, "SHA256", 0}, {NULL, 0, NULL, 0}};
    EVP_MAC_init(hmac->ctx, hmac->key, hmac->key_length, params);
}

_CC_API_PRIVATE(void) __hmac_sha224_init(_cc_hash_t *ctx) {
    struct _hmac *hmac = (struct _hmac *)ctx->handle;
    OSSL_PARAM params[] = {{"digest", OSSL_PARAM_UTF8_STRING, "SHA224", 0}, {NULL, 0, NULL, 0}};
    EVP_MAC_init(hmac->ctx, hmac->key, hmac->key_length, params);
}

_CC_API_PRIVATE(void) __hmac_sha384_init(_cc_hash_t *ctx) {
    struct _hmac *hmac = (struct _hmac *)ctx->handle;
    OSSL_PARAM params[] = {{"digest", OSSL_PARAM_UTF8_STRING, "SHA384", 0}, {NULL, 0, NULL, 0}}; 
    EVP_MAC_init(hmac->ctx, hmac->key, hmac->key_length, params);
}

_CC_API_PRIVATE(void) __hmac_sha512_init(_cc_hash_t *ctx) {
    struct _hmac *hmac = (struct _hmac *)ctx->handle;
    OSSL_PARAM params[] = {{"digest", OSSL_PARAM_UTF8_STRING, "SHA512", 0}, {NULL, 0, NULL, 0}}; 
    EVP_MAC_init(hmac->ctx, hmac->key, hmac->key_length, params);
}

_CC_API_PRIVATE(void) __hmac_update(_cc_hash_t *ctx, const byte_t *input, size_t length) {
    struct _hmac *hmac = (struct _hmac *)ctx->handle;
    EVP_MAC_update(hmac->ctx, input, length);
}

_CC_API_PRIVATE(void) __hmac_final(_cc_hash_t *ctx, byte_t *digest, int32_t *digest_length) {
    size_t length = (size_t)*digest_length;
    struct _hmac *hmac = (struct _hmac *)ctx->handle;
     EVP_MAC_final(hmac->ctx, digest, (size_t*)&length, (size_t)length);
     if (*digest_length) {
         *digest_length = (int32_t)length;
     }
}

_CC_API_PRIVATE(void) __free_hmac(_cc_hash_t *ctx) {
    if (ctx) {
        struct _hmac *hmac = (struct _hmac *)ctx->handle;
        EVP_MAC_CTX_free(hmac->ctx);
        EVP_MAC_free(hmac->mac);
        _cc_free(hmac->key);
        _cc_free(hmac);
    }
}

_CC_API_PUBLIC(void) _cc_hmac_init(_cc_hash_t *ctx, byte_t method, const byte_t *key, size_t key_length) {
    struct _hmac *hmac = (struct _hmac *)_cc_malloc(sizeof(struct _hmac));
    hmac->mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
    hmac->ctx = EVP_MAC_CTX_new(hmac->mac);
    ctx->handle = (uintptr_t)hmac;

    ctx->update = __hmac_update;
    ctx->final = __hmac_final;
    ctx->free = __free_hmac;
    switch (method) {
        case _CC_SHA1_:
            ctx->init = __hmac_sha1_init;
            break;
        case _CC_SHA256_:
            ctx->init = __hmac_sha256_init;
            break;
        case _CC_SHA224_:
            ctx->init = __hmac_sha224_init;
            break;
        case _CC_SHA384_:
            ctx->init = __hmac_sha384_init;
            break;
        case _CC_SHA512_:
            ctx->init = __hmac_sha512_init;
            break;
        case _CC_MD5_:
            ctx->init = __hmac_md5_init;
            break;
    }

    if (key && key_length) {
        hmac->key = (byte_t*)_cc_malloc(key_length);
        memcpy(hmac->key, key, key_length);
        hmac->key_length = key_length;
    }

    ctx->init(ctx);
}