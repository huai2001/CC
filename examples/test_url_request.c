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
#include <libcc/widgets/widgets.h>
#include <libcc.h>

static _cc_OpenSSL_t *openSSL = nullptr;
static bool_t url_request(const tchar_t *url, pvoid_t args);
static bool_t url_request_connect(_cc_url_request_t *request);

const tchar_t *_user_agent[6] = {
    _T("Mozilla/5.0 (Linux; Android 13; Pixel 7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Mobile Safari/537.36 Edg/138.0.0.0"),
    _T("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.0 Safari/605.1.15"),
    _T("Mozilla/5.0 (iPhone; CPU iPhone OS 16_6 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.6 Mobile/15E148 Safari/604.1"),
    _T("Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Mobile Safari/537.36 Edg/138.0.0.0"),
    _T("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36 Edg/138.0.0.0"),
    _T("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36")
};
static bool_t url_response_header(_cc_url_request_t *request) {
    //_cc_http_response_header_t *response = request->response;
    return true;
}

static bool_t url_request_header(_cc_url_request_t *request, _cc_event_t *e) {
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
    _cc_buf_puts(buf, _T("Accept-Encoding: gzip\r\n"));
    _cc_buf_appendf(buf, _T("User-Agent: %s\r\nAccept: */*\r\n\r\n"),
                   _user_agent[rand() % _cc_countof(_user_agent)]);
    return _cc_url_request_header(request, e);
}

static bool_t url_request_success(_cc_url_request_t *request) {
    _cc_file_t *fp = (_cc_file_t *)request->args;
    _cc_http_response_header_t *response = request->response;
    switch (response->status) {
    case HTTP_STATUS_OK:
    case HTTP_STATUS_PARTIAL_CONTENTS:
        _cc_buf_cleanup(&request->buffer);
        _cc_logger_debug(_T("url_request success,%s"), request->url.host);
        return true;
    case HTTP_STATUS_MOVED_TEMPORARILY: // 目标跳转
    case HTTP_STATUS_MOVED_PERMANENTLY: // 目标跳转
    case HTTP_STATUS_SEE_OTHER:         // 目标跳转
    {
        const _cc_map_element_t *location = _cc_map_find(&response->headers, _T("Location"));
        if (location) {
            return url_request(location->element.uni_string, nullptr);
        }
        break;
    }
    }
    _cc_file_close(fp);

    _cc_logger_warin(_T("url_request fail,%s"), request->url.host);
    return true;
}

static bool_t url_request_read(_cc_url_request_t *request) {
    _cc_buf_cleanup(&request->buffer);
    return true;
}

static bool_t _url_timeout_callback(_cc_event_cycle_t *timer, _cc_event_t *e, const uint16_t which) {
    _cc_url_request_t *request = (_cc_url_request_t *)e->args;
    if (request == nullptr || !_cc_event_loop_is_running()) {
        return false;
    }
    if (which == _CC_EVENT_DISCONNECT_) {
        return false;
    }
    _cc_logger_warin(_T("url-timout reset connect,%d"),e->ident);
    return (url_request_connect(request))?false:true;
}


static bool_t _url_request_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t which) {
    _cc_url_request_t *request = (_cc_url_request_t *)e->args;

    if (_CC_ISSET_BIT(_CC_EVENT_DISCONNECT_, which)) {
        //printf("disconnect\n");
        printf("_cc_url_request_ _CC_EVENT_DISCONNECT_ %d\n",e->ident);
        if (_cc_event_loop_is_running()) {
            _cc_add_event_timeout(_cc_get_event_cycle(), 10000, _url_timeout_callback, request);
        } else {
            _cc_free_url_request(request);
        }
        return false;
    } else if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, which)) {
        //printf("timeout\n");
        return url_request_header(request, e);
    }else if (_CC_ISSET_BIT(_CC_EVENT_CONNECTED_, which)) {
        if (request->url.scheme.ident != _CC_SCHEME_HTTPS_) {
            return url_request_header(request, e);
        }
    } 

    if (request->handshaking && request->url.scheme.ident == _CC_SCHEME_HTTPS_) {
        if (!_cc_url_request_ssl_handshake(request, e)) {
            return false;
        }

        //printf("ssl handshake\n");
        if (request->handshaking == false) {
            return url_request_header(request, e);
        }

        return true;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_, which)) {
        //printf("send buffer\n");
        if (!_cc_url_request_sendbuf(request, e)) {
            return false;
        }
    }

    if (_CC_ISSET_BIT(_CC_EVENT_READABLE_, which)) {
        _cc_event_buffer_t *rw = e->buffer;
        if (e->buffer == nullptr) {
            return false;
        }

        if (!_cc_url_request_read(request, e)) {
            return false;
        }

        if (rw->r.length > 0) {
            if (request->status == _CC_HTTP_STATUS_HEADER_) {
                //printf("response header\n");
                if (!_cc_url_request_response_header(request, &rw->r)) {
                    return false;
                }
                //Response header completed.
                if (request->status == _CC_HTTP_STATUS_PAYLOAD_) {
                    url_response_header(request);
                }
            }
        }

        if (request->status == _CC_HTTP_STATUS_PAYLOAD_) {
            //printf("response body\n");
            if (!_cc_url_request_response_body(request, &rw->r)) {
                return false;
            }

            url_request_read(request);

            if (request->status == _CC_HTTP_STATUS_FINISHED_) {
                //printf("response successful\n");
                url_request_success(request);
                return request->response->keep_alive;
            }
        }
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
    _cc_event_cycle_t *cycle = _cc_get_event_cycle();
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

    e = _cc_event_alloc(cycle, _CC_EVENT_BUFFER_|_CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_);
    if (e == nullptr) {
        return false;
    }

    e->fd = fd;
    e->callback = _url_request_callback;
    e->timeout = 30000;
    e->args = request;
    _cc_reset_url_request(request);

    if (request->url.scheme.ident == _CC_SCHEME_HTTPS_) {
        request->ssl = _SSL_alloc(openSSL);
        if (request->ssl == nullptr) {
            _cc_free_event(cycle, e);
            return false;
        }
        _SSL_set_host_name(request->ssl, request->url.host, _tcslen(request->url.host));
        _SSL_connect(request->ssl, e->fd);
    } 

    _cc_inet_ipv4_addr(&sa, request->url.host, request->url.port);

    /* required to get parallel v4 + v6 working */
    if (sa.sin_family == AF_INET6) {
        e->descriptor |= _CC_EVENT_DESC_IPV6_;
#if defined(IPV6_V6ONLY)
        _cc_socket_ipv6only(e->fd);
#endif
    }

    if (cycle->connect(cycle, e, (_cc_sockaddr_t*)&sa, sizeof(struct sockaddr_in))) {
        return true;
    }

    _cc_free_event(cycle, e);
    return false;
}

int main(int argc, char *const argv[]) {
    openSSL = _SSL_init(true);

    _cc_event_loop(0, nullptr);

    url_request("https://api.trongrid.io/wallet/getnowblock", nullptr);
    // url_request("https://api.trongrid.io/wallet/getnowblock", nullptr);
    // url_request("https://api.trongrid.io/wallet/getnowblock", nullptr);
    // url_request("https://api.trongrid.io/wallet/getnowblock", nullptr);
    // url_request("https://api.trongrid.io/wallet/getnowblock", nullptr);
    // url_request("https://api.trongrid.io/wallet/getnowblock", nullptr);
    // url_request("https://api.trongrid.io/wallet/getnowblock", nullptr);

    while (getchar() != 'q') {
        _cc_sleep(100);
    }
    _cc_quit_event_loop();
    return 0;
}