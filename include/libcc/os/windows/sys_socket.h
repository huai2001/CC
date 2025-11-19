#ifndef _C_CC_SYS_WINDOWS_SOCKET_H_INCLUDED_
#define _C_CC_SYS_WINDOWS_SOCKET_H_INCLUDED_

#include "../windows.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Socket constants */
#define _CC_INVALID_SOCKET_    ((_cc_socket_t)INVALID_SOCKET)
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

/*
 * Even though sizeof(SOCKET) is 8, it's safe to cast it to int, because
 * the value constitutes an index in per-process table of limited size
 * and not a real pointer. And we also depend on fact that all processors
 * Windows run on happen to be two's-complement, which allows to
 * interchange INVALID_SOCKET and -1.
 */
typedef int						_cc_socket_t;
typedef int                     _cc_socklen_t;
typedef struct sockaddr         _cc_sockaddr_t;

typedef union {
    struct sockaddr addr;
    struct sockaddr_in addr_in;
    struct sockaddr_in6 addr_in6;
} _cc_union_sockaddr_t;

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

/**/
int _udp_reset_connect(SOCKET fd, BOOL bNewBehavior);
/**/
LPFN_ACCEPTEX get_accept_func_ptr(SOCKET fd);
/**/
LPFN_GETACCEPTEXSOCKADDRS get_accept_sockaddrs_func_ptr(SOCKET fd);
/**/
LPFN_DISCONNECTEX get_disconnect_func_ptr(SOCKET fd);
/**/
LPFN_CONNECTEX get_connectex_func_ptr(SOCKET fd);
/**/
LPFN_GETQUEUEDCOMPLETIONSTATUSEX get_queued_completion_status_func_ptr(void);
/**/
int32_t _win_recv(_cc_socket_t fd, byte_t* buf, int32_t length);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_SYS_WINDOWS_SOCKET_H_INCLUDED_*/




