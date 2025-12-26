#include <libcc/atomic.h>
#include <libcc/loadso.h>
#include <libcc/logger.h>

#include <libcc/socket.h>
#include <mstcpip.h>

#ifdef __CC_MSVC__
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

static LPFN_ACCEPTEX _accept_func_ptr = NULL;
static LPFN_GETACCEPTEXSOCKADDRS _accept_sockaddrs_func_ptr = NULL;
static LPFN_DISCONNECTEX _disconnect_func_ptr = NULL;
static LPFN_CONNECTEX _connectex_func_ptr = NULL;
#if (_WIN32_WINNT < 0x0600)
static LPFN_GETQUEUEDCOMPLETIONSTATUSEX _get_queued_completion_status_func_ptr = NULL;
#endif
static LPFN_TRANSMITFILE _transmit_file_func_ptr = NULL;

_cc_sockaddr_t* _cc_win_get_ipv4_any_addr(void) {
    return (_cc_sockaddr_t *)&_win_addr_ipv4_any;
}

_cc_sockaddr_t* _cc_win_get_ipv6_any_addr(void) {
    return (_cc_sockaddr_t *)&_win_addr_ipv6_any;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_install_socket(void) {

    /* Skip initialization in safe mode without network support */
    if (1 == GetSystemMetrics(SM_CLEANBOOT)) {
        return true;
    }
    
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

            _cc_close_socket((_cc_socket_t)fd);
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
#if defined(TCP_KEEPIDLE) && defined(TCP_KEEPINTVL)
/*
 * Check if Windows version is 10.0.16299 (Windows 10, version 1709) or later.
 */
_CC_API_PRIVATE(int)  _windows10_version1709(void) {
    uint32_t major;
    uint32_t minor;
    uint32_t build;

    _cc_get_os_version(&major, &minor, &build);

    if (major > 10){
        return 1;
    } else if (major < 10) {
        return 0;
    }

    if (minor > 0){
        return 1;
    }

    return build >= 16299;
}
#endif
/**/
_CC_API_PUBLIC(int) _cc_set_socket_keepalive(_cc_socket_t fd, int opt, int delay) {
    tcp_keepalive_t klive;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&opt, sizeof opt) == -1) {
        return WSAGetLastError();
    }
#if defined(TCP_KEEPIDLE) && defined(TCP_KEEPINTVL)
    /* Windows 10, version 1709 (build 10.0.16299) and later require second units
    * for TCP keepalive options. */
    if (_windows10_version1709()) {
        if (setsockopt(fd,
                       IPPROTO_TCP,
                       TCP_KEEPIDLE,
                       (const char*)&delay,
                       sizeof delay) == -1) {
            return WSAGetLastError();
        }

        if (setsockopt(fd,
                       IPPROTO_TCP,
                       TCP_KEEPINTVL,
                       (const char*)&delay,
                       sizeof delay) == -1) {
            return WSAGetLastError();
        }

        if (setsockopt(fd,
                       IPPROTO_TCP,
                       TCP_KEEPCNT,
                       (const char*)&delay,
                       sizeof delay) == -1) {
            return WSAGetLastError();
        }
    }
#endif
    klive.onoff = 1;
    klive.keepalivetime = delay * 1000;
    klive.keepaliveinterval = delay * 1000;

    if (WSAIoctl(fd, SIO_KEEPALIVE_VALS, (LPVOID) &klive, sizeof(tcp_keepalive_t), 
                NULL, 0, (unsigned long *)&opt, 0, NULL) == -1) {
        return WSAGetLastError();
    }

    return 0;
}

/**/
void *get_extension_func_ptr(SOCKET sock, GUID guid) {
    DWORD dwBytes;
    PVOID pfn = NULL;

    if (SOCKET_ERROR == WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &pfn, sizeof(pfn),
                                 &dwBytes, NULL, NULL)) {
        _cc_logger_error(_T("fd:%d, WSAIoctl Error:%s"), sock, _cc_last_error(_cc_last_errno()));
        return NULL;
    }

    return pfn;
}

/**/
LPFN_ACCEPTEX get_accept_func_ptr(SOCKET sock) {
    if (_cc_unlikely(_accept_func_ptr == NULL)) {
        GUID guid = WSAID_ACCEPTEX;
        _accept_func_ptr = (LPFN_ACCEPTEX)get_extension_func_ptr(sock, guid);
    }

    return _accept_func_ptr;
}

/**/
LPFN_GETACCEPTEXSOCKADDRS get_accept_sockaddrs_func_ptr(SOCKET sock) {
    if (_cc_unlikely(_accept_sockaddrs_func_ptr == NULL)) {
        GUID guid = WSAID_GETACCEPTEXSOCKADDRS;
        _accept_sockaddrs_func_ptr = (LPFN_GETACCEPTEXSOCKADDRS)get_extension_func_ptr(sock, guid);
    }

    return _accept_sockaddrs_func_ptr;
}

/**/
LPFN_TRANSMITFILE get_transmitfile_func_ptr(SOCKET sock) {
    if (_cc_unlikely(_transmit_file_func_ptr == NULL)) {
        GUID guid = WSAID_TRANSMITFILE;
        _transmit_file_func_ptr = (LPFN_TRANSMITFILE)get_extension_func_ptr(sock, guid);
    }
    return _transmit_file_func_ptr;
}

/**/
LPFN_CONNECTEX get_connectex_func_ptr(SOCKET sock) {
    GUID guid = WSAID_CONNECTEX;
    if (_cc_unlikely(_connectex_func_ptr == NULL)) {
        _connectex_func_ptr = (LPFN_CONNECTEX)get_extension_func_ptr(sock, guid);
    }

    return _connectex_func_ptr;
}

/**/
LPFN_DISCONNECTEX get_disconnect_func_ptr(SOCKET sock) {
    GUID guid = WSAID_DISCONNECTEX;
    if (_cc_unlikely(_disconnect_func_ptr == NULL)) {
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

    if (WSAIoctl(sock, SIO_UDP_CONNRESET, (LPVOID)&bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytes, NULL, NULL) ==
        SOCKET_ERROR) {
        result = WSAGetLastError();
        if (result == WSAEWOULDBLOCK) {
            result = NO_ERROR;
        }
    }

    return result;
}

/**/
int32_t _win_recv(_cc_socket_t fd, byte_t* buf, int32_t length) {
    DWORD count_received;
    DWORD flags = 0;
    WSABUF wsabuf;

    wsabuf.buf = (CHAR*)buf;
    wsabuf.len = (ULONG)length;

    if (WSARecv(fd, &wsabuf, 1, &count_received, &flags, NULL, NULL) != 0) {
        return -1;
    }

    return (int32_t)count_received;
}