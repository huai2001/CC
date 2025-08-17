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

static CHAR _wsa_buf[8] = {0};

/**/
int _WSA_socket_accept(_iocp_overlapped_t *overlapped) {
    int result = NO_ERROR;
    LPFN_ACCEPTEX accept_func_ptr = nullptr;
    WSABUF WSABuf;
    WSABuf.buf = _wsa_buf;
    WSABuf.len = 0;
    
    _cc_assert(overlapped != nullptr && overlapped->e != nullptr);

    accept_func_ptr = get_accept_func_ptr(overlapped->fd);
    if (accept_func_ptr && !accept_func_ptr(overlapped->e->fd, overlapped->fd, WSABuf.buf, 0, sizeof(SOCKADDR_IN) + 16,
                                            sizeof(SOCKADDR_IN) + 16, nullptr, &overlapped->overlapped)) {
        result = _cc_last_errno();
        if (result == WSA_IO_PENDING) {
            return NO_ERROR;
        }
    }

    return result;
}

/**/
int _WSA_socket_send(_iocp_overlapped_t *overlapped) {
    int result = NO_ERROR;
    DWORD dwNumberOfByteSent = 0;
    WSABUF WSABuf;
    WSABuf.buf = _wsa_buf;
    WSABuf.len = 0;
    _cc_assert(overlapped != nullptr && overlapped->e != nullptr);

    if (WSASend(overlapped->e->fd, &WSABuf, 1, &dwNumberOfByteSent, 0, &overlapped->overlapped, nullptr) ==
        SOCKET_ERROR) {
        result = _cc_last_errno();
        if (result == WSA_IO_PENDING) {
            return NO_ERROR;
        }
    }

    return result;
}

/**/
int _WSA_socket_receive(_iocp_overlapped_t *overlapped) {
    int result = NO_ERROR;
    DWORD dwFlag = 0, dwNumberOfByteRecvd = 0;
    WSABUF WSABuf;
    WSABuf.buf = _wsa_buf;
    WSABuf.len = 0;
    _cc_assert(overlapped != nullptr && overlapped->e != nullptr);

    if (WSARecv(overlapped->e->fd, &WSABuf, 1, &dwNumberOfByteRecvd, &dwFlag, &overlapped->overlapped, nullptr) ==
        SOCKET_ERROR) {
        result = _cc_last_errno();
        if (result == WSA_IO_PENDING) {
            return NO_ERROR;
        }
    }

    return result;
}

/**/
int _WSA_socket_sendto(_iocp_overlapped_t *overlapped, _cc_sockaddr_t *sa, _cc_socklen_t sa_len) {
    int result = NO_ERROR;
    DWORD dwNumberOfByteSent = 0;
    WSABUF WSABuf;
    WSABuf.buf = _wsa_buf;
    WSABuf.len = 0;
    _cc_assert(overlapped != nullptr && overlapped->e != nullptr);

    if (WSASendTo(overlapped->e->fd, &WSABuf, 1, &dwNumberOfByteSent, 0, (struct sockaddr *)sa, sa_len,
                  &overlapped->overlapped, nullptr) ==
        SOCKET_ERROR) {
        result = _cc_last_errno();
        if (result == WSA_IO_PENDING) {
            return NO_ERROR;
        }
    }

    return result;
}

/**/
int _WSA_socket_receivefrom(_iocp_overlapped_t *overlapped, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len) {
    int result = NO_ERROR;
    DWORD dwFlag = 0, dwNumberOfByteRecvd = 0;
    WSABUF WSABuf;
    WSABuf.buf = _wsa_buf;
    WSABuf.len = 0;
    _cc_assert(overlapped != nullptr && overlapped->e != nullptr);

    if (WSARecvFrom(overlapped->e->fd, &WSABuf, 1, &dwNumberOfByteRecvd, &dwFlag, (struct sockaddr *)&sa, sa_len,
                    &overlapped->overlapped, nullptr) == SOCKET_ERROR) {
        result = _cc_last_errno();
        if (result == WSA_IO_PENDING) {
            return NO_ERROR;
        }
    }

    return result;
}