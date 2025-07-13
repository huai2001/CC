/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#include <libcc/atomic.h>
#include <libcc/loadso.h>
#include <libcc/logger.h>

#include <libcc/socket/socket.h>

#include <mstcpip.h>

#ifdef _CC_MSVC_
    #ifndef _WIN32_WCE
        #pragma comment(lib, "ws2_32")
    #else
        #pragma comment(lib, "ws2")
    #endif
#endif

typedef struct tcp_keepalive tcp_keepalive_t;

static _cc_atomic32_t _socket_started = 0;

static struct sockaddr_in _win_addr_ipv4_any = {0};
static struct sockaddr_in6 _win_addr_ipv6_any = {0};

static LPFN_ACCEPTEX _accept_func_ptr = nullptr;
static LPFN_GETACCEPTEXSOCKADDRS _accept_sockaddrs_func_ptr = nullptr;
static LPFN_DISCONNECTEX _disconnect_func_ptr = nullptr;
static LPFN_CONNECTEX _connectex_func_ptr = nullptr;
#if (_WIN32_WINNT < 0x0600)
static LPFN_GETQUEUEDCOMPLETIONSTATUSEX _get_queued_completion_status_func_ptr = nullptr;
#endif
static LPFN_TRANSMITFILE _transmit_file_func_ptr = nullptr;

_CC_API_PUBLIC(_cc_sockaddr_t*) _cc_win_get_ipv4_any_addr(void) {
    return (_cc_sockaddr_t *)&_win_addr_ipv4_any;
}

_CC_API_PUBLIC(_cc_sockaddr_t*) _cc_win_get_ipv6_any_addr(void) {
    return (_cc_sockaddr_t *)&_win_addr_ipv6_any;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_install_socket(void) {
    /* Start up the windows networking */
    if (_cc_atomic32_inc_ref(&_socket_started)) {
        SOCKET fd;
        WSADATA wsaData;
        /* Start up the windows networking */
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            _cc_logger_error(_T("Winsock 2.2 initialization failed: %s."), _cc_last_error(_cc_last_errno()));
            return false;
        }

        _cc_inet_ipv4_addr(&_win_addr_ipv4_any, _T("0.0.0.0"), 0);
        _cc_inet_ipv6_addr(&_win_addr_ipv6_any, _T("::"), 0);

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (_cc_likely(fd != _CC_INVALID_SOCKET_)) {
            get_accept_func_ptr(fd);
            get_accept_sockaddrs_func_ptr(fd);
            // get_transmitfile_func_ptr(fd);
            get_connectex_func_ptr(fd);
            get_disconnect_func_ptr(fd);
            get_queued_completion_status_func_ptr();

            _cc_close_socket(fd);
        }
    }
    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_uninstall_socket(void) {
    if (_cc_unlikely(_socket_started == 0)) {
        return true;
    }

    if (_cc_atomic32_dec_ref(&_socket_started)) {
        /* Clean up windows networking */
        if (WSACleanup() == SOCKET_ERROR) {
            if (_cc_last_errno() == WSAEINPROGRESS) {
#ifndef __CC_WIN32_CE__
                WSACancelBlockingCall();
#endif
                WSACleanup();
            }
        }
    }

    return true;
}

/**/
_CC_API_PUBLIC(int) _cc_close_socket(_cc_socket_t fd) {
    int request = closesocket(fd);

#ifdef _CC_DEBUG_
    if (_cc_unlikely(request == SOCKET_ERROR)) {
        int32_t err = _cc_last_errno();
        _cc_logger_error(_T(" closesocket() failed with error:%d, %s"), err, _cc_last_error(err));
    }
#endif

    return request;
}

_CC_API_PUBLIC(int) _cc_set_socket_nodelay(_cc_socket_t fd, int opt) {
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&opt, sizeof(opt)) == -1) {
        return WSAGetLastError();
    }
    return 0;
}

/**
 * Set the socket to nonblocking mode
 */
_CC_API_PUBLIC(int) _cc_set_socket_nonblock(_cc_socket_t fd, int nonblocking) {
    int flags = ioctlsocket(fd, FIONBIO, (unsigned long *)&nonblocking);
    if (_cc_unlikely(flags == SOCKET_ERROR)) {
        flags = _cc_last_errno();
        _cc_logger_error(_T("FIONBIO socket_nonblock(%d) failed with error:%d, %s "), fd, flags, _cc_last_error(flags));
    }
    return flags;
}

