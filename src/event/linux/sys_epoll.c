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
#include "../event.c.h"
#include <cc/alloc.h>
#include <cc/event/timeout.h>
#include <cc/logger.h>
#include <sys/epoll.h>

#define _CC_EPOLL_EVENTS_ _CC_MAX_CHANGE_EVENTS_

struct _cc_event_cycle_priv {
    int fd;
};

/**/
static bool_t _epoll_event_update(int efd, _cc_event_t *e, bool_t rm) {
    uint16_t marks = 0;
    int op = EPOLL_CTL_DEL;
    struct epoll_event ev;

    bzero(&ev, sizeof(struct epoll_event));
    ev.data.fd = e->fd;
    ev.data.ptr = e;
    ev.events = 0;

    if (!rm) {
        /*Setting the readable event flag*/
        if (_CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags)) {
            ev.events |= EPOLLIN;
        }

        /*Setting the writable event flag*/
        if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_ | _CC_EVENT_CONNECT_, e->flags)) {
            ev.events |= EPOLLOUT;
        }

        if (ev.events) {
            marks = _CC_EVENT_IS_SOCKET(e->flags);
            op = e->marks ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        }
    }

    if (epoll_ctl(efd, op, e->fd, &ev) == -1) {
        int err = _cc_last_errno();
        switch (op) {
        case EPOLL_CTL_MOD: {
            if (err == ENOENT) {
                /* If a MOD operation fails with ENOENT, the
                 * fd was probably closed and re-opened.  We
                 * should retry the operation as an ADD.
                 */
                if (epoll_ctl(efd, EPOLL_CTL_ADD, e->fd, &ev) == -1) {
                    _cc_logger_error(_T("Epoll MOD(%d) on %d retried as MOD; that failed too"), (int)ev.events, e->fd);
                    return false;
                }
            }
        } break;
        case EPOLL_CTL_ADD: {
            if (err == EEXIST) {
                /* If an ADD operation fails with EEXIST,
                 * either the operation was redundant (as with a
                 * precautionary add), or we ran into a fun
                 * kernel bug where using dup*() to duplicate the
                 * same file into the same fd gives you the same epitem
                 * rather than a fresh one.  For the second case,
                 * we must retry with MOD. */
                if (epoll_ctl(efd, EPOLL_CTL_MOD, e->fd, &ev) == -1) {
                    _cc_logger_error(_T("Epoll ADD(%d) on %d retried as ADD; that failed too"), (int)ev.events, e->fd);
                    return false;
                }
            }
        } break;
        case EPOLL_CTL_DEL: {
            if (err != ENOENT && err != EBADF && err != EPERM) {
                _cc_logger_error(_T("Epoll DEL(%d) on %d retried as DEL; that failed too"), (int)ev.events, e->fd);
                return false;
            }
        } break;
        }
    }

    e->marks = marks;
    return true;
}

/**/
bool_t _epoll_event_attach(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_assert(cycle != NULL && e != NULL);
    e->descriptor |= _CC_EVENT_DESC_POLL_EPOLL_;
    return _cc_event_wait_reset(cycle, e);
}

/**/
bool_t _epoll_event_connect(_cc_event_cycle_t *cycle, _cc_event_t *e, const _cc_sockaddr_t *sa,
                            const _cc_socklen_t sa_len) {
    if (__cc_stdlib_socket_connect(e->fd, sa, sa_len)) {
        return _epoll_event_attach(cycle, e);
    }
    return false;
}

/**/
static _cc_socket_t _epoll_event_accept(_cc_event_cycle_t *cycle, _cc_event_t *e, _cc_sockaddr_t *sa,
                                        _cc_socklen_t *sa_len) {
    return _cc_socket_accept(e->fd, sa, sa_len);
}

/**/
static void _reset_event(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    if (_CC_ISSET_BIT(_CC_EVENT_DISCONNECT_, e->flags) && _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags) == 0) {
        if (_CC_EVENT_IS_SOCKET(e->marks)) {
            _epoll_event_update(cycle->priv->fd, e, true);
        }
        _cc_cleanup_event(cycle, e);
        return;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_PENDING_, e->flags)) {
        if (_CC_EVENT_IS_SOCKET(e->marks)) {
            _epoll_event_update(cycle->priv->fd, e, true);
        }

        _cc_list_iterator_swap(&cycle->pending, &e->lnk);
        return;
    }

    /*update event*/
    if (_CC_ISSET_BIT(_CC_EVENT_DESC_SOCKET_, e->descriptor) &&
        _CC_EVENT_IS_SOCKET(e->flags) != _CC_EVENT_IS_SOCKET(e->marks)) {
        _epoll_event_update(cycle->priv->fd, e, false);
    }

    _cc_reset_event_timeout(cycle, e);
}

