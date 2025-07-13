/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#include <libcc/alloc.h>
#include <libcc/logger.h>
#include <sys/event.h>

#define _CC_KQUEUE_EVENTS_ _CC_MAX_CHANGE_EVENTS_

struct _cc_event_cycle_priv {
    int fd;
    int nchanges;
    struct kevent changelist[_CC_KQUEUE_EVENTS_];
};

_CC_API_PRIVATE(bool_t) _update_kevent(_cc_event_cycle_priv_t *priv) {
    /**/
    int r = kevent(priv->fd, priv->changelist, priv->nchanges, nullptr, 0, nullptr);
    if (_cc_unlikely(r)) {
        _cc_logger_error(_T("kevent error %d. events:%d, error:%s"), r, priv->nchanges, _cc_last_error(r));
        return false;
    }
    priv->nchanges = 0;
    return true;
}

#define EV_UPDATE_KEVENT(_FILTER, _FLAGS)                                                                               \
    do {                                                                                                                \
        EV_SET(&priv->changelist[priv->nchanges++], e->fd, _FILTER, _FLAGS, 0, 0, e);                                   \
        if (_cc_unlikely(priv->nchanges == _cc_countof(priv->changelist))) {                                            \
            _update_kevent(priv);                                                                                       \                                                                                                           \
        }                                                                                                               \
    } while (0)

/**/
_CC_API_PRIVATE(bool_t) _kqueue_event_update(_cc_event_cycle_priv_t *priv, _cc_event_t *e, bool_t rm) {
    uint16_t addevents = e->flags & ~e->marks;
    uint16_t delevents = ~e->flags & e->marks;

    if (rm) {
        if (_CC_ISSET_BIT(_CC_EVENT_READABLE_ | _CC_EVENT_ACCEPT_, e->marks)) {
            EV_UPDATE_KEVENT(EVFILT_READ, EV_DELETE);
        }

        if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_ | _CC_EVENT_CONNECT_, e->marks)) {
            EV_UPDATE_KEVENT(EVFILT_WRITE, EV_DELETE);
        }

        e->marks = _CC_EVENT_UNKNOWN_;
        return true;
    }

    /*Setting the readable event flag*/
    if (_CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, addevents)) {
        EV_UPDATE_KEVENT(EVFILT_READ, EV_ADD | EV_ENABLE);
    } else if (_CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, delevents)) {
        EV_UPDATE_KEVENT(EVFILT_READ, EV_DELETE);
    }

    /*Setting the writable event flag*/
    if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_ | _CC_EVENT_CONNECT_, addevents)) {
        EV_UPDATE_KEVENT(EVFILT_WRITE, EV_ADD | EV_ENABLE);
    } else if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_ | _CC_EVENT_CONNECT_, delevents)) {
        EV_UPDATE_KEVENT(EVFILT_WRITE, EV_ADD | EV_DELETE);
    }

    _CC_MODIFY_BIT(addevents, delevents, e->marks);
    return true;
}

#undef EV_UPDATE_KEVENT
/**/
_CC_API_PRIVATE(bool_t) _kqueue_event_attach(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_assert(cycle != nullptr && e != nullptr);
    e->descriptor |= _CC_EVENT_DESC_POLL_KQUEUE_;
    return _reset_event(cycle, e);
}

/**/
_CC_API_PRIVATE(bool_t) _kqueue_event_reset(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    return _reset_event(cycle, e);
}

/**/
_CC_API_PRIVATE(bool_t) _kqueue_event_disconnect(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    return _disconnect_event(cycle, e);
}

/**/
_CC_API_PRIVATE(bool_t) _kqueue_event_connect(_cc_event_cycle_t *cycle, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len) {
    if (__cc_stdlib_socket_connect(e->fd, sa, sa_len)) {
        return _kqueue_event_attach(cycle, e);
    }
    return false;
}

