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

/*
** This source code file implements a small, simple, stand-alone HTTP server.
*/

#include <cc/http/SSL.h>
#include <cc/http/http.h>
#include <sys/stat.h>
#include "../kv.h"

#define RBUF_LEN(rw) (rw)->r.length
#define RBUF(rw) (rw)->r.buf

#define WBUF_LEN(rw) (rw)->w.length
#define WBUF(rw) (rw)->w.buf

_CC_API_PRIVATE(void) _free_and_init_http(_cc_http_t* res) {
    res->request.keep_alive = false;
    res->request.parsing = false;
    res->request.finished = false;
    res->request.cookies = 0;
    res->request.length = 0;
    res->request.content_length = 0;
    res->request.range_start = 0;
    res->request.range_end = 0;
    res->request.server_port = 0;

    bzero(&res->websocket, sizeof(res->websocket));
    res->websocket.status = 0;

    _cc_safe_free(res->request.method);
    _cc_safe_free(res->request.script);
    _cc_safe_free(res->request.protocol);
    _cc_safe_free(res->request.server_host);

    if (res->request.body) {
        _cc_destroy_buf(&res->request.body);
    }

    _cc_destroy_kv(&res->request.headers);
    _cc_destroy_kv(&res->response.headers);

    if (res->response.descriptor == 1) {
        if (res->response.content.rfp) {
            _cc_file_close(res->response.content.rfp);
        }
    } else if (res->response.descriptor == 2) {
        _cc_destroy_buf(&res->response.content.buf);
    }

    res->response.descriptor = 0;
    res->response.content.buf = NULL;
    res->response.length = 0;
}

_CC_API_PRIVATE(void) _destroy_http(_cc_http_t** http_res) {
    _cc_http_t* res = *http_res;
    _cc_assert(res != NULL);
    if (res == NULL) {
        return;
    }

    _cc_safe_free(res->request.method);
    _cc_safe_free(res->request.script);
    _cc_safe_free(res->request.protocol);
    _cc_safe_free(res->request.server_host);

    if (res->request.body) {
        _cc_destroy_buf(&res->request.body);
    }

    _cc_destroy_kv(&res->request.headers);
    _cc_destroy_kv(&res->response.headers);

    if (res->response.descriptor == 1) {
        _cc_file_close(res->response.content.rfp);
    } else if (res->response.descriptor == 2) {
        _cc_destroy_buf(&res->response.content.buf);
    }

    _cc_free(res);
    *http_res = NULL;
}

_CC_API_PRIVATE(_cc_http_t*) _init_http(_cc_http_listener_t* listener) {
    _cc_http_t* res = (_cc_http_t*)_cc_malloc(sizeof(_cc_http_t));
    if (res == NULL) {
        return NULL;
    }

    bzero(res, sizeof(_cc_http_t));
    res->ssl = NULL;
    res->listener = listener;
    res->request.keep_alive = false;
    res->request.parsing = false;
    res->request.finished = false;
    res->request.body = NULL;
    res->request.length = 0;
    res->request.content_length = 0;

    res->websocket.status = 0;
    res->request.server_port = 0;
    res->request.server_host = NULL;
    res->request.method = NULL;
    res->request.script = NULL;
    res->request.protocol = NULL;
    res->request.query = NULL;
    res->request.cookies = 0;
    res->request.range_start = 0;
    res->request.range_end = 0;

    _CC_RB_INIT_ROOT(&res->request.headers);
    _CC_RB_INIT_ROOT(&res->response.headers);

    res->response.descriptor = 0;
    res->response.content.buf = NULL;
    res->response.length = 0;

    return res;
}

bool_t _disconnecting(_cc_event_t* e) {
    _cc_http_t* res = (_cc_http_t*)e->args;
    _cc_event_buffer_t* rw = e->buffer;

    if (res->response.length > 0 && rw->w.length > 0) {
        _cc_shutdown_socket(e->fd, _CC_SHUT_RD_);
        _CC_MODIFY_BIT(_CC_EVENT_DISCONNECT_ | _CC_EVENT_WRITABLE_,
                       _CC_EVENT_READABLE_, e->flags);
        return true;
    }

    _destroy_http((_cc_http_t**)&e->args);
    return false;
}

