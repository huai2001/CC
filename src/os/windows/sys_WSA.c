#include "sys_iocp.h"

static CHAR _wsa_buf[8] = {0};

/**/
int _WSA_socket_accept(const _cc_event_t *e, _io_context_t *overlapped) {
    int result = NO_ERROR;
    LPFN_ACCEPTEX accept_func_ptr = NULL;
    WSABUF WSABuf;
    WSABuf.buf = _wsa_buf;
    WSABuf.len = 0;
    
    _cc_assert(overlapped != NULL && e != NULL);

    accept_func_ptr = get_accept_func_ptr(overlapped->fd);
    if (accept_func_ptr && !accept_func_ptr(e->fd, overlapped->fd, WSABuf.buf, 0, sizeof(SOCKADDR_IN) + 16,
                                            sizeof(SOCKADDR_IN) + 16, NULL, &overlapped->overlapped)) {
        result = _cc_last_errno();
        if (result == WSA_IO_PENDING) {
            return NO_ERROR;
        }
    }

    return result;
}

/**/
int _WSA_socket_send(const _cc_event_t *e, _io_context_t *overlapped) {
    int result = NO_ERROR;
    DWORD dwNumberOfByteSent = 0;
    WSABUF WSABuf;
    WSABuf.buf = _wsa_buf;
    WSABuf.len = 0;
    _cc_assert(overlapped != NULL && e != NULL);

    if (WSASend(e->fd, &WSABuf, 1, &dwNumberOfByteSent, 0, &overlapped->overlapped, NULL) ==
        SOCKET_ERROR) {
        result = _cc_last_errno();
        if (result == WSA_IO_PENDING) {
            return NO_ERROR;
        }
    }

    return result;
}

/**/
int _WSA_socket_receive(const _cc_event_t *e, _io_context_t *overlapped) {
    int result = NO_ERROR;
    DWORD dwFlag = 0, dwNumberOfByteRecvd = 0;
    WSABUF WSABuf;
    WSABuf.buf = _wsa_buf;
    WSABuf.len = 0;
    _cc_assert(overlapped != NULL && e != NULL);

    if (WSARecv(e->fd, &WSABuf, 1, &dwNumberOfByteRecvd, &dwFlag, &overlapped->overlapped, NULL) ==
        SOCKET_ERROR) {
        result = _cc_last_errno();
        if (result == WSA_IO_PENDING) {
            return NO_ERROR;
        }
    }

    return result;
}

/**/
int _WSA_socket_sendto(const _cc_event_t *e, _io_context_t *overlapped, _cc_sockaddr_t *sa, _cc_socklen_t sa_len) {
    int result = NO_ERROR;
    DWORD dwNumberOfByteSent = 0;
    WSABUF WSABuf;
    WSABuf.buf = _wsa_buf;
    WSABuf.len = 0;
    _cc_assert(overlapped != NULL && e != NULL);

    if (WSASendTo(e->fd, &WSABuf, 1, &dwNumberOfByteSent, 0, (struct sockaddr *)sa, sa_len,
                  &overlapped->overlapped, NULL) ==
        SOCKET_ERROR) {
        result = _cc_last_errno();
        if (result == WSA_IO_PENDING) {
            return NO_ERROR;
        }
    }

    return result;
}

/**/
int _WSA_socket_receivefrom(const _cc_event_t *e, _io_context_t *overlapped, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len) {
    int result = NO_ERROR;
    DWORD dwFlag = 0, dwNumberOfByteRecvd = 0;
    WSABUF WSABuf;
    WSABuf.buf = _wsa_buf;
    WSABuf.len = 0;
    _cc_assert(overlapped != NULL && e != NULL);

    if (WSARecvFrom(e->fd, &WSABuf, 1, &dwNumberOfByteRecvd, &dwFlag, (struct sockaddr *)&sa, sa_len,
                    &overlapped->overlapped, NULL) == SOCKET_ERROR) {
        result = _cc_last_errno();
        if (result == WSA_IO_PENDING) {
            return NO_ERROR;
        }
    }

    return result;
}