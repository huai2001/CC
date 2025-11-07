#ifndef _C_CC_HMAC_H_INCLUDED_
#define _C_CC_HMAC_H_INCLUDED_

#include "md5.h"
#include "sha.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**/
_CC_API_PUBLIC(void) _cc_hmac_init(_cc_hash_t *ctx, byte_t method, const byte_t *key, size_t key_length);
/**/
_CC_API_PUBLIC(int) _cc_hmac(byte_t type, const byte_t *input, size_t input_length, const byte_t *key, size_t key_length, tchar_t *output);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_HMAC_H_INCLUDED_*/