/**/
_CC_API_PUBLIC(int) _cc_set_socket_keepalive(_cc_socket_t fd, int opt, int delay) {
    tcp_keepalive_t klive;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&opt, sizeof opt) == -1) {
        return WSAGetLastError();
    }

    klive.onoff = 1;
    klive.keepalivetime = delay;
    klive.keepaliveinterval = delay;

    WSAIoctl(fd, SIO_KEEPALIVE_VALS, &klive, sizeof(tcp_keepalive_t), nullptr, 0, (unsigned long *)&opt, 0, nullptr);

    return 0;
}

/**/
void *get_extension_func_ptr(SOCKET sock, GUID guid) {
    DWORD dwBytes;
    PVOID pfn = nullptr;

    if (SOCKET_ERROR == WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &pfn, sizeof(pfn),
                                 &dwBytes, nullptr, nullptr)) {
        _cc_logger_error(_T("fd:%d, WSAIoctl Error:%s"), sock, _cc_last_error(_cc_last_errno()));
        return nullptr;
    }

    return pfn;
}

/**/
LPFN_ACCEPTEX get_accept_func_ptr(SOCKET sock) {
    if (_cc_unlikely(_accept_func_ptr == nullptr)) {
        GUID guid = WSAID_ACCEPTEX;
        _accept_func_ptr = (LPFN_ACCEPTEX)get_extension_func_ptr(sock, guid);
    }

    return _accept_func_ptr;
}

/**/
LPFN_GETACCEPTEXSOCKADDRS get_accept_sockaddrs_func_ptr(SOCKET sock) {
    if (_cc_unlikely(_accept_sockaddrs_func_ptr == nullptr)) {
        GUID guid = WSAID_GETACCEPTEXSOCKADDRS;
        _accept_sockaddrs_func_ptr = (LPFN_GETACCEPTEXSOCKADDRS)get_extension_func_ptr(sock, guid);
    }

    return _accept_sockaddrs_func_ptr;
}

/**/
LPFN_TRANSMITFILE get_transmitfile_func_ptr(SOCKET sock) {
    if (_cc_unlikely(_transmit_file_func_ptr == nullptr)) {
        GUID guid = WSAID_TRANSMITFILE;
        _transmit_file_func_ptr = (LPFN_TRANSMITFILE)get_extension_func_ptr(sock, guid);
    }
    return _transmit_file_func_ptr;
}

/**/
LPFN_CONNECTEX get_connectex_func_ptr(SOCKET sock) {
    GUID guid = WSAID_CONNECTEX;
    if (_cc_unlikely(_connectex_func_ptr == nullptr)) {
        _connectex_func_ptr = (LPFN_CONNECTEX)get_extension_func_ptr(sock, guid);
    }

    return _connectex_func_ptr;
}

/**/
LPFN_DISCONNECTEX get_disconnect_func_ptr(SOCKET sock) {
    GUID guid = WSAID_DISCONNECTEX;
    if (_cc_unlikely(_disconnect_func_ptr == nullptr)) {
        _disconnect_func_ptr = (LPFN_DISCONNECTEX)get_extension_func_ptr(sock, guid);
    }

    return _disconnect_func_ptr;
}

/**/
LPFN_GETQUEUEDCOMPLETIONSTATUSEX get_queued_completion_status_func_ptr() {
#if (_WIN32_WINNT >= 0x0600)
    return GetQueuedCompletionStatusEx;
#else
    if (_cc_unlikely(_get_queued_completion_status_func_ptr)) {
        return _get_queued_completion_status_func_ptr;
    }

    _get_queued_completion_status_func_ptr =
        (LPFN_GETQUEUEDCOMPLETIONSTATUSEX)_cc_load_function(_cc_load_windows_kernel32(), "GetQueuedCompletionStatusEx");

    return _get_queued_completion_status_func_ptr;
#endif
}

/**/
int _udp_reset_connect(SOCKET sock, BOOL bNewBehavior) {
    int result = NO_ERROR;
    DWORD dwBytes;

    if (WSAIoctl(sock, SIO_UDP_CONNRESET, (LPVOID)&bNewBehavior, sizeof(bNewBehavior), nullptr, 0, &dwBytes, nullptr, nullptr) ==
        SOCKET_ERROR) {
        result = WSAGetLastError();
        if (result == WSAEWOULDBLOCK)
            result = NO_ERROR;
    }

    return result;
}