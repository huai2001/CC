#include <libcc/http_request.h>
#include <libcc/gzip.h>
#include <libcc/event.h>

/**/
_CC_API_PUBLIC(bool_t) _cc_http_response_body(_cc_http_request_t *request, byte_t *source, size_t length);
_CC_API_PUBLIC(bool_t) _cc_http_response_chunked(_cc_http_request_t *, _cc_io_buffer_t *io);

/**/
_CC_API_PUBLIC(void) _cc_reset_http_request(_cc_http_request_t *request) {
    _cc_assert(request != NULL);
    request->state = _CC_HTTP_STATE_HEADER_;
    if (request->response) {
        _cc_http_free_response_header(&request->response);
    }
#ifdef _CC_USE_OPENSSL_
    if (request->io && request->io->ssl) {
        _SSL_free(request->io->ssl);
        request->io->ssl = NULL;
    }
#endif
}

/**/
_CC_API_PUBLIC(void) _cc_free_http_request(_cc_http_request_t *request) {
    _cc_assert(request != NULL);

    if (request->io) {
        _cc_free_io_buffer(request->io);
        request->io = NULL;
    }

    if (request->gzip) {
        _gzip_free(request->gzip);
        request->gzip = NULL;
    }

    if (request->response) {
        _cc_http_free_response_header(&request->response);
    }

    _cc_free_buf(&request->buffer);
    _cc_free_url(&request->url);
    _cc_free(request);
}

/**/
_CC_API_PUBLIC(bool_t) _cc_http_request_header(_cc_http_request_t *request, _cc_event_t *e) {
    _cc_assert(request != NULL);
    _cc_assert(request->io != NULL);
    _cc_assert(request->buffer.length > 0);

    request->state = _CC_HTTP_STATE_HEADER_;
    if (request->response) {
        _cc_http_free_response_header(&request->response);
    }

    if (request->buffer.length <= 0) {
        return false;
    }
#ifdef _CC_UNICODE_
    _cc_buf_utf16_to_utf8(&request->buffer, 0);
#endif
    return _cc_io_buffer_send(e, request->io, request->buffer.bytes, (int32_t)request->buffer.length);
}

_CC_API_PRIVATE(int64_t) get_content_length(_cc_rbtree_t *headers) {
    const _cc_http_header_t *data = _cc_http_header_find(headers, _T("Content-Length"));
    return data ? _ttoi(data->value) : 0;
}

