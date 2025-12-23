#ifndef _C_CC_URL_REQUEST_H_INCLUDED_
#define _C_CC_URL_REQUEST_H_INCLUDED_

#include "http.h"
#include "gzip.h"
#include "url.h"
#include "event.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

enum {
    _CC_URL_REQUEST_HEADER_ = 1,
    _CC_URL_REQUEST_RESPONSE_HEADER_,
    _CC_URL_REQUEST_RESPONSE_BODY_,
    _CC_URL_REQUEST_RESPONSE_SUCCESS_,
    _CC_URL_REQUEST_OUT_OF_MEMORY_,
    _CC_URL_REQUEST_HEADER_ERROR_,
    _CC_URL_REQUEST_ERROR_,
    _CC_URL_REQUEST_HTTPS_ERROR_,

    _CC_URL_REQUEST_INVALID_URL_,
    _CC_URL_REQUEST_INVALID_HOST_,

    _CC_URL_REQUEST_CONNECT_FAILED_,
    _CC_URL_REQUEST_SEND_FAILED_,
    _CC_URL_REQUEST_TIMEOUT_
};

enum {
    _CC_URL_TRANSFER_ENCODING_IDENTITY_ = 0,
    _CC_URL_TRANSFER_ENCODING_CHUNKED_ = 1
};

enum {
    _CC_URL_CONTENT_ENCODING_PLAINTEXT_ = 0,
    _CC_URL_CONTENT_ENCODING_GZIP_ = 1,
};

/**/
typedef struct _cc_http_request {
    uint8_t state;
    _cc_url_t url;
    _cc_buf_t buffer;
    _cc_io_buffer_t *io;
    _gzip_t *gzip;
    _cc_http_response_header_t *response;
    pvoid_t args;
} _cc_http_request_t;

/**/
_CC_API_PUBLIC(_cc_http_request_t*) _cc_http_request(const tchar_t *url, pvoid_t args);
/**/
_CC_API_PUBLIC(void) _cc_reset_http_request(_cc_http_request_t *request);
/**/
_CC_API_PUBLIC(void) _cc_free_http_request(_cc_http_request_t *request);
/**/
_CC_API_PUBLIC(bool_t) _cc_http_request_header(_cc_http_request_t *request, _cc_event_t *e);
/**/
_CC_API_PUBLIC(bool_t) _cc_http_request_response_header(_cc_http_request_t *request);
/**/
_CC_API_PUBLIC(bool_t) _cc_http_request_response_body(_cc_http_request_t *request);
/**/
_CC_API_PUBLIC(bool_t) _cc_http_request_ssl_handshake(_cc_http_request_t *request, _cc_event_t *e);
/**/
_CC_API_PUBLIC(bool_t) _cc_http_request_ssl(_cc_OpenSSL_t *openSSL,_cc_http_request_t *request, _cc_event_t *e);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif


#endif /*_C_CC_URL_REQUEST_H_INCLUDED_*/