/**/
_CC_API_PRIVATE(_cc_socket_t) _kqueue_event_accept(_cc_event_cycle_t *cycle, _cc_event_t *e, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len) {
    return _cc_socket_accept(e->fd, sa, sa_len);
}

/**/
_CC_API_PRIVATE(void) _reset(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    if (_CC_ISSET_BIT(_CC_EVENT_DISCONNECT_, e->flags) && _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags) == 0) {
        if (_CC_EVENT_IS_SOCKET(e->marks)) {
            _kqueue_event_update(cycle->priv, e, true);
        }
        _cleanup_event(cycle, e);
        return;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_PENDING_, e->flags)) {
        if (_CC_EVENT_IS_SOCKET(e->marks)) {
            _kqueue_event_update(cycle->priv, e, true);
        }

        _cc_list_iterator_swap(&cycle->pending, &e->lnk);
        return;
    }

    /*update event*/
    if (_CC_ISSET_BIT(_CC_EVENT_DESC_SOCKET_, e->descriptor) &&
        _CC_EVENT_IS_SOCKET(e->flags) != _CC_EVENT_IS_SOCKET(e->marks)) {
        _kqueue_event_update(cycle->priv, e, false);
    }

    _reset_event_timeout(cycle, e);
}

/**/
_CC_API_PRIVATE(bool_t) _kqueue_event_wait(_cc_event_cycle_t *cycle, uint32_t timeout) {
    int32_t rc, i;

    struct timespec tv;
    struct kevent actives[_CC_KQUEUE_EVENTS_];
    _cc_event_cycle_priv_t *priv = cycle->priv;

    bzero(&actives, sizeof(struct kevent) * _CC_KQUEUE_EVENTS_);
    /**/
    _reset_event_pending(cycle, _reset);

    if (cycle->diff > 0) {
        timeout -= cycle->diff;
    }

    tv.tv_sec = timeout / 1000;
    tv.tv_nsec = (timeout % 1000) * 1000 * 1000;

    rc = kevent(priv->fd, priv->changelist, priv->nchanges, actives, _CC_KQUEUE_EVENTS_, &tv);
    priv->nchanges = 0;

    if (_cc_unlikely(rc < 0)) {
        int32_t lerrno = _cc_last_errno();
        if (lerrno != _CC_EINTR_) {
            _cc_logger_error(_T("error:%d, %s"), lerrno, _cc_last_error(lerrno));
        }
        goto KEVENT_END;
    }

    for (i = 0; i < rc; ++i) {
        _cc_event_t *e = (_cc_event_t *)actives[i].udata;
        int32_t what = (int32_t)actives[i].filter;
        uint16_t which = _CC_EVENT_UNKNOWN_;

        if (actives[i].flags & EV_ERROR) {
            switch (actives[i].data) {
            /* Can occur on delete if we are not currently
             * watching any events on this fd.  That can
             * happen when the fd was closed and another
             * file was opened with that fd. */
            case ENOENT:
                /* resubmit changes on ENOENT */
                if (e) {
                    e->marks = _CC_EVENT_UNKNOWN_;
                }
            /* Can occur for reasons not fully understood
             * on FreeBSD. */
            case EINVAL:
                continue;
#if defined(__CC_FREEBSD__)
            /*
             * This currently occurs if an FD is closed
             * before the EV_DELETE makes it out via kevent().
             * The FreeBSD capabilities code sees the blank
             * capability set and rejects the request to
             * modify an event.
             *
             * To be strictly correct - when an FD is closed,
             * all the registered events are also removed.
             * Queuing EV_DELETE to a closed FD is wrong.
             * The event(s) should just be deleted from
             * the pending changelist.
             */
            case ENOTCAPABLE:
                continue;
#endif
            /* Can occur on a delete if the fd is closed. */
            case EBADF:
                /* XXXX On NetBSD, we can also get EBADF if we
                 * try to add the write side of a pipe, but
                 * the read side has already been closed.
                 * Other BSDs call this situation 'EPIPE'. It
                 * would be good if we had a way to report
                 * this situation. */
                continue;
            /* These two can occur on an add if the fd was one side
             * of a pipe, and the other side was closed. */
            case EPERM:
            case EPIPE:
                /* Report read events, if we're listening for
                 * them, so that the user can learn about any
                 * add errors.  (If the operation was a
                 * delete, then udata should be cleared.) */
                if (e) {
                    /* The operation was an add:
                     * report the error as a read. */
                    which = _CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags);
                    break;
                } else {
                    /* The operation was a del:
                     * report nothing. */
                    continue;
                }
            /* Other errors shouldn't occur. */
            default:
                _cc_logger_error(_T("Other errors shouldn't occur:%d(%s)."), (int32_t)actives[i].data,
                                 _cc_last_error((int32_t)actives[i].data));
                goto KEVENT_END;
            }
        } else if (what == EVFILT_READ) {
            which = _CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags);
        } else if (what == EVFILT_WRITE) {
            which = _valid_connected(e, _CC_ISSET_BIT(_CC_EVENT_CONNECT_ | _CC_EVENT_WRITABLE_, e->flags));
        }

        if (which) {
            _event_callback(cycle, e, which);
        }
    }

