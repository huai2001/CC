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
#include <cc/widgets/widgets.h>
#include <libcc.h>

static char quit = 0;
static bool_t url_request(const tchar_t *url, pvoid_t args);

static tchar_t *_user_agent[] = {
    _T("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12) AppleWebKit/602.1.50 (KHTML, like Gecko) Version/10.0 ")
    _T("Safari/602.1.50"),
    _T("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_6) AppleWebKit/602.1.50 (KHTML, like Gecko) Version/10.0 ")
    _T("Safari/602.1.50"),
    _T("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.103 ")
    _T("Safari/537.36"),
    _T("Mozilla/5.0 (Macintosh; Intel Mac OS X 10.12; rv:46.0) Gecko/20100101 Firefox/46.0"),
    _T("Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:46.0) Gecko/20100101 Firefox/46.0"),
    _T("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.103 ")
    _T("Safari/537.36"),
    _T("Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 6.0)"),
    _T("Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.0; Trident/4.0)"),
    _T("Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)"),
    _T("Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.2; Win64; x64; Trident/6.0)"),
    _T("Mozilla/5.0 (Windows NT 6.3; Win64, x64; Trident/7.0; rv:11.0) like Gecko"),
    _T("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.135 ")
    _T("Safari/537.36 Edge/13.10586"),
    _T("Mozilla/5.0 (iPod; CPU iPhone OS 10_0 like Mac OS X) AppleWebKit/602.1.38 (KHTML, like Gecko) Version/10.0 ")
    _T("Mobile/14A300 Safari/602.1")};

static bool_t url_response_header(_cc_url_request_t *request) {
    //_cc_http_response_header_t *response = request->response;

    tchar_t file_path[_CC_MAX_PATH_];
    // tchar_t *p;

    if (*request->url.path == '/' && *(request->url.path + 1) == 0) {
        _cc_get_module_directory(_T("index.html"), file_path, _cc_countof(file_path));
    } else {
        _cc_get_module_directory(request->url.path, file_path, _cc_countof(file_path));
        _cc_realpath(file_path);
        _cc_mkdir(file_path);
    }
    request->args = _cc_open_file(file_path, _T("wb"));
    if (request->args == NULL) {
        return false;
    }
    return true;
}

static bool_t url_request_header(_cc_url_request_t *request, _cc_event_t *e) {
    _cc_url_t *u = &request->url;
    _cc_buf_t *buf = &request->buffer;
    _cc_buf_cleanup(buf);

    _cc_buf_puttsf(buf, _T("GET %s HTTP/1.1\r\n"), u->request);
    if ((u->scheme.ident == _CC_SCHEME_HTTPS_ && u->port == _CC_PORT_HTTPS_) ||
        (u->scheme.ident == _CC_SCHEME_HTTP_ && u->port == _CC_PORT_HTTP_)) {
        /* if(HTTPS on port 443) OR (HTTP on port 80) then don't include the port number in the host string */
        if (u->ipv6) {
            _cc_buf_puttsf(buf, _T("Host: [%s]\r\n"), u->host);
        } else {
            _cc_buf_puttsf(buf, _T("Host: %s\r\n"), u->host);
        }
    } else {
        _cc_buf_puttsf(buf, _T("Host: %s:%d\r\n"), u->host, u->port);
    }
    //
    _cc_buf_puttsf(buf, _T("User-Agent: %s\r\nAccept: */*\r\n\r\n"),
                   _user_agent[rand() % _cc_countof(_user_agent)]);
    return _cc_url_request_header(request, e);
}

static bool_t url_request_success(_cc_url_request_t *request) {
    _cc_file_t *fp = (_cc_file_t *)request->args;
    _cc_http_response_header_t *response = request->response;
    switch (response->status) {
    case HTTP_STATUS_OK:
    case HTTP_STATUS_PARTIAL_CONTENTS:
        if (fp) {
            _cc_file_write(fp, request->buffer.bytes, sizeof(byte_t), request->buffer.length);
        }
        _cc_file_close(fp);
        _cc_buf_cleanup(&request->buffer);
        printf("\nPlease enter the url:\n");
        return true;
    case HTTP_STATUS_MOVED_TEMPORARILY: // 目标跳转
    case HTTP_STATUS_MOVED_PERMANENTLY: // 目标跳转
    case HTTP_STATUS_SEE_OTHER:         // 目标跳转
    {
        const tchar_t *location = _cc_dict_find(&response->headers, _T("Location"));
        if (location) {
            return url_request(location, request->args);
        }
        break;
    }
    }
    _cc_file_close(fp);
    return true;
}


