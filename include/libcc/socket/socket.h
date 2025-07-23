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
#ifndef _C_CC_SOCKET_H_INCLUDED_
#define _C_CC_SOCKET_H_INCLUDED_

#include "../core.h"
#include "../mutex.h"

#ifdef __CC_WINDOWS__
    #include "windows/sys_socket.h"
#elif defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__) \
    || defined(__CC_FREEBSD__) || defined(__CC_OPENBSD__) || defined(__CC_NETBSD__)
    #include "unix/sys_socket.h"
#elif defined(__CC_LINUX__) || defined(__CC_ANDROID__)
    #include "linux/sys_socket.h"
#else
    #include "unix/sys_socket.h"
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

#ifndef __CC_WINDOWS__
int __cc_get_fcntl(_cc_socket_t fd);
int __cc_set_fcntl(_cc_socket_t fd, int flags);
#endif
/**/
int __cc_stdlib_socket_connect(_cc_socket_t fd, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len);

/**
 * @brief Initialize System Socket
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_install_socket(void);

/**
 * @brief Uninitialize System Socket
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_uninstall_socket(void);

/*

*/
_CC_API_PUBLIC(_cc_socket_t) _cc_socket(uint32_t domain, uint32_t type, uint32_t protocol);

/**
 * @brief Shutdown Socket
 *
 * @param fd The socket to be shutdown
 * @param how 
 *
 * @return 0 if successful or Socket on error
 */
_CC_FORCE_INLINE_ int _cc_shutdown_socket(_cc_socket_t fd, byte_t how) {
    return shutdown(fd, how);
}

/** 
 * @brief Close socket
 *
 * @param fd The socket to be closed
 *
 * @return 0 if successful or Socket on error
 */
_CC_API_PUBLIC(int) _cc_close_socket(_cc_socket_t fd);

#ifndef __CC_WINDOWS__
/** 
 * @brief closeexec
 *
 * @param fd socket handle
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_set_socket_closeonexec(_cc_socket_t fd);
#endif

/** 
 * @brief reuse socket port
 *
 * @param fd socket handle
 * @param opt socket option
 *
 * @return 0 if successful or Socket on error
 */
_CC_API_PUBLIC(int) _cc_set_socket_reuseport(_cc_socket_t fd,int opt);
/** 
 * @brief reuse socket
 *
 * @param fd socket handle
 *
 * @return 0 if successful or Socket on error
 */
_CC_API_PUBLIC(int) _cc_set_socket_reuseaddr(_cc_socket_t fd);
/**
 * @brief Set the socket to nodelay mode
 *
 * @param fd socket handle
 * @param opt 1: enabled or 0: disabled
 *
 * @return 0 if successful or Socket on error
 */
_CC_API_PUBLIC(int) _cc_set_socket_nodelay(_cc_socket_t fd, int opt);
/**
 * @brief Set the socket to nonblocking mode
 *
 * @param fd socket handle
 * @param opt 1: enabled or 0: disabled
 *
 * @return 0 if successful or Socket on error
 */
_CC_API_PUBLIC(int) _cc_set_socket_nonblock(_cc_socket_t fd, int opt);
/**
 * @brief Set socket keepalive
 *
 * @param fd socket handle
 * @param opt
 * @param delay
 *
 * @return 0 if successful or Socket on error
 */
_CC_API_PUBLIC(int) _cc_set_socket_keepalive(_cc_socket_t fd, int opt, int delay);

/**
 * @brief Set the socket send/recv timeout (SO_SNDTIMEO/SO_RCVTIMEO socket option) to the specified
 *        number of milliseconds, or disable it if the 'ms' argument is zero. 
 *
 * @param fd Socket handle
 * @param ms
 *
 * @return 0 if successful or Socket on error
 */
_CC_API_PUBLIC(int) _cc_set_socket_timeout(_cc_socket_t fd, long ms);
/**
 * @brief Set ipv6 only bind socket option to make listener work only in ipv6 sockets. 
 *        According to RFC3493 and most Linux distributions, default value for the
 *        sockets is to work in IPv4-mapped mode. In IPv4-mapped mode, it is not possible
 *        to bind same port from different IPv4 and IPv6 handlers.
 * @param fd Socket handle
 *
 * @return 0 if successful or Socket on error
 */
_CC_API_PUBLIC(int) _cc_socket_ipv6only(_cc_socket_t fd);
/**
 * @brief Get Socket accept
 *
 * @param fd socket handle
 * @param sa _cc_sockaddr_t structure
 * @param sa_len Size of _cc_sockaddr_t structure
 *
 * @return socket hanel
 */
_CC_API_PUBLIC(_cc_socket_t) _cc_socket_accept(_cc_socket_t fd, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len);

/**
 * @brief Read socket data
 *
 * @param fd socket handle
 * @param buf Byte buffer
 * @param len Size of byte buffer
 *
 * @return Length of read byte buffer
*/
_CC_API_PUBLIC(int32_t) _cc_recv(_cc_socket_t fd, byte_t* buf, int32_t len);

/**
 * @brief Send socket data
 *
 * @param fd socket handle
 * @param buf Byte buffer
 * @param len Size of byte buffer
 *
 * @return Length of send byte buffer
*/
_CC_API_PUBLIC(int32_t) _cc_send(_cc_socket_t fd, const byte_t* buf, int32_t len);
/**
 * @brief Send socket data
 *
 * @param fd socket handle
 * @param buf Byte buffer
 * @param len Size of byte buffer
 * @param sa _cc_sockaddr_t structure
 * @param sa_len _cc_socklen_t
 *
 * @return Length of send byte buffer
*/
_CC_API_PUBLIC(int32_t) _cc_sendto(_cc_socket_t fd, const byte_t* buf, int32_t len, const _cc_sockaddr_t *sa, _cc_socklen_t sa_len);

/**
 * @brief Convert the network host address and store it in the struct in_addr structure
 *
 * @param af AF_INET or AF_INET6
 * @param src IP Address
 * @param dst in_addr structure
 *
 * @return true if successful or false on error.
 */
_CC_FORCE_INLINE_ bool_t _cc_inet_pton(int af, const tchar_t *src, byte_t *dst) {
    return inet_pton(af, src, dst) == 1;
}
/**
 * @brief 
 *
 * @param af AF_INET or AF_INET6
 * @param src in_addr structure
 * @param dst IP Address
 * @param size Maximum length of dst buffer
 *
 * @return true if successful or false on error.
 */
_CC_FORCE_INLINE_ bool_t _cc_inet_ntop(int af,  const byte_t *src, tchar_t *dst, int32_t size){
    return inet_ntop(af, src, dst, size) != nullptr;
}
/**
 * @brief 
 *
 * @param sa struct sockaddr_in
 * @param ip IP address
 * @param port Port
 */
_CC_API_PUBLIC(void) _cc_inet_ipv4_addr(struct sockaddr_in *sa, const tchar_t *ip, int port);
/**
 * @brief
 *
 * @param sa struct sockaddr_in6
 * @param ip IP address
 * @param port Port
 */
_CC_API_PUBLIC(void) _cc_inet_ipv6_addr(struct sockaddr_in6 *sa, const tchar_t *ip, int port);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_SOCKET_H_INCLUDED_ */
