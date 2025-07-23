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
#ifndef _C_CC_SYS_WINDOWS_SOCKET_H_INCLUDED_
#define _C_CC_SYS_WINDOWS_SOCKET_H_INCLUDED_

#include "../../core/windows.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Socket constants */
#define _CC_INVALID_SOCKET_    INVALID_SOCKET
#define _CC_SOCKET_ERROR_      SOCKET_ERROR

/* errno define */
#define _CC_ETIMEDOUT_        WSAETIMEDOUT
#define _CC_ENOMEM_           WSAENOBUFS
#define _CC_EINVAL_           WSAEINVAL
#define _CC_ECONNREFUSED_     WSAECONNREFUSED
#define _CC_ECONNRESET_       WSAECONNRESET
#define _CC_EHOSTDOWN_        WSAEHOSTDOWN
#define _CC_EHOSTUNREACH_     WSAEHOSTUNREACH
#define _CC_EINTR_            WSAEINTR
#define _CC_ENETDOWN_         WSAENETDOWN
#define _CC_ENETUNREACH_      WSAENETUNREACH
#define _CC_ENOTCONN_         WSAENOTCONN
#define _CC_EISCONN_          WSAEISCONN
#define _CC_EWOULDBLOCK_      WSAEWOULDBLOCK
#define _CC_EAGAIN_           _CC_EWOULDBLOCK_    /* xxx */
#define _CC_ENOBUFS_          WSAENOBUFS
#define _CC_ECONNABORTED_     WSAECONNABORTED
#define _CC_EINPROGRESS_      WSAEWOULDBLOCK

/*
* -- manifest constants for shutdown()
*/
#define _CC_SHUT_RD_          SD_RECEIVE
#define _CC_SHUT_WR_          SD_SEND
#define _CC_SHUT_RD_WR_       SD_BOTH

/* This is the system-independent socket info structure */
typedef SOCKET                  _cc_socket_t;
typedef int                     _cc_socklen_t;

typedef union {
    struct sockaddr addr;
    struct sockaddr_in addr_in;
    struct sockaddr_in6 addr_in6;
} _cc_sockaddr_t;

#ifdef _CC_UNICODE_
    #define _cc_getaddrinfo      GetAddrInfoW
    #define _cc_freeaddrinfo     FreeAddrInfoW
    typedef ADDRINFOW            _cc_addrinfo_t;
#else
    #define _cc_getaddrinfo      GetAddrInfo
    #define _cc_freeaddrinfo     FreeAddrInfo
    typedef ADDRINFOT            _cc_addrinfo_t;
#endif

#define _cc_getsockopt(__sock, __level, __optname, __optval , __optlen)\
    getsockopt(__sock, __level, __optname, (const char*)__optval , (int*)__optlen)

#define _cc_setsockopt(__sock, __level, __optname, __optval , __optlen) \
    setsockopt( __sock , __level , __optname , (const char*)__optval , __optlen )

/**/
_cc_sockaddr_t *_cc_win_get_ipv4_any_addr(void);
/**/
_cc_sockaddr_t *_cc_win_get_ipv6_any_addr(void);

typedef BOOL (WINAPI *LPFN_GETQUEUEDCOMPLETIONSTATUSEX)(
    __in  HANDLE CompletionPort,
    __out_ecount_part(ulCount, *ulNumEntriesRemoved) LPOVERLAPPED_ENTRY lpCompletionPortEntries,
    __in  ULONG ulCount,
    __out PULONG ulNumEntriesRemoved,
    __in  DWORD dwMilliseconds,
    __in  BOOL fAlertable);

/**
 * @brief IOCP Socket UDP reset connect
 *
 * @param fd Socket handle
 * @param bNewBehavior reset
 *
 * @return 0 if successful or socket on error.
 */
int _udp_reset_connect(SOCKET fd, BOOL bNewBehavior);

/**
 * @brief Get IOCP accept function ptr
 *
 * @param fd Socket Handle
 *
 * @return LPFN_ACCEPTEX function ptr
 */
LPFN_ACCEPTEX get_accept_func_ptr(SOCKET fd);
/**
 * @brief Get IOCP accept sockaddrs function ptr
 *
 * @param fd Socket Handle
 *
 * @return LPFN_ACCEPTEX function ptr
 */
LPFN_GETACCEPTEXSOCKADDRS get_accept_sockaddrs_func_ptr(SOCKET fd);
/**
 * @brief Get IOCP disconnect function ptr
 *
 * @param fd Socket Handle
 *
 * @return LPFN_GETACCEPTEXSOCKADDRS function ptr
 */
LPFN_DISCONNECTEX get_disconnect_func_ptr(SOCKET fd);
/**
 * @brief Get IOCP connectex function ptr
 *
 * @param fd Socket Handle
 *
 * @return LPFN_DISCONNECTEX function ptr
 */
LPFN_CONNECTEX get_connectex_func_ptr(SOCKET fd);
/**
 * @brief Get IOCP GetQueuedCompletionStatusEx function ptr
 *
 * @return LPFN_GETQUEUEDCOMPLETIONSTATUSEX function ptr
 */
LPFN_GETQUEUEDCOMPLETIONSTATUSEX get_queued_completion_status_func_ptr(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_SYS_WINDOWS_SOCKET_H_INCLUDED_*/