/**/
static bool_t _epoll_event_wait(_cc_event_cycle_t *cycle, uint32_t timeout) {
    int32_t rc, i;

    struct epoll_event actives[_CC_EPOLL_EVENTS_];
    _cc_event_cycle_priv_t *priv = cycle->priv;

    bzero(&actives, sizeof(struct epoll_event) * _CC_EPOLL_EVENTS_);

    /**/
    _cc_reset_pending_event(cycle, _reset_event);

    if (cycle->diff > 0) {
        timeout -= cycle->diff;
    }

    rc = epoll_wait(priv->fd, actives, _CC_EPOLL_EVENTS_, timeout);
    if (rc < 0) {
        int32_t lerrno = _cc_last_errno();
        if (lerrno != _CC_EINTR_) {
            _cc_logger_error(_T("error:%d, %s"), lerrno, _cc_last_error(lerrno));
        }
        goto EPOLL_END;
    }

    for (i = 0; i < rc; ++i) {
        uint16_t events = _CC_EVENT_UNKNOWN_;
        int32_t epoll_events = (int32_t)actives[i].events;
        _cc_event_t *e = (_cc_event_t *)actives[i].data.ptr;

        if (_CC_ISSET_BIT(EPOLLERR, epoll_events) == 0) {
            if (_CC_ISSET_BIT(EPOLLIN, epoll_events) != 0) {
                events = _CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags);
            }

            if (_CC_ISSET_BIT(EPOLLOUT, epoll_events) != 0) {
                events = _cc_valid_connected(e, _CC_ISSET_BIT(_CC_EVENT_CONNECT_ | _CC_EVENT_WRITABLE_, e->flags));
            }
        } else {
            events = _CC_EVENT_DISCONNECT_;
        }

        if (events == _CC_EVENT_UNKNOWN_) {
            continue;
        }

        /*callback*/
        _cc_event_callback(cycle, e, events);
    }

EPOLL_END:
    _cc_update_event_timeout(cycle, timeout);
    return true;
}

/**/
static bool_t _epoll_event_quit(_cc_event_cycle_t *cycle) {
    _cc_assert(cycle != NULL);
    if (cycle == NULL) {
        return false;
    }

    if (cycle->priv) {
        if (cycle->priv->fd != -1) {
            _cc_close_socket(cycle->priv->fd);
        }

        _cc_free(cycle->priv);
        cycle->priv = NULL;
    }

    return _cc_event_cycle_quit(cycle);
}

/**/
static bool_t _epoll_event_init(_cc_event_cycle_t *cycle) {
    _cc_socket_t fd = -1;
    if (!_cc_event_cycle_init(cycle)) {
        return false;
    }

#ifdef EPOLL_CLOEXEC
    fd = epoll_create1(EPOLL_CLOEXEC);
#endif
    if (fd == -1) {
        /* Initialize the kernel queue using the old interface.  (The
           size field is ignored   since 2.6.8.) */
        if ((fd = epoll_create(1024)) == -1) {
            if (errno != ENOSYS) {
                _cc_logger_error(_T("cannot create epoll!"));
            }
            return false;
        }
        _cc_set_socket_closeonexec(fd);
    }

    cycle->priv = (_cc_event_cycle_priv_t *)_cc_malloc(sizeof(_cc_event_cycle_priv_t));
    cycle->priv->fd = fd;

    return true;
}

/**/
bool_t _cc_init_event_epoll(_cc_event_cycle_t *cycle) {
#define ASET(x) cycle->driver.x = _epoll_event_##x
#define XSET(x) cycle->driver.x = _cc_event_##x

    if (!_epoll_event_init(cycle)) {
        return false;
    }

    ASET(attach);
    ASET(connect);
    XSET(disconnect);
    ASET(accept);
    ASET(wait);
    ASET(quit);

#undef ASET
#undef XSET

    return true;
}