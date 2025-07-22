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
#ifndef _C_CC_SYS_LINUX_SOCKET_H_INCLUDED_
#define _C_CC_SYS_LINUX_SOCKET_H_INCLUDED_

#include "../../types.h"

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <arpa/inet.h>

#include <net/if.h>
#include <netdb.h>

#include <errno.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Socket constants */
#define _CC_INVALID_SOCKET_     (-1)
#define _CC_SOCKET_ERROR_       (-1)

/* errno define */
#define _CC_ETIMEDOUT_        ETIMEDOUT
#define _CC_ENOMEM_           ENOMEM
#define _CC_EINVAL_           EINVAL
#define _CC_ECONNREFUSED_     ECONNREFUSED
#define _CC_ECONNRESET_       ECONNRESET
#define _CC_EHOSTDOWN_        EHOSTDOWN
#define _CC_EHOSTUNREACH_     EHOSTUNREACH
#define _CC_EINTR_            EINTR
#define _CC_EAGAIN_           EAGAIN
#define _CC_ENETDOWN_         ENETDOWN
#define _CC_ENETUNREACH_      ENETUNREACH
#define _CC_ENOTCONN_         ENOTCONN
#define _CC_EISCONN_          EISCONN
#define _CC_EWOULDBLOCK_      EWOULDBLOCK
#define _CC_ENOBUFS_          ENOBUFS
#define _CC_ECONNABORTED_     ECONNABORTED
#define _CC_EINPROGRESS_      EINPROGRESS

#define _CC_SHUT_RD_          SHUT_RD
#define _CC_SHUT_WR_          SHUT_WR
#define _CC_SHUT_RD_WR_       SHUT_RDWR

#define _cc_getaddrinfo       getaddrinfo
#define _cc_freeaddrinfo      freeaddrinfo

/* This is the system-independent socket info structure */
typedef int                    _cc_socket_t;
typedef struct addrinfo        _cc_addrinfo_t;
typedef struct sockaddr        _cc_sockaddr_t;
typedef socklen_t              _cc_socklen_t;

#define _cc_getsockopt(__sock, __level, __optname, __optval , __optlen)\
    getsockopt(__sock, __level, __optname, (pvoid_t)__optval , __optlen)

#define _cc_setsockopt(__sock, __level, __optname, __optval , __optlen) \
    setsockopt( __sock , __level , __optname , (pvoid_t)__optval , __optlen )

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_SYS_LINUX_SOCKET_H_INCLUDED_*/
