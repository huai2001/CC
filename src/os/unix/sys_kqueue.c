#include "../../event/event.c.h"
#include <libcc/alloc.h>
#include <libcc/logger.h>
#include <sys/event.h>

#define _CC_KQUEUE_EVENTS_ _CC_MAX_CHANGE_EVENTS_
#define _CC_KQUEUE_MAX_UPDATE_ (_CC_KQUEUE_EVENTS_ - 4)

struct _cc_async_event_priv {
    int fd;
    int number_of_changes;
    struct kevent changes[_CC_KQUEUE_EVENTS_];
};

/**/
_CC_API_PRIVATE(bool_t) _emit_kevent(_cc_async_event_priv_t *priv, _cc_event_t *e, bool_t clean) {
    if (priv->number_of_changes >= _CC_KQUEUE_MAX_UPDATE_) {
        /**/
        int r = kevent(priv->fd, priv->changes, priv->number_of_changes, NULL, 0, NULL);
        if (_cc_unlikely(r)) {
            _cc_logger_error(_T("kevent error %d. events:%d, error:%s"), r, priv->number_of_changes, _cc_last_error(r));
            return false;
        }
        priv->number_of_changes = 0;
    }

    if (clean) {
        if (_CC_ISSET_BIT(_CC_EVENT_READABLE_ | _CC_EVENT_ACCEPT_, e->filter)) {
            EV_SET(&priv->changes[priv->number_of_changes++], e->fd, EVFILT_READ, EV_DELETE, 0, 0, e);
        }
        if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_ | _CC_EVENT_CONNECT_, e->filter)) {
            EV_SET(&priv->changes[priv->number_of_changes++], e->fd, EVFILT_WRITE, EV_DELETE, 0, 0, e);
        }
        e->filter = _CC_EVENT_UNKNOWN_;
    } else {
        if (_CC_ISSET_BIT(_CC_EVENT_CONNECT_, e->flags)) {
            EV_SET(&priv->changes[priv->number_of_changes++], e->fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, e);
            e->filter |= _CC_EVENT_CONNECT_;
        } else {
            uint16_t m = _CC_EVENT_IS_SOCKET(e->flags);
            uint16_t add_filter = m & ~e->filter;
            uint16_t del_filter = ~m & e->filter;
            if (_CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, add_filter)) {
                EV_SET(&priv->changes[priv->number_of_changes++], e->fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, e);
            } else if (_CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, del_filter)) {
                EV_SET(&priv->changes[priv->number_of_changes++], e->fd, EVFILT_READ, EV_DELETE, 0, 0, e);
            }

            if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_, add_filter)) {
                EV_SET(&priv->changes[priv->number_of_changes++], e->fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, e);
            } else if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_ | _CC_EVENT_CONNECT_, del_filter)) {
                EV_SET(&priv->changes[priv->number_of_changes++], e->fd, EVFILT_WRITE, EV_DELETE, 0, 0, e);
            }
            _CC_MODIFY_BIT(add_filter, del_filter, e->filter);
        }
    }

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _kqueue_event_attach(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_assert(async != NULL && e != NULL);
    return _reset_event(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _kqueue_event_reset(_cc_async_event_t *async, _cc_event_t *e) {
    return _reset_event(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _kqueue_event_disconnect(_cc_async_event_t *async, _cc_event_t *e) {
    return _disconnect_event(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _kqueue_event_connect(_cc_async_event_t *async, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len) {
    if (__cc_stdlib_socket_connect(e->fd, sa, sa_len)) {
        return _kqueue_event_attach(async, e);
    }
    return false;
}

/**/
_CC_API_PRIVATE(_cc_socket_t) _kqueue_event_accept(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len) {
    return _cc_socket_accept(e->fd, sa, sa_len);
}

/**/
_CC_API_PRIVATE(void) _reset(_cc_async_event_t *async, _cc_event_t *e) {
    uint16_t m = _CC_EVENT_IS_SOCKET(e->filter), u;
    if (_CC_ISSET_BIT(_CC_EVENT_CLOSED_, e->flags) && _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags) == 0) {
        if (m) {
            _emit_kevent(async->priv, e, true);
        }
        _cc_free_event(async, e);
        return;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_PENDING_, e->flags)) {
        if (m) {
            _emit_kevent(async->priv, e, true);
        }
        if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, e->flags) == 0) {
            _cc_list_iterator_swap(&async->pending, &e->lnk);
            return;
        }
    } else {
        /*update event*/
        u = _CC_EVENT_IS_SOCKET(e->flags);
        if (u && u != m) {
            _emit_kevent(async->priv, e, false);
        }
    }
    _reset_event_timeout(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _kqueue_event_wait(_cc_async_event_t *async, uint32_t timeout) {
    int32_t rc, i;

    struct timespec tv;
    struct kevent actives[_CC_KQUEUE_EVENTS_];
    _cc_async_event_priv_t *priv = async->priv;

    bzero(&actives, sizeof(struct kevent) * _CC_KQUEUE_EVENTS_);
    /**/
    _reset_event_pending(async, _reset);

    if (async->diff > 0) {
        timeout -= async->diff;
    }

    tv.tv_sec = timeout / 1000;
    tv.tv_nsec = (timeout % 1000) * 1000 * 1000;

    rc = kevent(priv->fd, priv->changes, priv->number_of_changes, actives, _CC_KQUEUE_EVENTS_, &tv);
    priv->number_of_changes = 0;

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
        uint32_t which = _CC_EVENT_UNKNOWN_;

        if (actives[i].flags & EV_ERROR) {
            switch (actives[i].data) {
            /* Can occur on delete if we are not currently
             * watching any events on this fd.  That can
             * happen when the fd was closed and another
             * file was opened with that fd. */
            case ENOENT:
                /* resubmit changes on ENOENT */
                if (e) {
                    e->filter = _CC_EVENT_UNKNOWN_;
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
             * the pending changes.
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
            which = _CC_ISSET_BIT(_CC_EVENT_CONNECT_ | _CC_EVENT_WRITABLE_, e->flags);
            if (which & _CC_EVENT_CONNECT_) {
                if (_valid_fd(e->fd)) {
                    _CC_UNSET_BIT(_CC_EVENT_CONNECT_, e->flags);
                } else {
                    which = _CC_EVENT_CLOSED_;
                }
            }
        }

        if (which) {
            _event_callback(async, e, which);
        }
    }

KEVENT_END:
    _update_event_timeout(async, timeout);

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _kqueue_event_free(_cc_async_event_t *async) {
    _cc_assert(async != NULL);

    if (async->priv) {
        if (async->priv->fd != -1) {
            _cc_close_socket(async->priv->fd);
        }

        _cc_free(async->priv);
        async->priv = NULL;
    }

    return _unregister_async_event(async);
}

/**/
_CC_API_PRIVATE(bool_t) _kqueue_event_alloc(_cc_async_event_t *async) {
    int r = 0;
    _cc_async_event_priv_t *priv;
#ifdef __CC_MACOSX__
    struct kevent changes[2];
#endif
    
    if (!_register_async_event(async)) {
        return false;
    }

    priv = (_cc_async_event_priv_t *)_cc_malloc(sizeof(_cc_async_event_priv_t));
    priv->number_of_changes = 0;
    priv->fd = kqueue();
    if (_cc_unlikely(priv->fd == -1)) {
        _cc_free(priv);
        _cc_logger_error(_T("cannot create kqueue!"));
        return false;
    }

    _cc_set_socket_closeonexec(priv->fd);

#ifdef __CC_MACOSX__
    /* Check for Mac OS X kqueue bug. */
    bzero(&changes, sizeof(changes));

    changes[0].ident = -1;
    changes[0].filter = EVFILT_READ;
    changes[0].flags = EV_ADD;

    /*
     * If kqueue works, then kevent will succeed, and it will
     * stick an error in events[0].  If kqueue is broken, then
     * kevent will fail.
     */
    if (kevent(priv->fd, changes, 1, changes, 2, NULL) != 1 ||
        (int)changes[0].ident != -1 || !(changes[0].flags & EV_ERROR)) {
        _cc_logger_error(_T("detected broken kqueue; not using."));
        _cc_close_socket(priv->fd);
        _cc_free(priv);
        return false;
    }
#endif
    async->priv = priv;
    do {
        r = ioctl(async->priv->fd, FIOCLEX);
    } while (r == -1 && _cc_last_errno() == EINTR);

    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_register_kqueue(_cc_async_event_t *async) {
    if (!_kqueue_event_alloc(async)) {
        return false;
    }
    async->reset = _kqueue_event_reset;
    async->attach = _kqueue_event_attach;
    async->connect = _kqueue_event_connect;
    async->disconnect = _kqueue_event_disconnect;
    async->accept = _kqueue_event_accept;
    async->wait = _kqueue_event_wait;
    async->free = _kqueue_event_free;
    async->reset = _kqueue_event_reset;
    return true;
}
