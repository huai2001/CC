
#ifndef _C_CC_OPENSSL_C_H_INCLUDED_
#define _C_CC_OPENSSL_C_H_INCLUDED_

#include "alloc.h"
#include "socket.h"

#define _CC_SSL_SSLv2_    0x0002
#define _CC_SSL_SSLv3_    0x0004
#define _CC_SSL_TLSv1_    0x0008
#define _CC_SSL_TLSv1_1_  0x0010
#define _CC_SSL_TLSv1_2_  0x0020
#define _CC_SSL_TLSv1_3_  0x0040

#define _CC_SSL_DEFAULT_PROTOCOLS_  (_CC_SSL_TLSv1_2_|_CC_SSL_TLSv1_3_)

#define _CC_SSL_HS_ERROR_					0xff
#define _CC_SSL_HS_ESTABLISHED_				0x01
#define _CC_SSL_HS_WANT_READ_				0x02
#define _CC_SSL_HS_WANT_WRITE_				0x03
#define _CC_SSL_HS_SYSCALL_WOULDBLOCK_      0x04

#define OPENSSL_VERSION_1_0_2	0x10002000L
#define OPENSSL_VERSION_1_1_0	0x10100000L
#define OPENSSL_VERSION_3_0_0	0x30000000L

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cc_OpenSSL _cc_OpenSSL_t;
typedef struct _cc_SSL _cc_SSL_t;
#ifdef _CC_USE_OPENSSL_
#if OPENSSL_VERSION_NUMBER < OPENSSL_VERSION_3_0_0
	#define _SSL_X509_NAME_HASH	X509_NAME_hash
#else
	_CC_FORCE_INLINE_ unsigned long _SSL_X509_NAME_HASH(const X509_NAME *x) {
		return X509_NAME_hash_ex(x, nullptr, nullptr, nullptr);
	}
#endif
#endif
_CC_API_PUBLIC(bool_t) _SSL_setup(_cc_OpenSSL_t *ssl,
                                const tchar_t *cert_file,
                                const tchar_t *key_file,
                                const tchar_t *key_password);

_CC_API_PUBLIC(_cc_OpenSSL_t*) _SSL_init(uint32_t protocols);
/**/
_CC_API_PUBLIC(void) _SSL_quit(_cc_OpenSSL_t *);
/**/
_CC_API_PUBLIC(_cc_SSL_t*) _SSL_alloc(_cc_OpenSSL_t*);
/**/
_CC_API_PUBLIC(bool_t) _SSL_free(_cc_SSL_t*);
/**/
_CC_API_PUBLIC(_cc_SSL_t*) _SSL_accept(_cc_OpenSSL_t *ctx, _cc_socket_t fd);
/**/
_CC_API_PUBLIC(_cc_SSL_t*) _SSL_connect(_cc_OpenSSL_t *ctx, _cc_socket_t fd);
/**/
_CC_API_PUBLIC(void) _SSL_set_host_name(_cc_SSL_t*, const tchar_t*, size_t);
/**/
_CC_API_PUBLIC(uint8_t) _SSL_do_handshake(_cc_SSL_t*);
/**/
_CC_API_PUBLIC(int32_t) _SSL_send(_cc_SSL_t*, const byte_t*, int32_t);
/**/
_CC_API_PUBLIC(int32_t) _SSL_read(_cc_SSL_t*, byte_t*, int32_t);
/**/
_CC_API_PUBLIC(int) _SSL_set_alpn_protos(_cc_SSL_t*, const unsigned char *protos, unsigned int protos_len);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif
