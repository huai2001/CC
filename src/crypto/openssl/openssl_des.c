#include <openssl/evp.h>
#include <openssl/err.h>
#include <libcc/des.h>

/**
 * @brief          Initialize DES context
 *
 * @param ctx      DES context to be initialized
 */
_CC_API_PUBLIC(void) _cc_des_init(_cc_des_t *ctx) {
    EVP_CIPHER_CTX *cipher = EVP_CIPHER_CTX_new();
    if (cipher == NULL) {
        ERR_raise(ERR_LIB_EVP, ERR_R_MALLOC_FAILURE);
        return;
    }
    ctx->sk = (uintptr_t)cipher;
    EVP_CIPHER_CTX_init(cipher);
}

/**
 * @brief          Initialize Triple-DES context
 *
 * @param ctx      DES3 context to be initialized
 */
_CC_API_PUBLIC(void) _cc_des3_init(_cc_des3_t *ctx) {
    EVP_CIPHER_CTX *cipher = EVP_CIPHER_CTX_new();
    if (cipher == NULL) {
        ERR_raise(ERR_LIB_EVP, ERR_R_MALLOC_FAILURE);
        return;
    }
    ctx->sk = (uintptr_t)cipher;
    EVP_CIPHER_CTX_init(cipher);
}

/**
 * @brief          Set key parity on the given key to odd.
 *
 *                 DES keys are 56 bits long, but each byte is padded with
 *                 a parity bit to allow verification.
 *
 * @param key      8-byte secret key
 */
_CC_API_PUBLIC(void) _cc_des_key_set_parity(byte_t key[_CC_DES_KEY_SIZE_]) {
    for (int i = 0; i < _CC_DES_KEY_SIZE_; i++) {
        int parity = 0;
        for (int j = 0; j < 7; j++) {
            parity ^= (key[i] >> j) & 1;
        }
        key[i] = (key[i] & 0xFE) | (parity & 1);
    }
}

/**
 * @brief          Check that key parity on the given key is odd.
 *
 *                 DES keys are 56 bits long, but each byte is padded with
 *                 a parity bit to allow verification.
 *
 * @param key      8-byte secret key
 *
 * @return         true is parity was ok, false if parity was not correct.
 */