static bool_t url_request_read(_cc_url_request_t *request) {
    _cc_file_t *fp = (_cc_file_t *)request->args;
    if (fp) {
        _cc_file_write(fp, request->buffer.bytes, sizeof(byte_t), request->buffer.length);
    }
    _cc_buf_cleanup(&request->buffer);
    return true;
}

static bool_t _url_request_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t events) {
    _cc_url_request_t *request = (_cc_url_request_t *)e->args;
    if (request == NULL) {
        return false;
    }

    if (events == _CC_EVENT_DELETED_) {
        _cc_free_url_request(request);
        printf("free\n");
        return false;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_DISCONNECT_, events)) {
        printf("disconnect\n");
        return false;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, events)) {
        printf("timeout\n");
        return false;
    }

    if (request->handshaking && request->url.scheme.ident == _CC_SCHEME_HTTPS_) {
        if (!_cc_url_request_ssl_handshake(request, e)) {
            return false;
        }
        printf("ssl handshake\n");
        if (request->handshaking == false) {
            printf("send http header\n");
            return url_request_header(request, e);
        }
        return true;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_, events)) {
        printf("send buffer\n");
        if (!_cc_url_request_sendbuf(request, e)) {
            return false;
        }
    }

    if (_CC_ISSET_BIT(_CC_EVENT_READABLE_, events)) {
        _cc_event_buffer_t *rw = e->buffer;
        if (e->buffer == NULL) {
            return false;
        }

        if (!_cc_url_request_read(request, e)) {
            return false;
        }

        if (rw->r.length > 0) {
            if (request->status == _CC_HTTP_RESPONSE_HEADER_) {
                printf("response header\n");
                if (!_cc_url_request_response_header(request, &rw->r)) {
                    return false;
                }
                if (request->status == _CC_HTTP_RESPONSE_BODY_) {
                    url_response_header(request);
                }
            }
        }

        if (request->status == _CC_HTTP_RESPONSE_BODY_) {
            printf("response body\n");
            if (!_cc_url_request_response_body(request, &rw->r)) {
                return false;
            }

            url_request_read(request);

            if (request->status == _CC_HTTP_RESPONSE_SUCCESS_) {
                printf("response successful\n");
                url_request_success(request);
                return request->response->keep_alive;
            }
        }
        return true;
    }

    return true;
}


static bool_t url_request(const tchar_t *url, pvoid_t args) {
    struct sockaddr_in sa;
    _cc_event_t *e;
    _cc_event_cycle_t *cycle = _cc_get_event_cycle();
    _cc_url_request_t *request = _cc_url_request(url, args);
    if (request == NULL) {
        return false;
    }
    _cc_inet_ipv4_addr(&sa, request->url.host, request->url.port);

    e = _cc_alloc_event(cycle, _CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_);
    if (e == NULL) {
        return false;
    }
    e->callback = _url_request_callback;
    e->timeout = 30000;
    e->args = request;

    if (!_cc_tcp_connect(cycle, e, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
        _cc_free_event(cycle, e);
        return false;
    }
    return true;
}

int getLine(char *result) {
    int point = 0;
    int word;

    while (1) {
        word = getchar();
        if (word != '\n') {
            *result = word;
            result++;
            point++;
        } else {
            *result = 0;
            result = result - point;
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *const argv[]) {
    char_t URL[1024];
    char_t *p = NULL;
    if (argc < 2) {
        printf("Please enter the url:");
    } else {
        p = argv[1];
    }
    _cc_event_loop(1, NULL);

    p = "https://baidu.com";
    if (p) {
        url_request(p, NULL);
    }
    while (quit != 'q') {
        getLine(URL);
        if (_strnicmp(URL, "exit", 4) == 0) {
            quit = 'q';
            break;
        } else if (_strnicmp(URL, "r", 1) == 0) {
            url_request(p, NULL);
        } else {
            url_request(URL, NULL);
        }
    }
    _cc_quit_event_loop();
    return 0;
}
