#include "../../event/event.c.h"
#include <libcc/alloc.h>
#include <libcc/logger.h>
#include <libcc/timeout.h>
#include <sys/epoll.h>

#define _CC_EPOLL_EVENTS_ _CC_MAX_CHANGE_EVENTS_

struct _cc_async_event_priv {
    int fd;
};

/**/
_CC_API_PRIVATE(bool_t) _emit_epoll_event(int efd, _cc_event_t *e, bool_t clean) {
    uint16_t filter = _CC_EVENT_UNKNOWN_;
    int op = EPOLL_CTL_DEL;
    struct epoll_event ev;

    //bzero(&ev, sizeof(struct epoll_event));
    ev.data.fd = e->fd;
    ev.data.ptr = e;
    ev.events = 0;

    if (!clean) {
        if (_CC_ISSET_BIT(_CC_EVENT_CONNECT_, e->flags)) {
            ev.events |= EPOLLOUT;
            filter = _CC_EVENT_CONNECT_;
        } else {
            /*Setting the readable event flag*/
            if (_CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags)) {
                ev.events |= EPOLLIN;
            }
            /*Setting the writable event flag*/
            if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags)) {
                ev.events |= EPOLLOUT;
            }
            filter = _CC_EVENT_IS_SOCKET(e->flags);
        }
        if (ev.events) {
            op = e->filter ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
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
                    _cc_logger_error("Epoll MOD(%d) on %d retried as ADD; that failed too", (int)ev.events, e->fd);
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
                    _cc_logger_error("Epoll ADD(%d) on %d retried as MOD; that failed too", (int)ev.events, e->fd);
                    return false;
                }
            }
        } break;
        case EPOLL_CTL_DEL: {
            if (err != ENOENT && err != EBADF && err != EPERM) {
                _cc_logger_error("Epoll DEL(%d) on %d retried as DEL; that failed too", (int)ev.events, e->fd);
                return false;
            }
        } break;
        }
    }

    e->filter = filter;
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _epoll_event_attach(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_assert(async != NULL && e != NULL);
    return _reset_event(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _epoll_event_reset(_cc_async_event_t *async, _cc_event_t *e) {
    return _reset_event(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _epoll_event_disconnect(_cc_async_event_t *async, _cc_event_t *e) {
    return _disconnect_event(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _epoll_event_connect(_cc_async_event_t *async, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len) {
    if (__cc_stdlib_socket_connect(e->fd, sa, sa_len)) {
        return _epoll_event_attach(async, e);
    }
    return false;
}

/**/
_CC_API_PRIVATE(_cc_socket_t) _epoll_event_accept(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len) {
    return _cc_socket_accept(e->fd, sa, sa_len);
}

/**/
_CC_API_PRIVATE(void) _reset(_cc_async_event_t *async, _cc_event_t *e) {
    uint16_t m = _CC_EVENT_IS_SOCKET(e->filter), u;
    if (_CC_ISSET_BIT(_CC_EVENT_CLOSED_, e->flags) && _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags) == 0) {
        if (m) {
            _emit_epoll_event(async->priv->fd, e, true);
        }
        _cc_free_event(async, e);
        return;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_PENDING_, e->flags)) {
        if (m) {
            _emit_epoll_event(async->priv->fd, e, true);
        }
        if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, e->flags) == 0) {
            _cc_list_swap(&async->pending, &e->lnk);
            return;
        }
    } else {
        /*update event*/
        u = _CC_EVENT_IS_SOCKET(e->flags);
        if (u && u != m) {
            _emit_epoll_event(async->priv->fd, e, false);
        }
    }

    _reset_event_timeout(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _epoll_event_wait(_cc_async_event_t *async, uint32_t timeout) {
    int32_t rc, i;

    struct epoll_event actives[_CC_EPOLL_EVENTS_];
    _cc_async_event_priv_t *priv = async->priv;

    //bzero(&actives, sizeof(struct epoll_event) * _CC_EPOLL_EVENTS_);

    /**/
    _reset_event_pending(async, _reset);

    if (async->diff > 0) {
        timeout = (async->diff >= timeout) ? 0 : (timeout - async->diff);
    }

    rc = epoll_wait(priv->fd, actives, _CC_EPOLL_EVENTS_, timeout);
    if (rc < 0) {
        int32_t lerrno = _cc_last_errno();
        if (lerrno != _CC_EINTR_) {
            _cc_logger_error("error:%d, %s", lerrno, _cc_last_error(lerrno));
        }
        goto EPOLL_END;
    }

    for (i = 0; i < rc; ++i) {
        uint32_t which = _CC_EVENT_UNKNOWN_;
        int32_t what = (int32_t)actives[i].events;
        _cc_event_t *e = (_cc_event_t *)actives[i].data.ptr;

        if (what & EPOLLERR) {
            which = _CC_EVENT_CLOSED_;
        } else if (what & EPOLLHUP) {
            /*
                The EPOLLHUP event will be triggered on this end when the fd is closed on the other end.
                the EPOLLHUP event as a readable event.
                If the read 0 bytes, it indicates that the opposite end has closed.
            */
            which = _CC_EVENT_READABLE_;//or _CC_EVENT_CLOSED_
        } else {
            if (what & EPOLLIN) {
                which |= _CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags);
            }
            if (what & EPOLLOUT) {
                which |= _CC_ISSET_BIT(_CC_EVENT_CONNECT_ | _CC_EVENT_WRITABLE_, e->flags);
                if (which & _CC_EVENT_CONNECT_) {
                    if (_valid_fd(e->fd)) {
                        _CC_UNSET_BIT(_CC_EVENT_CONNECT_, e->flags);
                    } else {
                        which = _CC_EVENT_CLOSED_;
                    }
                }
            }
            if (what & EPOLLRDHUP) {
                which = _CC_EVENT_CLOSED_;
            }
        }

        if (which) {
            _event_callback(async, e, which);
        }
    }

EPOLL_END:
    _update_event_timeout(async, timeout);
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _epoll_event_free(_cc_async_event_t *async) {
    _cc_assert(async != NULL);
    if (async == NULL) {
        return false;
    }

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
_CC_API_PRIVATE(bool_t) _epoll_event_alloc(_cc_async_event_t *async) {
    _cc_socket_t fd = -1;

    if (!_register_async_event(async)) {
        return false;
    }
#ifdef EPOLL_CLOEXEC
    fd = epoll_create1(EPOLL_CLOEXEC);
#endif
    if (fd == -1) {
        /* Initialize the kernel queue using the old interface.  (The
           size field is ignored   since 2.6.8.) */
        if ((fd = epoll_create(1024)) == -1) {
            if (_cc_last_errno() != ENOSYS) {
                _cc_logger_error("cannot create epoll!");
            }
            _unregister_async_event(async);
            return false;
        }
        _cc_set_socket_closeonexec(fd);
    }

    async->priv = (_cc_async_event_priv_t *)_cc_malloc(sizeof(_cc_async_event_priv_t));
    async->priv->fd = fd;

    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_register_epoll(_cc_async_event_t *async) {
    if (!_epoll_event_alloc(async)) {
        return false;
    }
    async->reset = _epoll_event_reset;
    async->attach = _epoll_event_attach;
    async->connect = _epoll_event_connect;
    async->disconnect = _epoll_event_disconnect;
    async->accept = _epoll_event_accept;
    async->wait = _epoll_event_wait;
    async->free = _epoll_event_free;
    return true;
}