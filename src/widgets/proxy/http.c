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
#include <stdio.h>
#include <locale.h>
#include <cc/widgets/http.h>
#include "../common/http.request.parser.c"

_CC_API_PRIVATE(bool_t) _SendResp(_cc_event_t *e, const char_t *str) {
    return _cc_send(e->fd, (byte_t*)str, strlen(str));
}
/*
_CC_API_PRIVATE(bool_t) _SendAuthorization407(_cc_event_t *e) {
    _cc_strA_t authorization = _cc_string("HTTP/1.0 407 Proxy Authentication Required\r\nProxy-Authenticate: Basic realm=\".Qiu\"\r\n\r\n");
    return _cc_event_send(e, (byte_t*)authorization.data, authorization.length);
}
_CC_API_PRIVATE(bool_t) _SendAuthorization401(_cc_event_t *e) {
    _cc_strA_t authorization = _cc_string("HTTP/1.1 401 Authentication Required\r\nWWWW-Authenticate: Basic realm=\".Qiu\"\r\n\r\n");
    return _cc_event_send(e, (byte_t*)authorization.data, authorization.length);
}*/

_CC_API_PRIVATE(bool_t) _proxy_event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t events) {
    if (events & _CC_EVENT_ACCEPT_) {
        _cc_socket_t fd;
        _cc_event_t *event;
        _cc_proxy_http_t *proxy;
        _cc_sockaddr_t remote_addr = {0};
        _cc_socklen_t remote_addr_len = sizeof(_cc_sockaddr_t);
        _cc_event_cycle_t *cycle_new = _cc_get_event_cycle();

        fd = _cc_event_accept(cycle, e, &remote_addr, &remote_addr_len);
        if (fd == _CC_INVALID_SOCKET_) {
            _cc_logger_debug(_T("accept fail %s."), _cc_last_error(_cc_last_errno()));
            return true;
        }

        event = _cc_alloc_event(cycle_new, _CC_EVENT_TIMEOUT_ | _CC_EVENT_READABLE_ | _CC_EVENT_BUFFER_);
        if (event == NULL) {
            _cc_close_socket(fd);
            return true;
        }
        _cc_set_socket_nonblock(fd, 1);

        proxy = _cc_malloc(sizeof(_cc_proxy_http_t));
        proxy->status = _CC_HTTP_RESPONSE_HEADER_;
        proxy->request = NULL;
        proxy->server_host = NULL;
        proxy->server_port = 0;

        event->fd = fd;
        event->args = proxy;
        event->timeout = e->timeout;
        event->callback = e->callback;
        if (cycle_new->driver.attach(cycle_new, event) == false) {
            _cc_logger_debug(_T("add socket (%d) event fial."), fd);
            _cc_free_event(cycle_new, event);
            return true;
        }
        
        {
            struct sockaddr_in* remote_ip = (struct sockaddr_in*)&remote_addr;
            byte_t *ip_addr = (byte_t *)&remote_ip->sin_addr.s_addr;
            _cc_logger_debug(_T("TCP accept [%d,%d,%d,%d] fd:%d"), ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], fd);
        }

        return true;
    }
    
    if (events & _CC_EVENT_DISCONNECT_) {
        _cc_logger_debug(_T("%d client disconnect."), e->fd);
        return false;
    }
    
    if (events & _CC_EVENT_READABLE_) {
        _cc_event_buffer_t *rw = e->buffer;
        _cc_proxy_http_t *proxy = (_cc_proxy_http_t*)e->args;
        if (!_cc_event_recv(e)) {
            _cc_logger_debug(_T("%d client close."), e->fd);
            return false;
        }

        /**/
        if (proxy->status == _CC_HTTP_RESPONSE_HEADER_) {
            //const tchar_t *v;
            proxy->status = _cc_http_header_parser((_cc_http_header_fn_t)_alloc_request_header, (pvoid_t*)&proxy->request, &rw->r);
            /**/
            if (proxy->status == _CC_HTTP_RESPONSE_HEADER_) {
                return true;
            } else if (proxy->status != _CC_HTTP_RESPONSE_BODY_) {
                return false;
            }

            printf("%s %s %s\n", proxy->request->method, proxy->request->script, proxy->request->protocol);

            _cc_rbtree_for_each(it, &proxy->request->headers,{
                _cc_dict_node_t *dict = _cc_upcast(it, _cc_dict_node_t, node);
                printf("%s:%s\n", dict->name, dict->value);
            });

            printf("-----------------------------------\n");

            proxy->status = _CC_HTTP_RESPONSE_SUCCESS_;
            _SendResp(e, "HTTP/1.1 200 Connection Established\r\n\r\n");
            /*
            v = _cc_dict_find(&proxy->request->headers,"Proxy-Authorization");
            if (!v) {
                v = _cc_dict_find(&proxy->request->headers,"Authorization");
                if (!v) {
                    _SendAuthorization407(e);
                }
            } else {
                printf("Authorization:%s\n", v);
                _SendResp(e, "HTTP/1.1 200 Connection Established\r\n\r\n");
            }*/

        } else if (proxy->status == _CC_HTTP_RESPONSE_SUCCESS_) {

        }

        return true;
    }
    
    if (events & _CC_EVENT_WRITABLE_) {
        int32_t off = _cc_event_sendbuf(e);
        if (off < 0) {
            return false;
        }

        _cc_sleep(1);

        if ((e->flags & _CC_EVENT_WRITABLE_) == 0 && e->flags & _CC_EVENT_DISCONNECT_) {
            return false;
        }
    }
    
    if (events & _CC_EVENT_TIMEOUT_) {
        _cc_logger_debug(_T("%d client timeout."), e->fd);
        return false;
    }

    if (events == _CC_EVENT_DELETED_) {
        _cc_proxy_http_t *proxy = (_cc_proxy_http_t*)e->args;
        if (proxy) {
            e->args = NULL;
            if (proxy->request) {
                _free_request_header(&proxy->request);
            }
            _cc_free(proxy);
        }
        return false;
    }
    
    return true;
}

void _cc_http_starting(uint16_t port) {
    struct sockaddr_in sa;
    _cc_event_cycle_t *cycle = _cc_get_event_cycle();
    _cc_event_t *e = _cc_alloc_event(cycle, _CC_EVENT_ACCEPT_);
    if (e == NULL) {
        return;
    }
    e->timeout = 300000;
    e->callback = _proxy_event_callback;

    _cc_inet_ipv4_addr(&sa, NULL, port);
    if (!_cc_tcp_listen(cycle, e, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
        _cc_free_event(cycle, e);
    }
    _cc_logger_debug("listen:%d", port);
}