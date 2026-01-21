#include <libcc/alloc.h>
#include <libcc/logger.h>
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

struct _cc_async_event_priv {
    int32_t nfds;
    _cc_event_t *list[FD_SETSIZE];
};

/**/
_CC_API_PRIVATE(bool_t) _select_event_reset(_cc_async_event_t *async, _cc_event_t *e) {
    return _reset_event(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _select_event_attach(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_async_event_priv_t *fset;
    _cc_assert(async != NULL);
    fset = async->priv;

    if (e->fd && _CC_EVENT_IS_SOCKET(e->flags) && fset->nfds >= FD_SETSIZE) {
        _cc_logger_error("The maximum number of descriptors supported by the select() is %d", FD_SETSIZE);
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
_CC_API_PRIVATE(bool_t) _select_event_disconnect(_cc_async_event_t *async, _cc_event_t *e) {
    return _disconnect_event(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _select_event_connect(_cc_async_event_t *async, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len) {
    if (__cc_stdlib_socket_connect(e->fd, sa, sa_len)) {
        return _select_event_attach(async, e);
    }
    return false;
}

/**/
_CC_API_PRIVATE(_cc_socket_t) _select_event_accept(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sa,
                                                    _cc_socklen_t *sa_len) {
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
_CC_API_PRIVATE(void) _reset(_cc_async_event_t *async, _cc_event_t *e) {
    if (_CC_ISSET_BIT(_CC_EVENT_CLOSED_, e->flags) && _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags) == 0) {
        /*delete*/
        _event_cleanup(async, e);
        return;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_PENDING_, e->flags) && _CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, e->flags) == 0) {
        _cc_list_iterator_swap(&async->pending, &e->lnk);
    } else {
		_reset_event_timeout(async, e);
	}
}

/**/
_CC_API_PRIVATE(bool_t) _set_fd_event(_cc_event_t *e, struct _fd_list *fds) {
    if (_CC_ISSET_BIT(_CC_EVENT_PENDING_, e->flags)) {
        return false;
    }

    if (_CC_EVENT_IS_SOCKET(e->flags) == 0) {
        return false;
    }
    if (_CC_ISSET_BIT(_CC_EVENT_CONNECT_, e->flags)) {
        FD_SET(e->fd, &fds->wfds);
        fds->wc++;
        e->filter |= _CC_EVENT_WRITABLE_;
    } else {
        if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags)) {
            FD_SET(e->fd, &fds->wfds);
            fds->wc++;
            e->filter |= _CC_EVENT_WRITABLE_;
        }

        if (_CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags)) {
            FD_SET(e->fd, &fds->rfds);
            fds->rc++;
            e->filter |= _CC_EVENT_READABLE_;
        }
    }
#ifndef _CC_WINDOWS
    if (fds->max_fd == _CC_INVALID_SOCKET_ || fds->max_fd < e->fd) {
        fds->max_fd = e->fd;
    }
#endif
    return true;
}
/**/
_CC_API_PRIVATE(bool_t) _select_event_wait(_cc_async_event_t *async, uint32_t timeout) {
    struct timeval tv;
    int32_t i;
    int32_t ready;
    struct _fd_list fds;
    _cc_async_event_priv_t *priv = async->priv;
    /**/
    _reset_event_pending(async, _reset);

    if (async->diff > 0) {
        timeout = (async->diff >= timeout) ? 0 : (timeout - async->diff);
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
        _set_fd_event(priv->list[i], &fds);
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
            _cc_event_t* e = priv->list[i];
            uint32_t which = 0;
            uint32_t what = _CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags);
            if (what && FD_ISSET(e->fd, &fds.rfds)) {
                which |= what;
            }

            what = _CC_ISSET_BIT(_CC_EVENT_CONNECT_ | _CC_EVENT_WRITABLE_, e->flags);
            if (what && FD_ISSET(e->fd, &fds.wfds)) {
                which |= what;
                if (which & _CC_EVENT_CONNECT_) {

                }
            }

            if (which) {
                ready--;
                _event_callback(async, e, which);
            }
        }
    } else {
        if (_cc_unlikely(ready == -1)) {
            int32_t lerrno = _cc_last_errno();
            if (lerrno != _CC_EINTR_) {
                _cc_logger_error("error:%d, %s", lerrno, _cc_last_error(lerrno));
            }
        }
    }

WHEEL_TIMER:
    _update_event_timeout(async, timeout);
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _select_event_free(_cc_async_event_t *async) {
    _cc_assert(async != NULL);
    if (async == NULL) {
        return false;
    }

    _cc_if_free(async->priv);

    return _unregister_async_event(async);
}

/**/
_CC_API_PRIVATE(bool_t) _select_event_alloc(_cc_async_event_t *async) {
    _cc_async_event_priv_t *priv;
    
    if (!_register_async_event(async)) {
        return false;
    }
    
    priv = (_cc_async_event_priv_t *)_cc_malloc(sizeof(_cc_async_event_priv_t));
    bzero(priv, sizeof(_cc_async_event_priv_t));

    async->priv = priv;
    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_register_select(_cc_async_event_t *async) {
    if (!_select_event_alloc(async)) {
        return false;
    }
    async->reset = _select_event_reset;
    async->attach = _select_event_attach;
    async->connect = _select_event_connect;
    async->disconnect = _select_event_disconnect;
    async->accept = _select_event_accept;
    async->wait = _select_event_wait;
    async->free = _select_event_free;
    async->reset = _select_event_reset;
    return true;
}
