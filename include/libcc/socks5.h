
#ifndef _C_CC_LIBSOCKS5_H_INCLUDED_
#define _C_CC_LIBSOCKS5_H_INCLUDED_

#include "event.h"

#define _CC_SOCKS5_AUTH_NONE_                   0x00
#define _CC_SOCKS5_AUTH_GSSAPI_                 0x01
#define _CC_SOCKS5_AUTH_ID_KEY_                 0x02
#define _CC_SOCKS5_AUTH_IANA_                   0x03
#define _CC_SOCKS5_AUTH_RESERVE_                0x04
#define _CC_SOCKS5_AUTH_NOT_SUPPORT_            0xFF

#define _CC_SOCKS5_AUTH_RESULT_OK_              0x00
#define _CC_SOCKS5_AUTH_RESULT_ERR_             0x0F

#define _CC_SOCKS5_CMD_CONNECT_                 0x01
#define _CC_SOCKS5_CMD_BIND_                    0x02
#define _CC_SOCKS5_CMD_UDPASS_                  0x03

#define _CC_SOCKS5_ADDRESS_TYPE_IPV4_           0x01
#define _CC_SOCKS5_ADDRESS_TYPE_DOMAIN_         0x03
#define _CC_SOCKS5_ADDRESS_TYPE_IPV6_           0x04

#define _CC_SOCKS5_CMD_RESPONSE_OK_             0x00
#define _CC_SOCKS5_CMD_RESPONSE_AGENT_ERR_      0x01
#define _CC_SOCKS5_CMD_RESPONSE_NOT_ALLOWED_    0x02
#define _CC_SOCKS5_CMD_RESPONSE_NETWORK_ERR_    0x03
#define _CC_SOCKS5_CMD_RESPONSE_TARGET_INVALID_ 0x04
#define _CC_SOCKS5_CMD_RESPONSE_TARGET_REFUSED_ 0x05
#define _CC_SOCKS5_CMD_RESPONSE_TTL_TIMEOUT_    0x06
#define _CC_SOCKS5_CMD_RESPONSE_NOT_SUPPORTED_  0x07
#define _CC_SOCKS5_CMD_RESPONSE_TGT_NOTSPT_     0x08

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/*
https://www.ietf.org/rfc/rfc1928.txt
+----+----------+----------+
|VER | NMETHODS | METHODS  |
+----+----------+----------+
| 1  |    1     | 1 to 255 |
+----+----------+----------+
METHOD:
 0x00 No certification required (commonly used)
 0x01 GSSAPI
 0x02 Account Password Authentication (Commonly Used)
 0x03 - 0x7F IANA
 0x80 - 0xFE reserved
 0xFF No supported authentication method
*/
typedef struct _socks5 {
    byte_t state;
    byte_t method;
    _cc_event_t *e;
    _cc_union_sockaddr_t addr;
    uint16_t port;
} _cc_socks5_t;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_LIBSOCKS5_H_INCLUDED_ */


