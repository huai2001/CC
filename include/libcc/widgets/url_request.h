/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#ifndef _C_CC_WIDGETS_URL_REQUEST_H_INCLUDED_
#define _C_CC_WIDGETS_URL_REQUEST_H_INCLUDED_

#include "http.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Successful 2xx.  */
#define HTTP_STATUS_OK                    200
#define HTTP_STATUS_CREATED               201
#define HTTP_STATUS_ACCEPTED              202
#define HTTP_STATUS_NO_CONTENT            204
#define HTTP_STATUS_PARTIAL_CONTENTS      206

/* Redirection 3xx.  */
#define HTTP_STATUS_MULTIPLE_CHOICES      300
#define HTTP_STATUS_MOVED_PERMANENTLY     301
#define HTTP_STATUS_MOVED_TEMPORARILY     302
#define HTTP_STATUS_SEE_OTHER             303 /* from HTTP/1.1 */
#define HTTP_STATUS_NOT_MODIFIED          304
#define HTTP_STATUS_TEMPORARY_REDIRECT    307 /* from HTTP/1.1 */
#define HTTP_STATUS_PERMANENT_REDIRECT    308 /* from HTTP/1.1 */

/* Client error 4xx.  */
#define HTTP_STATUS_BAD_REQUEST           400
#define HTTP_STATUS_UNAUTHORIZED          401
#define HTTP_STATUS_FORBIDDEN             403
#define HTTP_STATUS_NOT_FOUND             404
#define HTTP_STATUS_RANGE_NOT_SATISFIABLE 416

/* Server errors 5xx.  */
#define HTTP_STATUS_INTERNAL              500
#define HTTP_STATUS_NOT_IMPLEMENTED       501
#define HTTP_STATUS_BAD_GATEWAY           502
#define HTTP_STATUS_UNAVAILABLE           503
#define HTTP_STATUS_GATEWAY_TIMEOUT       504

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
    _CC_URL_TRANSFER_ENCODING_UNKNOWN_ = 0,
    _CC_URL_TRANSFER_ENCODING_CHUNKED_ = 1
};

enum {
    _CC_URL_CONTENT_ENCODING_PLAINTEXT_ = 0,
    _CC_URL_CONTENT_ENCODING_GZIP_ = 1,
};

/************************************************************************/
typedef struct _cc_url_request _cc_url_request_t;

/************************************************************************/

/**/
struct _cc_url_request {
    byte_t status;
    bool_t handshaking;

    _cc_url_t url;
    _cc_buf_t buffer;

    _cc_http_response_header_t *response;

    _cc_SSL_t* ssl;
    pvoid_t gzip;

    pvoid_t args;
};

/**
 * @brief An URL HTTP request
 *
 * @param url address string
 * @param args A user-supplied argument
 *
 * @return _cc_url_request_t
 */
_CC_WIDGETS_API(_cc_url_request_t*) _cc_url_request(const tchar_t *url, pvoid_t args);
/**/
_CC_WIDGETS_API(void) _cc_reset_url_request(_cc_url_request_t *request);
/**/
_CC_WIDGETS_API(void) _cc_free_url_request(_cc_url_request_t *request);
/**/
_CC_WIDGETS_API(bool_t) _cc_url_request_header(_cc_url_request_t *request, _cc_event_t *e);
/**/
_CC_WIDGETS_API(bool_t) _cc_url_request_response_header(_cc_url_request_t *request, _cc_event_rbuf_t *r);
/**/
_CC_WIDGETS_API(bool_t) _cc_url_request_response_body(_cc_url_request_t *request, _cc_event_rbuf_t *r);
/**/
_CC_WIDGETS_API(bool_t) _cc_url_request_ssl_handshake(_cc_url_request_t *request, _cc_event_t *e);
/**/
_CC_WIDGETS_API(bool_t) _cc_url_request_read(_cc_url_request_t *request, _cc_event_t *e);
/**/
_CC_WIDGETS_API(bool_t) _cc_url_request_sendbuf(_cc_url_request_t *request, _cc_event_t *e);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif


#endif /*_C_CC_URL_REQUEST_H_INCLUDED_*/
