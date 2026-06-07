#ifndef _C_CC_BASE64_H_INCLUDED_
#define _C_CC_BASE64_H_INCLUDED_

#include "../cores.h"

/*
3*8bit=4*6bit=24
11010101 11000101 00110011
00110101 00011100 00010100 00110011

*/
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
/**/
#define _CC_BASE64_EN_LEN(N) (N * 4 / 3 + 4)
/**/
#define _CC_BASE64_DE_LEN(N) ((N / 4) * 3)

/**/
_CC_API_PUBLIC(size_t) _cc_base64_encode(const byte_t *input, size_t length, tchar_t *output, size_t output_length);
/**/
_CC_API_PUBLIC(size_t) _cc_base64_decode(const tchar_t *input, size_t length, byte_t *output, size_t output_length);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_BASE64_H_INCLUDED_ */
