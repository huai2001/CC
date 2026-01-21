#include <libcc/http_request.h>
#include <libcc/json.h>
#include <libcc/timeout.h>

static _cc_OpenSSL_t *openSSL = NULL;
static bool_t url_request(const tchar_t *url, pvoid_t args);
static bool_t url_request_connect(_cc_http_request_t *request);

const tchar_t *_user_agent[6] = {
    _T("Mozilla/5.0 (Linux; Android 13; Pixel 7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Mobile Safari/537.36 Edg/138.0.0.0"),
    _T("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.0 Safari/605.1.15"),
    _T("Mozilla/5.0 (iPhone; CPU iPhone OS 16_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.6 Mobile/15E148 Safari/604.1"),
    _T("Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Mobile Safari/537.36 Edg/138.0.0.0"),
    _T("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36 Edg/138.0.0.0"),
    _T("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36")
};
static bool_t url_response_header(_cc_http_request_t *request) {
    //_cc_http_response_header_t *response = request->response;
    return true;
}

static bool_t url_request_header(_cc_http_request_t *request, _cc_event_t *e) {
    _cc_url_t *u = &request->url;
    _cc_buf_t *buf = &request->buffer;
    _cc_buf_cleanup(buf);

    _cc_buf_appendf(buf, _T("GET %s HTTP/1.1\r\n"), u->request);
    if ((u->scheme.ident == _CC_SCHEME_HTTPS_ && u->port == _CC_PORT_HTTPS_) ||
        (u->scheme.ident == _CC_SCHEME_HTTP_ && u->port == _CC_PORT_HTTP_)) {
        /* if(HTTPS on port 443) OR (HTTP on port 80) then don't include the port number in the host string */
        if (u->ipv6) {
            _cc_buf_appendf(buf, _T("Host: [%s]\r\n"), u->host);
        } else {
            _cc_buf_appendf(buf, _T("Host: %s\r\n"), u->host);
        }
    } else {
        _cc_buf_appendf(buf, _T("Host: %s:%d\r\n"), u->host, u->port);
    }
    //
    _cc_buf_puts(buf, _T("Connection: Keep-Alive\r\nAccept-Encoding: gzip\r\n"));
    _cc_buf_appendf(buf, _T("User-Agent: %s\r\nAccept: */*\r\n\r\n"),
                   _user_agent[rand() % _cc_countof(_user_agent)]);
    return _cc_http_request_header(request, e);
}

static bool_t url_request_success(_cc_http_request_t *request) {
    _cc_http_response_header_t *response = request->response;
    _cc_rbtree_iterator_t *node;

    _cc_rbtree_for_next(node, &response->headers) {
        _cc_http_header_t *header = _cc_upcast(node, _cc_http_header_t, lnk);
        _cc_logger_debug("header:%s=%s", header->keyword, header->value);
    }

    switch (response->status) {
    case _CC_HTTP_STATUS_OK_:{
        _cc_json_t *root = _cc_json_parse((tchar_t*)request->buffer.bytes, request->buffer.length);
        if (root) {
            _cc_logger_warin("url_request success,%s", request->url.host);
            _cc_free_json(root);
        } else {
            _cc_logger_alert("json parse fail,%s\n\n", _cc_json_error());
        }
        _cc_buf_cleanup(&request->buffer);
    }
        return true;
    case _CC_HTTP_STATUS_MOVED_PERMANENTLY_: // 目标跳转
    case _CC_HTTP_STATUS_FOUND_: // 目标跳转
    case _CC_HTTP_STATUS_SEE_OTHER_:         // 目标跳转
    {
        const _cc_http_header_t *location = _cc_http_header_find(&response->headers, _T("Location"));
        if (location) {
            return url_request(location->value, NULL);
        }
        break;
    }
    }

    _cc_logger_warin("url_request fail,%s\n\n", request->url.host);
    return true;
}

static bool_t url_request_read(_cc_http_request_t *request) {
    //_cc_buf_cleanup(&request->buffer);
    return true;
}

static bool_t _url_timeout_callback(_cc_async_event_t *timer, _cc_event_t *e, const uint32_t which) {
    _cc_http_request_t *request = (_cc_http_request_t *)e->data;
    if (request == NULL || !_cc_async_event_is_running()) {
        return false;
    }
    if (which == _CC_EVENT_CLOSED_) {
        return false;
    }
    _cc_logger_warin("url-timout reset connect,%d",e->ident);
    return (url_request_connect(request))?false:true;
}

static bool_t _handshaking(_cc_event_t *e, _cc_http_request_t *request) {
    if (_SSL_do_handshake(request->io->ssl) == _CC_SSL_HS_ERROR_) {
        return false;
    }
    if (request->io->ssl->is_handshaked) {
        e->timeout = 60000;
        _CC_SET_BIT(_CC_EVENT_READABLE_, e->flags);
        return url_request_header(request, e);
    }
    //wait SSL handshake complete
    return true;
}

