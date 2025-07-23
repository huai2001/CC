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
#include <libcc/alloc.h>
#include "event.c.h"

/**< The backlog that listen() should use. */
#define _NET_LISTEN_BACKLOG_ SOMAXCONN
/**/
_CC_API_PUBLIC(bool_t) _cc_tcp_listen(_cc_event_cycle_t *cycle, _cc_event_t *e, _cc_sockaddr_t *sockaddr, _cc_socklen_t socklen) {
    /*Open then socket*/
    e->fd = _cc_socket(AF_INET, _CC_SOCK_NONBLOCK_ | _CC_SOCK_CLOEXEC_ | SOCK_STREAM, 0);
    if (e->fd == -1) {
        return false;
    }
    /* if we can't terminate nicely, at least allow the socket to be reused*/
    _cc_set_socket_reuseaddr(e->fd);
    /* required to get parallel v4 + v6 working */
    if (sockaddr->addr.sa_family == AF_INET6) {
        e->descriptor |= _CC_EVENT_DESC_IPV6_;
#if defined(IPV6_V6ONLY)
        _cc_socket_ipv6only(e->fd);
#endif
    } else {
        e->flags = _CC_EVENT_ACCEPT_;
    }
    /* Bind the socket for listening */
    if (bind(e->fd, &sockaddr->addr, socklen) < 0) {
        _cc_logger_error(_T("Couldn't bind to local port: %s"), _cc_last_error(_cc_last_errno()));
        return false;
    }

    if (listen(e->fd, _NET_LISTEN_BACKLOG_) < 0) {
        _cc_logger_error(_T("Couldn't listen to local port: %s"), _cc_last_error(_cc_last_errno()));
        return false;
    }

    return cycle->attach(cycle, e);
}

_CC_API_PUBLIC(bool_t) _cc_tcp_connect(_cc_event_cycle_t *cycle, _cc_event_t *e, _cc_sockaddr_t *sockaddr, _cc_socklen_t socklen) {
    /*Open then socket*/
    e->fd = _cc_socket(AF_INET, _CC_SOCK_NONBLOCK_ | _CC_SOCK_CLOEXEC_ | SOCK_STREAM, 0);
    if (e->fd == -1) {
        _cc_logger_error(_T("socket fail:%s."), _cc_last_error(_cc_last_errno()));
        return false;
    }
    /* if we can't terminate nicely, at least allow the socket to be reused*/
    _cc_set_socket_reuseaddr(e->fd);
    /* required to get parallel v4 + v6 working */
    if (sockaddr->addr.sa_family == AF_INET6) {
        e->descriptor |= _CC_EVENT_DESC_IPV6_;
#if defined(IPV6_V6ONLY)
        _cc_socket_ipv6only(e->fd);
#endif
    }

    return cycle->connect(cycle, e, sockaddr, socklen);
}
