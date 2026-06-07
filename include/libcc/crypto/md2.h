#ifndef _C_CC_MD2_H_INCLUDED_
#define _C_CC_MD2_H_INCLUDED_

#include "../cores.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_MD2_DIGEST_LENGTH_ 16
/**/
typedef struct _cc_md2 {
    byte_t cksum[16];  /*!< checksum of the data block */
    byte_t state[48];  /*!< intermediate digest state  */
    byte_t buffer[16]; /*!< data block being processed */
    size_t left;       /*!< amount of data in buffer   */
} _cc_md2_t;

/**/
_CC_API_PUBLIC(void) _cc_md2_init(_cc_md2_t* ctx);
/**/
_CC_API_PUBLIC(void) _cc_md2_update(_cc_md2_t* ctx, const byte_t* input, size_t length);
/**/
_CC_API_PUBLIC(void) _cc_md2_final(_cc_md2_t* ctx, byte_t* output);
/**/
_CC_API_PUBLIC(bool_t) _cc_md2_fp(FILE* fp, tchar_t* output);
/**/
_CC_API_PUBLIC(bool_t) _cc_md2_from_file(const tchar_t* file, tchar_t* output);
/**/
_CC_API_PUBLIC(void) _cc_md2(const byte_t* input, size_t length, tchar_t* output);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_MD2_H_INCLUDED_*/
