#ifndef _C_CC_DES_H_INCLUDED_
#define _C_CC_DES_H_INCLUDED_

#include "../cores.h"

#define _CC_DES_ENCRYPT_ 1
#define _CC_DES_DECRYPT_ 0

/**< The data input has an invalid length. */
#define _CC_ERR_DES_INVALID_INPUT_LENGTH_ -0x0032

#define _CC_DES_KEY_SIZE_ 8

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**/
typedef struct _cc_des {
    uint32_t sk[32]; /*!<  DES subkeys       */
} _cc_des_t;

/**/
typedef struct _cc_des3 {
    uint32_t sk[96]; /*!<  3DES subkeys      */
} _cc_des3_t;

/**/
_CC_API_PUBLIC(void) _cc_des_init(_cc_des_t *ctx);
/**/
_CC_API_PUBLIC(void) _cc_des3_init(_cc_des3_t *ctx);
/**/
_CC_API_PUBLIC(void) _cc_des_key_set_parity(byte_t key[_CC_DES_KEY_SIZE_]);
/**/
_CC_API_PUBLIC(bool_t) _cc_des_key_check_key_parity(const byte_t key[_CC_DES_KEY_SIZE_]);
/**/
_CC_API_PUBLIC(bool_t) _cc_des_key_check_weak(const byte_t key[_CC_DES_KEY_SIZE_]);
/**/
_CC_API_PUBLIC(void) _cc_des_setkey_enc(_cc_des_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_]);
/**/
_CC_API_PUBLIC(void) _cc_des_setkey_dec(_cc_des_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_]);
/**/
_CC_API_PUBLIC(void) _cc_des3_set2key_enc(_cc_des3_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_ * 2]);
/**/
_CC_API_PUBLIC(void) _cc_des3_set2key_dec(_cc_des3_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_ * 2]);
/**/
_CC_API_PUBLIC(void) _cc_des3_set3key_enc(_cc_des3_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_ * 3]);
/**/
_CC_API_PUBLIC(void) _cc_des3_set3key_dec(_cc_des3_t *ctx, const byte_t key[_CC_DES_KEY_SIZE_ * 3]);
/**/
_CC_API_PUBLIC(void) _cc_des_crypt_ecb(_cc_des_t *ctx, const byte_t input[8], byte_t output[8]);
/**/
_CC_API_PUBLIC(int) _cc_des_crypt_cbc(_cc_des_t *ctx, int mode, size_t length, byte_t iv[8], const byte_t *input, byte_t *output);
/**/
_CC_API_PUBLIC(void) _cc_des3_crypt_ecb(_cc_des3_t *ctx, const byte_t input[8], byte_t output[8]);
/**/
_CC_API_PUBLIC(int) _cc_des3_crypt_cbc(_cc_des3_t *ctx, int mode, size_t length, byte_t iv[8], const byte_t *input, byte_t *output);
/**/
_CC_API_PUBLIC(void) _cc_des_setkey(uint32_t SK[32], const byte_t key[_CC_DES_KEY_SIZE_]);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_DES_H_INCLUDED_*/
