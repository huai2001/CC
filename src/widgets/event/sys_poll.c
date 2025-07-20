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
#include "event.c.h"
#include <libcc/alloc.h>
#include <libcc/logger.h>
#include <sys/poll.h>

#define _CC_POLL_EVENTS_ 1024

struct _cc_event_cycle_priv {
    _cc_event_t *list[_CC_POLL_EVENTS_];
    _cc_event_t *fds[_CC_POLL_EVENTS_];
    nfds_t nfds;
};

/**/
_CC_API_PRIVATE(bool_t) _poll_event_reset(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    return _reset_event(cycle, e);
}

/**/
_CC_API_PRIVATE(bool_t) _poll_event_attach(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_event_cycle_priv_t *fset;
    _cc_assert(cycle != nullptr);
    fset = cycle->priv;

    if (e->fd && _CC_ISSET_BIT(_CC_EVENT_DESC_SOCKET_,e->descriptor) && fset->nfds >= _CC_POLL_EVENTS_) {
        _cc_logger_error(_T("The maximum number of descriptors supported by the poll() is %d"), _CC_POLL_EVENTS_);
        return false;
    }

    e->descriptor |= _CC_EVENT_DESC_POLL_POLLFD_;
    return _reset_event(cycle, e);
}

/**/
_CC_API_PRIVATE(bool_t) _poll_event_connect(_cc_event_cycle_t *cycle, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len) {
    if (__cc_stdlib_socket_connect(e->fd, sa, sa_len)) {
        return _poll_event_attach(cycle, e);
    }
    return false;
}

/**/
_CC_API_PRIVATE(bool_t) _poll_event_disconnect(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    return _disconnect_event(cycle, e);
}

/**/
_CC_API_PRIVATE(_cc_socket_t) _poll_event_accept(_cc_event_cycle_t *cycle, _cc_event_t *e, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len) {
    return _cc_socket_accept(e->fd, sa, sa_len);
}

/**/
_CC_API_PRIVATE(void) _poll_event_cleanup(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_event_cycle_priv_t *fset = cycle->priv;
    int32_t i;

    for (i = 0; i < fset->nfds; i++) {
        if (fset->list[i] == e) {
            fset->list[i] = fset->list[fset->nfds - 1];
            fset->nfds--;
            break;
        }
    }
}
/**/
_CC_API_PRIVATE(bool_t) _init_fd_event(_cc_event_t *e, struct pollfd *p) {
    if (_CC_ISSET_BIT(_CC_EVENT_PENDING_, e->flags)) {
        return false;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_DESC_SOCKET_, e->descriptor) == 0) {
        return false;
    }

    p->fd = e->fd;
    p->events = POLLERR;
    p->revents = 0;

    if (_CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags)) {
        p->events |= POLLIN;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_ | _CC_EVENT_CONNECT_, e->flags)) {
        p->events |= POLLOUT;
    }

    if (p->events != POLLERR) {
        return true;
    }
    return false;
}

/**/
_CC_API_PRIVATE(void) _reset(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_event_cycle_priv_t *priv = cycle->priv;
    if (_CC_ISSET_BIT(_CC_EVENT_DISCONNECT_, e->flags) && _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags) == 0) {
        /*delete*/
        _cleanup_event(cycle, e);
        return;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_PENDING_, e->flags)) {
        _cc_list_iterator_swap(&cycle->pending, &e->lnk);
        return;
    }

    if (_cc_list_iterator_empty(&e->lnk)) {
        priv->list[priv->nfds++] = e;
    }

    _reset_event_timeout(cycle, e);
}

/**/
_CC_API_PRIVATE(bool_t) _poll_event_wait(_cc_event_cycle_t *cycle, uint32_t timeout) {
    _cc_event_t *e;
    int32_t i;
    int32_t nfds;
    int32_t ready;
    uint16_t which = _CC_EVENT_UNKNOWN_, what;
    struct pollfd fds[_CC_POLL_EVENTS_];

    _cc_event_cycle_priv_t *priv = cycle->priv;

    /**/
    _reset_event_pending(cycle, _reset);

    if (cycle->diff > 0) {
        timeout -= cycle->diff;
    }

    /**/
    if (priv->nfds <= 0) {
        _cc_sleep(timeout);
        return true;
    }

    for (i = 0, nfds = 0; i < priv->nfds; i++) {
        e = priv->list[i];
        if (_init_fd_event(e, &fds[nfds])) {
            priv->fds[nfds++] = e;
        }
    }

    /**/
    ready = poll(fds, nfds, timeout);
    if (_cc_likely(ready)) {
        for (i = 0; i < priv->nfds && ready; i++) {
            e = priv->fds[i];
            which = 0;
            what = fds[i].events;
            if (what & POLLNVAL) {
                _cc_logger_warin(_T("poll() error fd:%d ev:%04Xd rev:%04Xd"), e->fd, e->flags, what);
            }

            if (what & ~(POLLIN | POLLOUT | POLLERR | POLLHUP | POLLNVAL)) {
                _cc_logger_warin(_T("strange poll() events fd:%d ev:%04Xd rev:%04Xd"), e->fd, e->flags, what);
            }

            if (what & POLLIN) {
                which |= _CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags);
            }

            if (what & POLLOUT) {
                which |= _valid_connected(e, _CC_ISSET_BIT(_CC_EVENT_CONNECT_ | _CC_EVENT_WRITABLE_, e->flags));
            }

            if (which) {
                ready--;
                _event_callback(cycle, e, which);
            }
        }
    } else {
        if (_cc_unlikely(ready < 0)) {
            int32_t lerrno = _cc_last_errno();
            if (lerrno != _CC_EINTR_) {
                _cc_logger_error(_T("error:%d, %s"), lerrno, _cc_last_error(lerrno));
            }
        }
    }

    _update_event_timeout(cycle, timeout);
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _poll_event_quit(_cc_event_cycle_t *cycle) {
    _cc_assert(cycle != nullptr);

    _cc_safe_free(cycle->priv);

    return _event_cycle_quit(cycle);
}

/**/
_CC_API_PRIVATE(bool_t) _poll_event_init(_cc_event_cycle_t *cycle) {
    _cc_event_cycle_priv_t *priv;
    if (!_event_cycle_init(cycle)) {
        return false;
    }
    priv = (_cc_event_cycle_priv_t *)_cc_calloc(1, sizeof(_cc_event_cycle_priv_t));
    priv->nfds = 0;

    cycle->priv = priv;
    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_init_event_poll(_cc_event_cycle_t *cycle) {
    if (!_poll_event_init(cycle)) {
        return false;
    }
    cycle->reset = _poll_event_reset;
    cycle->attach = _poll_event_attach;
    cycle->connect = _poll_event_connect;
    cycle->disconnect = _poll_event_disconnect;
    cycle->accept = _poll_event_accept;
    cycle->wait = _poll_event_wait;
    cycle->quit = _poll_event_quit;
    cycle->reset = _poll_event_reset;
    cycle->cleanup = _poll_event_cleanup;
    return true;
}
