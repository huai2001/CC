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
#include "sys_socket_c.h"
#include <libcc/alloc.h>
#include <libcc/logger.h>
#include <libcc/thread.h>
#include <libcc/widgets/timeout.h>

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
/*close socket*/
/**/
_CC_API_PRIVATE(void) _iocp_event_cleanup(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    if (e->accept_fd != _CC_INVALID_SOCKET_) {
        /**/
        LINGER linger = {1, 0};

        setsockopt(e->accept_fd, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger));

        /*Now close the socket handle.  This will do an abortive or  graceful close,
         * as requested.*/
        CancelIo((HANDLE)e->accept_fd);
        _cc_close_socket(e->accept_fd);
    }
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_accept_event(_iocp_overlapped_t *iocp_overlapped) {
    int result;

    iocp_overlapped->flag = _CC_EVENT_ACCEPT_;
    result = _WSA_socket_accept(iocp_overlapped);
    if (_cc_unlikely(NO_ERROR != result)) {
        _CC_UNSET_BIT(_CC_EVENT_ACCEPT_, iocp_overlapped->e->marks);
        _cc_logger_warin(_T("_WSA_socket_accept:%d, %s\n"), result, _cc_last_error(result));
        return false;
    }

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_write_event(_iocp_overlapped_t *iocp_overlapped) {
    int result;

    if (!_valid_event_fd(iocp_overlapped->e)) {
        return false;
    }

    iocp_overlapped->flag = _CC_EVENT_WRITABLE_;
    result = _WSA_socket_send(iocp_overlapped);
    if (_cc_unlikely(result != NO_ERROR)) {
        _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, iocp_overlapped->e->marks);
        _cc_logger_error(_T("WSASend fail:%d, %s\n"), result, _cc_last_error(result));
        return false;
    }

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_receive_event(_iocp_overlapped_t *iocp_overlapped) {
    int result;

    if (!_valid_event_fd(iocp_overlapped->e)) {
        return false;
    }
    iocp_overlapped->flag = _CC_EVENT_READABLE_;
    result = _WSA_socket_receive(iocp_overlapped);
    if (_cc_unlikely(result != NO_ERROR)) {
        _CC_UNSET_BIT(_CC_EVENT_READABLE_, iocp_overlapped->e->marks);
        _cc_logger_error(_T("_WSAReceive fail:%d, %s\n"), result, _cc_last_error(result));
        return false;
    }

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_update(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _iocp_overlapped_t *iocp_overlapped = nullptr;
    uint16_t addevents = e->flags & ~e->marks;
    uint16_t delevents = ~e->flags & e->marks;

    _CC_MODIFY_BIT(addevents, delevents, e->marks);

    if (_CC_ISSET_BIT(_CC_EVENT_ACCEPT_, addevents) != 0) {
        _cc_socket_t fd = _CC_INVALID_SOCKET_;

        if (_CC_ISSET_BIT(_CC_EVENT_DISCONNECT_, e->flags)) {
            return true;
        }

        fd = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
        if (_cc_unlikely(fd == _CC_INVALID_SOCKET_)) {
            int result = _cc_last_errno();
            _cc_logger_error(_T("WSASocket fail:%d, %s\n"), result, _cc_last_error(result));
            return false;
        }

        iocp_overlapped = _iocp_overlapped_alloc(cycle->priv, e);
        iocp_overlapped->fd = fd;

        if (_iocp_event_accept_event(iocp_overlapped)) {
            return true;
        }

        _cc_close_socket(fd);
        _iocp_overlapped_free(cycle->priv, iocp_overlapped);
        return false;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_, addevents)) {
        iocp_overlapped = _iocp_overlapped_alloc(cycle->priv, e);

        if (!_iocp_event_write_event(iocp_overlapped)) {
            _iocp_overlapped_free(cycle->priv, iocp_overlapped);
            return false;
        }
    }

    if (_CC_ISSET_BIT(_CC_EVENT_READABLE_, addevents)) {
        iocp_overlapped = _iocp_overlapped_alloc(cycle->priv, e);

        if (!_iocp_event_receive_event(iocp_overlapped)) {
            _iocp_overlapped_free(cycle->priv, iocp_overlapped);
            return false;
        }
    }
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_reset(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _iocp_overlapped_t *iocp_overlapped = _iocp_overlapped_alloc(cycle->priv, e);
    iocp_overlapped->flag = _CC_EVENT_PENDING_;

    if (PostQueuedCompletionStatus(IOCPPort, 0, _CC_IOCP_PENDING_, &iocp_overlapped->overlapped)) {
        return true;
    }

    _iocp_overlapped_free(cycle->priv, iocp_overlapped);
    return false;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_attach(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_assert(cycle != nullptr && e != nullptr);

    if (cycle->running == 0) {
        return false;
    }

    e->descriptor = _CC_EVENT_DESC_POLL_IOCP_ | (e->descriptor & 0xff);

    if (_CC_ISSET_BIT(_CC_EVENT_DESC_SOCKET_, e->descriptor)) {
        if (CreateIoCompletionPort((HANDLE)e->fd, IOCPPort, _CC_IOCP_SOCKET_, 0) == nullptr) {
            _cc_logger_error(_T("CreateIoCompletionPort Error Code:%d.\n"), _cc_last_errno());
            return false;
        }
    } else if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, e->flags) == 0) {
        return false;
    }

    e->marks = _CC_EVENT_UNKNOWN_;

    return _iocp_event_reset(cycle, e);
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_bind(_cc_event_cycle_t *cycle, const _cc_event_t *e, int family) {
    _cc_socklen_t socklen = 0;
    _cc_sockaddr_t *sockaddr_any;

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

    if (CreateIoCompletionPort((HANDLE)e->fd, IOCPPort, _CC_IOCP_SOCKET_, 0) == nullptr) {
		int err = _cc_last_errno();
		_cc_logger_error(_T("CreateIoCompletionPort Error Code:%d. %s\n"),err, _cc_last_error(err));
        return false;
    }
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_connect(_cc_event_cycle_t *cycle, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len) {
    _iocp_overlapped_t *iocp_overlapped = nullptr;

    LPFN_CONNECTEX connect_fn = get_connectex_func_ptr(e->fd);
    if (_cc_unlikely(connect_fn == nullptr)) {
        return false;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_CONNECT_, e->flags) == 0) {
        return false;
    }

    if (!_iocp_bind(cycle, e, sa->sa_family)) {
        return false;
    }

    iocp_overlapped = _iocp_overlapped_alloc(cycle->priv, e);
    iocp_overlapped->flag = _CC_EVENT_CONNECT_;

    if (!connect_fn(e->fd, (struct sockaddr *)sa, sa_len, nullptr, 0, nullptr, &iocp_overlapped->overlapped)) {
        int err = _cc_last_errno();
        if (err != WSA_IO_PENDING) {
            _iocp_overlapped_free(cycle->priv, iocp_overlapped);
            _cc_logger_error(_T("Socket Connect:(%d) %s\n"), err, _cc_last_error(err));
            return false;
        }
    }

    return true;
}

_CC_API_PRIVATE(bool_t) _iocp_event_disconnect(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    /*int result = 0;
    DWORD dwFlags = TF_REUSE_SOCKET;
    DWORD reserved = 0;
    _iocp_overlapped_t *iocp_overlapped = nullptr;

    LPFN_DISCONNECTEX disconnect_fn = get_disconnect_func_ptr(e->fd);
    if (disconnect_fn == nullptr) {
        return false;
    }

    _CC_SET_BIT(_CC_EVENT_DISCONNECT_, e->marks);
    _CC_MODIFY_BIT(_CC_EVENT_DISCONNECT_, _CC_EVENT_READABLE_, e->flags);

    iocp_overlapped = _iocp_overlapped_alloc(cycle->priv)
    iocp_overlapped->fd = 0;
    iocp_overlapped->e = e;
    iocp_overlapped->ident = e->ident;
    iocp_overlapped->flag = _CC_EVENT_DISCONNECT_;

    if (disconnect_fn(e->fd, &iocp_overlapped->overlapped, dwFlags, reserved) == false) {
        int err = _cc_last_errno();
        if (err != WSA_IO_PENDING) {
            _iocp_overlapped_free(cycle->priv, iocp_overlapped);
            _cc_logger_error(_T("Socket Disconnect:(%d) %s\n"), err, _cc_last_error(err));
            return false;
        }
    }*/

    return _disconnect_event(cycle, e);
}

/**/
_CC_API_PRIVATE(_cc_socket_t) _iocp_event_accept(_cc_event_cycle_t *cycle, _cc_event_t *e, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len) {
    _cc_socket_t accept_fd = e->accept_fd;

    if (_cc_unlikely(accept_fd != _CC_INVALID_SOCKET_)) {
        e->accept_fd = _CC_INVALID_SOCKET_;
        /* SO_UPDATE_ACCEPT_CONTEXT is required for shutdown() to work fine*/
        setsockopt(accept_fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&e->fd, sizeof(_cc_socket_t));

        _cc_assert(sa != nullptr);
        _cc_assert(sa_len != nullptr);

        if (sa && sa_len && getpeername(accept_fd, (struct sockaddr *)sa, sa_len) == -1) {
            int32_t err = _cc_last_errno();
            _cc_logger_warin(_T("discovery client information failed, fd=%d, ")
                             _T("errno=%d(%#x).\n"), accept_fd, err, err);
        }  
    } else {
        _cc_logger_error(_T("Listening object is null"));
    }
    return accept_fd;
}

/**/
_CC_API_PRIVATE(void) _iocp_event_dispatch(_cc_event_cycle_t *cycle, _iocp_overlapped_t *iocp_overlapped) {
    _cc_event_t *e = iocp_overlapped->e;
    uint16_t which = _CC_EVENT_IS_SOCKET(iocp_overlapped->flag);

    _CC_UNSET_BIT(iocp_overlapped->flag, e->marks);

    if (iocp_overlapped->flag == _CC_EVENT_ACCEPT_) {
        e->accept_fd = iocp_overlapped->fd;
    } else if (iocp_overlapped->flag == _CC_EVENT_CONNECT_) {
        if (NT_SUCCESS(iocp_overlapped->overlapped.Internal) &&
            _cc_setsockopt(e->fd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0) == 0) {
            _CC_MODIFY_BIT(_CC_EVENT_READABLE_, _CC_EVENT_CONNECT_, e->flags);
        } else {
            which = _CC_EVENT_DISCONNECT_;
        }
    }

    if (which) {
        _event_callback(cycle, e, which);
    }
}

/**/
_CC_API_PRIVATE(void) _reset(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    if (_CC_ISSET_BIT(_CC_EVENT_DISCONNECT_, e->flags) && _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags) == 0) {
        /*delete*/
        _cleanup_event(cycle, e);
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
            if(e->callback) {
                e->callback(cycle, e, _CC_EVENT_DISCONNECT_);
            }
            _cleanup_event(cycle, e);
            return;
        }
    }

    _reset_event_timeout(cycle, e);
}

