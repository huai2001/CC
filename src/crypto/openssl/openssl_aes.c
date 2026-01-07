#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string.h>

void _cc_aes_init(_cc_aes_t *ctx) {
    bzero(ctx, sizeof(_cc_aes_t));
}

_CC_API_PUBLIC(int) _cc_aes_setkey_enc(_cc_aes_t *ctx, const byte_t *key, uint32_t keybits) {
    AES_KEY *aes_key = (AES_KEY *)ctx->buf;
    // OpenSSL rounds: 10 for 128, 12 for 192, 14 for 256
    ctx->nr = keybits / 32 + 6;
    return AES_set_encrypt_key(key, 256, &aes_key);
}

_CC_API_PUBLIC(int) _cc_aes_setkey_dec(_cc_aes_t *ctx, const byte_t *key, uint32_t keybits) {
    AES_KEY *aes_key = (AES_KEY *)ctx->buf;
    // OpenSSL rounds: 10 for 128, 12 for 192, 14 for 256
    ctx->nr = keybits / 32 + 6;
    return AES_set_decrypt_key(key, 256, &aes_key);
}

_CC_API_PUBLIC(int) _cc_aes_setkey(_cc_aes_t *ctx, int mode, const byte_t *key, uint32_t keybits) {
    if (keybits != 128 && keybits != 192 && keybits != 256) {
        return _CC_ERR_AES_INVALID_KEY_LENGTH_;
    }
    if (mode == _CC_AES_ENCRYPT_) {
        return _cc_aes_setkey_enc(ctx, key, keybits);
    } else {
        return _cc_aes_setkey_dec(ctx, key, keybits);
    }
    return 0;
}

/*
 * AES-ECB block encryption
 */
_CC_API_PUBLIC(void) _cc_aes_encrypt(_cc_aes_t *ctx, const byte_t input[16], byte_t output[16]) {
    AES_KEY *aes_key = (AES_KEY *)ctx->buf;
    AES_encrypt(input, output, aes_key);
}
/*
 * AES-ECB block decryption
 */
_CC_API_PUBLIC(void) _cc_aes_decrypt(_cc_aes_t *ctx, const byte_t input[16], byte_t output[16]) {
    AES_KEY *aes_key = (AES_KEY *)ctx->buf;
    AES_decrypt(input, output, aes_key);
}

_CC_API_PUBLIC(int) _cc_aes_crypt_cbc(_cc_aes_t *ctx, int mode, const byte_t *input, size_t length, byte_t iv[16], byte_t *output) {
    AES_KEY *aes_key = (AES_KEY *)ctx->buf;
    if (mode == _CC_AES_ENCRYPT_) {
        // TODO: Implement AES-CBC encryption
        return AES_cbc_encrypt(input, output, length, aes_key, iv, AES_ENCRYPT);
    }
    // TODO: Implement AES-CBC decryption
    return AES_cbc_encrypt(input, output, length, aes_key, iv, AES_DECRYPT);
}

_CC_API_PUBLIC(int) _cc_aes_crypt_cfb128(_cc_aes_t *ctx, int mode, const byte_t *input, size_t length, size_t *iv_off, byte_t iv[16], byte_t *output) {
    AES_KEY *aes_key = (AES_KEY *)ctx->buf;
    if (mode == _CC_AES_ENCRYPT_) {
        // TODO: Implement AES-CFB128 encryption
        return AES_cfb128_encrypt(input, output, length, aes_key, iv, iv_off, AES_ENCRYPT);
    }
    
    // TODO: Implement AES-CFB128 decryption
    return AES_cfb128_encrypt(input, output, length, aes_key, iv, iv_off, AES_DECRYPT);
}

_CC_API_PUBLIC(int) _cc_aes_crypt_cfb8(_cc_aes_t *ctx, int mode, const byte_t *input, size_t length, byte_t iv[16], byte_t *output) {
    AES_KEY *aes_key = (AES_KEY *)ctx->buf;
    if (mode == _CC_AES_ENCRYPT_) {
        // TODO: Implement AES-CFB8 encryption
        return AES_cfb8_encrypt(input, output, length, aes_key, iv, AES_ENCRYPT);
    }

    // TODO: Implement AES-CFB8 decryption
    return AES_cfb8_encrypt(input, output, length, aes_key, iv, AES_DECRYPT);
}
_CC_API_PUBLIC(int) _cc_aes_crypt_ctr(_cc_aes_t *ctx, const byte_t *input, size_t length, size_t *nc_off, byte_t nonce_counter[16], byte_t stream_block[16], byte_t *output) {
    AES_KEY *aes_key = (AES_KEY *)ctx->buf;
    // TODO: Implement AES-CTR encryption/decryption
    return AES_ctr_encrypt(input, output, length, aes_key, nonce_counter, nc_off);
}