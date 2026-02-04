#include <libcc/alloc.h>
#include <libcc/logger.h>
#include <libcc/thread.h>
#include <libcc/timeout.h>
#include "sys_iocp.h"

#ifdef _CC_EVENT_USE_IOCP_

#ifndef NT_SUCCESS
#define NT_SUCCESS(status) (((status)) >= 0)
#endif

#define _CC_IOCP_EVENTS_    _CC_MAX_CHANGE_EVENTS_

#define IOCPPort (async->priv->port)
/**/
/*close socket*/
/**/
_CC_API_PRIVATE(void) _event_cleanup(_cc_async_event_t *async, _cc_event_t *e) {
    SOCKET fd = (SOCKET)e->accept_fd;
    if (fd != INVALID_SOCKET) {
        /**/
        LINGER linger = {1, 0};

        setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger));

        /*Now close the socket handle.  This will do an abortive or  graceful close,
         * as requested.*/
        CancelIo((HANDLE)fd);
        _cc_close_socket(e->accept_fd);
    }
    _cc_free_event(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_accept_event(_io_context_t *io_context) {
    int result;

    io_context->flag = _CC_EVENT_ACCEPT_;
    result = _WSA_socket_accept(io_context);
    if (NO_ERROR != result) {
        _CC_UNSET_BIT(_CC_EVENT_ACCEPT_, io_context->e->filter);
        _cc_logger_warin("_WSA_socket_accept:%d, %s", result, _cc_last_error(result));
        return false;
    }

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_write_event(_io_context_t *io_context) {
    int result;

    io_context->flag = _CC_EVENT_WRITABLE_;
    result = _WSA_socket_send(io_context);
    if (result != NO_ERROR) {
        _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, io_context->e->filter);
        _cc_logger_error("WSASend fail:%d, %s", result, _cc_last_error(result));
        return false;
    }

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_receive_event(_io_context_t *io_context) {
    int result;

    io_context->flag = _CC_EVENT_READABLE_;
    result = _WSA_socket_receive(io_context);
    if (result != NO_ERROR) {
        _CC_UNSET_BIT(_CC_EVENT_READABLE_, io_context->e->filter);
        _cc_logger_error("_WSAReceive fail:%d, %s", result, _cc_last_error(result));
        return false;
    }

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _emit_iocp_event(_cc_async_event_t *async, _cc_event_t *e) {
    _io_context_t *io_context;
    uint32_t addevents = e->flags & ~e->filter;
    uint32_t delevents = ~e->flags & e->filter;

    _CC_MODIFY_BIT(addevents, delevents, e->filter);

    if (_CC_ISSET_BIT(_CC_EVENT_ACCEPT_, addevents) != 0) {
        _cc_socket_t fd = _CC_INVALID_SOCKET_;

        if (_CC_ISSET_BIT(_CC_EVENT_CLOSED_, e->flags)) {
            return true;
        }

        fd = (_cc_socket_t)WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (fd == _CC_INVALID_SOCKET_) {
            int result = _cc_last_errno();
            _cc_logger_error("WSASocket fail:%d, %s", result, _cc_last_error(result));
            return false;
        }

        io_context = _io_context_alloc(async->priv, e);
        io_context->fd = fd;

        if (_iocp_event_accept_event(io_context)) {
            return true;
        }

        _cc_close_socket(fd);
        _io_context_free(async->priv, io_context);
        return false;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_, addevents)) {
        io_context = _io_context_alloc(async->priv, e);

        if (!_iocp_event_write_event(io_context)) {
            _io_context_free(async->priv, io_context);
            return false;
        }
    }

    if (_CC_ISSET_BIT(_CC_EVENT_READABLE_, addevents)) {
        io_context = _io_context_alloc(async->priv, e);

        if (!_iocp_event_receive_event(io_context)) {
            _io_context_free(async->priv, io_context);
            return false;
        }
    }
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_reset(_cc_async_event_t *async, _cc_event_t *e) {
    _io_context_t *io_context = _io_context_alloc(async->priv, e);
    io_context->flag = _CC_EVENT_PENDING_;

    if (PostQueuedCompletionStatus(IOCPPort, 0, _CC_IOCP_PENDING_, &io_context->overlapped)) {
        return true;
    }

    _io_context_free(async->priv, io_context);
    return false;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_attach(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_assert(async != NULL && e != NULL);

    if (async->running == 0) {
        return false;
    }

    if (_CC_EVENT_IS_SOCKET(e->flags)) {
        if (CreateIoCompletionPort((HANDLE)(uintptr_t)e->fd, IOCPPort, _CC_IOCP_SOCKET_, 0) == NULL) {
            _cc_logger_error("CreateIoCompletionPort Error Code:%d.", _cc_last_errno());
            return false;
        }
    } else if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, e->flags) == 0) {
        return false;
    }

    e->filter = _CC_EVENT_UNKNOWN_;

    return _iocp_event_reset(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_bind(_cc_async_event_t *async, const _cc_event_t *e, int family) {
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
        _cc_logger_error("bind Error Code:%d. %s",err, _cc_last_error(err));
        return false;
    }

    if (CreateIoCompletionPort((HANDLE)(uintptr_t)e->fd, IOCPPort, _CC_IOCP_SOCKET_, 0) == NULL) {
        int err = _cc_last_errno();
        _cc_logger_error("CreateIoCompletionPort Error Code:%d. %s",err, _cc_last_error(err));
        return false;
    }
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_connect(_cc_async_event_t *async, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len) {
    _io_context_t *io_context = NULL;

    LPFN_CONNECTEX connect_fn = get_connectex_func_ptr(e->fd);
    if (_cc_unlikely(connect_fn == NULL)) {
        return false;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_CONNECT_, e->flags) == 0) {
        return false;
    }

    if (!_iocp_bind(async, e, sa->sa_family)) {
        return false;
    }

    io_context = _io_context_alloc(async->priv, e);
    io_context->flag = _CC_EVENT_CONNECT_;

    if (!connect_fn(e->fd, (struct sockaddr *)sa, sa_len, NULL, 0, NULL, &io_context->overlapped)) {
        int err = _cc_last_errno();
        if (err != WSA_IO_PENDING) {
            _io_context_free(async->priv, io_context);
            _cc_logger_error("Socket Connect:(%d) %s", err, _cc_last_error(err));
            return false;
        }
    }

    return true;
}

_CC_API_PRIVATE(bool_t) _iocp_event_disconnect(_cc_async_event_t *async, _cc_event_t *e) {
    /*int result = 0;
    DWORD dwFlags = TF_REUSE_SOCKET;
    DWORD reserved = 0;
    _io_context_t *io_context = NULL;

    LPFN_DISCONNECTEX disconnect_fn = get_disconnect_func_ptr(e->fd);
    if (disconnect_fn == NULL) {
        return false;
    }

    _CC_SET_BIT(_CC_EVENT_CLOSED_, e->filter);
    _CC_MODIFY_BIT(_CC_EVENT_CLOSED_, _CC_EVENT_READABLE_, e->flags);

    io_context = _io_context_alloc(async->priv, e)
    io_context->flag = _CC_EVENT_CLOSED_;

    if (disconnect_fn(e->fd, &io_context->overlapped, dwFlags, reserved) == false) {
        int err = _cc_last_errno();
        if (err != WSA_IO_PENDING) {
            _io_context_free(async->priv, io_context);
            _cc_logger_error("Socket Disconnect:(%d) %s\n", err, _cc_last_error(err));
            return false;
        }
    }*/

    return _disconnect_event(async, e);
}

/**/
_CC_API_PRIVATE(_cc_socket_t) _iocp_event_accept(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len) {
    if (_cc_unlikely(e->accept_fd != _CC_INVALID_SOCKET_)) {
        SOCKET accept_fd = (SOCKET)e->accept_fd;
        SOCKET fd = e->fd;
        /* SO_UPDATE_ACCEPT_CONTEXT is required for shutdown() to work fine*/
        /* sizeof(SOCKET) is 8 */
        setsockopt(accept_fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&fd, sizeof(SOCKET));

        _cc_assert(sa != NULL);
        _cc_assert(sa_len != NULL);

        if (sa && sa_len && getpeername(accept_fd, (struct sockaddr *)sa, sa_len) == -1) {
            int32_t err = _cc_last_errno();
            _cc_logger_warin("discovery client information failed, fd=%d, errno=%d(%#x).", accept_fd, err, err);
        }

        e->accept_fd = _CC_INVALID_SOCKET_;
        return (_cc_socket_t)accept_fd;
    } else {
        _cc_logger(_CC_LOG_LEVEL_ERROR_, "Listening object is null");
    }
    return _CC_INVALID_SOCKET_;
}

/**/
_CC_API_PRIVATE(void) _iocp_handle_entry(_cc_async_event_t *async, _io_context_t *io_context) {
    _cc_event_t *e = io_context->e;
    uint32_t which = _CC_EVENT_IS_SOCKET(io_context->flag);

    if (_CC_ISSET_BIT(_CC_EVENT_CLOSED_, e->flags)) {
        if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags) == 0 || _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, io_context->flag) == 0) {
            return ;
        }
        if (!_valid_fd(e->fd)) {
            return ;
        }
    }

    _CC_UNSET_BIT(io_context->flag, e->filter);
    if (io_context->flag == _CC_EVENT_ACCEPT_) {
        e->accept_fd = io_context->fd;
        io_context->fd = _CC_INVALID_SOCKET_;
    } else if (io_context->flag == _CC_EVENT_CONNECT_) {
        if (NT_SUCCESS(io_context->overlapped.Internal)) {
            _cc_setsockopt(e->fd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
        } else {
            which = _CC_EVENT_CLOSED_;
        }
        _CC_UNSET_BIT(_CC_EVENT_CONNECT_, e->flags);
    }
    
    if (which) {
        _event_callback(async, e, which);
    }
}

/**/
_CC_API_PRIVATE(void) _reset(_cc_async_event_t *async, _cc_event_t *e) {
    if (_CC_ISSET_BIT(_CC_EVENT_CLOSED_, e->flags) && _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags) == 0) {
        /*delete*/
        _event_cleanup(async, e);
        return;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_PENDING_, e->flags)) {
        if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, e->flags) == 0) {
            _cc_list_swap(&async->pending, &e->lnk);
            return;
        }
    } else {
        /*update event*/
        uint32_t u = _CC_EVENT_IS_SOCKET(e->flags);
        if (u && u != _CC_EVENT_IS_SOCKET(e->filter)) {
            _emit_iocp_event(async, e);
        }
    }
    /*update event*/
    _reset_event_timeout(async, e);
}

