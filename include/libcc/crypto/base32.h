#ifndef _C_CC_BASE32_H_INCLUDED_
#define _C_CC_BASE32_H_INCLUDED_

#include "../cores.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**/
#define _CC_BASE32_EN_LEN(N) (N * 2)
/**/
#define _CC_BASE32_DE_LEN(N) (((N + 7) / 8) * 5)

/**/
_CC_API_PUBLIC(size_t) _cc_base32_encode(const byte_t *input, size_t length, tchar_t *output, size_t output_length);
/**/
_CC_API_PUBLIC(size_t) _cc_base32_decode(const tchar_t *input, size_t length, byte_t *output, size_t output_length);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_BASE32_H_INCLUDED_ */
