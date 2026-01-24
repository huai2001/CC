#ifndef _C_CC_HTTP_C_H_INCLUDED_
#define _C_CC_HTTP_C_H_INCLUDED_

#include <math.h>
#include <wchar.h>
#include "sds.h"
#include "rbtree.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

enum {
    _CC_HTTP_STATE_HEADER_ = 0,
    _CC_HTTP_STATE_PAYLOAD_ = 1,
    _CC_HTTP_STATE_ESTABLISHED_ = 2,

    _CC_HTTP_ERROR_UNIMPLEMENTED_,
    _CC_HTTP_ERROR_NOFOUND_,
    _CC_HTTP_ERROR_BADREQUEST_,
    _CC_HTTP_ERROR_TOOLARGE_,
};

/* Status Codes */
#define _CC_HTTP_STATUS_MAP(XX)                                               \
    XX(100, CONTINUE,                        Continue)                        \
    XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
    XX(102, PROCESSING,                      Processing)                      \
    /* Successful 2xx.  */                                                    \
    XX(200, OK,                              OK)                              \
    XX(201, CREATED,                         Created)                         \
    XX(202, ACCEPTED,                        Accepted)                        \
    XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \
    XX(204, NO_CONTENT,                      No Content)                      \
    XX(205, RESET_CONTENT,                   Reset Content)                   \
    XX(206, PARTIAL_CONTENT,                 Partial Content)                 \
    XX(207, MULTI_STATUS,                    Multi-Status)                    \
    XX(208, ALREADY_REPORTED,                Already Reported)                \
    XX(226, IM_USED,                         IM Used)                         \
    /* Redirection 3xx.  */                                                   \
    XX(300, MULTIPLE_CHOICES,                Multiple Choices)                \
    XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
    XX(302, FOUND,                           Found)                           \
    XX(303, SEE_OTHER,                       See Other)                       \
    XX(304, NOT_MODIFIED,                    Not Modified)                    \
    XX(305, USE_PROXY,                       Use Proxy)                       \
    XX(307, TEMPORARY_REDIRECT,              Temporary Redirect)              \
    XX(308, PERMANENT_REDIRECT,              Permanent Redirect)              \
    /* Client error 4xx.  */                                                  \
    XX(400, BAD_REQUEST,                     Bad Request)                     \
    XX(401, UNAUTHORIZED,                    Unauthorized)                    \
    XX(402, PAYMENT_REQUIRED,                Payment Required)                \
    XX(403, FORBIDDEN,                       Forbidden)                       \
    XX(404, NOT_FOUND,                       Not Found)                       \
    XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
    XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
    XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
    XX(408, REQUEST_TIMEOUT,                 Request Timeout)                 \
    XX(409, CONFLICT,                        Conflict)                        \
    XX(410, GONE,                            Gone)                            \
    XX(411, LENGTH_REQUIRED,                 Length Required)                 \
    XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
    XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
    XX(414, URI_TOO_LONG,                    URI Too Long)                    \
    XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
    XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
    XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
    XX(421, MISDIRECTED_REQUEST,             Misdirected Request)             \
    XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
    XX(423, LOCKED,                          Locked)                          \
    XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
    XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
    XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
    XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
    XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
    XX(451, UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   \
    /* Server errors 5xx. */                                                  \
    XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \
    XX(501, NOT_IMPLEMENTED,                 Not Implemented)                 \
    XX(502, BAD_GATEWAY,                     Bad Gateway)                     \
    XX(503, SERVICE_UNAVAILABLE,             Service Unavailable)             \
    XX(504, GATEWAY_TIMEOUT,                 Gateway Timeout)                 \
    XX(505, HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      \
    XX(506, VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         \
    XX(507, INSUFFICIENT_STORAGE,            Insufficient Storage)            \
    XX(508, LOOP_DETECTED,                   Loop Detected)                   \
    XX(510, NOT_EXTENDED,                    Not Extended)                    \
    XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) \

enum {
#define XX(NUM, NAME, STRING) _CC_HTTP_STATUS_##NAME##_ = NUM,
    _CC_HTTP_STATUS_MAP(XX)
#undef XX
};

/* Request Methods */
#define _CC_HTTP_METHOD_MAP(XX)       \
    XX(0,  DELETE,      DELETE)       \
    XX(1,  GET,         GET)          \
    XX(2,  HEAD,        HEAD)         \
    XX(3,  POST,        POST)         \
    XX(4,  PUT,         PUT)          \
    /* pathological */                \
    XX(5,  CONNECT,     CONNECT)      \
    XX(6,  OPTIONS,     OPTIONS)      \
    XX(7,  TRACE,       TRACE)        \
    /* WebDAV */                      \
    XX(8,  COPY,        COPY)         \
    XX(9,  LOCK,        LOCK)         \
    XX(10, MKCOL,       MKCOL)        \
    XX(11, MOVE,        MOVE)         \
    XX(12, PROPFIND,    PROPFIND)     \
    XX(13, PROPPATCH,   PROPPATCH)    \
    XX(14, SEARCH,      SEARCH)       \
    XX(15, UNLOCK,      UNLOCK)       \
    XX(16, BIND,        BIND)         \
    XX(17, REBIND,      REBIND)       \
    XX(18, UNBIND,      UNBIND)       \
    XX(19, ACL,         ACL)          \
    /* subversion */                  \
    XX(20, REPORT,      REPORT)       \
    XX(21, MKACTIVITY,  MKACTIVITY)   \
    XX(22, CHECKOUT,    CHECKOUT)     \
    XX(23, MERGE,       MERGE)        \
    /* upnp */                        \
    XX(24, MSEARCH,     M-SEARCH)     \
    XX(25, NOTIFY,      NOTIFY)       \
    XX(26, SUBSCRIBE,   SUBSCRIBE)    \
    XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
    /* RFC-5789 */                    \
    XX(28, PATCH,       PATCH)        \
    XX(29, PURGE,       PURGE)        \
    /* CalDAV */                      \
    XX(30, MKCALENDAR,  MKCALENDAR)   \
    /* RFC-2068, section 19.6.1.2 */  \
    XX(31, LINK,        LINK)         \
    XX(32, UNLINK,      UNLINK)       \
    /* icecast */                     \
    XX(33, SOURCE,      SOURCE)       \

enum {
#define XX(NUM, NAME, STRING) _CC_HTTP_METHOD_##NAME##_ = NUM,
    _CC_HTTP_METHOD_MAP(XX)
#undef XX
};

/*
 * Define maximum number of headers that we accept.
 * This should be big enough to handle legitimate cases,
 * but limited to avoid DoS.
 */
#ifndef _CC_HTTP_MAX_HEADERS_
#define _CC_HTTP_MAX_HEADERS_   256
#endif

/**/
typedef struct _cc_http_header {
    _cc_sds_t keyword;
    _cc_sds_t value;
    _cc_rbtree_iterator_t lnk;
} _cc_http_header_t;

/**/
typedef struct _cc_http_request_header {
    _cc_sds_t method;
    _cc_sds_t script;
    _cc_sds_t protocol;

    _cc_rbtree_t headers;
    
} _cc_http_request_header_t;

/**/
typedef struct _cc_http_response_header {
    bool_t keep_alive;
    byte_t content_encoding;
    byte_t transfer_encoding;
    
    int32_t status;

    uint64_t download_length;
    uint64_t content_length;

    _cc_sds_t protocol;
    _cc_sds_t description;
    _cc_sds_t location;

    _cc_rbtree_t headers;
} _cc_http_response_header_t;

typedef bool_t (*_cc_http_header_fn_t)(pvoid_t *arg, tchar_t *line, int32_t length);

_CC_API_PUBLIC(_cc_http_header_t*) _cc_http_header_alloc(void);
/**/
_CC_API_PUBLIC(void) _cc_http_header_free(_cc_http_header_t *m);
/**/
_CC_API_PUBLIC(bool_t) _cc_http_header_push(_cc_rbtree_t *ctx, _cc_http_header_t *data);
/**/
_CC_API_PUBLIC(const _cc_http_header_t*) _cc_http_header_find(_cc_rbtree_t *ctx, const tchar_t *keyword);
/**/
_CC_API_PUBLIC(void) _cc_http_header_free_all(_cc_rbtree_t *ctx);
/**/
_CC_API_PUBLIC(bool_t) _cc_http_header_line(_cc_rbtree_t *headers, tchar_t *line, int32_t length);
/**/
_CC_API_PUBLIC(int) _cc_http_header_parser(_cc_http_header_fn_t fn, pvoid_t *arg, byte_t *bytes, int32_t *length);
/**/
_CC_API_PUBLIC(bool_t) _cc_http_alloc_request_header(_cc_http_request_header_t **http_header, tchar_t *line, int32_t length);
/**/
_CC_API_PUBLIC(void) _cc_http_free_request_header(_cc_http_request_header_t **http_header);
/**/
_CC_API_PUBLIC(bool_t) _cc_http_alloc_response_header(_cc_http_response_header_t **url_response, tchar_t *line, int32_t length);
/**/
_CC_API_PUBLIC(void) _cc_http_free_response_header(_cc_http_response_header_t **response_header);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_HTTP_C_H_INCLUDED_*/