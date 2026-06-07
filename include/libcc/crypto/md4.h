#ifndef _C_CC_MD4_H_INCLUDED_
#define _C_CC_MD4_H_INCLUDED_

#include "../cores.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_MD4_DIGEST_LENGTH_ 16

/**/
typedef struct _cc_md4 {
    uint32_t total[2]; /*!< number of bytes processed  */
    uint32_t state[4]; /*!< intermediate digest state  */
    byte_t buffer[64]; /*!< data block being processed */
} _cc_md4_t;

/**/
_CC_API_PUBLIC(void) _cc_md4_init(_cc_md4_t* ctx);
/**/
_CC_API_PUBLIC(void) _cc_md4_update(_cc_md4_t* ctx, const byte_t* input, size_t length);
/**/
_CC_API_PUBLIC(void) _cc_md4_final(_cc_md4_t* ctx, byte_t* output);
/**/
_CC_API_PUBLIC(bool_t) _cc_md4_fp(FILE* fp, tchar_t* output);
/**/
_CC_API_PUBLIC(bool_t) _cc_md4_from_file(const tchar_t* file, tchar_t* output);
/**/
_CC_API_PUBLIC(void) _cc_md4(const byte_t* input, size_t length, tchar_t* output);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_MD2_H_INCLUDED_*/