_CC_API_PUBLIC(bool_t)
_cc_des_key_check_key_parity(const byte_t key[_CC_DES_KEY_SIZE_]) {
    for (int i = 0; i < _CC_DES_KEY_SIZE_; i++) {
        int parity = 0;
        for (int j = 0; j < 7; j++) {
            parity ^= (key[i] >> j) & 1;
        }
        if ((key[i] & 1) != (parity & 1)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief          Check that key is not a weak or semi-weak DES key
 *
 * @param key      8-byte secret key
 *
 * @return         false if no weak key was found, true if a weak key was
 * identified.
 */
_CC_API_PUBLIC(bool_t) _cc_des_key_check_weak(const byte_t key[_CC_DES_KEY_SIZE_]) {
    static const byte_t weak_keys[16][8] = {
        {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},
        {0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE},
        {0x1F, 0x1F, 0x1F, 0x1F, 0x0E, 0x0E, 0x0E, 0x0E},
        {0xE0, 0xE0, 0xE0, 0xE0, 0xF1, 0xF1, 0xF1, 0xF1},
        {0x01, 0x1F, 0x01, 0x1F, 0x01, 0x0E, 0x01, 0x0E},
        {0x1F, 0x01, 0x1F, 0x01, 0x0E, 0x01, 0x0E, 0x01},
        {0x01, 0xE0, 0x01, 0xE0, 0x01, 0xF1, 0x01, 0xF1},
        {0xE0, 0x01, 0xE0, 0x01, 0xF1, 0x01, 0xF1, 0x01},
        {0x01, 0xFE, 0x01, 0xFE, 0x01, 0xFE, 0x01, 0xFE},
        {0xFE, 0x01, 0xFE, 0x01, 0xFE, 0x01, 0xFE, 0x01},
        {0x1F, 0xE0, 0x1F, 0xE0, 0x0E, 0xF1, 0x0E, 0xF1},
        {0xE0, 0x1F, 0xE0, 0x1F, 0xF1, 0x0E, 0xF1, 0x0E},
        {0x1F, 0xFE, 0x1F, 0xFE, 0x0E, 0xFE, 0x0E, 0xFE},
        {0xFE, 0x1F, 0xFE, 0x1F, 0xFE, 0x0E, 0xFE, 0x0E},
        {0xE0, 0xFE, 0xE0, 0xFE, 0xF1, 0xFE, 0xF1, 0xFE},
        {0xFE, 0xE0, 0xFE, 0xE0, 0xFE, 0xF1, 0xFE, 0xF1}
    };

    for (int i = 0; i < 16; i++) {
        if (memcmp(key, weak_keys[i], _CC_DES_KEY_SIZE_) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * @brief          Internal function for key expansion.
 *                 (Only exposed to allow overriding it,
 *                 see _CC_DES_SETKEY_ALT)
 *
 * @param SK       Round keys
 * @param key      Base key
 */
_CC_API_PUBLIC(void)
_cc_des_setkey(uint32_t SK[32], const byte_t key[_CC_DES_KEY_SIZE_]) {
    DES_key_schedule ks;
    DES_set_key_unchecked((const_DES_cblock *)key, &ks);
    memcpy(SK, ks.ks->cblock, 32 * sizeof(uint32_t));
}

/**
 * @brief          DES key schedule (56-bit, encryption)
 *
 * @param ctx      DES context to be initialized
 * @param key      8-byte secret key
 *
 */
_CC_API_PUBLIC(void)
_cc_des_setkey_enc(_cc_des_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_]) {
    EVP_CIPHER_CTX *cipher = (EVP_CIPHER_CTX *)ctx->sk;
    if (EVP_EncryptInit_ex(cipher, EVP_des_ecb(), NULL, key, NULL) != 1) {
        ERR_raise(ERR_LIB_EVP, ERR_R_EVP_LIB);
    }
}

/**
 * @brief          DES key schedule (56-bit, decryption)
 *
 * @param ctx      DES context to be initialized
 * @param key      8-byte secret key
 *
 */
_CC_API_PUBLIC(void)
_cc_des_setkey_dec(_cc_des_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_]) {
    EVP_CIPHER_CTX *cipher = (EVP_CIPHER_CTX *)ctx->sk;
    if (EVP_DecryptInit_ex(cipher, EVP_des_ecb(), NULL, key, NULL) != 1) {
        ERR_raise(ERR_LIB_EVP, ERR_R_EVP_LIB);
    }
}

/**
 * @brief          Triple-DES key schedule (112-bit, encryption)
 *
 * @param ctx      3DES context to be initialized
 * @param key      16-byte secret key
 *
 */
_CC_API_PUBLIC(void)
_cc_des3_set2key_enc(_cc_des3_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_ * 2]) {
    EVP_CIPHER_CTX *cipher = (EVP_CIPHER_CTX *)ctx->sk;
    if (EVP_EncryptInit_ex(cipher, EVP_des_ede(), NULL, key, NULL) != 1) {
        ERR_raise(ERR_LIB_EVP, ERR_R_EVP_LIB);
    }
}

/**
 * @brief          Triple-DES key schedule (112-bit, decryption)
 *
 * @param ctx      3DES context to be initialized
 * @param key      16-byte secret key
 *
 */
_CC_API_PUBLIC(void)
_cc_des3_set2key_dec(_cc_des3_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_ * 2]) {
    EVP_CIPHER_CTX *cipher = (EVP_CIPHER_CTX *)ctx->sk;
    if (EVP_DecryptInit_ex(cipher, EVP_des_ede(), NULL, key, NULL) != 1) {
        ERR_raise(ERR_LIB_EVP, ERR_R_EVP_LIB);
    }
}

/**
 * @brief          Triple-DES key schedule (168-bit, encryption)
 *
 * @param ctx      3DES context to be initialized
 * @param key      24-byte secret key
 *
 */
_CC_API_PUBLIC(void)
_cc_des3_set3key_enc(_cc_des3_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_ * 3]) {
    EVP_CIPHER_CTX *cipher = (EVP_CIPHER_CTX *)ctx->sk;
    if (EVP_EncryptInit_ex(cipher, EVP_des_ede3(), NULL, key, NULL) != 1) {
        ERR_raise(ERR_LIB_EVP, ERR_R_EVP_LIB);
    }
}

/**
 * @brief          Triple-DES key schedule (168-bit, decryption)
 *
 * @param ctx      3DES context to be initialized
 * @param key      24-byte secret key
 *
 */
_CC_API_PUBLIC(void)
_cc_des3_set3key_dec(_cc_des3_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_ * 3]) {
    EVP_CIPHER_CTX *cipher = (EVP_CIPHER_CTX *)ctx->sk;
    if (EVP_DecryptInit_ex(cipher, EVP_des_ede3(), NULL, key, NULL) != 1) {
        ERR_raise(ERR_LIB_EVP, ERR_R_EVP_LIB);
    }
}

/**
 * @brief          DES-ECB block encryption/decryption
 *
 * @param ctx      DES context
 * @param input    64-bit input block
 * @param output   64-bit output block
 *
 */
_CC_API_PUBLIC(void)
_cc_des_crypt_ecb(_cc_des_t *ctx, const byte_t input[8], byte_t output[8]) {
    EVP_CIPHER_CTX *cipher = (EVP_CIPHER_CTX *)ctx->sk;
    int outlen;
    if (EVP_EncryptUpdate(cipher, output, &outlen, input, 8) != 1) {
        ERR_raise(ERR_LIB_EVP, ERR_R_EVP_LIB);
    }
}

