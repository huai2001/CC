#include <openssl/evp.h>
#include <openssl/err.h>

/*
 */
_CC_API_PRIVATE(void) __sha1_init(_cc_hash_t *ctx) {
    ctx->method = _CC_SHA1_;
    EVP_DigestInit_ex((EVP_MD_CTX *)ctx->handle, EVP_sha1(), NULL);
}
/*
 */
_CC_API_PRIVATE(void) __update(_cc_hash_t *ctx, const byte_t *input, size_t length) {
    EVP_DigestUpdate((EVP_MD_CTX *)ctx->handle, input, length);
}
/*
 */
_CC_API_PRIVATE(void) __final(_cc_hash_t *ctx, byte_t *digest, int32_t *digest_length) {
    EVP_DigestFinal_ex((EVP_MD_CTX *)ctx->handle, digest, (unsigned int *)digest_length);
}

_CC_API_PRIVATE(void) __free_sha(_cc_hash_t *ctx) {
    if (ctx) {
        EVP_MD_CTX_free((EVP_MD_CTX *)ctx->handle);
    }
}

_CC_API_PUBLIC(void) _cc_sha1_init(_cc_hash_t *sha) {
    sha->handle = (uintptr_t)EVP_MD_CTX_new();
    sha->init = __sha1_init;
    sha->update = __update;
    sha->final = __final;
    sha->free = __free_sha;

    __sha1_init(sha);
}