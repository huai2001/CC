#include <libcc/url_request.h>
#include <libcc/json.h>
#include <libcc/timeout.h>

static _cc_OpenSSL_t *openSSL = nullptr;
static bool_t url_request(const tchar_t *url, pvoid_t args);
static bool_t url_request_connect(_cc_url_request_t *request);

static bool_t url_request_header(_cc_url_request_t *request, _cc_event_t *e) {
    _cc_url_t *u = &request->url;
    _cc_buf_t *buf = &request->buffer;
    _cc_io_buffer_t *io = request->io;

    _cc_buf_cleanup(buf);

    /* send client connection preface */
    const char preface[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
    io->w.off = sizeof(preface) - 1;
    memcpy(io->w.bytes, preface, io->w.off);

    return _cc_io_buffer_flush(e, io) >= 0;
}

static bool_t _handshaking(_cc_event_t *e) {
    request->handshake = _SSL_do_handshake(request->io->ssl);
    if (request->handshake == _CC_SSL_HS_ESTABLISHED_) {
        e->timeout = 10000;
        _CC_SET_BIT(_CC_EVENT_READABLE_, e->flags);
        return url_request_header(request, e);
    } else if (request->handshake == _CC_SSL_HS_ERROR_) {
        return false;
    }
    //wait SSL handshake complete
    return true;
}

static bool_t _url_request_callback(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    _cc_url_request_t *request = (_cc_url_request_t *)e->data;

    if (_CC_ISSET_BIT(_CC_EVENT_CLOSED_, which)) {
        //printf("disconnect\n");
        _cc_logger_warin("_cc_url_request_ _CC_EVENT_CLOSED_ %d",e->ident);
        _cc_free_url_request(request);
        return false;
    } else if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, which)) {    
        if (request->url.scheme.ident == _CC_SCHEME_HTTPS_ && request->handshake != _CC_SSL_HS_ESTABLISHED_) {
            return _handshaking(e);
        }
        if (request->response && request->response->keep_alive && request->state == _CC_HTTP_STATUS_ESTABLISHED_) {
            return url_request_header(request, e);;
        }
        return false;
    } else if (_CC_ISSET_BIT(_CC_EVENT_CONNECT_, which)) {
        _cc_logger_info(_T("url_request connected,%s"), request->url.host);
        if (request->url.scheme.ident == _CC_SCHEME_HTTPS_) {
            return _handshaking(e);
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
        printf("read %.*s bytes\n", request->io->r.off,request->io->r.bytes);
    }
    return true;
}

static bool_t url_request(const tchar_t *url, pvoid_t args) {
    _cc_url_request_t *request = _cc_url_request(url, args);

    if (!url_request_connect(request)) {
        _cc_free_url_request(request);
        return false;
    }
    return true;
}

static bool_t url_request_connect(_cc_url_request_t *request) {
    struct sockaddr_in sa;
    _cc_socket_t fd;
    _cc_event_t *e;
    _cc_async_event_t *async = _cc_get_async_event();
    if (request == nullptr) {
        return false;
    }

    /*Open then socket*/
    fd = _cc_socket(AF_INET, _CC_SOCK_NONBLOCK_ | _CC_SOCK_CLOEXEC_ | SOCK_STREAM, 0);
    if (fd == -1) {
        _cc_logger_error(_T("socket fail:%s."), _cc_last_error(_cc_last_errno()));
        return false;
    }

    /* if we can't terminate nicely, at least allow the socket to be reused*/
    _cc_set_socket_reuseaddr(fd);

    e = _cc_event_alloc(async, _CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_);
    if (e == nullptr) {
        return false;
    }

    e->fd = fd;
    e->callback = _url_request_callback;
    e->timeout = 1000;
    e->data = (uintptr_t)request;

    _cc_reset_url_request(request);
#ifdef _CC_USE_OPENSSL_
    if (request->url.scheme.ident == _CC_SCHEME_HTTPS_) {
        _cc_url_request_ssl(openSSL, request, e);
    }
#endif
    _cc_inet_ipv4_addr(&sa, request->url.host, request->url.port);

    /* required to get parallel v4 + v6 working */
    if (sa.sin_family == AF_INET6) {
        e->flags |= _CC_EVENT_SOCKET_IPV6_;
#if defined(IPV6_V6ONLY)
        _cc_socket_ipv6only(e->fd);
#endif
    }

    if (async->connect(async, e, (_cc_sockaddr_t*)&sa, sizeof(struct sockaddr_in))) {
        return true;
    }

    _cc_free_event(async, e);
    return false;
}

int main(int argc, char *const argv[]) {
    openSSL = _SSL_init(_CC_SSL_DEFAULT_PROTOCOLS_);

    _cc_alloc_async_event(0, nullptr);

    url_request("https://ws.libcc.cn", nullptr);
    while (getchar() != 'q') {
        _cc_sleep(100);
    }
    _cc_free_async_event();
    _SSL_quit(openSSL);
    return 0;
}