bool_t _websocket_callback(_cc_event_t* e,
                           int32_t opcode,
                           byte_t* data,
                           int32_t len) {
    if (opcode == WS_TXTDATA) {
        // printf("recv:%s %d\n", data, len);
        _cc_http_websocket_send(e, WS_TXTDATA, data, len);
    }

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _event_sendbuf(_cc_event_t* e, _SSL_t* ssl) {
    _cc_event_wbuf_t* wbuf;
    int32_t off;
    if (e->buffer == NULL) {
        _cc_logger_error(_T("No write cache was created. e->buffer == NULL"));
        return false;
    }

    wbuf = &e->buffer->w;
    if (wbuf->r == wbuf->w) {
        _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        return true;
    }

    _cc_spin_lock(&wbuf->wlock);
#ifdef _CC_OPENSSL_HTTPS_
    if (ssl) {
        off = _SSL_send(http->ssl, wbuf->buf + wbuf->r, wbuf->w - wbuf->r);
    } else
#endif
    {
        off = _cc_send(e->fd, wbuf->buf + wbuf->r, wbuf->w - wbuf->r);
    }
    if (off) {
        wbuf->r += off;

        if (wbuf->r == wbuf->w) {
            _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        }
    }
    _cc_spin_unlock(&wbuf->wlock);
    return (off >= 0);
}

_CC_API_PRIVATE(bool_t) _http_event_callback(_cc_event_cycle_t* cycle,
                                   _cc_event_t* e,
                                   const uint16_t events) {
    if (events & _CC_EVENT_ACCEPT_) {
        _cc_socket_t fd = _CC_INVALID_SOCKET_;
        _cc_event_cycle_t* cycle_xx;
        _cc_http_t* res;
        _cc_http_listener_t* listener = (_cc_http_listener_t*)e->args;
        _cc_sockaddr_t remote_addr = {0};
        _cc_socklen_t remote_addr_len = sizeof(_cc_sockaddr_t);
        fd = _cc_event_accept(cycle, e, &remote_addr, &remote_addr_len);
        if (fd == _CC_INVALID_SOCKET_) {
            _cc_logger_error(_T("thread %d accept fail %s.\n"),
                             _cc_get_thread_id(NULL),
                             _cc_last_error(_cc_last_errno()));
            return true;
        }

        res = _init_http((_cc_http_listener_t*)e->args);
        if (res == NULL) {
            _cc_close_socket(fd);
            return true;
        }

        if (listener->ssl) {
            res->ssl = _SSL_accept("www.bdstoa.com", fd);
        }

        cycle_xx = _cc_httpd_get_cycle();
        if (cycle_xx == NULL) {
            cycle_xx = cycle;
        }

        _cc_set_socket_nonblock(fd, 1);
        if (!cycle_xx->driver.add(
                cycle_xx,
                _CC_EVENT_TIMEOUT_ | _CC_EVENT_READABLE_ | _CC_EVENT_BUFFER_,
                fd, e->timeout, _http_event_callback, res)) {
            _cc_free(res);
            _cc_logger_error(_T("thread %d add socket (%d) event fial.\n"),
                             _cc_get_thread_id(NULL), fd);
            return true;
        }
        /*
        {
            struct sockaddr_in* remote_ip = (struct sockaddr_in*)&remote_addr;
            byte_t *ip_addr = (byte_t *)&remote_ip->sin_addr.s_addr;
            _tprintf(_T("TCP accept [%d,%d,%d,%d] fd:%d\n"), ip_addr[0],
        ip_addr[1], ip_addr[2], ip_addr[3], fd);
        }*/

        return true;
    }

    if (events & _CC_EVENT_READABLE_) {
        bool_t rc;
        _cc_http_t* res = (_cc_http_t*)e->args;
        _cc_http_listener_t* listener = (_cc_http_listener_t*)res->listener;
        _cc_event_buffer_t* rw = e->buffer;

#ifdef _CC_OPENSSL_HTTPS_
        if (res->ssl) {
            rc = _SSL_recv(e, res->ssl);
        } else
#endif
        {
            rc = _cc_event_recv(e);
        }

        if (!rc) {
            _destroy_http((_cc_http_t**)&e->args);
            return false;
        }

        if (rw->r.length <= 0) {
            return true;
        }

        RBUF(rw)[RBUF_LEN(rw) - 1] = 0;

        /* websocket */
        if (res->websocket.status == 2) {
            while (1) {
                int rc = _cc_http_websocket_recv(e, _websocket_callback);
                if (rc == 0) {
                    break;
                }

                if (rc == -1) {
                    _destroy_http((_cc_http_t**)&e->args);
                    return false;
                }
            }
            return true;
        }

        if (res->request.finished == false) {
            if (res->request.parsing == false) {
                if (!_http_parsing_header(res, rw)) {
                    /**/
                    return _disconnecting(e);
                }

                if (res->request.parsing == false) {
                    return true;
                }
            }

            if (res->request.body) {
                if (RBUF_LEN(rw) > 0) {
                    res->request.length += RBUF_LEN(rw);
                    memcpy(res->request.body, RBUF(rw), RBUF_LEN(rw));
                    RBUF_LEN(rw) = 0;
                }

                if (res->request.length < res->request.content_length) {
                    return true;
                } else {
                    res->request.parsing = false;
                    res->request.finished = true;
                }
            }

            if (res->request.finished == false) {
                return true;
            }

            //
            _cc_http_response_header(res);
            /* add writeable event*/
            _CC_SET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        }

        RBUF_LEN(rw) = 0;
        return true;
    }

    if (events & _CC_EVENT_WRITABLE_) {
        int32_t sent;
        _cc_event_wbuf_t* w = &e->buffer->w;
        _cc_http_t* res = (_cc_http_t*)e->args;

        _cc_http_response(res, w);

        if (w->length > 0) {
#ifdef _CC_OPENSSL_HTTPS_
            if (res->ssl) {
                if (_event_sendbuf(e, res->ssl) < 0) {
                    _destroy_http((_cc_http_t**)&e->args);
                    return false;
                }
            } else
#endif
                if (_cc_event_sendbuf(e) < 0) {
                _destroy_http((_cc_http_t**)&e->args);
                return false;
            }

            if (res->response.length > 0) {
                _CC_SET_BIT(_CC_EVENT_WRITABLE_, e->flags);
            } else if (w->length == 0) {
                if (e->flags & _CC_EVENT_DISCONNECT_) {
                    _destroy_http((_cc_http_t**)&e->args);
                    return false;
                }
            }
        }

        /* websocket */
        if (res->websocket.status == 1 && w->length == 0) {
            res->websocket.status = 2;
            return true;
        }

        /*web*/
        if (res->websocket.status == 0 && w->length == 0 &&
            res->response.length == 0) {
            if (res->request.keep_alive) {
                _free_and_init_http(res);
                return true;
            }
            _destroy_http((_cc_http_t**)&e->args);
            return false;
        }
    }

    if (events & _CC_EVENT_TIMEOUT_) {
        _cc_http_t* res = (_cc_http_t*)e->args;

        if (res->websocket.status == 2) {
            _cc_http_websocket_send(e, WS_DISCONNECT, NULL, 0);
        }

        if (e->args) {
            _destroy_http((_cc_http_t**)&e->args);
        }
        return false;
    }

    if (events & _CC_EVENT_DISCONNECT_) {
        if (e->args) {
            _destroy_http((_cc_http_t**)&e->args);
        }
        return false;
    }

    return true;
}

bool_t _cc_http_starting(_cc_http_listener_t* listener, byte_t proto) {
    _cc_sockaddr_t *sockaddr;
    struct sockaddr_in sa;
    struct sockaddr_in6 sa6;
    _cc_event_cycle_t* cycle = _cc_httpd_get_cycle();
    if (cycle == NULL) {
        _cc_logger_error(_T("_cc_httpd_get_cycle fail."));
        return false;
    }
    if (proto) {
        sockaddr = &sa6;
        _cc_inet_ipv6_addr(&sa6, listener->host, listener->port);
    } else {
        sockaddr = &sa;
        _cc_inet_ipv4_addr(&sa, listener->host, listener->port);
    }
    return _cc_tcp_listen(&accept_event, sockaddr, 60000, _http_event_callback, listener) != NULL;
}

#undef RBUF_LEN
#undef WBUF_LEN
#undef RBUF
#undef WBUF
