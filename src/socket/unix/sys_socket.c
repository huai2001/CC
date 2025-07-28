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
#include <libcc/atomic.h>
#include <libcc/generic.h>
#include <libcc/logger.h>
#include <libcc/socket/socket.h>
#include <string.h>
#include <unistd.h>

static _cc_atomic32_t _socket_started = 0;

_CC_API_PUBLIC(bool_t) _cc_install_socket(void) {
    void (*handler)(int);

    if (_cc_atomic32_inc_ref(&_socket_started)) {
        /* Stops the SIGPIPE signal being raised when writing to a closed socket */
        handler = signal(SIGPIPE, SIG_IGN);

        if (handler != SIG_DFL) {
            signal(SIGPIPE, handler);
        }
    }

    return true;
}

_CC_API_PUBLIC(bool_t) _cc_uninstall_socket(void) {
    void (*handler)(int);

    if (_socket_started == 0) {
        return true;
    }

    if (_cc_atomic32_dec_ref(&_socket_started)) {
        /* Restore the SIGPIPE handler */
        handler = signal(SIGPIPE, SIG_DFL);
        if (handler != SIG_IGN) {
            signal(SIGPIPE, handler);
        }
    }

    return true;
}

/**/
_CC_API_PUBLIC(int) _cc_close_socket(_cc_socket_t fd) {
#ifdef __CC_OS2__
#define _sys_close soclose
#else /* !__CC_OS2__ */
#define _sys_close close
#endif /* __CC_OS2__ */

#ifdef _CC_DEBUG_
    int result = _sys_close(fd);

    if (_cc_unlikely(result == _CC_SOCKET_ERROR_)) {
        int32_t last_errno = _cc_last_errno();
        _cc_logger_error(_T(" closesocket(%d) failed with error:%d, %s "), fd, last_errno, _cc_last_error(last_errno));
    }
    return result;
#else
    return _sys_close(fd);
#endif

#undef _sys_close
}

_CC_API_PUBLIC(int) _cc_set_socket_nodelay(_cc_socket_t fd, int enable) {
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
}

_CC_API_PUBLIC(int) _cc_set_socket_nonblock(_cc_socket_t fd, int nonblocking) {
#if defined(__BEOS__) && defined(SO_NONBLOCK)
    /* On BeOS r5 there is O_NONBLOCK but it's for files only */
    return setsockopt(fd, SOL_SOCKET, SO_NONBLOCK, &nonblocking, sizeof(nonblocking));
#elif defined(__CC_OS2__)
    return ioctl(fd, FIONBIO, &nonblocking);
#else
    int flags = __cc_get_fcntl(fd);
    if (nonblocking) {
#if defined(O_NONBLOCK)
        flags |= O_NONBLOCK;
#elif defined(O_NDELAY)
        flags |= O_NDELAY;
#elif defined(FNDELAY)
        flags |= FNDELAY;
#endif
    } else {
#if defined(O_NONBLOCK)
        flags &= ~O_NONBLOCK;
#elif defined(O_NDELAY)
        flags &= ~O_NDELAY;
#elif defined(FNDELAY)
        flags &= ~FNDELAY;
#endif
    }
    return __cc_set_fcntl(fd, flags);
#endif
}

_CC_API_PUBLIC(int) _cc_set_socket_keepalive(_cc_socket_t fd, int enable, int delay) {
    int err = 0;
    err = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
    if (err == -1) {
        return errno;
    }

    if (enable != 1) {
        return 0;
    }

#if defined(__CC_APPLE__)
    err = setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &delay, sizeof(delay));
    if (err < 0) {
        return err;
    }
#elif defined(__GLIBC__) && !defined(__FreeBSD_kernel__)
    err = setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &delay, sizeof(delay));
    if (err < 0) {
        return err;
    }

    delay /= 3;
    if (delay == 0) {
        delay = 1;
    }
    err = setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &delay, sizeof(delay));
    if (err < 0) {
        return err;
    }

    delay = 3;
    err = setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &delay, sizeof(delay));
    if (err < 0) {
        return err;
    }
#endif

    return 0;
}
