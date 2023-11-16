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
#include "sys_socket_c.h"
#include <cc/alloc.h>
#include <cc/event/timeout.h>
#include <cc/logger.h>
#include <cc/thread.h>

#ifdef _CC_EVENT_USE_IOCP_

#ifndef NT_SUCCESS
#define NT_SUCCESS(status) (((status)) >= 0)
#endif

#define _CC_IOCP_STATUS_PENDING_ 0x01
#define _CC_IOCP_STATUS_ACCEPT_ 0x02

#define _CC_IOCP_EVENTS_ _CC_MAX_CHANGE_EVENTS_
#define _CC_IOCP_ACCEPT_EVENTS 16

#define IOCPPort (cycle->priv->port)
/**/
//_CC_API_PRIVATE(bool_t) _iocp_event_disconnect(_cc_event_cycle_t *cycle,
//_cc_event_t *e);
_CC_API_PRIVATE(bool_t) _iocp_event_del(_cc_event_cycle_t *cycle, _cc_event_t *e);

/*close socket*/
void _iocp_event_close_socket(_cc_socket_t fd) {
    /**/
    LINGER linger = {1, 0};

    setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger));

    /*Now close the socket handle.  This will do an abortive or  graceful close,
     * as requested.*/
    CancelIo((HANDLE)fd);
    _cc_close_socket(fd);
}

