#include <libcc/syslog.h>
#include <libcc/socket.h>
#include <libcc/atomic.h>
#include <libcc/time.h>
#include <libcc/sds.h>


#ifdef _CC_USE_SYSLOG_

#ifdef __CC_LINUX__
#include <unistd.h>
#endif

static struct {
    bool_t enabled;
    byte_t facility;

    uint32_t pid;

    _cc_socket_t fd;
    _cc_union_sockaddr_t sockaddr;
    _cc_socklen_t socklen;
    _cc_mutex_t *lock;

    _cc_sds_t host;
    _cc_sds_t app;

} syslog = {0};

/**/
_CC_API_PUBLIC(void) _cc_syslog_lock(void) {
    _cc_mutex_lock(syslog.lock);
}

/**/
_CC_API_PUBLIC(void) _cc_syslog_unlock(void) {
    _cc_mutex_unlock(syslog.lock);
}

_CC_API_PUBLIC(void) _cc_syslog_set_host(const tchar_t *host) {
    if (syslog.host) {
        _cc_sds_free(syslog.host);
    }
    syslog.host = _cc_sds_alloc(host, -1);
}

_CC_API_PUBLIC(void) _cc_syslog_set_app(const tchar_t *app) {
    if (syslog.app) {
        _cc_sds_free(syslog.app);
    }
    syslog.app = _cc_sds_alloc(app, -1);
}

_CC_API_PUBLIC(void) _cc_syslogW(uint8_t level, const wchar_t* msg, size_t length) {
    tchar_t buffer[_CC_8K_BUFFER_SIZE_];
    size_t buffer_length,remaining;
#ifdef _CC_UNICODE_
    uint8_t utf8_buffer[_CC_16K_BUFFER_SIZE_];
#endif
    if (syslog.enabled == false || syslog.fd == _CC_INVALID_SOCKET_) {
        return;
    }
    if (msg == NULL || length < 0) {

    }

    buffer_length = _cc_syslog_header(_CC_SYSLOG_PRI(syslog.facility,level), buffer, _cc_countof(buffer));
    remaining = (_CC_8K_BUFFER_SIZE_ - buffer_length);
#ifdef _CC_UNICODE_
    if (remaining < length) {
        length = remaining;
    }
    memcpy(buffer + buffer_length, msg, length * sizeof(wchar_t));
    buffer_length += length;
#else
    buffer_length += _cc_w2a(msg, (int32_t)length, buffer + buffer_length, (int32_t)remaining);
#endif
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
    if (syslog.enabled == false || syslog.fd == _CC_INVALID_SOCKET_) {
        return;
    }
    if (msg && length > 0) {
        size_t buffer_length = _cc_syslog_header(_CC_SYSLOG_PRI(syslog.facility,level), buffer, _cc_countof(buffer));
        size_t remaining = (_CC_8K_BUFFER_SIZE_ - buffer_length);
#ifdef _CC_UNICODE_
        buffer_length += _cc_a2w(msg, (int32_t)length, buffer + buffer_length, (int32_t)remaining);
#else
        if (remaining < length) {
            length = remaining;
        }
        memcpy(buffer + buffer_length, msg, length);
        buffer_length += length;
#endif
        buffer[buffer_length % _CC_8K_BUFFER_SIZE_] = 0;

#ifdef _CC_UNICODE_
        buffer_length = _cc_utf16_to_utf8((const uint16_t *)buffer, (const uint16_t *)&buffer[buffer_length], 
                                        utf8_buffer, &utf8_buffer[_cc_countof(utf8_buffer) - 1]);
        _cc_syslog_send((byte_t*)utf8_buffer, buffer_length);
#else
        _cc_syslog_send((byte_t*)buffer, buffer_length);
#endif
    }
}

_CC_API_PUBLIC(size_t) _cc_syslog_header(uint8_t pri, tchar_t *buffer, size_t buffer_length) {
#ifndef _CC_SYSLOG_RFC5424_
    tchar_t syslog_timestamp[64];
#endif
    struct tm tm_now;
    time_t now = time(NULL);

    _cc_localtime(&now, &tm_now);

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
    _cc_mutex_lock(syslog.lock);
    _cc_sendto(syslog.fd, msg, (int32_t)length, &syslog.sockaddr.addr, syslog.socklen);
    _cc_mutex_unlock(syslog.lock);
}

/**/
_CC_API_PUBLIC(void) _cc_open_syslog(uint8_t facility, const tchar_t *app, const tchar_t *ip, const uint16_t port) {
    tchar_t host[512] = {0};
    int rcv_buf_size = _CC_8K_BUFFER_SIZE_;
    int snd_buf_size = _CC_8K_BUFFER_SIZE_;
    if (syslog.enabled) {
        return;
    }
    if (ip) {
        syslog.fd = (_cc_socket_t)socket(AF_INET, SOCK_DGRAM, 0);
        syslog.socklen = sizeof(struct sockaddr_in);
        memset(&syslog.sockaddr, 0, syslog.socklen);
        _cc_inet_ipv4_addr((struct sockaddr_in*) &syslog.sockaddr.addr_in, ip, port);
#ifndef __CC_WINDOWS__
    } else {
        struct sockaddr_un *addr = &syslog.sockaddr.addr_un;
        syslog.fd = socket(AF_UNIX, SOCK_DGRAM, 0);
        syslog.socklen = sizeof(struct sockaddr_un);
        memset(addr, 0, syslog.socklen);
        addr->sun_family = AF_UNIX;
        _tcsncpy(addr->sun_path, _T("/dev/log"), _cc_countof(addr->sun_path));
        addr->sun_path[_cc_countof(addr->sun_path) - 1] = 0;
#else
    } else {
        syslog.fd = _CC_INVALID_SOCKET_
#endif
    }
    if (syslog.fd == _CC_INVALID_SOCKET_) {
        return;
    }
    setsockopt(syslog.fd, SOL_SOCKET, SO_RCVBUF, (const char*) &rcv_buf_size, sizeof(rcv_buf_size));
    setsockopt(syslog.fd, SOL_SOCKET, SO_SNDBUF, (const char*) &snd_buf_size, sizeof(snd_buf_size));
    if (gethostname(host, _cc_countof(host))) {
        host[0] = '-';
        host[1] = 0;
    }

    syslog.lock = _cc_alloc_mutex();
    syslog.pid = _cc_getpid();
    syslog.enabled = true;
    syslog.facility = facility;
    syslog.app = _cc_sds_alloc(syslog.app, -1);
    syslog.host = _cc_sds_alloc(host,-1);
}

/**/
_CC_API_PUBLIC(void) _cc_close_syslog(void) {
    if (syslog.enabled == false) {
        return;
    }
    _cc_mutex_lock(syslog.lock);
    if (syslog.fd) {
        _cc_close_socket(syslog.fd);
    }
    syslog.enabled = false;
    _cc_sds_free(syslog.app);
    _cc_sds_free(syslog.host);
    _cc_mutex_unlock(syslog.lock);
    _cc_free_mutex(syslog.lock);
    syslog.lock = NULL;
}
#endif