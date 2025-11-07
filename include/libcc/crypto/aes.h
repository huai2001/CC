#ifndef _C_CC_AES_H_INCLUDED_
#define _C_CC_AES_H_INCLUDED_

#include "../os.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_AES_ENCRYPT_        1
#define _CC_AES_DECRYPT_        0

#define _CC_AES_KEY_SIZE_       16
#define _CC_AES_BLOCK_SIZE_     16

#define _CC_AES_BUFFER_LEN(N)   ((N + (_CC_AES_BLOCK_SIZE_ - 1)) / _CC_AES_BLOCK_SIZE_) * _CC_AES_BLOCK_SIZE_

/**< Invalid key length. */
#define _CC_ERR_AES_INVALID_KEY_LENGTH_     -0x0020
/**< Invalid data input length. */
#define _CC_ERR_AES_INVALID_INPUT_LENGTH_   -0x0022

/**/
typedef struct _cc_aes {
    int nr;           /*!<  number of rounds  */
    uintptr_t rk;     /*!<  AES round keys    */
    uint32_t buf[68]; /*!<  unaligned data    */
} _cc_aes_t;

/**/
_CC_API_PUBLIC(void) _cc_aes_init(_cc_aes_t *ctx);
/**/
_CC_API_PUBLIC(int) _cc_aes_setkey_enc(_cc_aes_t *ctx, const byte_t *key, unsigned int keybits);
/**/
_CC_API_PUBLIC(int) _cc_aes_setkey_dec(_cc_aes_t *ctx, const byte_t *key, unsigned int keybits);
/**/
_CC_API_PUBLIC(int) _cc_aes_setkey(_cc_aes_t *ctx, int mode, const byte_t *key, uint32_t keybits);
/**/
_CC_API_PUBLIC(int) _cc_aes_crypt_cbc(_cc_aes_t *ctx, int mode, const byte_t *input, size_t length, byte_t iv[16], byte_t *output);
/**/
_CC_API_PUBLIC(int) _cc_aes_crypt_cfb128(_cc_aes_t *ctx, int mode, const byte_t *input, size_t length, size_t *iv_off, byte_t iv[16], byte_t *output);
/**/
_CC_API_PUBLIC(int) _cc_aes_crypt_cfb8(_cc_aes_t *ctx, int mode, const byte_t *input, size_t length, byte_t iv[16], byte_t *output);
/**/
_CC_API_PUBLIC(int) _cc_aes_crypt_ctr(_cc_aes_t *ctx, const byte_t *input, size_t length, size_t *nc_off, byte_t nonce_counter[16], byte_t stream_block[16], byte_t *output);
/**/
_CC_API_PUBLIC(void) _cc_aes_encrypt(_cc_aes_t *ctx, const byte_t input[16], byte_t output[16]);
/**/
_CC_API_PUBLIC(void) _cc_aes_decrypt(_cc_aes_t *ctx, const byte_t input[16], byte_t output[16]);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_AES_H_INCLUDED_*/
