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
#include <libcc/alloc.h>
#include <libcc/logger.h>
#include <libcc/socket/socket.h>
#include <libcc/time.h>

#if defined(__CC_WINDOWS__)
#define SETSOCKOPT_OPTVAL_TYPE (const char *)
#else
#define SETSOCKOPT_OPTVAL_TYPE (void *)
#endif

int __cc_stdlib_socket_connect(_cc_socket_t fd, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len) {
    if (connect(fd, (struct sockaddr *)sa, sa_len) == -1) {
        int err = _cc_last_errno();

        if (err != _CC_EINPROGRESS_) {
            return 0;
        }
    }
    return 1;
}

#ifndef __CC_WINDOWS__
int __cc_get_fcntl(_cc_socket_t fd) {
    int r;
    do {
        r = fcntl(fd, F_GETFD);
    } while (r == -1 && errno == EINTR);

#ifdef _CC_DEBUG_
    if (r == -1) {
        r = _cc_last_errno();
        _cc_logger_error(_T("F_GETFL fcntl(%d) failed with error:%d, %s "), fd, r, _cc_last_error(r));
    }
#endif

    return r;
}

int __cc_set_fcntl(_cc_socket_t fd, int flags) {
    int r;
    do {
        r = fcntl(fd, F_SETFD, flags);
    } while (r == -1 && errno == EINTR);
#ifdef _CC_DEBUG_
    if (r == -1) {
        flags = _cc_last_errno();
        _cc_logger_error(_T("F_SETFL fcntl(%d) failed with error:%d, %s "), fd, flags, _cc_last_error(flags));
    }
#endif
    return r;
}

/* Enable the FD_CLOEXEC on the given fd to avoid fd leaks.
 * This function should be invoked for fd's on specific places
 * where fork + execve system calls are called. */

_CC_API_PUBLIC(bool_t) _cc_set_socket_closeonexec(_cc_socket_t fd) {
#if defined(FD_CLOEXEC)
    int flags = __cc_get_fcntl(fd);
    if (flags == -1 || (flags & FD_CLOEXEC)) {
        return false;
    }

    if (__cc_set_fcntl(fd, flags | FD_CLOEXEC) < 0) {
        return false;
    }
#endif
    return true;
}
#endif
_CC_API_PUBLIC(_cc_socket_t) _cc_socket(uint32_t domain, uint32_t type, uint32_t protocol) {
    _cc_socket_t fd;
#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
    fd = socket(domain, type, protocol);
    if (fd >= 0) {
        return fd;
    } else if ((type & (SOCK_NONBLOCK | SOCK_CLOEXEC)) == 0) {
        return -1;
    }
#endif
    fd = socket(domain, (type & (~(_CC_SOCK_NONBLOCK_ | _CC_SOCK_CLOEXEC_))), protocol);
    if (fd < 0) {
        return -1;
    }

    if (type & _CC_SOCK_NONBLOCK_) {
        if (_cc_set_socket_nonblock(fd, 1) < 0) {
            _cc_close_socket(fd);
            return -1;
        }
    }

#ifdef __CC_LINUX__
    if (type & _CC_SOCK_CLOEXEC_) {
        if (_cc_set_socket_closeonexec(fd) < 0) {
            _cc_close_socket(fd);
            return -1;
        }
    }
#endif

    return fd;
}

_CC_API_PUBLIC(int) _cc_set_socket_reuseport(_cc_socket_t fd, int optval) {
    int res = 0;
#ifdef SO_REUSEPORT
    res = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, SETSOCKOPT_OPTVAL_TYPE & optval, (socklen_t)(sizeof(int)));
    if (res < 0 && optval) {
        _cc_logger_error(_T("SO_REUSEPORT failed: %s(%d)"), _cc_last_error(res), res);
    }
#endif
    return res;
}

