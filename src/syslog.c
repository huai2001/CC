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
#include <libcc/syslog.h>
#include <libcc/socket/socket.h>
#include <libcc/atomic.h>
#include <libcc/time.h>

static struct {
    bool_t enabled;
    byte_t facility;

    uint32_t pid;

    _cc_socket_t fd;
    _cc_union_sockaddr_t sockaddr;
    _cc_socklen_t socklen;
    _cc_atomic_lock_t lock;

    tchar_t host[64];
    tchar_t app[64];

} syslog = {0};

/**/
_CC_API_PUBLIC(void) _cc_syslog_lock(void) {
    _cc_spin_lock(&syslog.lock);
}

/**/
_CC_API_PUBLIC(void) _cc_syslog_unlock(void) {
    _cc_unlock(&syslog.lock);
}

_CC_API_PUBLIC(void) _cc_syslog_set_host(const tchar_t *host) {
    _tcsncpy(syslog.host, host, _cc_countof(syslog.host));
    syslog.host[_cc_countof(syslog.host) - 1] = 0;
}

_CC_API_PUBLIC(void) _cc_syslog_set_app(const tchar_t *app) {
    _tcsncpy(syslog.app, app, _cc_countof(syslog.app));
    syslog.app[_cc_countof(syslog.app) - 1] = 0;
}

_CC_API_PUBLIC(void) _cc_syslogW(uint8_t level, const wchar_t* msg, size_t length) {
    tchar_t buffer[_CC_8K_BUFFER_SIZE_];
#ifdef _CC_UNICODE_
    uint8_t utf8_buffer[_CC_16K_BUFFER_SIZE_];
#endif
    size_t buffer_length = _cc_syslog_header(_CC_SYSLOG_PRI(syslog.facility,level), buffer, _cc_countof(buffer));

    if (msg && length > 0) {
        size_t remaining = (_CC_8K_BUFFER_SIZE_ - buffer_length);
#ifdef _CC_UNICODE_
        if (remaining < length) {
            length = remaining;
        }
        memcpy(buffer + buffer_length, msg, length * sizeof(wchar_t));
        buffer_length += length;
#else
        buffer_length += _cc_w2a(msg, (int32_t)length, buffer + buffer_length, (int32_t)remaining);
#endif
    }

    if (level == _CC_LOG_LEVEL_ERROR_ && buffer_length < _CC_8K_BUFFER_SIZE_) {
        buffer_length += _cc_get_resolve_symbol(buffer, _CC_8K_BUFFER_SIZE_ - buffer_length);
    }

    buffer[buffer_length % _CC_8K_BUFFER_SIZE_] = 0;
#ifdef _CC_UNICODE_
    buffer_length = _cc_utf16_to_utf8((const uint16_t *)buffer, (const uint16_t *)&buffer[buffer_length], 
                                       utf8_buffer, &utf8_buffer[_cc_countof(utf8_buffer) - 1]);
    _cc_syslog_send((byte_t*)utf8_buffer, buffer_length);
#else
    _cc_syslog_send((byte_t*)buffer, buffer_length);
#endif
}

/**/
_CC_API_PUBLIC(void) _cc_syslogA(uint8_t level, const char_t* msg, size_t length) {
    tchar_t buffer[_CC_8K_BUFFER_SIZE_];
#ifdef _CC_UNICODE_
    uint8_t utf8_buffer[_CC_16K_BUFFER_SIZE_];
#endif
    size_t buffer_length = _cc_syslog_header(_CC_SYSLOG_PRI(syslog.facility,level), buffer, _cc_countof(buffer));

    if (msg && length > 0) {
        size_t remaining = (_CC_8K_BUFFER_SIZE_ - buffer_length);
#ifdef _CC_UNICODE_
        buffer_length += _cc_a2w(msg, (int32_t)length, buffer + buffer_length, (int32_t)remaining);
#else
        if (remaining < length) {
            length += remaining;
        }
        memcpy(buffer + buffer_length, msg, length);
        buffer_length += length;
#endif
    }

    if (level == _CC_LOG_LEVEL_ERROR_ && buffer_length < _CC_8K_BUFFER_SIZE_) {
        buffer_length += _cc_get_resolve_symbol(buffer + buffer_length, _CC_8K_BUFFER_SIZE_ - buffer_length);
    }
    
    buffer[buffer_length % _CC_8K_BUFFER_SIZE_] = 0;
#ifdef _CC_UNICODE_
    buffer_length = _cc_utf16_to_utf8((const uint16_t *)buffer, (const uint16_t *)&buffer[buffer_length], 
                                       utf8_buffer, &utf8_buffer[_cc_countof(utf8_buffer) - 1]);
    _cc_syslog_send((byte_t*)utf8_buffer, buffer_length);
#else
    _cc_syslog_send((byte_t*)buffer, buffer_length);
#endif
}

