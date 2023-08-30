/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
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
#include <cc/alloc.h>
#include <cc/string.h>
#include <cc/url.h>
#include <cc/widgets/url_request.h>
#include "../../common/http.response.parser.c"

/**/
bool_t _cc_url_read_response_body(_cc_url_request_t *request, byte_t *source, size_t length);
bool_t _cc_url_read_response_chunked(_cc_url_request_t *, _cc_event_rbuf_t *);

/**/
bool_t _cc_free_url_request(_cc_url_request_t *request) {
    if (request == NULL) {
        return false;
    }

#ifdef _CC_OPENSSL_HTTPS_
    if (request->ssl) {
        _SSL_free(request->ssl);
        request->ssl = NULL;
    }
#endif

    if (request->response) {
        _free_response_header(&request->response);
    }

    _cc_buf_free(&request->buffer);
    _cc_free_url(&request->url);
    _cc_free(request);

    return true;
}

/**/
bool_t _cc_url_request_header(_cc_url_request_t *request, _cc_event_t *e) {
    request->status = _CC_HTTP_RESPONSE_HEADER_;
    if (request->response) {
        _free_response_header(&request->response);
    }

#ifdef _CC_UNICODE_
    _cc_buf_utf16_to_utf8(&request->buffer, 0);
#endif

    if (_cc_copy_event_wbuf(&e->buffer->w, request->buffer.bytes, request->buffer.length)) {
        _cc_event_change_flag(_cc_get_event_cycle(), e, _CC_EVENT_WRITABLE_);
        return true;
    }
    return false;
}

/**/
bool_t _cc_url_request_ssl_handshake(_cc_url_request_t *request, _cc_event_t *e) {
#ifdef _CC_OPENSSL_HTTPS_
    switch (_SSL_do_handshake(request->ssl)) {
    case _CC_SSL_HS_ESTABLISHED_:
        request->handshaking = false;
        break;
    case _CC_SSL_HS_WANT_READ_:
        _CC_SET_BIT(_CC_EVENT_READABLE_, e->flags);
        break;
    case _CC_SSL_HS_WANT_WRITE_:
        _CC_SET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        break;
    default:
        return false;
    }
#else
    _CC_UNUSED(request);
    _CC_UNUSED(e);
#endif
    return true;
}

static long get_content_length(_cc_dict_t *headers) {
    const tchar_t *data;
    long content_length = 0;

    data = _cc_dict_find(headers, _T("Content-Length"));

    if (data) {
        content_length = _ttoi(data);
    }

    return content_length;
}

static int is_chunked_transfer(_cc_dict_t *headers) {
    const tchar_t *data;
    data = _cc_dict_find(headers, _T("Transfer-Encoding"));
    return (data && !_tcsicmp(data, _T("chunked"))) ? _CC_URL_TRANSFER_ENCODING_CHUNKED_ :
                                                      _CC_URL_TRANSFER_ENCODING_UNKNOWN_;
}

static int get_content_encoding(_cc_dict_t *headers) {
    const tchar_t *data;
    data = _cc_dict_find(headers, _T("Content-Encoding"));
    return (data && !_tcsicmp(data, _T("gzip"))) ? _CC_URL_CONTENT_ENCODING_GZIP_ : _CC_URL_CONTENT_ENCODING_PLAINTEXT_;
}

static bool_t is_keep_alive(_cc_dict_t *headers) {
    const tchar_t *data;
    data = _cc_dict_find(headers, _T("Connection"));
    return data ? !_tcsicmp(data, _T("keep-alive")) : false;
}

/**/
bool_t _cc_url_request_response_header(_cc_url_request_t *request, _cc_event_rbuf_t *r) {
    _cc_assert(r != NULL);
    if (request->status == _CC_HTTP_RESPONSE_HEADER_) {
        _cc_http_response_header_t *response;
        request->status =
            _cc_http_header_parser((_cc_http_header_fn_t)_alloc_response_header, (pvoid_t *)&request->response, r);
        /**/
        switch (request->status) {
        case _CC_HTTP_RESPONSE_HEADER_:
            return true;
        case _CC_HTTP_RESPONSE_BODY_:
            _cc_buf_cleanup(&request->buffer);
            break;
        default:
            return false;
        }
        response = request->response;
        response->length = get_content_length(&response->headers);
        response->content_encoding = get_content_encoding(&response->headers);
        response->transfer_encoding = is_chunked_transfer(&response->headers);
        response->keep_alive = is_keep_alive(&response->headers);
    }
    return true;
}

/**/
bool_t _cc_url_request_response_body(_cc_url_request_t *request, _cc_event_rbuf_t *r) {
    _cc_http_response_header_t *response = request->response;
    _cc_assert(r != NULL);
    /**/
    if (r->length > 0) {
        if (response->transfer_encoding == _CC_URL_TRANSFER_ENCODING_CHUNKED_) {
            if (!_cc_url_read_response_chunked(request, r)) {
                return false;
            }
        } else {
            if (!_cc_url_read_response_body(request, r->buf, r->length)) {
                return false;
            }
            /**/
            response->download_length += r->length;
            r->length = 0;
            if (response->length > 0 && response->download_length >= response->length) {
                request->status = _CC_HTTP_RESPONSE_SUCCESS_;
            }
        }
    } else if (response->length == 0 && response->transfer_encoding != _CC_URL_TRANSFER_ENCODING_CHUNKED_) {
        request->status = _CC_HTTP_RESPONSE_SUCCESS_;
    }

    return true;
}

/**/
bool_t _cc_url_request_read(_cc_url_request_t *request, _cc_event_t *e) {
#ifdef _CC_OPENSSL_HTTPS_
    if (request->url.scheme.ident == _CC_SCHEME_HTTPS_ && request->ssl) {
        return _SSL_event_read(request->ssl, e);
    }
#endif
    return _cc_event_recv(e);
}

/**/
bool_t _cc_url_request_sendbuf(_cc_url_request_t *request, _cc_event_t *e) {
#ifdef _CC_OPENSSL_HTTPS_
    if (request->url.scheme.ident == _CC_SCHEME_HTTPS_ && request->ssl) {
        return _SSL_sendbuf(request->ssl, e) >= 0;
    }
#endif
    return _cc_event_sendbuf(e) >= 0;
}

/**/
_cc_url_request_t *_cc_url_request(const tchar_t *url, pvoid_t args) {
    _cc_url_request_t *request;
    _cc_assert(url != NULL);
    if (url == NULL) {
        return NULL;
    }

    request = (_cc_url_request_t *)_cc_malloc(sizeof(_cc_url_request_t));
    bzero(request, sizeof(_cc_url_request_t));

    if (!_cc_parse_url(&request->url, url)) {
        _cc_free(request);
        return NULL;
    }

    request->status = _CC_HTTP_RESPONSE_HEADER_;
    request->response = NULL;
    request->args = args;
    request->ssl = NULL;
    request->timeout = 0;
    request->handshaking = true;
    _cc_buf_alloc(&request->buffer, _CC_IO_BUFFER_SIZE_);

    return request;
}
