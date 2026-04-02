#ifndef _C_CC_SOCKET_H_INCLUDED_
#define _C_CC_SOCKET_H_INCLUDED_

#include "os.h"
#include "mutex.h"

#ifdef __CC_WINDOWS__
    #include "os/windows/sys_socket.h"
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__) \
    || defined(__CC_FREEBSD__) || defined(__CC_OPENBSD__) || defined(__CC_NETBSD__)
    #include "os/unix/sys_socket.h"
#elif defined(__CC_LINUX__) || defined(__CC_ANDROID__)
    #include "os/linux/sys_socket.h"
#else
    #include "os/unix/sys_socket.h"
#endif


#ifdef SOCK_NONBLOCK
    #define _CC_SOCK_NONBLOCK_ SOCK_NONBLOCK
#else
    #define _CC_SOCK_NONBLOCK_ 0x4000000
#endif

#ifdef SOCK_CLOEXEC
    #define _CC_SOCK_CLOEXEC_ SOCK_CLOEXEC
#else
    #define _CC_SOCK_CLOEXEC_ 0x80000000
#endif

#ifdef EFD_NONBLOCK
    #define _CC_EFD_NONBLOCK_ EFD_NONBLOCK
#else
    #define _CC_EFD_NONBLOCK_ 0x4000
#endif

#ifdef EFD_CLOEXEC
    #define _CC_EFD_CLOEXEC_ EFD_CLOEXEC
#else
    #define _CC_EFD_CLOEXEC_ 0x8000
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**/
int __cc_stdlib_socket_connect(_cc_socket_t fd, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len);

/***/
_CC_API_PUBLIC(bool_t) _cc_install_socket(void);
/**/
_CC_API_PUBLIC(bool_t) _cc_uninstall_socket(void);

/**/
_CC_API_PUBLIC(_cc_socket_t) _cc_socket(uint32_t domain, uint32_t type, uint32_t protocol);
/**/
_CC_API_PUBLIC(int) _cc_close_socket(_cc_socket_t fd);

/**/
#ifndef __CC_WINDOWS__
_CC_API_PUBLIC(bool_t) _cc_set_socket_closeonexec(_cc_socket_t fd);
#endif
/**/
_CC_API_PUBLIC(int) _cc_set_socket_reuseport(_cc_socket_t fd,int opt);
/**/
_CC_API_PUBLIC(int) _cc_set_socket_reuseaddr(_cc_socket_t fd);
/**/
_CC_API_PUBLIC(int) _cc_set_socket_nodelay(_cc_socket_t fd, int opt);
/**/
_CC_API_PUBLIC(int) _cc_set_socket_nonblock(_cc_socket_t fd, int opt);
/**/
_CC_API_PUBLIC(int) _cc_set_socket_keepalive(_cc_socket_t fd, int opt, int delay);
/**/
_CC_API_PUBLIC(int) _cc_set_socket_timeout(_cc_socket_t fd, long ms);
/**/
_CC_API_PUBLIC(int) _cc_socket_ipv6only(_cc_socket_t fd);
/**/
_CC_API_PUBLIC(_cc_socket_t) _cc_socket_accept(_cc_socket_t fd, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len);

/**/
_CC_API_PUBLIC(int32_t) _cc_recv(_cc_socket_t fd, byte_t* buf, int32_t len);
/**/
_CC_API_PUBLIC(int32_t) _cc_send(_cc_socket_t fd, const byte_t* buf, int32_t len);
/**/
_CC_API_PUBLIC(int32_t) _cc_sendto(_cc_socket_t fd, const byte_t* buf, int32_t len, const _cc_sockaddr_t *sa, _cc_socklen_t sa_len);

/**/
_CC_API_PUBLIC(void) _cc_inet_ipv4_addr(struct sockaddr_in *sa, const tchar_t *ip, int port);
/**/
_CC_API_PUBLIC(void) _cc_inet_ipv6_addr(struct sockaddr_in6 *sa, const tchar_t *ip, int port);
/**/
_CC_API_PUBLIC(bool_t) _cc_is_valid_ipv4_addr(const tchar_t *str, const tchar_t *endptr);
/**/
_CC_API_PUBLIC(bool_t) _cc_is_valid_ipv6_addr(const tchar_t *str, const tchar_t *endptr);
/**/
_CC_FORCE_INLINE_ int _cc_shutdown_socket(_cc_socket_t fd, byte_t how) {
    return shutdown(fd, how);
}

/**/
_CC_FORCE_INLINE_ bool_t _cc_inet_pton(int af, const tchar_t *src, pvoid_t dst) {
#ifdef __CC_WINDOWS__
    return InetPton(af, (PCSTR)src, dst) == 1;
#else
    return inet_pton(af, src, dst) == 1;
#endif
}

/**/
_CC_FORCE_INLINE_ bool_t _cc_inet_ntop(int af,  const pvoid_t src, tchar_t *dst, int32_t size) {
#ifdef __CC_WINDOWS__
    return InetNtop(af, (PVOID)src, dst, size) != NULL;
#else
    return inet_ntop(af, src, dst, size) != NULL;
#endif
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_SOCKET_H_INCLUDED_ */