_io_context_t* _iocp_upcast(_cc_async_event_t *async, LPOVERLAPPED overlapped) {
    _io_context_t *io_context;
    if (overlapped == NULL) {
        return NULL;
    }

    /**/
    io_context = _cc_upcast(overlapped, _io_context_t, overlapped);

    /**/
    if (io_context->e != NULL && io_context->e->ident == io_context->ident) {
        return io_context;
    }

    _io_context_free(async->priv, io_context);
    return NULL;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_wait(_cc_async_event_t *async, uint32_t timeout) {
    ULONG_PTR key = 0;
    LPOVERLAPPED overlapped = NULL;
    DWORD number_of_bytes;
    _io_context_t *io_context = NULL;
#if (_WIN32_WINNT >= 0x0600)
    OVERLAPPED_ENTRY entries[_CC_IOCP_EVENTS_] = {0};
    ULONG number_of_entries = 0, i;
#endif
    BOOL result;

    _cc_assert(IOCPPort != NULL);
    if (_cc_unlikely(IOCPPort == NULL)) {
        return false;
    }

    if (async->diff > 0) {
        timeout = (async->diff >= timeout) ? 0 : (timeout - async->diff);
    }

    _reset_event_pending(async, _reset);

#if (_WIN32_WINNT >= 0x0600)
    //API: GetQueuedCompletionStatusEx
    result = GetQueuedCompletionStatusEx(IOCPPort, entries, _cc_countof(entries), &number_of_entries, timeout, false);
    if (result) {
        for (i = 0; i < number_of_entries; i++) {
            key = entries[i].lpCompletionKey;
            overlapped = entries[i].lpOverlapped;
            number_of_bytes = entries[i].dwNumberOfBytesTransferred;
            /*exist work thread*/
            if (key == _CC_IOCP_EXIT_) {
                return (bool_t)result;
            }
            /**/
            io_context = _iocp_upcast(async, overlapped);
            if (io_context) {
                io_context->number_of_bytes = number_of_bytes;
                if (key == _CC_IOCP_PENDING_) {
                    _reset(async, io_context->e);
                } else {
                    _iocp_handle_entry(async, io_context);
                }
                _io_context_free(async->priv, io_context);
            }
        }
    }
#else
    //API: GetQueuedCompletionStatus
    result = GetQueuedCompletionStatus(IOCPPort, &number_of_bytes, (PULONG_PTR)&key, &overlapped, timeout);
    if (result) {
        /*exist work thread*/
        if (key == _CC_IOCP_EXIT_) {
            return (bool_t)result;
        }
        io_context = _iocp_upcast(async, overlapped);
        if (io_context) {
            io_context->number_of_bytes = number_of_bytes;
            if (key == _CC_IOCP_PENDING_) {
                _reset(async, io_context->e);
            } else {
                _iocp_handle_entry(async, io_context);
            }
            _io_context_free(async->priv, io_context);
        }
    }
#endif

    if (!result) {
        int32_t last_error = _cc_last_errno();
#if (_WIN32_WINNT >= 0x0600)
        if (last_error != WAIT_TIMEOUT) {
            _cc_logger_error("GetQueuedCompletionStatusEx errorCode: %i, %s", last_error, _cc_last_error(last_error));
        }
#else
        if (last_error != ERROR_NETNAME_DELETED && last_error != WAIT_TIMEOUT) {
            _cc_logger_error("GetQueuedCompletionStatus errorCode: %i, %s", last_error, _cc_last_error(last_error));
        }
#endif
    }
    _update_event_timeout(async, timeout);
    return result;
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_free(_cc_async_event_t *async) {
    _cc_assert(async != NULL);
    if (async == NULL) {
        return false;
    }
    /**/
    if (async->priv) {
        if (async->priv->port) {
            CloseHandle(async->priv->port);
        }
        _io_context_quit(async->priv);
        _cc_free(async->priv);
    }
    return _unregister_async_event(async);
}

/**/
_CC_API_PRIVATE(bool_t) _iocp_event_alloc(_cc_async_event_t *async) {
    _cc_async_event_priv_t *priv;

    if (!_register_async_event(async)) {
        return false;
    }
    
    priv = (_cc_async_event_priv_t *)_cc_malloc(sizeof(_cc_async_event_priv_t));
    priv->port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (priv->port == NULL) {
        _cc_logger_error("CreateIoCompletionPort Error Code:%d.", _cc_last_errno());
        return false;
    }

    _io_context_init(priv);

    async->priv = priv;
    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_register_iocp(_cc_async_event_t *async) {
    if (!_iocp_event_alloc(async)) {
        return false;
    }
    async->attach = _iocp_event_attach;
    async->connect = _iocp_event_connect;
    async->disconnect = _iocp_event_disconnect;
    async->accept = _iocp_event_accept;
    async->wait = _iocp_event_wait;
    async->free = _iocp_event_free;
    async->reset = _iocp_event_reset;

    return true;
}

#endif