/**/
_CC_API_PRIVATE(void) _iocp_event_cleanup(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    if (e->accept_fd != _CC_INVALID_SOCKET_) {
        _iocp_event_close_socket(e->accept_fd);
    }
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_accept_event(_cc_socket_t fd, _iocp_overlapped_t *iocp_overlapped) {
    int result;

    iocp_overlapped->flag = _CC_EVENT_ACCEPT_;
    result = _WSA_socket_accept(iocp_overlapped->e, fd, &iocp_overlapped->overlapped);
    if (_cc_unlikely(NO_ERROR != result)) {
        _CC_UNSET_BIT(_CC_EVENT_ACCEPT_, iocp_overlapped->e->marks);
        _cc_logger_error(_T("_WSA_socket_accept:%d, %s\n"), result, _cc_last_error(result));
        return false;
    }

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_write_event(_iocp_overlapped_t *iocp_overlapped) {
    int result;

    if (!_cc_event_fd_valid(iocp_overlapped->e)) {
        return false;
    }

    iocp_overlapped->flag = _CC_EVENT_WRITABLE_;
    result = _WSA_socket_send(iocp_overlapped->e, &iocp_overlapped->overlapped);
    if (_cc_unlikely(result != NO_ERROR)) {
        _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, iocp_overlapped->e->marks);
        //_cc_logger_error(_T("_WSA_socket_send:%d, %s\n"), result,
        //_cc_last_error(result));
        return false;
    }

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_receive_event(_iocp_overlapped_t *iocp_overlapped) {
    int result;

    if (!_cc_event_fd_valid(iocp_overlapped->e)) {
        return false;
    }
    iocp_overlapped->flag = _CC_EVENT_READABLE_;
    result = _WSA_socket_receive(iocp_overlapped->e, &iocp_overlapped->overlapped);
    if (_cc_unlikely(result != NO_ERROR)) {
        _CC_UNSET_BIT(_CC_EVENT_READABLE_, iocp_overlapped->e->marks);
        //_cc_logger_error(_T("_WSA_socket_receive:%d, %s\n"), result,
        //_cc_last_error(result));
        return false;
    }

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_update(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _iocp_overlapped_t *iocp_overlapped = NULL;
    uint16_t addevents = e->flags & ~e->marks;
    uint16_t delevents = ~e->flags & e->marks;

    _CC_MODIFY_BIT(addevents, delevents, e->marks);

    if (_CC_ISSET_BIT(_CC_EVENT_ACCEPT_, addevents) != 0) {
        _cc_socket_t fd = _CC_INVALID_SOCKET_;

        if (_CC_ISSET_BIT(_CC_EVENT_DISCONNECT_, e->flags)) {
            return true;
        }
        fd = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (_cc_unlikely(fd == _CC_INVALID_SOCKET_)) {
            int result = _cc_last_errno();
            _cc_logger_error(_T("IOCP Create WSASocket fail:%d, %s\n"), result, _cc_last_error(result));
            return false;
        }

        if (_cc_likely(_cc_iocp_overlapped_alloc(cycle->priv, &iocp_overlapped))) {
            iocp_overlapped->fd = fd;
            iocp_overlapped->e = e;
            iocp_overlapped->ident = e->ident;

            if (!_iocp_event_accept_event(fd, iocp_overlapped)) {
                _cc_close_socket(fd);
                _cc_iocp_overlapped_free(cycle->priv, iocp_overlapped);
            }
        } else {
            _cc_close_socket(fd);
        }
        return true;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_, addevents)) {
        if (_cc_likely(_cc_iocp_overlapped_alloc(cycle->priv, &iocp_overlapped))) {
            iocp_overlapped->fd = 0;
            iocp_overlapped->e = e;
            iocp_overlapped->ident = e->ident;

            if (_iocp_event_write_event(iocp_overlapped)) {
                iocp_overlapped = NULL;
            }
        }
    }

    if (_CC_ISSET_BIT(_CC_EVENT_READABLE_, addevents)) {
        if (!iocp_overlapped && !_cc_iocp_overlapped_alloc(cycle->priv, &iocp_overlapped)) {
            return _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->marks);
        }

        iocp_overlapped->fd = 0;
        iocp_overlapped->e = e;
        iocp_overlapped->ident = e->ident;

        if (_iocp_event_receive_event(iocp_overlapped)) {
            iocp_overlapped = NULL;
        }
    }

    if (iocp_overlapped) {
        _cc_iocp_overlapped_free(cycle->priv, iocp_overlapped);
        return false;
    }

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_attach(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    int32_t i = 0;
    _cc_assert(cycle != NULL && e != NULL);

    if (cycle->running == 0) {
        return false;
    }
    
    e->descriptor |= _CC_EVENT_DESC_POLL_IOCP_;

    if (_CC_ISSET_BIT(_CC_EVENT_DESC_SOCKET_, e->descriptor)) {
        _cc_assert(IOCPPort != NULL);
        if (_cc_unlikely(IOCPPort == NULL)) {
            return false;
        }

        if (CreateIoCompletionPort((pid_t)e->fd, IOCPPort, _CC_IOCP_SOCKET_, 0) == NULL) {
            _cc_logger_error(_T("CreateIoCompletionPort Error Code:%d.\n"), _cc_last_errno());
            return false;
        }
    } else if (_CC_ISSET_BIT(_CC_EVENT_DESC_TIMER_, e->descriptor) == 0) {
        return false;
    }

    e->marks = _CC_EVENT_UNKNOWN_;
    return _cc_event_wait_reset(cycle, e);
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_bind_connect(_cc_event_cycle_t *cycle, const _cc_event_t *e, int family) {
    _cc_socklen_t socklen = 0;
    _cc_sockaddr_t *sockaddr_any;

    if (_CC_ISSET_BIT(_CC_EVENT_CONNECT_, e->flags) == 0) {
        return false;
    }

    _cc_assert(IOCPPort != NULL);
    if (_cc_unlikely(IOCPPort == NULL)) {
        return false;
    }

    if (family == AF_INET) {
        socklen = sizeof(struct sockaddr_in);
        sockaddr_any = _cc_win_get_ipv4_any_addr();
    } else {
        socklen = sizeof(struct sockaddr_in6);
        sockaddr_any = _cc_win_get_ipv6_any_addr();
    }

    if (bind(e->fd, (struct sockaddr *)sockaddr_any, socklen) == SOCKET_ERROR) {
		int err = _cc_last_errno();
		_cc_logger_error(_T("bind Error Code:%d. %s\n"),err, _cc_last_error(err));
        return false;
    }

    if (CreateIoCompletionPort((pid_t)e->fd, IOCPPort, (UINT_PTR)cycle, 0) == NULL) {
		int err = _cc_last_errno();
		_cc_logger_error(_T("CreateIoCompletionPort Error Code:%d. %s\n"),err, _cc_last_error(err));
        return false;
    }

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_connect(_cc_event_cycle_t *cycle, _cc_event_t *e, const _cc_sockaddr_t *sa,
                                  const _cc_socklen_t sa_len) {
    _iocp_overlapped_t *iocp_overlapped = NULL;

    LPFN_CONNECTEX connect_fn = get_connectex_func_ptr(e->fd);
    if (_cc_unlikely(connect_fn == NULL)) {
        return false;
    }

    if (!_iocp_bind_connect(cycle, e, sa->sa_family)) {
        return false;
    }

    if (!_cc_iocp_overlapped_alloc(cycle->priv, &iocp_overlapped)) {
        return false;
    }

    iocp_overlapped->fd = 0;
    iocp_overlapped->e = e;
    iocp_overlapped->ident = e->ident;
    iocp_overlapped->flag = _CC_EVENT_CONNECT_;

    if (!connect_fn(e->fd, (struct sockaddr *)sa, sa_len, NULL, 0, NULL, &iocp_overlapped->overlapped)) {
        int err = _cc_last_errno();
        if (err != WSA_IO_PENDING) {
            _cc_iocp_overlapped_free(cycle->priv, iocp_overlapped);
            _cc_logger_error(_T("Socket Connect:(%d) %s\n"), err, _cc_last_error(err));
            return false;
        }
    }

    return true;
}
/*
_CC_API_PRIVATE(bool_t) _iocp_event_disconnect(_cc_event_cycle_t *cycle,
                                                          _cc_event_t *e) {
    int result = 0;
    DWORD dwFlags = TF_REUSE_SOCKET;
    DWORD reserved = 0;
    _iocp_overlapped_t *iocp_overlapped = NULL;

    LPFN_DISCONNECTEX disconnect_fn = get_disconnect_func_ptr(e->fd);
    if (disconnect_fn == NULL) {
        return false;
    }

    if (!_cc_iocp_overlapped_alloc(cycle->priv, &iocp_overlapped)) {
        return false;
    }

    _CC_SET_BIT(_CC_EVENT_DISCONNECT_, e->marks);
    _CC_MODIFY_BIT(_CC_EVENT_DISCONNECT_, _CC_EVENT_READABLE_, e->flags);

    iocp_overlapped->fd = 0;
    iocp_overlapped->e = e;
    iocp_overlapped->ident = e->ident;
    iocp_overlapped->flag = _CC_EVENT_DISCONNECT_;

    if (disconnect_fn(e->fd, &iocp_overlapped->overlapped, dwFlags, reserved) == false) {
        int err = _cc_last_errno();
        if (err != WSA_IO_PENDING) {
            _cc_iocp_overlapped_free(cycle->priv, iocp_overlapped);
            _cc_logger_error(_T("Socket Disconnect:(%d) %s\n"), err, _cc_last_error(err));
            return false;
        }
    }
    return true;
}
*/
/**/
_CC_API_PRIVATE(_cc_socket_t) _iocp_event_accept(_cc_event_cycle_t *cycle, _cc_event_t *e, _cc_sockaddr_t *sa,
                                       _cc_socklen_t *sa_len) {
    long i = 0;
    _cc_socket_t fd = e->accept_fd;

    if (_cc_unlikely(fd != _CC_INVALID_SOCKET_)) {
        e->accept_fd = _CC_INVALID_SOCKET_;

        /* SO_UPDATE_ACCEPT_CONTEXT is required for shutdown() to work fine*/
        setsockopt(fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&e->fd, sizeof(_cc_socket_t));

        _cc_assert(sa != NULL);
        _cc_assert(sa_len != NULL);

        if (sa && sa_len && getpeername(fd, (struct sockaddr *)sa, sa_len) == -1) {
            int32_t err = _cc_last_errno();
            _cc_logger_error(_T("discovery client information failed, fd=%d, ")
                             _T("errno=%d(%#x).\n"),
                             fd, err, err);
        }
    } else {
        _cc_logger_error(_T("Listening object is NULL\n"));
    }

    return fd;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_dispatch(_cc_event_cycle_t *cycle, LPOVERLAPPED overlapped,
                                   _iocp_overlapped_t *iocp_overlaaped) {
    uint16_t events = _CC_EVENT_UNKNOWN_;
    _cc_event_t *e = iocp_overlaaped->e;

    _CC_UNSET_BIT(iocp_overlaaped->flag, e->marks);

    switch (iocp_overlaaped->flag) {
    case _CC_EVENT_ACCEPT_:
        _CC_SET_BIT(_CC_EVENT_ACCEPT_, events);
        if (iocp_overlaaped->fd) {
            e->accept_fd = iocp_overlaaped->fd;
        }
        break;
    case _CC_EVENT_READABLE_:
        _CC_SET_BIT(_CC_EVENT_READABLE_, events);
        break;
    case _CC_EVENT_WRITABLE_:
        _CC_SET_BIT(_CC_EVENT_WRITABLE_, events);
        break;
    case _CC_EVENT_CONNECT_:
        /*connect*/
        if (NT_SUCCESS(overlapped->Internal) &&
            _cc_setsockopt(e->fd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0) == 0) {
            _CC_SET_BIT(_CC_EVENT_CONNECTED_, events);
            _CC_MODIFY_BIT(_CC_EVENT_READABLE_, _CC_EVENT_CONNECT_, e->flags);
        } else {
            _CC_SET_BIT(_CC_EVENT_DISCONNECT_, events);
        }
        break;
    default:
        events = _CC_EVENT_DISCONNECT_;
        break;
    }

    _cc_iocp_overlapped_free(cycle->priv, iocp_overlaaped);

    return _cc_event_callback(cycle, e, events);
}

/**/
_CC_API_PRIVATE(void) _reset_event(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    if (_CC_ISSET_BIT(_CC_EVENT_DISCONNECT_, e->flags) && _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags) == 0) {
        /*delete*/
        _cc_cleanup_event(cycle, e);
        return;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_PENDING_, e->flags)) {
        _cc_list_iterator_swap(&cycle->pending, &e->lnk);
        return;
    }

    /*update event*/
    if (_CC_ISSET_BIT(_CC_EVENT_DESC_SOCKET_, e->descriptor) &&
        _CC_EVENT_IS_SOCKET(e->flags) != _CC_EVENT_IS_SOCKET(e->marks)) {
        if (_iocp_event_update(cycle, e) == false) {
            e->callback && e->callback(cycle, e, _CC_EVENT_DISCONNECT_);
            _cc_cleanup_event(cycle, e);
            return;
        }
    }

    _cc_reset_event_timeout(cycle, e);
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_wait(_cc_event_cycle_t *cycle, uint32_t timeout) {
    ULONG_PTR key = 0;
    DWORD transferred = 0;
    LPOVERLAPPED overlapped = NULL;

    _iocp_overlapped_t *iocp_overlaaped = NULL;
    OVERLAPPED_ENTRY overlappeds[_CC_IOCP_EVENTS_] = {0};

    LPFN_GETQUEUEDCOMPLETIONSTATUSEX get_queued_completion_status_fn = get_queued_completion_status_func_ptr();

    ULONG results_count = 0, i;
    BOOL result;

    int32_t last_error;

    _cc_assert(IOCPPort != NULL);
    if (_cc_unlikely(IOCPPort == NULL)) {
        return false;
    }

    if (cycle->diff > 0) {
        timeout -= cycle->diff;
    }

    /**/
    _cc_reset_pending_event(cycle, _reset_event);

    if (get_queued_completion_status_fn) {
        result = get_queued_completion_status_fn(IOCPPort, overlappeds, _cc_countof(overlappeds), &results_count,
                                                 timeout, false);
        if (_cc_likely(result)) {
            for (i = 0; i < results_count; i++) {
                key = overlappeds[i].lpCompletionKey;
                overlapped = overlappeds[i].lpOverlapped;

                /*exist work thread*/
                if (key == _CC_IOCP_EXIT_ && overlapped == NULL) {
                    return false;
                }

                if (overlapped == NULL)
                    continue;
                /**/
                iocp_overlaaped = _cc_upcast(overlapped, _iocp_overlapped_t, overlapped);

                /**/
                if (iocp_overlaaped->e == NULL || iocp_overlaaped->e->ident != iocp_overlaaped->ident) {
                    _cc_iocp_overlapped_free(cycle->priv, iocp_overlaaped);
                    continue;
                }

                _iocp_event_dispatch(cycle, overlapped, iocp_overlaaped);
            }
        } else {
            last_error = _cc_last_errno();
            if (last_error != WAIT_TIMEOUT) {
                _cc_logger_error(_T("GetQueuedCompletionStatusEx %d errorCode: %i, %s\n"), results_count, last_error,
                                 _cc_last_error(last_error));
                for (i = 0; i < results_count; i++) {
                    key = overlappeds[i].lpCompletionKey;
                    overlapped = overlappeds[i].lpOverlapped;
                    if (overlapped) {
                        _cc_logger_error(_T("overlapped error: %d\n"), i);
                        _cc_iocp_overlapped_free(cycle->priv, _cc_upcast(overlapped, _iocp_overlapped_t, overlapped));
                    }
                }
            }
        }
    } else {
        result = GetQueuedCompletionStatus(IOCPPort, &transferred, (PULONG_PTR)&key, &overlapped, timeout);

        /*exist work thread*/
        if (key == _CC_IOCP_EXIT_ && overlapped == NULL) {
            return false;
        }

        if (!result) {
            last_error = _cc_last_errno();
            /**/
            if (overlapped) {
                iocp_overlaaped = _cc_upcast(overlapped, _iocp_overlapped_t, overlapped);
                _cc_iocp_overlapped_free(cycle->priv, iocp_overlaaped);
            }

            /**/
            if (last_error != ERROR_NETNAME_DELETED && last_error != WAIT_TIMEOUT) {
                _cc_logger_error(_T("GetQueuedCompletionStatus errorCode: %i, %s\n"), last_error,
                                 _cc_last_error(last_error));
            }
        } else if (overlapped) {
            /**/
            iocp_overlaaped = _cc_upcast(overlapped, _iocp_overlapped_t, overlapped);
            /**/
            if (iocp_overlaaped->e == NULL || iocp_overlaaped->e->ident != iocp_overlaaped->ident) {
                _cc_iocp_overlapped_free(cycle->priv, iocp_overlaaped);
            } else {
                _iocp_event_dispatch(cycle, overlapped, iocp_overlaaped);
            }
        }
    }

    _cc_update_event_timeout(cycle, timeout);

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_quit(_cc_event_cycle_t *cycle) {
    int32_t i = 0;
    _cc_assert(cycle != NULL);
    if (cycle == NULL) {
        return false;
    }

    /**/
    if (cycle->priv) {
        if (cycle->priv->port) {
            CloseHandle(cycle->priv->port);
        }

        _cc_iocp_overlapped_quit(cycle->priv);
        _cc_free(cycle->priv);
    }
    return _cc_event_cycle_quit(cycle);
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_init(_cc_event_cycle_t *cycle) {
    _cc_event_cycle_priv_t *priv;

    if (!_cc_event_cycle_init(cycle)) {
        return false;
    }

    priv = (_cc_event_cycle_priv_t *)_cc_malloc(sizeof(_cc_event_cycle_priv_t));
    priv->port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (priv->port == NULL) {
        _cc_logger_error(_T("CreateIoCompletionPort Error Code:%d."), _cc_last_errno());
        return false;
    }

    _cc_iocp_overlapped_init(priv);

    cycle->priv = priv;

    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_init_event_iocp(_cc_event_cycle_t *cycle) {
#define ASET(x) cycle->driver.x = _iocp_event_##x
#define XSET(x) cycle->driver.x = _cc_event_##x

    if (!_iocp_event_init(cycle)) {
        return false;
    }

    ASET(attach);
    ASET(connect);
    XSET(disconnect);
    ASET(accept);
    ASET(wait);
    ASET(quit);

    cycle->cleanup = _iocp_event_cleanup;

#undef ASET
#undef XSET

    return true;
}

#endif