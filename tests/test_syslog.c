#include <stdio.h>
#include <libcc.h>


static struct {
    bool_t enabled;
    byte_t facility;

    uint32_t pid;

    _cc_socket_t fd;
    _cc_sockaddr_storage_t sockaddr;
    _cc_socklen_t socklen;
    _cc_mutex_t *lock;

    _cc_sds_t host;
    _cc_sds_t app;

} syslog = {0};

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

static void _alog(uint8_t level, time_t timestamp, const char_t *msg, int32_t length) {
    char_t buffer[_CC_8K_BUFFER_SIZE_];
    size_t buffer_length;
    if (!syslog.enabled) {
        return;
    }
    buffer_length = _cc_syslog_header(_CC_SYSLOG_PRI(syslog.facility,level), buffer, _cc_countof(buffer));
    memcpy(buffer + buffer_length, msg, length);
    buffer_length += length;
    buffer[buffer_length] = '\0';
    _cc_sendto(syslog.fd, buffer, (int32_t)buffer_length, &syslog.sockaddr, syslog.socklen);
}

static void _wlog(uint8_t level, time_t timestamp, const wchar_t *msg, int32_t length) {
    char_t buffer[_CC_8K_BUFFER_SIZE_];
    size_t buffer_length;
    if (!syslog.enabled) {
        return;
    }
    buffer_length = _cc_syslog_header(_CC_SYSLOG_PRI(syslog.facility,level), buffer, _cc_countof(buffer));
    buffer_length += _cc_w2a(msg, (int32_t)length, buffer + buffer_length, (int32_t)(_CC_8K_BUFFER_SIZE_ - buffer_length));
    buffer[buffer_length] = '\0';
    _cc_sendto(syslog.fd, buffer, (int32_t)buffer_length, &syslog.sockaddr, syslog.socklen);
}

static int _dump(void *args) {
    while (syslog.enabled) {
        _cc_loggerA_dump(_alog);
        _cc_loggerW_dump(_wlog);
        _cc_sleep(100);
    }
    return 1;
}

static void _log_init(void) {
    tchar_t host[512] = {0};
    syslog.pid = _cc_getpid();
    syslog.app = _cc_sds_alloc("test_app",0);
    syslog.facility = _CC_LOG_FACILITY_USER_;

    bzero(&syslog.sockaddr, sizeof(syslog.sockaddr));

    if (gethostname(host, _cc_countof(host))) {
        host[0] = '-';
        host[1] = 0;
    }
    syslog.host = _cc_sds_alloc(host,0);

#ifdef __CC_WINDOWS__
    syslog.fd = (_cc_socket_t)socket(AF_INET, SOCK_DGRAM, 0);
    syslog.socklen = sizeof(struct sockaddr_in);
    _cc_inet_ipv4_addr((struct sockaddr_in*) &syslog.sockaddr.addr_in, "127.0.0.1", "514");
#else
    struct sockaddr_un *addr = (struct sockaddr_un *)&syslog.sockaddr;
    syslog.fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    syslog.socklen = sizeof(struct sockaddr_un);
    addr->sun_family = AF_UNIX;
    _tcsncpy(addr->sun_path, _T("/dev/log"), _cc_countof(addr->sun_path));
    addr->sun_path[_cc_countof(addr->sun_path) - 1] = 0;
#endif

    if (syslog.fd == _CC_INVALID_SOCKET_) {
        return;
    }

    syslog.enabled = true;
    setsockopt(syslog.fd, SOL_SOCKET, SO_RCVBUF, (const char*) &rcv_buf_size, sizeof(rcv_buf_size));
    setsockopt(syslog.fd, SOL_SOCKET, SO_SNDBUF, (const char*) &snd_buf_size, sizeof(snd_buf_size));
    _cc_thread_start(_dump, "logger.dump", NULL);
}

static int thread_logger(void *args) {
    int i = 0;
    int thread_id = _cc_get_thread_id(NULL);
    //while (1) {
    for (;i < 200;) {
        _cc_logger_debug("%s id:%d, i:%03f, i:%06u, i:0x%0.7x, %s :0x%0.8xAA", "test string", thread_id, (float)i*0.002f, i, i, "i << 8", i << 8);    
        i++;
    }
    return 1;
}

int main (int argc, char * const argv[]) {
	char c = 0;
    _log_init();
    _cc_thread_start(thread_logger, "syslog", NULL);

    while((c = getchar()) != 'q') {
        _cc_sleep(100);
    }

    syslog.enabled = false;
    _cc_sleep(100);

    return 0;
}