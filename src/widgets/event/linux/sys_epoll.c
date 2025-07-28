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
#include "../event.c.h"
#include <libcc/alloc.h>
#include <libcc/logger.h>
#include <libcc/widgets/timeout.h>
#include <sys/epoll.h>

#define _CC_EPOLL_EVENTS_ _CC_MAX_CHANGE_EVENTS_

struct _cc_event_cycle_priv {
    int fd;
};

/**/
_CC_API_PRIVATE(bool_t) _epoll_event_update(int efd, _cc_event_t *e, bool_t rm) {
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
_CC_API_PRIVATE(bool_t) _epoll_event_attach(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_assert(cycle != nullptr && e != nullptr);
    e->descriptor = _CC_EVENT_DESC_POLL_EPOLL_ | (e->descriptor & 0xff);
    return _reset_event(cycle, e);
}

/**/
_CC_API_PRIVATE(bool_t) _epoll_event_reset(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    return _reset_event(cycle, e);
}

/**/
_CC_API_PRIVATE(bool_t) _epoll_event_disconnect(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    return _disconnect_event(cycle, e);
}

/**/
_CC_API_PRIVATE(bool_t) _epoll_event_connect(_cc_event_cycle_t *cycle, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len) {
    if (__cc_stdlib_socket_connect(e->fd, sa, sa_len)) {
        return _epoll_event_attach(cycle, e);
    }
    return false;
}

/**/
_CC_API_PRIVATE(_cc_socket_t) _epoll_event_accept(_cc_event_cycle_t *cycle, _cc_event_t *e, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len) {
    return _cc_socket_accept(e->fd, sa, sa_len);
}

/**/
_CC_API_PRIVATE(void) _reset(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    if (_CC_ISSET_BIT(_CC_EVENT_DISCONNECT_, e->flags) && _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags) == 0) {
        if (_CC_EVENT_IS_SOCKET(e->marks)) {
            _epoll_event_update(cycle->priv->fd, e, true);
        }
        _cleanup_event(cycle, e);
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

    _reset_event_timeout(cycle, e);
}

/**/
_CC_API_PRIVATE(bool_t) _epoll_event_wait(_cc_event_cycle_t *cycle, uint32_t timeout) {
    int32_t rc, i;

    struct epoll_event actives[_CC_EPOLL_EVENTS_];
    _cc_event_cycle_priv_t *priv = cycle->priv;

    bzero(&actives, sizeof(struct epoll_event) * _CC_EPOLL_EVENTS_);

    /**/
    _reset_event_pending(cycle, _reset);

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
        uint16_t which = _CC_EVENT_UNKNOWN_;
        int32_t what = (int32_t)actives[i].events;
        _cc_event_t *e = (_cc_event_t *)actives[i].data.ptr;

        if (what & EPOLLERR) {
            which = _CC_EVENT_IS_SOCKET(e->flags);
        } else if ((what & EPOLLHUP) && !(what & EPOLLRDHUP)) {
            which = _CC_EVENT_IS_SOCKET(e->flags);
        } else {
            if (what & EPOLLIN) {
                which |= _CC_ISSET_BIT(_CC_EVENT_ACCEPT_ | _CC_EVENT_READABLE_, e->flags);
            }
            if (what & EPOLLOUT) {
                which |= _CC_ISSET_BIT(_CC_EVENT_CONNECT_ | _CC_EVENT_WRITABLE_, e->flags);
                if (_CC_ISSET_BIT(_CC_EVENT_CONNECT_, which)) {
                    which = _valid_connected(e, which);
                }
            }
            if (what & EPOLLRDHUP) {
                which = _CC_EVENT_DISCONNECT_;
            }
        }

        if (which) {
            _event_callback(cycle, e, which);
        }
    }

EPOLL_END:
    _update_event_timeout(cycle, timeout);
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _epoll_event_quit(_cc_event_cycle_t *cycle) {
    _cc_assert(cycle != nullptr);
    if (cycle == nullptr) {
        return false;
    }

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
_CC_API_PRIVATE(bool_t) _epoll_event_init(_cc_event_cycle_t *cycle) {
    _cc_socket_t fd = -1;
    if (!_event_cycle_init(cycle)) {
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
_CC_API_PUBLIC(bool_t) _cc_init_event_epoll(_cc_event_cycle_t *cycle) {
    if (!_epoll_event_init(cycle)) {
        return false;
    }
    cycle->reset = _epoll_event_reset;
    cycle->attach = _epoll_event_attach;
    cycle->connect = _epoll_event_connect;
    cycle->disconnect = _epoll_event_disconnect;
    cycle->accept = _epoll_event_accept;
    cycle->wait = _epoll_event_wait;
    cycle->quit = _epoll_event_quit;
    cycle->reset = _epoll_event_reset;
    return true;
}