/**
 * @brief          DES-CBC buffer encryption/decryption
 *
 * @note           Upon exit, the content of the IV is updated so that you can
 *                 call the function same function again on the following
 *                 block(s) of data and get the same result as if it was
 *                 encrypted in one call. This allows a "streaming" usage.
 *                 If on the other hand you need to retain the contents of the
 *                 IV, you should either save it manually or use the cipher
 *                 module instead.
 *
 * @param ctx      DES context
 * @param mode     _CC_DES_ENCRYPT_ or _CC_DES_DECRYPT_
 * @param length   length of the input data
 * @param iv       initialization vector (updated after use)
 * @param input    buffer holding the input data
 * @param output   buffer holding the output data
 */
_CC_API_PUBLIC(int)
_cc_des_crypt_cbc(_cc_des_t *ctx, int mode, size_t length, byte_t iv[8], const byte_t *input, byte_t *output) {
    EVP_CIPHER_CTX *cipher = (EVP_CIPHER_CTX *)ctx->sk;
    int outlen;
    if (mode == _CC_DES_ENCRYPT_) {
        if (EVP_EncryptInit_ex(cipher, EVP_des_cbc(), NULL, NULL, iv) != 1 ||
            EVP_EncryptUpdate(cipher, output, &outlen, input, length) != 1) {
            ERR_raise(ERR_LIB_EVP, ERR_R_EVP_LIB);
            return _CC_ERR_DES_INVALID_INPUT_LENGTH_;
        }
    } else {
        if (EVP_DecryptInit_ex(cipher, EVP_des_cbc(), NULL, NULL, iv) != 1 ||
            EVP_DecryptUpdate(cipher, output, &outlen, input, length) != 1) {
            ERR_raise(ERR_LIB_EVP, ERR_R_EVP_LIB);
            return _CC_ERR_DES_INVALID_INPUT_LENGTH_;
        }
    }
    return 0;
}

/**
 * @brief          3DES-ECB block encryption/decryption
 *
 * @param ctx      3DES context
 * @param input    64-bit input block
 * @param output   64-bit output block
 *
 */
_CC_API_PUBLIC(void)
_cc_des3_crypt_ecb(_cc_des3_t *ctx, const byte_t input[8], byte_t output[8]) {
    EVP_CIPHER_CTX *cipher = (EVP_CIPHER_CTX *)ctx->sk;
    int outlen;
    if (EVP_EncryptUpdate(cipher, output, &outlen, input, 8) != 1) {
        ERR_raise(ERR_LIB_EVP, ERR_R_EVP_LIB);
    }
}

/**
 * @brief          3DES-CBC buffer encryption/decryption
 *
 * @note           Upon exit, the content of the IV is updated so that you can
 *                 call the function same function again on the following
 *                 block(s) of data and get the same result as if it was
 *                 encrypted in one call. This allows a "streaming" usage.
 *                 If on the other hand you need to retain the contents of the
 *                 IV, you should either save it manually or use the cipher
 *                 module instead.
 *
 * @param ctx      3DES context
 * @param mode     _CC_DES_ENCRYPT_ or _CC_DES_DECRYPT_
 * @param length   length of the input data
 * @param iv       initialization vector (updated after use)
 * @param input    buffer holding the input data
 * @param output   buffer holding the output data
 *
 * @return         0 if successful, or _CC_ERR_DES_INVALID_INPUT_LENGTH_
 */
_CC_API_PUBLIC(int)
_cc_des3_crypt_cbc(_cc_des3_t *ctx, int mode, size_t length, byte_t iv[8], const byte_t *input, byte_t *output) {
    EVP_CIPHER_CTX *cipher = (EVP_CIPHER_CTX *)ctx->sk;
    int outlen;
    if (mode == _CC_DES_ENCRYPT_) {
        if (EVP_EncryptInit_ex(cipher, EVP_des_ede3_cbc(), NULL, NULL, iv) != 1 ||
            EVP_EncryptUpdate(cipher, output, &outlen, input, length) != 1) {
            ERR_raise(ERR_LIB_EVP, ERR_R_EVP_LIB);
            return _CC_ERR_DES_INVALID_INPUT_LENGTH_;
        }
    } else {
        if (EVP_DecryptInit_ex(cipher, EVP_des_ede3_cbc(), NULL, NULL, iv) != 1 ||
            EVP_DecryptUpdate(cipher, output, &outlen, input, length) != 1) {
            ERR_raise(ERR_LIB_EVP, ERR_R_EVP_LIB);
            return _CC_ERR_DES_INVALID_INPUT_LENGTH_;
        }
    }
    return 0;
}