_iocp_overlapped_t* _iocp_upcast(_cc_event_cycle_t *cycle, LPOVERLAPPED overlapped) {
    _iocp_overlapped_t *iocp_overlapped;
    if (overlapped == nullptr) {
        return nullptr;
    }

    /**/
    iocp_overlapped = _cc_upcast(overlapped, _iocp_overlapped_t, overlapped);

    /**/
    if (iocp_overlapped->e != nullptr && iocp_overlapped->e->round == iocp_overlapped->round) {
        return iocp_overlapped;
    }

    _iocp_overlapped_free(cycle->priv, iocp_overlapped);
    return nullptr;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_wait(_cc_event_cycle_t *cycle, uint32_t timeout) {
    ULONG_PTR key = 0;
    DWORD transferred = 0;
    LPOVERLAPPED overlapped = nullptr;

    _iocp_overlapped_t *iocp_overlapped = nullptr;
    OVERLAPPED_ENTRY overlappeds[_CC_IOCP_EVENTS_] = {0};

    LPFN_GETQUEUEDCOMPLETIONSTATUSEX get_queued_completion_status_fn = get_queued_completion_status_func_ptr();

    ULONG results_count = 0, i;
    BOOL result;

    int32_t last_error;

    _cc_assert(IOCPPort != nullptr);
    if (_cc_unlikely(IOCPPort == nullptr)) {
        return false;
    }

    if (cycle->diff > 0) {
        timeout -= (uint32_t)cycle->diff;
    }

    /**/
    _reset_event_pending(cycle, _reset);

    if (get_queued_completion_status_fn) {
        result = get_queued_completion_status_fn(IOCPPort, overlappeds, _cc_countof(overlappeds), &results_count, timeout, false);
        if (result) {
            for (i = 0; i < results_count; i++) {
                key = overlappeds[i].lpCompletionKey;
                overlapped = overlappeds[i].lpOverlapped;
                /*exist work thread*/
                if (key == _CC_IOCP_EXIT_) {
                    return false;
                }
                /**/
                iocp_overlapped = _iocp_upcast(cycle,overlapped);
                if (iocp_overlapped) {
                    if (key == _CC_IOCP_PENDING_) {
                        _reset(cycle, iocp_overlapped->e);
                    } else {
                        _iocp_event_dispatch(cycle, iocp_overlapped);
                    }
                    _iocp_overlapped_free(cycle->priv, iocp_overlapped);
                }
            }
        } else {
            last_error = _cc_last_errno();
            if (last_error != WAIT_TIMEOUT) {
                _cc_logger_error(_T("GetQueuedCompletionStatusEx %d errorCode: %i, %s"), results_count, last_error, _cc_last_error(last_error));
                for (i = 0; i < results_count; i++) {
                    key = overlappeds[i].lpCompletionKey;
                    iocp_overlapped = _iocp_upcast(cycle,overlappeds[i].lpOverlapped);
                    if (iocp_overlapped) {
                        if (key == _CC_IOCP_PENDING_) {
                            _cleanup_event(cycle, iocp_overlapped->e);
                        }
                        _iocp_overlapped_free(cycle->priv, iocp_overlapped);
                    }
                    _cc_logger_error(_T("overlapped error: %d\n"), i);
                }
            }
        }
    } else {
        result = GetQueuedCompletionStatus(IOCPPort, &transferred, (PULONG_PTR)&key, &overlapped, timeout);

        /*exist work thread*/
        if (key == _CC_IOCP_EXIT_) {
            return false;
        }

        iocp_overlapped = _iocp_upcast(cycle,overlapped);
        if (result) {
            /**/
            if (iocp_overlapped) {
                if (key == _CC_IOCP_PENDING_) {
                    _reset(cycle, iocp_overlapped->e);
                } else {
                    _iocp_event_dispatch(cycle, iocp_overlapped);
                }
                _iocp_overlapped_free(cycle->priv, iocp_overlapped);
            }
        } else {
            if (iocp_overlapped) {
                if (key == _CC_IOCP_PENDING_) {
                    _cleanup_event(cycle, iocp_overlapped->e);
                }
                _iocp_overlapped_free(cycle->priv, iocp_overlapped);
            }
            last_error = _cc_last_errno();
            /**/
            if (last_error != ERROR_NETNAME_DELETED && last_error != WAIT_TIMEOUT) {
                _cc_logger_error(_T("GetQueuedCompletionStatus errorCode: %i, %s"), last_error, _cc_last_error(last_error));
            }
        }
    }

    _update_event_timeout(cycle, timeout);

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_quit(_cc_event_cycle_t *cycle) {
    _cc_assert(cycle != nullptr);
    if (cycle == nullptr) {
        return false;
    }

    /**/
    if (cycle->priv) {
        if (cycle->priv->port) {
            CloseHandle(cycle->priv->port);
        }

        _iocp_overlapped_quit(cycle->priv);
        _cc_free(cycle->priv);
    }
    return _event_cycle_quit(cycle);
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_init(_cc_event_cycle_t *cycle) {
    _cc_event_cycle_priv_t *priv;

    if (!_event_cycle_init(cycle)) {
        return false;
    }

    priv = (_cc_event_cycle_priv_t *)_cc_malloc(sizeof(_cc_event_cycle_priv_t));
    priv->port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (priv->port == nullptr) {
        _cc_logger_error(_T("CreateIoCompletionPort Error Code:%d."), _cc_last_errno());
        return false;
    }

    _iocp_overlapped_init(priv);

    cycle->priv = priv;

    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_init_event_iocp(_cc_event_cycle_t *cycle) {
    if (!_iocp_event_init(cycle)) {
        return false;
    }
    cycle->attach = _iocp_event_attach;
    cycle->connect = _iocp_event_connect;
    cycle->disconnect = _iocp_event_disconnect;
    cycle->accept = _iocp_event_accept;
    cycle->wait = _iocp_event_wait;
    cycle->quit = _iocp_event_quit;
    cycle->reset = _iocp_event_reset;
    cycle->cleanup = _iocp_event_cleanup;
    return true;
}

#endif