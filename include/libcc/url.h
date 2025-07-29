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
#ifndef _C_CC_URL_H_INCLUDED_
#define _C_CC_URL_H_INCLUDED_

#include "rbtree.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_PORT_FTP_       21
#define _CC_PORT_FTPS_      990
#define _CC_PORT_TELNET_    23
#define _CC_PORT_HTTP_      80
#define _CC_PORT_HTTPS_     443
#define _CC_PORT_DICT_      2628
#define _CC_PORT_LDAP_      389
#define _CC_PORT_LDAPS_     636
#define _CC_PORT_TFTP_      69
#define _CC_PORT_SSH_       22
#define _CC_PORT_IMAP_      143
#define _CC_PORT_IMAPS_     993
#define _CC_PORT_POP3_      110
#define _CC_PORT_POP3S_     995
#define _CC_PORT_SMB_       445
#define _CC_PORT_SMBS_      445
#define _CC_PORT_SMTP_      25
#define _CC_PORT_SMTPS_     465 /* sometimes called SSMTP */
#define _CC_PORT_SYSLOG_    514
#define _CC_PORT_RTSP_      554
#define _CC_PORT_RTMP_      1935
#define _CC_PORT_RTMPT_     _CC_PORT_HTTP_
#define _CC_PORT_RTMPS_     _CC_PORT_HTTPS_
#define _CC_PORT_GOPHER_    70
#define _CC_PORT_MQTT_      1883

#define _CC_PORT_NNTP_      119
#define _CC_PORT_NEWS_      119
#define _CC_PORT_MYSQL_     3306
#define _CC_PORT_SQLSERVER_ 1433
#define _CC_PORT_ORACLE_    1521
#define _CC_PORT_SQLITE_    0
#define _CC_PORT_EXCEL_     0

/*Scheme name*/
enum _CC_SCHEME_TYPES_ {
    _CC_SCHEME_UNKNOWN_ = 0,
    _CC_SCHEME_HTTPS_ = 1,
    _CC_SCHEME_HTTP_,
    _CC_SCHEME_FTP_,
    _CC_SCHEME_FTPS_,
    _CC_SCHEME_NNTP_,
    _CC_SCHEME_NEWS_,
    _CC_SCHEME_MYSQL_,
    _CC_SCHEME_SQLSERVER_,
    _CC_SCHEME_SQLITE_,
    _CC_SCHEME_EXCEL_,
    _CC_SCHEME_ORACLE_
};

typedef struct _cc_fields {
    tchar_t *name;
    tchar_t *value;

    _cc_rbtree_iterator_t lnk;
} _cc_fields_t;

/*:
 * <scheme>://<net_loc>/<path>;<params>?<query>#<fragment>
 Scheme     eg.: http:\\,https:\\,ftp:\\
 Net_loc    host and port
 Path       URL-PATH
 request    URL-REQUEST
 query      URL-QUERY
 Fragment   URL-FRAGMENT
*/

/* http://user_name:user_password@localhost/index.html?id=1&tid=2#abc */
typedef struct _cc_url {
    /* (eg: http,ftp,maito) */
    struct {
        uint32_t ident;
        tchar_t *value;
    } scheme;
    /* host IPv6*/
    bool_t ipv6;
    /* (eg: port) */
    uint32_t port;

    /* (eg: localhost) */
    tchar_t *host;
    /* (eg: /v1/index.html) */
    tchar_t *path;
    /* (eg: /v1/index.html?id=1&tid=2#abc) */
    tchar_t *request;
    /* (eg: id=1&tid=2) */
    tchar_t *query;
    /* (eg: abc) */
    tchar_t *fragment;
    /* (eg: user_name) */
    tchar_t *username;
    /* (eg: user_password) */
    tchar_t *password;
} _cc_url_t;

#define _cc_parse_url _cc_alloc_url
/**/

/*create url*/
_CC_API_PUBLIC(bool_t) _cc_alloc_url(_cc_url_t*, const tchar_t *);
/**/
_CC_API_PUBLIC(bool_t) _cc_free_url(_cc_url_t*);
/* examples 1:
char_t *urls = "http://www.domain.com/index.php?id=1&tid=2#top";
_cc_url_t url;
_cc_alloc_url(&url, urls);
printf("-%s - %s- %d-\n",url.scheme.value, url.host, url.port);
_cc_free_url(&url);
*/

/**/
_CC_API_PUBLIC(int32_t)
_cc_url_encode(const tchar_t *src, int32_t src_len, tchar_t *dst, int32_t dst_len);
/**/
_CC_API_PUBLIC(int32_t)
_cc_url_decode(const tchar_t *src, int32_t src_len, tchar_t *dst, int32_t dst_len);
/**/
_CC_API_PUBLIC(int32_t)
_cc_raw_url_encode(const tchar_t *src, int32_t src_len, tchar_t *dst, int32_t dst_len);
/**/
_CC_API_PUBLIC(int32_t)
_cc_raw_url_decode(const tchar_t *src, int32_t src_len, tchar_t *dst, int32_t dst_len);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_URL_H_INCLUDED_ */
