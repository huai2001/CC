#include "event.c.h"
#include <libcc/alloc.h>
#include <libcc/logger.h>
#include <sys/poll.h>

#define _CC_POLL_EVENTS_ 1024

struct _cc_async_event_priv {
    _cc_event_t *list[_CC_POLL_EVENTS_];
    _cc_event_t *fds[_CC_POLL_EVENTS_];
    nfds_t nfds;
};

/**/
_CC_API_PRIVATE(bool_t) _poll_event_reset(_cc_async_event_t *async, _cc_event_t *e) {
    return _reset_event(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _poll_event_attach(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_async_event_priv_t *fset;
    _cc_assert(async != NULL);
    fset = async->priv;

    if (e->fd && _CC_EVENT_IS_SOCKET(e->flags) && fset->nfds >= _CC_POLL_EVENTS_) {
        _cc_logger_error("The maximum number of descriptors supported by the poll() is %d", _CC_POLL_EVENTS_);
        return false;
    }

    if(!_reset_event(async, e)) {
        return false;
    }

    _event_lock(async);
    fset->list[fset->nfds++] = e;
    _event_unlock(async);
    
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _poll_event_connect(_cc_async_event_t *async, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len) {
    if (__cc_stdlib_socket_connect(e->fd, sa, sa_len)) {
        return _poll_event_attach(async, e);
    }
    return false;
}

/**/
_CC_API_PRIVATE(bool_t) _poll_event_disconnect(_cc_async_event_t *async, _cc_event_t *e) {
    return _disconnect_event(async, e);
}

/**/
_CC_API_PRIVATE(_cc_socket_t) _poll_event_accept(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len) {
    return _cc_socket_accept(e->fd, sa, sa_len);
}

/**/
_CC_API_PRIVATE(void) _event_cleanup(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_async_event_priv_t *fset = async->priv;
    int32_t i;

    for (i = 0; i < fset->nfds; i++) {
        if (fset->list[i] == e) {
            fset->list[i] = fset->list[fset->nfds - 1];
            fset->nfds--;
            break;
        }
    }
    _cc_free_event(async, e);
}
/**/
_CC_API_PRIVATE(bool_t) _set_fd_event(_cc_event_t *e, struct pollfd *p) {
    if (_CC_ISSET_BIT(_CC_EVENT_PENDING_, e->flags)) {
        return false;
    }
    p->events = POLLERR;
    if (_CC_ISSET_BIT(_CC_EVENT_CONNECT_, e->flags)) {
        p->events |= POLLOUT;
    } else {
        if (_CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags)) {
            p->events |= POLLIN;
        }
        if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags)) {
            p->events |= POLLOUT;
        }
    }

    if (p->events != POLLERR) {
        p->fd = e->fd;
        p->revents = 0;
        return true;
    }
    return false;
}

/**/
_CC_API_PRIVATE(void) _reset(_cc_async_event_t *async, _cc_event_t *e) {
    if (_CC_ISSET_BIT(_CC_EVENT_CLOSED_, e->flags) && _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags) == 0) {
        /*delete*/
        _event_cleanup(async, e);
        return;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_PENDING_, e->flags) && _CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, e->flags) == 0) {
        _cc_list_swap(&async->pending, &e->lnk);
    } else {
        _reset_event_timeout(async, e);
    }
}

/**/
_CC_API_PRIVATE(bool_t) _poll_event_wait(_cc_async_event_t *async, uint32_t timeout) {
    _cc_event_t *e;
    int32_t i;
    int32_t nfds;
    int32_t ready;
    uint32_t which = _CC_EVENT_UNKNOWN_, what;
    struct pollfd fds[_CC_POLL_EVENTS_];
    _cc_async_event_priv_t *priv = async->priv;

    /**/
    _reset_event_pending(async, _reset);

    if (async->diff > 0) {
        timeout = (async->diff >= timeout) ? 0 : (timeout - async->diff);
    }

    /**/
    if (priv->nfds <= 0) {
        _cc_sleep(timeout);
        return true;
    }

    for (i = 0, nfds = 0; i < priv->nfds; i++) {
        e = priv->list[i];
        if (_set_fd_event(e, &fds[nfds])) {
            priv->fds[nfds++] = e;
        }
    }

    /**/
    ready = poll(fds, nfds, timeout);
    if (_cc_likely(ready)) {
        for (i = 0; i < nfds && ready; i++) {
            e = priv->fds[i];
            which = 0;
            what = fds[i].events;
            if (what & POLLNVAL) {
                _cc_logger_warin("poll() error fd:%d ev:%04Xd rev:%04Xd", e->fd, e->flags, what);
            }

            if (what & ~(POLLIN | POLLOUT | POLLERR | POLLHUP | POLLNVAL)) {
                _cc_logger_warin("strange poll() events fd:%d ev:%04Xd rev:%04Xd", e->fd, e->flags, what);
            }

            if (what & POLLIN) {
                which |= _CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags);
            }

            if (what & POLLOUT) {
                which |= _CC_ISSET_BIT(_CC_EVENT_CONNECT_ | _CC_EVENT_WRITABLE_, e->flags);
                if (which & _CC_EVENT_CONNECT_) {
                    if (_valid_fd(e->fd)) {
                        _CC_UNSET_BIT(_CC_EVENT_CONNECT_, e->flags);
                    } else {
                        which = _CC_EVENT_CLOSED_;
                    }
                }
            }

            if (which) {
                ready--;
                _event_callback(async, e, which);
            }
        }
    } else {
        if (_cc_unlikely(ready < 0)) {
            int32_t lerrno = _cc_last_errno();
            if (lerrno != _CC_EINTR_) {
                _cc_logger_error("error:%d, %s", lerrno, _cc_last_error(lerrno));
            }
        }
    }

    _update_event_timeout(async, timeout);
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _poll_event_free(_cc_async_event_t *async) {
    _cc_assert(async != NULL);

    _cc_if_free(async->priv);

    return _unregister_async_event(async);
}

/**/
_CC_API_PRIVATE(bool_t) _poll_event_alloc(_cc_async_event_t *async) {
    _cc_async_event_priv_t *priv;

    if (!_register_async_event(async)) {
        return false;
    }
    
    priv = (_cc_async_event_priv_t *)_cc_calloc(1, sizeof(_cc_async_event_priv_t));
    priv->nfds = 0;

    async->priv = priv;
    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_register_poll(_cc_async_event_t *async) {
    if (!_poll_event_alloc(async)) {
        return false;
    }
    async->reset = _poll_event_reset;
    async->attach = _poll_event_attach;
    async->connect = _poll_event_connect;
    async->disconnect = _poll_event_disconnect;
    async->accept = _poll_event_accept;
    async->wait = _poll_event_wait;
    async->free = _poll_event_free;
    async->reset = _poll_event_reset;
    return true;
}
