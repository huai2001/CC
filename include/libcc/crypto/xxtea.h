#ifndef _C_CC_XXTEA_H_INCLUDED_
#define _C_CC_XXTEA_H_INCLUDED_

#include "../types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**/
_CC_API_PUBLIC(byte_t*) _cc_xxtea_encrypt(const byte_t *data, size_t length, const byte_t *key, size_t *output_length);
/**/
_CC_API_PUBLIC(byte_t*) _cc_xxtea_decrypt(const byte_t *data, size_t length, const byte_t *key, size_t *output_length);

#ifdef __cplusplus
}
#endif

#endif