static bool_t _http_request_callback(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    _cc_http_request_t *request = (_cc_http_request_t *)e->data;

    if (_CC_ISSET_BIT(_CC_EVENT_CLOSED_, which)) {
        //printf("disconnect\n");
        _cc_logger_warin("_cc_http_request_ _CC_EVENT_CLOSED_ %d",e->ident);
        if (_cc_async_event_is_running()) {
            _cc_add_event_timeout(_cc_get_async_event(), 10000, _url_timeout_callback, (uintptr_t)request);
        } else {
            _cc_free_http_request(request);
        }
        return false;
    } else if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, which)) {    
        if (request->url.scheme.ident == _CC_SCHEME_HTTPS_) {
            if (request->io && request->io->ssl && !request->io->ssl->is_handshaked) {
                return _handshaking(e, request);
            }
        }
        if (request->response && request->response->keep_alive) {
            return url_request_header(request, e);
        }
        return false;
    } else if (_CC_ISSET_BIT(_CC_EVENT_CONNECT_, which)) {
        _cc_logger_info("url_request connected,%s", request->url.host);
        if (request->url.scheme.ident == _CC_SCHEME_HTTPS_) {
            return _handshaking(e, request);
        }
        _CC_SET_BIT(_CC_EVENT_READABLE_, e->flags);
        return url_request_header(request, e);
    }

    if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_, which)) {
        //printf("send buffer\n");
        return _cc_io_buffer_flush(e,request->io) >= 0;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_READABLE_, which)) {
        int32_t off = _cc_io_buffer_read(e, request->io);
        if (off < 0) {
            return false;
        } else if (off == 0) {
            return true;
        }
        if (request->state == _CC_HTTP_STATE_HEADER_) {
            //printf("response header\n");
            if (!_cc_http_request_response_header(request)) {
                return false;
            }
            //Response header completed.
            if (request->state == _CC_HTTP_STATE_PAYLOAD_) {
                url_response_header(request);
            }
        }

        if (request->state == _CC_HTTP_STATE_PAYLOAD_) {
            //printf("response body\n");
            if (!_cc_http_request_response_body(request)) {
                return false;
            }

            url_request_read(request);

            if (request->state == _CC_HTTP_STATE_ESTABLISHED_) {
                //printf("response successful\n");
                url_request_success(request);
                return request->response->keep_alive;
            }
        }
    }
    return true;
}

static bool_t url_request_connect(_cc_http_request_t *request) {
    _cc_socket_t fd;
    _cc_event_t *e;
    _cc_sockaddr_t *sockaddr_ptr;
    _cc_union_sockaddr_t sockaddr;
    socklen_t sockaddr_len;
    _cc_async_event_t *async = _cc_get_async_event();
    if (request == NULL) {
        return false;
    }

    /*Open then socket*/
    fd = _cc_socket(AF_INET, _CC_SOCK_NONBLOCK_ | _CC_SOCK_CLOEXEC_ | SOCK_STREAM, 0);
    if (fd == -1) {
        _cc_logger_error("socket fail:%s.", _cc_last_error(_cc_last_errno()));
        return false;
    }

    /* if we can't terminate nicely, at least allow the socket to be reused*/
    _cc_set_socket_reuseaddr(fd);

    e = _cc_alloc_event(async, _CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_);
    if (e == NULL) {
        _cc_close_socket(fd);
        return false;
    }

    e->fd = fd;
    e->callback = _http_request_callback;
    e->timeout = 1000;
    e->data = (uintptr_t)request;

    _cc_reset_http_request(request);

    if (request->url.scheme.ident == _CC_SCHEME_HTTPS_) {
        _cc_http_request_ssl(openSSL, request, e);
    }

    /* Determine address family */
    if (request->url.ipv6) {
        _cc_inet_ipv6_addr(&sockaddr.addr_in6, request->url.host, request->url.port);
        sockaddr_ptr = (_cc_sockaddr_t*)&sockaddr.addr_in6;
        sockaddr_len = sizeof(struct sockaddr_in6);
        /* required to get parallel v4 + v6 working */
        e->flags |= _CC_EVENT_SOCKET_IPV6_;
#if defined(IPV6_V6ONLY)
        _cc_socket_ipv6only(e->fd);
#endif
    } else {
        _cc_inet_ipv4_addr(&sockaddr.addr_in, request->url.host, request->url.port);
        sockaddr_ptr = (_cc_sockaddr_t*)&sockaddr.addr_in;
        sockaddr_len = sizeof(struct sockaddr_in);
    }

    if (async->connect(async, e, sockaddr_ptr, sockaddr_len)) {
        return true;
    }

    _cc_free_event(async, e);
    return false;
}

static bool_t url_request(const tchar_t *url, pvoid_t args) {
    _cc_http_request_t *request = _cc_http_request(url, args);

    if (!url_request_connect(request)) {
        _cc_free_http_request(request);
        return false;
    }
    return true;
}

int main(int argc, char *const argv[]) {
    openSSL = _SSL_init(_CC_SSL_DEFAULT_PROTOCOLS_);

    _cc_alloc_async_event(0, NULL);

    url_request("https://api.trongrid.io/wallet/getnowblock", NULL);
    //url_request("https://api.trongrid.io/wallet/getnowblock", NULL);
    //url_request("https://api.trongrid.io/wallet/getnowblock", NULL);
    //url_request("https://api.trongrid.io/wallet/getnowblock", NULL);
    //url_request("https://api.trongrid.io/wallet/getnowblock", NULL);
    //url_request("https://api.trongrid.io/wallet/getnowblock", NULL);

    while (getchar() != 'q') {
        _cc_sleep(100);
    }
    _cc_free_async_event();
    _SSL_quit(openSSL);
    return 0;
}