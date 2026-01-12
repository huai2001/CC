#ifndef _C_CC_MD5_H_INCLUDED_
#define _C_CC_MD5_H_INCLUDED_

#include "hash.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_MD5_DIGEST_LENGTH_ 16

_CC_API_PUBLIC(void) _cc_md5_hash_init(_cc_hasher_t *ctx);

/**/
typedef struct _cc_md5 {
    uint32_t total[2]; /*!< number of bytes processed  */
    uint32_t state[4]; /*!< intermediate digest state  */
    byte_t buffer[64]; /*!< data block being processed */
} _cc_md5_t;

/**/
_CC_API_PUBLIC(void) _cc_md5_init(_cc_md5_t* ctx);
/**/
_CC_API_PUBLIC(void) _cc_md5_update(_cc_md5_t* ctx, const byte_t* input, size_t length);
/**/
_CC_API_PUBLIC(void) _cc_md5_final(_cc_md5_t* ctx, byte_t* output);
/**/
_CC_API_PUBLIC(bool_t) _cc_md5_fp(FILE* fp, tchar_t* output);
/**/
_CC_API_PUBLIC(bool_t) _cc_md5_from_file(const tchar_t* file, tchar_t* output);
/**/
_CC_API_PUBLIC(void) _cc_md5(const byte_t* input, size_t length, tchar_t* output);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_MD5_H_INCLUDED_*/