_CC_API_PUBLIC(int) _cc_set_socket_reuseaddr(_cc_socket_t fd) {
    int yes = 1;
    /* Make sure connection-intensive things like the redis benchmark
     * will be able to close/open sockets a zillion of times */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, SETSOCKOPT_OPTVAL_TYPE & yes, sizeof(yes)) == -1) {
        int err = _cc_last_errno();
        _cc_logger_error(_T("SO_REUSEADDR failed: %s(%d)"), _cc_last_error(err), err);
        return err;
    }
    return 0;
}

_CC_API_PUBLIC(int) _cc_socket_ipv6only(_cc_socket_t fd) {
#if defined(IPV6_V6ONLY)
    int one = 1;
    return setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&one, (_cc_socklen_t)sizeof(one));
#endif
    return 0;
}

_CC_API_PUBLIC(_cc_socket_t) _cc_socket_accept(_cc_socket_t fd, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len) {
    _cc_socket_t accept_fd;
    int err = 0;

    for (;;) {
        accept_fd = accept(fd, (struct sockaddr *)sa, sa_len);
        if (_cc_unlikely(accept_fd == -1)) {
            err = _cc_last_errno();
            if (err == _CC_EINTR_) {
                continue;
            }
        }
        break;
    }
    return accept_fd;
}

/* Set the socket send/recv timeout (SO_SNDTIMEO/SO_RCVTIMEO socket option) to
 * the specified number of milliseconds, or disable it if the 'ms' argument is
 * zero. */
_CC_API_PUBLIC(int) _cc_set_socket_timeout(_cc_socket_t fd, long ms) {
    int err = 0;
    struct timeval tv;

    tv.tv_sec = (ms / 1000);
    tv.tv_usec = (ms % 1000) * 1000 * 1000;

    if (_cc_setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        err = _cc_last_errno();
        _cc_logger_error(_T("setsockopt SO_RCVTIMEO: (%d)%s"), err, _cc_last_error(err));
        return err;
    }

    if (_cc_setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
        err = _cc_last_errno();
        _cc_logger_error(_T("setsockopt SO_SNDTIMEO: (%d)%s"), err, _cc_last_error(err));
        return err;
    }
    return 0;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_send(_cc_socket_t fd, const byte_t* buf, int32_t length) {
    int32_t result;
#ifdef __CC_WINDOWS__
    result = send(fd, (char *)buf, length, 0);
#else
    result = (int32_t)write(fd, (char *)buf, length);
#endif
    if (result <= 0) {
        int err = _cc_last_errno();
        if ((result == 0 && err == 0) || err == _CC_EINTR_ || err == _CC_EAGAIN_) {
            return 0;
        }
        return _CC_SOCKET_ERROR_;
    }

    return result;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_sendto(_cc_socket_t fd, const byte_t* buf, int32_t length, const _cc_sockaddr_t *sa, _cc_socklen_t sa_len) {
    int32_t sent = 0;
    int32_t result = 0;
    
    unsigned int flags = 0;
#ifdef __CC_ANDROID__
    flags = MSG_NOSIGNAL;
#endif

    _cc_set_last_errno(0);
    do {
        result = (int32_t)sendto(fd, ((const char *)buf) + sent, (length - sent), flags, (struct sockaddr *)sa, sa_len);
        if (result <= 0) {
            int err = _cc_last_errno();
            if (result == 0 && err == 0) {
                return sent;
            } else if (err == _CC_EINTR_ || err == _CC_EAGAIN_) {
                continue;
            }
            return _CC_SOCKET_ERROR_;
        }

        sent += result;
    } while (sent != length);

    return sent;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_recv(_cc_socket_t fd, byte_t* buf, int32_t length) {
    int32_t result = 0;

GOTO_SRECV_CONTINUE:
    _cc_set_last_errno(0);

#ifdef __CC_ANDROID__
    result = (int32_t)recv(fd, (char *)buf, length, MSG_NOSIGNAL);
#else
    result = (int32_t)recv(fd, (char *)buf, length, 0);
#endif

    if (result < 0) {
        int err = _cc_last_errno();
        if (err == _CC_EINTR_ || err == _CC_EAGAIN_) {
            goto GOTO_SRECV_CONTINUE;
        }
    }
    return result;
}
