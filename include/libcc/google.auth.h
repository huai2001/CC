
#ifndef _C_CC_GOOGLE_AUTH_H_INCLUDED_
#define _C_CC_GOOGLE_AUTH_H_INCLUDED_

#include "event.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
/* Generate a random Base32 secret key */
_CC_API_PUBLIC(int32_t) _cc_generate_secret(char *secret, size_t length);
/**/
_CC_API_PUBLIC(uint32_t) _cc_generate_totp(const tchar_t *secret, uint32_t time_step_seconds);
/* Verify TOTP code with a window of allowed steps */
_CC_API_PUBLIC(bool_t) _cc_verify_totp(const char *secret, uint32_t code, uint32_t time_step_seconds, int window);
    
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_GOOGLE_AUTH_H_INCLUDED_ */


