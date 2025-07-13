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
#ifndef _C_CC_WIDGETS_HTTP_C_H_INCLUDED_
#define _C_CC_WIDGETS_HTTP_C_H_INCLUDED_

#include <math.h>
#include <wchar.h>
#include "map.h"
#include "OpenSSL.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/*http methods*/
enum {
    _CC_HTTP_METHOD_INVALID_ = 0,
    _CC_HTTP_METHOD_GET_ = 1,
    _CC_HTTP_METHOD_POST_,
    _CC_HTTP_METHOD_PUT_,
    _CC_HTTP_METHOD_HEAD_,
    _CC_HTTP_METHOD_DELETE_,
    _CC_HTTP_METHOD_OPTIONS_,
    _CC_HTTP_METHOD_TRACE_,
    _CC_HTTP_METHOD_CONNECT_
};

enum {
    _CC_HTTP_RESPONSE_HEADER_ = 0,
    _CC_HTTP_RESPONSE_BODY_ = 1,
    _CC_HTTP_RESPONSE_SUCCESS_ = 2,
    _CC_HTTP_ERROR_UNIMPLEMENTED_,
    _CC_HTTP_ERROR_NOFOUND_,
    _CC_HTTP_ERROR_BADREQUEST_,
    _CC_HTTP_ERROR_TOOLARGE_,
};

/*
 * Define maximum number of headers that we accept.
 * This should be big enough to handle legitimate cases,
 * but limited to avoid DoS.
 */
#define _CC_HTTP_MAX_HEADERS_   256

typedef struct _cc_http_request_header {
    tchar_t *method;
    tchar_t *script;
    tchar_t *protocol;
    _cc_map_t headers;
    
    uint16_t count;
} _cc_http_request_header_t;

/**/
typedef struct _cc_http_response_header {
    bool_t keep_alive;
    byte_t content_encoding;
    byte_t transfer_encoding;
    
    int32_t status;
    uint64_t download_length;
    uint64_t length;

    tchar_t *protocol;
    tchar_t *description;
    tchar_t *location;

    _cc_map_t headers;
} _cc_http_response_header_t;


typedef struct _cc_http_method {
    int32_t method;
    int32_t method_length;
    tchar_t* value;
} _cc_http_method_t;

typedef bool_t (*_cc_http_header_fn_t)(pvoid_t *arg, tchar_t *line, int length);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_HTTP_C_H_INCLUDED_*/