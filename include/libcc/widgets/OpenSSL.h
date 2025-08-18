/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
 * All rights reserved.org>
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _C_CC_WIDGETS_OPENSSL_C_H_INCLUDED_
#define _C_CC_WIDGETS_OPENSSL_C_H_INCLUDED_

//#define _CC_ENABLE_OPENSSL_ 1

#include "event.h"


/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_SSL_SSLv2_    0x0002
#define _CC_SSL_SSLv3_    0x0004
#define _CC_SSL_TLSv1_    0x0008
#define _CC_SSL_TLSv1_1_  0x0010
#define _CC_SSL_TLSv1_2_  0x0020
#define _CC_SSL_TLSv1_3_  0x0040

#define _CC_SSL_HS_ERROR_           -1
#define _CC_SSL_HS_ESTABLISHED_     1
#define _CC_SSL_HS_WANT_READ_       2
#define _CC_SSL_HS_WANT_WRITE_      3

#define OPENSSL_VERSION_1_0_2	0x10002000L
#define OPENSSL_VERSION_1_1_0	0x10100000L
#define OPENSSL_VERSION_3_0_0	0x30000000L

typedef struct _cc_OpenSSL _cc_OpenSSL_t;
typedef struct _cc_SSL _cc_SSL_t;

#if OPENSSL_VERSION_NUMBER < OPENSSL_VERSION_3_0_0
	#define _SSL_X509_NAME_HASH	X509_NAME_hash
#else
	_CC_FORCE_INLINE_ unsigned long _SSL_X509_NAME_HASH(const X509_NAME *x) {
		return X509_NAME_hash_ex(x, nullptr, nullptr, nullptr);
	}
#endif


_CC_WIDGETS_API(_cc_OpenSSL_t*) _SSL_init(bool_t is_client);
/**/
_CC_WIDGETS_API(void) _SSL_quit(_cc_OpenSSL_t *);
/**/
_CC_WIDGETS_API(_cc_SSL_t*) _SSL_alloc(_cc_OpenSSL_t*);
/**/
_CC_WIDGETS_API(bool_t) _SSL_free(_cc_SSL_t*);
/**/
_CC_WIDGETS_API(bool_t) _SSL_connect(_cc_SSL_t *ctx, _cc_socket_t fd);
/**/
_CC_WIDGETS_API(void) _SSL_set_host_name(_cc_SSL_t*, tchar_t*, size_t);
/**/
_CC_WIDGETS_API(uint16_t) _SSL_do_handshake(_cc_SSL_t*);
/**/
_CC_WIDGETS_API(int32_t) _SSL_send(_cc_SSL_t*, const byte_t*, int32_t);
/**/
_CC_WIDGETS_API(int32_t) _SSL_read(_cc_SSL_t*, byte_t*, int32_t);
/**/
_CC_WIDGETS_API(int32_t) _SSL_sendbuf(_cc_SSL_t*, _cc_event_t*);
/**/
_CC_API_PUBLIC(int32_t) _SSL_event_send(_cc_SSL_t*, _cc_event_t*, const byte_t*, int32_t);
/**/
_CC_WIDGETS_API(bool_t) _SSL_event_read(_cc_SSL_t*, _cc_event_t*);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif
