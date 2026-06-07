

#ifndef _C_CC_UUID_H_INCLUDED_
#define _C_CC_UUID_H_INCLUDED_

#include "cores.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**/
typedef byte_t _cc_uuid_t[16];

/**/
_CC_API_PUBLIC(void) _cc_uuid(_cc_uuid_t *u);
/**/
_CC_API_PUBLIC(void) _cc_uuid_to_bytes(_cc_uuid_t *u, const tchar_t *buf);

/**/
_CC_API_PUBLIC(int32_t) _cc_uuid_lower(_cc_uuid_t *u, tchar_t *buf, int32_t length);
/**/
_CC_API_PUBLIC(int32_t) _cc_uuid_upper(_cc_uuid_t *u, tchar_t *buf, int32_t length);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#endif /*_C_CC_UUID_H_INCLUDED_*/