_CC_API_PUBLIC(size_t) _cc_syslog_header(uint8_t pri, tchar_t *buffer, size_t buffer_length) {
#ifndef _CC_SYSLOG_RFC5424_
    tchar_t syslog_timestamp[64];
#endif
    struct tm tm_now;
    time_t now = time(nullptr);

    _cc_gmtime(&now, &tm_now);

#ifdef _CC_SYSLOG_RFC5424_
    // RFC 5424
    return _sntprintf(buffer, buffer_length, _T("<%d>%d %04d-%02d-%02dT%02d:%02d:%02dZ %s %s %d ID:%lld "),
                                (int)pri, _CC_SYSLOG_VERSIOV_, 
                                tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                                tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, 
                                syslog.host, syslog.app, syslog.pid, syslog.fd);
#else
    // RFC 3164
    _tcsftime(syslog_timestamp, _cc_countof(syslog_timestamp), _T("%b %d %H:%M:%S"), &tm_now);
    return _sntprintf(buffer, buffer_length, _T("<%d>%s %s %s[%d]: "),
                                (int)pri, syslog_timestamp, syslog.host, syslog.app, syslog.pid);
#endif
}

/**/
_CC_API_PUBLIC(void) _cc_syslog_send(const byte_t *msg, size_t length) {
    if (syslog.enabled == false || syslog.fd == _CC_INVALID_SOCKET_) {
        return;
    }
    _cc_syslog_lock();
    _cc_sendto(syslog.fd, msg, (int32_t)length, &syslog.sockaddr.addr, syslog.socklen);
    _cc_syslog_unlock();
}

/**/
_CC_API_PUBLIC(void) _cc_open_syslog(byte_t facility, const tchar_t *app, const tchar_t *ip, const uint16_t port) {
    int rcv_buf_size = _CC_8K_BUFFER_SIZE_;
    int snd_buf_size = _CC_8K_BUFFER_SIZE_;
    if (syslog.enabled) {
        return;
    }
    if (ip) {
        syslog.fd = socket(AF_INET, SOCK_DGRAM, 0);
        syslog.socklen = sizeof(struct sockaddr_in);
        bzero(&syslog.sockaddr, syslog.socklen);
        _cc_inet_ipv4_addr((struct sockaddr_in*) &syslog.sockaddr.addr_in, ip, port);
#ifndef __CC_WINDOWS__
    } else {
        struct sockaddr_un *addr = &syslog.sockaddr.addr_un;
        syslog.fd = socket(AF_UNIX, SOCK_DGRAM, 0);
        syslog.socklen = sizeof(struct sockaddr_un);
        bzero(addr, syslog.socklen);
        addr->sun_family = AF_UNIX;
        _tcsncpy(addr->sun_path, _T("/dev/log"), _cc_countof(addr->sun_path));
        addr->sun_path[_cc_countof(addr->sun_path) - 1] = 0;
#endif
    }

    if (syslog.fd == _CC_INVALID_SOCKET_) {
        return;
    }

    setsockopt(syslog.fd, SOL_SOCKET, SO_RCVBUF, (const char*) &rcv_buf_size, sizeof(rcv_buf_size));
    setsockopt(syslog.fd, SOL_SOCKET, SO_SNDBUF, (const char*) &snd_buf_size, sizeof(snd_buf_size));

    _cc_lock_init(&syslog.lock);
    syslog.pid = _cc_getpid();
    syslog.enabled = true;
    syslog.facility = facility;
    
    _tcsncpy(syslog.app, app, _cc_countof(syslog.app));
    syslog.app[_cc_countof(syslog.app) - 1] = 0;

    if (gethostname(syslog.host, _cc_countof(syslog.host))) {
        _tcsncpy(syslog.host, _T("-"), _cc_countof(syslog.host));
        syslog.host[_cc_countof(syslog.host) - 1] = 0;
    }
}

/**/
_CC_API_PUBLIC(void) _cc_close_syslog(void) {
    if (syslog.enabled == false) {
        return;
    }

    _cc_syslog_lock();

    if (syslog.fd) {
        _cc_close_socket(syslog.fd);
    }

    syslog.enabled = false;
    _cc_syslog_unlock();
}