_CC_API_PRIVATE(int) is_chunked_transfer(_cc_rbtree_t *headers) {
    const _cc_http_header_t *data = _cc_http_header_find(headers, _T("Transfer-Encoding"));
    #if 0
    if (data) {
        if (_tcsicmp(data->value, _T("chunked") == 0) {
            return _CC_URL_TRANSFER_ENCODING_CHUNKED_;
        } else if (_tcsicmp(data->value, _T("identity") == 0) {
            return _CC_URL_TRANSFER_ENCODING_IDENTITY_;
        }
        return _CC_URL_TRANSFER_ENCODING_IDENTITY_;
    }
    #endif
    return (data && _tcsicmp(data->value, _T("chunked")) == 0) ? _CC_URL_TRANSFER_ENCODING_CHUNKED_ : _CC_URL_TRANSFER_ENCODING_IDENTITY_;
}

_CC_API_PRIVATE(int) get_content_encoding(_cc_rbtree_t *headers) {
    const _cc_http_header_t *data = _cc_http_header_find(headers, _T("Content-Encoding"));
    return (data && _tcsicmp(data->value, _T("gzip")) == 0) ? _CC_URL_CONTENT_ENCODING_GZIP_ : _CC_URL_CONTENT_ENCODING_PLAINTEXT_;
}

_CC_API_PRIVATE(bool_t) is_keep_alive(_cc_rbtree_t *headers) {
    const _cc_http_header_t *data = _cc_http_header_find(headers, _T("Connection"));
    return (data && _tcsicmp(data->value, _T("keep-alive")) == 0);
}

/**/
_CC_API_PUBLIC(bool_t) _cc_http_request_response_header(_cc_http_request_t *request) {
    _cc_http_response_header_t *response;
    _cc_io_buffer_t *io;
    _cc_assert(request != NULL);
    _cc_assert(request->io != NULL);
    _cc_assert(request->state == _CC_HTTP_STATE_HEADER_);

    io = request->io;
    request->state = _cc_http_header_parser((_cc_http_header_fn_t)_cc_http_alloc_response_header, (pvoid_t *)&request->response, io->r.bytes, (int32_t *)&io->r.off);
    
    /**/
    if (request->state != _CC_HTTP_STATE_PAYLOAD_) {
        return request->state == _CC_HTTP_STATE_HEADER_;
    } else if (request->response == NULL) {
        return false;
    }

    response = request->response;
    response->content_encoding = get_content_encoding(&response->headers);
    response->transfer_encoding = is_chunked_transfer(&response->headers);
    response->keep_alive = is_keep_alive(&response->headers);
    if (response->transfer_encoding == _CC_URL_TRANSFER_ENCODING_IDENTITY_) {
        response->content_length = get_content_length(&response->headers);
    } else {
        response->content_length = 0;
    }

    _cc_buf_cleanup(&request->buffer);

    if (response->content_encoding == _CC_URL_CONTENT_ENCODING_GZIP_) {
        if (request->gzip) {
            _gzip_reset(request->gzip);
        } else {
            request->gzip = _gzip_alloc(_GZIP_INF_);
            return request->gzip != NULL;
        }
    }
    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_http_request_response_body(_cc_http_request_t *request) {
    _cc_http_response_header_t *response;
    _cc_io_buffer_t *io;
    _cc_assert(request != NULL);
    _cc_assert(request->io != NULL);
    _cc_assert(request->state == _CC_HTTP_STATE_PAYLOAD_);

    /**/
    response = request->response;
    io = request->io;

    if (io->r.off > 0) {
        if (response->transfer_encoding == _CC_URL_TRANSFER_ENCODING_CHUNKED_) {
            if (!_cc_http_response_chunked(request, io)) {
                return false;
            }
        } else if (response->content_length > 0) {
            if (!_cc_http_response_body(request, io->r.bytes, io->r.off)) {
                return false;
            }
            /**/
            response->download_length += io->r.off;
            io->r.off = 0;
            if (response->download_length >= response->content_length) {
                request->state = _CC_HTTP_STATE_ESTABLISHED_;
            }
        } else {
            request->state = _CC_HTTP_STATE_ESTABLISHED_;
        }
        return true;
    } else if (response->transfer_encoding == _CC_URL_TRANSFER_ENCODING_IDENTITY_ && response->content_length == 0) {
        request->state = _CC_HTTP_STATE_ESTABLISHED_;
    }
    return true;
}

/**/
_CC_API_PUBLIC(_cc_http_request_t*) _cc_http_request(const tchar_t *url, pvoid_t args) {
    _cc_http_request_t *request;
    _cc_assert(url != NULL);
    if (url == NULL) {
        return NULL;
    }

    request = (_cc_http_request_t *)_cc_malloc(sizeof(_cc_http_request_t));
    memset(request, 0, sizeof(_cc_http_request_t));

    if (!_cc_parse_url(&request->url, url)) {
        _cc_free(request);
        return NULL;
    }

    request->io = _cc_alloc_io_buffer(_CC_8K_BUFFER_SIZE_);

    request->state = _CC_HTTP_STATE_HEADER_;
    request->response = NULL;
    request->args = args;
    request->gzip = NULL;

    _cc_alloc_buf(&request->buffer, _CC_IO_BUFFER_SIZE_);

    return request;
}

_CC_API_PUBLIC(bool_t) _cc_http_request_ssl(_cc_OpenSSL_t *openSSL, _cc_http_request_t *request, _cc_event_t *e) {
#ifdef _CC_USE_OPENSSL_
    request->io->ssl = _SSL_connect(openSSL, e->fd);
    if (request->io->ssl == NULL) {
        return false;
    }
    _SSL_set_host_name(request->io->ssl, request->url.host, _cc_sds_length(request->url.host));
#else
    _CC_UNUSED(openSSL);
    _CC_UNUSED(request);
    _CC_UNUSED(e);
#endif
    return true;
}