KEVENT_END:
    _update_event_timeout(cycle, timeout);

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _kqueue_event_quit(_cc_event_cycle_t *cycle) {
    _cc_assert(cycle != nullptr);

    if (cycle->priv) {
        if (cycle->priv->fd != -1) {
            _cc_close_socket(cycle->priv->fd);
        }

        _cc_free(cycle->priv);
        cycle->priv = nullptr;
    }

    return _event_cycle_quit(cycle);
}

/**/
_CC_API_PRIVATE(bool_t) _kqueue_event_init(_cc_event_cycle_t *cycle) {
    int r = 0;
    _cc_event_cycle_priv_t *priv;

#ifdef __CC_MACOSX__
    struct kevent changes[2];
#endif

    if (!_event_cycle_init(cycle)) {
        return false;
    }

    priv = (_cc_event_cycle_priv_t *)_cc_malloc(sizeof(_cc_event_cycle_priv_t));
    priv->nchanges = 0;
    priv->fd = kqueue();
    if (_cc_unlikely(priv->fd == -1)) {
        _cc_free(priv);
        _cc_logger_error(_T("cannot create kqueue!"));
        return false;
    }

    _cc_set_socket_closeonexec(priv->fd);

#ifdef __CC_MACOSX__
    /* Check for Mac OS X kqueue bug. */
    bzero(&changes, sizeof changes);

    changes[0].ident = -1;
    changes[0].filter = EVFILT_READ;
    changes[0].flags = EV_ADD;
    /*
     * If kqueue works, then kevent will succeed, and it will
     * stick an error in events[0].  If kqueue is broken, then
     * kevent will fail.
     */
    if (kevent(priv->fd, changes, 1, changes, 2, nullptr) != 1 || (int)changes[0].ident != -1 ||
        !(changes[0].flags & EV_ERROR)) {
        _cc_logger_error(_T("detected broken kqueue; not using."));
        _cc_close_socket(priv->fd);
        _cc_free(priv);
        return false;
    }
#endif
    cycle->priv = priv;
    do {
        r = ioctl(cycle->priv->fd, FIOCLEX);
    } while (r == -1 && _cc_last_errno() == EINTR);

    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_init_event_kqueue(_cc_event_cycle_t *cycle) {
    if (!_kqueue_event_init(cycle)) {
        return false;
    }
    cycle->reset = _kqueue_event_reset;
    cycle->attach = _kqueue_event_attach;
    cycle->connect = _kqueue_event_connect;
    cycle->disconnect = _kqueue_event_disconnect;
    cycle->accept = _kqueue_event_accept;
    cycle->wait = _kqueue_event_wait;
    cycle->quit = _kqueue_event_quit;
    cycle->reset = _kqueue_event_reset;
    return true;
}
