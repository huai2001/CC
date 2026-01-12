#ifndef _C_CC_SHA_H_INCLUDED_
#define _C_CC_SHA_H_INCLUDED_

#include "hash.h"

#define _CC_SHA1_DIGEST_LENGTH_       20
#define _CC_SHA224_DIGEST_LENGTH_     28
#define _CC_SHA256_DIGEST_LENGTH_     32
#define _CC_SHA384_DIGEST_LENGTH_     48
#define _CC_SHA512_DIGEST_LENGTH_     64

#define _CC_KECCAK1600_WIDTH_         1600

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

_CC_API_PUBLIC(void) _cc_sha1_init(_cc_hasher_t *sha);
_CC_API_PUBLIC(void) _cc_sha224_init(_cc_hasher_t *sha);
_CC_API_PUBLIC(void) _cc_sha256_init(_cc_hasher_t *sha);
_CC_API_PUBLIC(void) _cc_sha384_init(_cc_hasher_t *sha);
_CC_API_PUBLIC(void) _cc_sha512_init(_cc_hasher_t *sha);

/**/
_CC_API_PUBLIC(void) _cc_sha1(const byte_t* input, size_t length, tchar_t* output);
/**/
_CC_API_PUBLIC(bool_t) _cc_sha1_fp(FILE* fp, tchar_t* output);
/**/
_CC_API_PUBLIC(bool_t) _cc_sha1_from_file(const tchar_t* file, tchar_t* output);
/**/
_CC_API_PUBLIC(void) _cc_sha256(const byte_t* input, size_t length, tchar_t* output, bool_t is224);
/**/
_CC_API_PUBLIC(bool_t) _cc_sha256_fp(FILE *fp, tchar_t *output, bool_t is224);
/**/
_CC_API_PUBLIC(bool_t) _cc_sha256_from_file(const tchar_t* file, tchar_t* output, bool_t is224);
/**/
_CC_API_PUBLIC(void) _cc_sha512(const byte_t* input, size_t length, tchar_t* output, bool_t is384);
/**/
_CC_API_PUBLIC(bool_t) _cc_sha512_fp(FILE *fp, tchar_t *output, bool_t is384);
/**/
_CC_API_PUBLIC(bool_t) _cc_sha512_from_file(const tchar_t* file, tchar_t* output, bool_t is384);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_SHA_H_INCLUDED_*/
