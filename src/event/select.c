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
#include <cc/alloc.h>
#include <cc/logger.h>
#include "event.c.h"

struct _fd_list {
    fd_set wfds;
    fd_set rfds;

    int32_t rc;
    int32_t wc;
#ifndef _CC_WINDOWS
    _cc_socket_t max_fd;
#endif
};

struct _cc_event_cycle_priv {
    int32_t nfds;
    _cc_event_t *list[FD_SETSIZE];
};

/**/
static bool_t _select_event_attach(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_event_cycle_priv_t *fset;
    _cc_assert(cycle != NULL);
    fset = cycle->priv;

    if (e->fd && _CC_EVENT_IS_SOCKET(e->flags) && fset->nfds >= FD_SETSIZE) {
        _cc_logger_error(_T("maximum number of descriptors, supported by select() is %d"), FD_SETSIZE);
        return false;
    }

    e->descriptor |= _CC_EVENT_DESC_POLL_SELECT_;
    return _cc_event_wait_reset(cycle, e);
}

/**/
static bool_t _select_event_connect(_cc_event_cycle_t *cycle, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len) {
    if (__cc_stdlib_socket_connect(e->fd, sa, sa_len)) {
        return _select_event_attach(cycle, e);
    }
    return false;
}

/**/
static _cc_socket_t _select_event_accept(_cc_event_cycle_t *cycle, _cc_event_t *e, _cc_sockaddr_t *sa,
                                                    _cc_socklen_t *sa_len) {
    return _cc_socket_accept(e->fd, sa, sa_len);
}

/**/
static void _select_event_cleanup(_cc_event_cycle_t *cycle, _cc_event_t *e) {
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
static void _reset_event(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_event_cycle_priv_t *fset = cycle->priv;

    if (_CC_ISSET_BIT(_CC_EVENT_DISCONNECT_, e->flags) && _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags) == 0) {
        /*delete*/
        _cc_cleanup_event(cycle, e);
        return;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_PENDING_, e->flags)) {
        _cc_list_iterator_swap(&cycle->pending, &e->lnk);
        return;
    }

    if (_cc_list_iterator_empty(&e->lnk)) {
        fset->list[fset->nfds++] = e;
    }

    _cc_reset_event_timeout(cycle, e);
}

/**/
static bool_t _init_fd_event(_cc_event_t *e, struct _fd_list *fds) {
    if (_CC_ISSET_BIT(_CC_EVENT_PENDING_, e->flags)) {
        return false;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_DESC_SOCKET_, e->descriptor) == 0) {
        return false;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_ | _CC_EVENT_CONNECT_, e->flags)) {
        FD_SET(e->fd, &fds->wfds);
        fds->wc++;
        e->marks |= _CC_EVENT_WRITABLE_;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags)) {
        FD_SET(e->fd, &fds->rfds);
        fds->rc++;
        e->marks |= _CC_EVENT_READABLE_;
    }
#ifndef _CC_WINDOWS
    if (fds->max_fd == _CC_INVALID_SOCKET_ || fds->max_fd < e->fd) {
        fds->max_fd = e->fd;
    }
#endif
    return true;
}
/**/
static bool_t _select_event_wait(_cc_event_cycle_t *cycle, uint32_t timeout) {
    struct timeval tv;
    int32_t i;
    int32_t ready;
    struct _fd_list fds;

    uint16_t events, revents;

    _cc_event_t *e;
    _cc_event_cycle_priv_t *priv = cycle->priv;

    /**/
    _cc_reset_pending_event(cycle, _reset_event);

    if (cycle->diff > 0) {
        timeout -= cycle->diff;
    }

    if (priv->nfds == 0) {
        _cc_sleep(timeout);
        goto WHEEL_TIMER;
    }

    fds.wc = fds.rc = 0;
    fds.max_fd = _CC_INVALID_SOCKET_;

    FD_ZERO(&fds.rfds);
    FD_ZERO(&fds.wfds);

    for (i = 0; i < priv->nfds; i++) {
        _init_fd_event(priv->list[i], &fds);
    }

    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    /**/
#ifndef __CC_WINDOWS__
    ready = select((int)fds.max_fd + 1, &fds.rfds, &fds.wfds, NULL, &tv);
#else
    ready = select(0, &fds.rfds, &fds.wfds, NULL, &tv);
#endif
    if (_cc_likely(ready)) {
        for (i = 0; i < priv->nfds && ready; i++) {
            e = priv->list[i];
            events = 0;
            revents = _CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags);
            if (revents && FD_ISSET(e->fd, &fds.rfds)) {
                events |= revents;
            }

            revents = _CC_ISSET_BIT(_CC_EVENT_CONNECT_ | _CC_EVENT_WRITABLE_, e->flags);
            if (revents && FD_ISSET(e->fd, &fds.wfds)) {
                events |= _cc_valid_connected(e, revents);
            }

            if (events) {
                ready--;
                _cc_event_callback(cycle, e, events);
            }
        }
    } else {
        if (_cc_unlikely(ready == -1)) {
            int32_t lerrno = _cc_last_errno();
            if (lerrno != _CC_EINTR_) {
                _cc_logger_error(_T("error:%d, %s"), lerrno, _cc_last_error(lerrno));
            }
        }
    }

WHEEL_TIMER:
    _cc_update_event_timeout(cycle, timeout);
    return true;
}

/**/
static bool_t _select_event_quit(_cc_event_cycle_t *cycle) {
    _cc_assert(cycle != NULL);
    if (cycle == NULL) {
        return false;
    }

    _cc_safe_free(cycle->priv);

    return _cc_event_cycle_quit(cycle);
}

/**/
static bool_t _select_event_init(_cc_event_cycle_t *cycle) {
    _cc_event_cycle_priv_t *priv;
    if (!_cc_event_cycle_init(cycle)) {
        return false;
    }
    priv = (_cc_event_cycle_priv_t *)_cc_malloc(sizeof(_cc_event_cycle_priv_t));
    bzero(priv, sizeof(_cc_event_cycle_priv_t));

    cycle->priv = priv;
    return true;
}

/**/
bool_t _cc_init_event_select(_cc_event_cycle_t *cycle) {
#define ASET(x) cycle->driver.x = _select_event_##x
#define XSET(x) cycle->driver.x = _cc_event_##x

    if (!_select_event_init(cycle)) {
        return false;
    }

    ASET(attach);
    ASET(connect);
    XSET(disconnect);
    ASET(accept);
    ASET(wait);
    ASET(quit);

    cycle->cleanup = _select_event_cleanup;

#undef ASET
#undef XSET

    return true;
}
