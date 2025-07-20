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

static CHAR _wsa_buf[64] = {0};

/**/
int _WSA_socket_accept(_cc_event_t *e, _cc_socket_t fd, LPOVERLAPPED overlapped) {
    int result = NO_ERROR;
    WSABUF WSABuf;
    LPFN_ACCEPTEX accept_func_ptr = nullptr;
    _cc_assert(overlapped != nullptr && e != nullptr);
    WSABuf.buf = _wsa_buf;
    WSABuf.len = 0;
    accept_func_ptr = get_accept_func_ptr(e->fd);
    if (accept_func_ptr && !accept_func_ptr(e->fd, fd, WSABuf.buf, 0, sizeof(SOCKADDR_IN) + 16,
                                            sizeof(SOCKADDR_IN) + 16, nullptr, overlapped)) {
        result = _cc_last_errno();
        if (result == WSA_IO_PENDING) {
            return NO_ERROR;
        }
    }

    return result;
}

/**/
int _WSA_socket_send(_cc_event_t *e, LPOVERLAPPED overlapped) {
    int result = NO_ERROR;
    DWORD dwNumberOfByteSent = 0;
    WSABUF WSABuf;
    _cc_assert(overlapped != nullptr && e != nullptr);
    WSABuf.buf = _wsa_buf;
    WSABuf.len = 0;

    if (WSASend(e->fd, &WSABuf, 1, &dwNumberOfByteSent, 0, overlapped, nullptr) == SOCKET_ERROR) {
        result = _cc_last_errno();
        if (result == WSA_IO_PENDING) {
            return NO_ERROR;
        }
    }

    return result;
}

/**/
int _WSA_socket_receive(_cc_event_t *e, LPOVERLAPPED overlapped) {
    int result = NO_ERROR;
    WSABUF WSABuf;
    DWORD dwFlag = 0, dwNumberOfByteRecvd = 0;
    _cc_assert(overlapped != nullptr && e != nullptr);
    WSABuf.buf = _wsa_buf;
    WSABuf.len = 0;

    if (WSARecv(e->fd, &WSABuf, 1, &dwNumberOfByteRecvd, &dwFlag, overlapped, nullptr) == SOCKET_ERROR) {
        result = _cc_last_errno();
        if (result == WSA_IO_PENDING) {
            return NO_ERROR;
        }
    }

    return result;
}

/**/
int _WSA_socket_sendto(_cc_event_t *e, LPOVERLAPPED overlapped, _cc_sockaddr_t *sa, _cc_socklen_t sa_len) {
    int result = NO_ERROR;
    WSABUF WSABuf;
    DWORD dwNumberOfByteSent = 0;
    _cc_assert(overlapped != nullptr && e != nullptr);
    WSABuf.buf = _wsa_buf;
    WSABuf.len = 0;

    if (WSASendTo(e->fd, &WSABuf, 1, &dwNumberOfByteSent, 0, (struct sockaddr *)sa, sa_len, overlapped, nullptr) ==
        SOCKET_ERROR) {
        result = _cc_last_errno();
        if (result == WSA_IO_PENDING) {
            return NO_ERROR;
        }
    }

    return result;
}

/**/
int _WSA_socket_receivefrom(_cc_event_t *e, LPOVERLAPPED overlapped, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len) {
    int result = NO_ERROR;
    WSABUF WSABuf;
    DWORD dwFlag = 0, dwNumberOfByteRecvd = 0;
    _cc_assert(overlapped != nullptr && e != nullptr);
    WSABuf.buf = _wsa_buf;
    WSABuf.len = 0;

    if (WSARecvFrom(e->fd, &WSABuf, 1, &dwNumberOfByteRecvd, &dwFlag, (struct sockaddr *)&sa, sa_len, overlapped,
                    nullptr) == SOCKET_ERROR) {
        result = _cc_last_errno();
        if (result == WSA_IO_PENDING) {
            return NO_ERROR;
        }
    }

    return result;
}