#include <libcc/alloc.h>
#include "event.c.h"

/**< The backlog that listen() should use. */
#define _NET_LISTEN_BACKLOG_ SOMAXCONN
/**/
_CC_API_PUBLIC(bool_t) _cc_tcp_listen(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sockaddr, _cc_socklen_t socklen) {
    /*Open then socket*/
    e->fd = _cc_socket(AF_INET, _CC_SOCK_NONBLOCK_ | _CC_SOCK_CLOEXEC_ | SOCK_STREAM, 0);
    if (e->fd == -1) {
        return false;
    }
    /* if we can't terminate nicely, at least allow the socket to be reused*/
    _cc_set_socket_reuseaddr(e->fd);
    /* required to get parallel v4 + v6 working */
    if (sockaddr->sa_family == AF_INET6) {
        e->flags |= _CC_EVENT_SOCKET_IPV6_;
#if defined(IPV6_V6ONLY)
        _cc_socket_ipv6only(e->fd);
#endif
    }
    /* Bind the socket for listening */
    if (bind(e->fd, sockaddr, socklen) < 0) {
        _cc_logger_error("Couldn't bind to local port: %s", _cc_last_error(_cc_last_errno()));
        return false;
    }

    if (listen(e->fd, _NET_LISTEN_BACKLOG_) < 0) {
        _cc_logger_error("Couldn't listen to local port: %s", _cc_last_error(_cc_last_errno()));
        return false;
    }

    return async->attach(async, e);
}

_CC_API_PUBLIC(bool_t) _cc_tcp_connect(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sockaddr, _cc_socklen_t socklen) {
    /*Open then socket*/
    e->fd = _cc_socket(AF_INET, _CC_SOCK_NONBLOCK_ | _CC_SOCK_CLOEXEC_ | SOCK_STREAM, 0);
    if (e->fd == -1) {
        _cc_logger_error("socket fail:%s.", _cc_last_error(_cc_last_errno()));
        return false;
    }
    /* if we can't terminate nicely, at least allow the socket to be reused*/
    _cc_set_socket_reuseaddr(e->fd);
    /* required to get parallel v4 + v6 working */
    if (sockaddr->sa_family == AF_INET6) {
        e->flags |= _CC_EVENT_SOCKET_IPV6_;
#if defined(IPV6_V6ONLY)
        _cc_socket_ipv6only(e->fd);
#endif
    }

    return async->connect(async, e, sockaddr, socklen);
}
