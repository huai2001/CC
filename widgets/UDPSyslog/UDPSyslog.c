#include <stdio.h>
#include <locale.h>
#include "UDPSyslog.h"

tchar_t currentPath[_CC_MAX_PATH_] = {0};
static bool_t running = false;
struct syslog_name {
    tchar_t *name;
    int code;
};
/* here we host some syslog specific names. There currently is no better place
 * to do it, but over here is also not ideal... -- rgerhards, 2008-02-14
 * rgerhards, 2008-04-16: note in LGPL move: the code tables below exist in
 * the same way in BSD, so it is not a problem to move them from GPLv3 to LGPL.
 * And nobody modified them since it was under LGPL, so we can also move it
 * to ASL 2.0.
 */

struct syslog_name _severities[] = {
  {"emerg",         _CC_LOG_LEVEL_EMERG_ },
  {"emergency",     _CC_LOG_LEVEL_EMERG_ },
  {"panic",         _CC_LOG_LEVEL_EMERG_ },
  {"alert",         _CC_LOG_LEVEL_ALERT_ },
  {"crit",          _CC_LOG_LEVEL_CRIT_ },
  {"critical",      _CC_LOG_LEVEL_CRIT_ },
  {"err",           _CC_LOG_LEVEL_ERROR_ },
  {"error",         _CC_LOG_LEVEL_ERROR_ },
  {"warning",       _CC_LOG_LEVEL_WARNING_ },
  {"warn",          _CC_LOG_LEVEL_WARNING_ },
  {"notice",        _CC_LOG_LEVEL_NOTICE_ },
  {"info",          _CC_LOG_LEVEL_INFO_ },
  {"informational", _CC_LOG_LEVEL_INFO_ },
  {"debug",         _CC_LOG_LEVEL_DEBUG_ },
  {NULL, -1}
};

struct syslog_name _facilities[] = {
  {"kern",          _CC_LOG_FACILITY_KERN_  },
  {"user",          _CC_LOG_FACILITY_USER_  },
  {"mail",          _CC_LOG_FACILITY_MAIL_  },
  {"daemon",        _CC_LOG_FACILITY_DAEMON_  },
  {"auth",          _CC_LOG_FACILITY_AUTH_  },
  {"syslog",        _CC_LOG_FACILITY_SYSLOG_  },
  {"lpr",           _CC_LOG_FACILITY_LPR_  },
  {"news",          _CC_LOG_FACILITY_NEWS_  },
  {"uucp",          _CC_LOG_FACILITY_UUCP_  },
  {"cron",          _CC_LOG_FACILITY_CRON_  },
  {"authpriv",      _CC_LOG_FACILITY_AUTHPRIV_ },
  {"megasafe",      _CC_LOG_FACILITY_AUTHPRIV_ }, /* DEC UNIX AdvFS logging */
  {"ftp",           _CC_LOG_FACILITY_FTP_ },
  {"ntp",           _CC_LOG_FACILITY_NTP_ },
  {"security",      _CC_LOG_FACILITY_SECURITY_ },
  {"console",       _CC_LOG_FACILITY_CONSOLE_ },
  {"solaris-cron",  _CC_LOG_FACILITY_CRON_ },

  {"local0",        _CC_LOG_FACILITY_LOCAL0_ },
  {"local1",        _CC_LOG_FACILITY_LOCAL1_ },
  {"local2",        _CC_LOG_FACILITY_LOCAL2_ },
  {"local3",        _CC_LOG_FACILITY_LOCAL3_ },
  {"local4",        _CC_LOG_FACILITY_LOCAL4_ },
  {"local5",        _CC_LOG_FACILITY_LOCAL5_ },
  {"local6",        _CC_LOG_FACILITY_LOCAL6_ },
  {"local7",        _CC_LOG_FACILITY_LOCAL7_ },
  {NULL, -1}
};


enum { 
    SYSLOG_ST_PRI, 
    SYSLOG_ST_TIMESTAMP,
    SYSLOG_ST_HOSTNAME,
    SYSLOG_ST_APPNAME,
    SYSLOG_ST_TAG,
    SYSLOG_ST_PID,
    SYSLOG_ST_MID,
    SYSLOG_ST_SD,
    SYSLOG_ST_MSG 
};

_CC_API_PRIVATE(tchar_t*) to_number(tchar_t *buf, int *dest) {
    int result = 0;

    /* is zero */
    while (*buf == '0') {
        buf++;
    }

    while (_CC_ISDIGIT(*buf)) {
        result = (result * 10) + (*buf++ - '0');
    }

    *dest = result;
    return buf;
}

_CC_API_PRIVATE(bool_t) parse(char_t *buf, size_t length) {
    _syslog_t syslog;
    char_t *tmp;
    char_t *log = buf;
    uint8_t state = SYSLOG_ST_PRI;
    struct tm timestamp;
    time_t now = time(nullptr);

    if (*log != '<') {
        return false;
    }
    bzero(&syslog, sizeof(_syslog_t));
    _cc_gmtime(&now, &timestamp);

    // RFC3164
    // <PRI>Timestamp Hostname Tag: Message
    // RFC5424
    // <PRI>Version Timestamp Hostname Appname pid mid [SD] Message

    syslog.version = 0;//0 = RFC3164 1 = RFC5424
    log++;
    while (*log) {
        /* Eat up white-space. */
        while (_istspace(*log)) {
            log++;
        }
        
        tmp = log;
        if (*log == 0) {
            break;
        }

        switch(state) {
            case SYSLOG_ST_PRI: {
                log = to_number(log, (int*)&syslog.priority);
                if (tmp == log || *log != '>') {
                    return false;
                }
                state = SYSLOG_ST_TIMESTAMP;
            }
                break;
            case SYSLOG_ST_TIMESTAMP: {
                if (*log == '1' && *(log + 1) == ' ') {
                    syslog.version = 1;
                    break;
                }
                if (syslog.version == 0) {
                    log = (char_t*)_cc_strptime(log, "%b %d %H:%M:%S", &timestamp);
                    if (log == nullptr) {
                        return false;
                    }
                } else if (syslog.version == 1) {
                    log = (char_t*)_cc_strptime(log, "%Y-%m-%dT%H:%M:%SZ", &timestamp);
                    if (log == nullptr) {
                        return false;
                    }
                }
                syslog.timestamp = mktime(&timestamp);
                state = SYSLOG_ST_HOSTNAME;
            }
                break;
            case SYSLOG_ST_PID:
                state = SYSLOG_ST_MID;
                log = to_number(log, &syslog.pid);
                if (tmp == log) {
                    continue;
                }
            break;
            case SYSLOG_ST_MID:
                state = SYSLOG_ST_SD;
                if (*log == 'I' && *(log + 1) == 'D' && *(log + 2) == ':') {
                    log = to_number(log + 3, &syslog.mid);
                    if (log == (tmp + 3)) {
                        state = SYSLOG_ST_MSG;
                        continue;
                    }
                    log++;
                }
                break;
            case SYSLOG_ST_SD:
                state = SYSLOG_ST_MSG;
                if (*log == '[') {
                    tmp = log;
                    log = strchr(log, ']');
                    if (log) {
                        syslog.sd.data = tmp;
                        syslog.sd.length = (log - tmp) + 1;
                    } else {
                        log = tmp;
                        continue;
                    }
                }
                break;
            default: {
                while (*log && !_istspace(*log) ) {
                    log++;
                }
                if (*log == 0) {
                    state = SYSLOG_ST_MSG;
                    continue;
                }
                switch (state) {
                case SYSLOG_ST_HOSTNAME:    //RFC5424 | RFC3164
                    state = syslog.version ? SYSLOG_ST_APPNAME:SYSLOG_ST_TAG;
                    syslog.host.data = tmp;
                    syslog.host.length = (log - tmp);
                    break;
                case SYSLOG_ST_APPNAME:     //RFC5424
                    state = SYSLOG_ST_PID;
                    syslog.app.length = (log - tmp);
                    syslog.app.data = tmp;
                    break;
                case SYSLOG_ST_TAG:         //RFC3164
                    state = SYSLOG_ST_MSG;
                    if (*(log - 1) == ':') {
                        syslog.app.length = (log - tmp) - 1;
                        syslog.app.data = tmp;
                    } else {
                        log = tmp;
                        continue;
                    }
                    break;
                }
            }
        }
        log++;
    }

    if (log > tmp) {
        syslog.msg.length = (log - tmp);
        syslog.msg.data = tmp;
    }

    sqlite3_syslog(&syslog);
    
    return true;
}

_CC_API_PRIVATE(bool_t) udp(_cc_event_cycle_t *cycle, _cc_event_t *e, uint16_t which) {
    if (which & _CC_EVENT_READABLE_) {
        struct sockaddr_in remote_addr;
        socklen_t addr_len = sizeof(struct sockaddr_in);
        char buffer[_CC_32K_BUFFER_SIZE_];
        size_t length = recvfrom(e->fd, buffer, _cc_countof(buffer), 0, (struct sockaddr *)&remote_addr, &addr_len);

        if (length < 0) {
            _cc_logger_error(_T("recvfrom error: %d"), errno);
            return true;
        }

        buffer[length % _CC_32K_BUFFER_SIZE_] = '\0';
        if (!parse(buffer, length)) {
            sqlite3_exception_syslog(buffer, length);
        }
        _tprintf(_T("Received Syslog: %s\n"), buffer);
    }
    return true;
}

/**/
_CC_API_PRIVATE(int32_t) thread_running(_cc_thread_t *thread, void *args) {
    _cc_event_cycle_t *cycle = (_cc_event_cycle_t *)args;
    while (running) {
        cycle->wait(cycle, 100);
    }
    cycle->quit(cycle);
    return 1;
}

_CC_API_PUBLIC(bool_t) start(int16_t port) {
    struct sockaddr_in sin;
    _cc_event_cycle_t cycle;
    _cc_event_t *event;
    _cc_socket_t io_fd = _CC_INVALID_SOCKET_;

    _cc_install_socket();
    _cc_get_cwd(currentPath,_CC_MAX_PATH_);
    openSQLite3();

    io_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(io_fd == -1) {
        printf("socket err\n");
        return false;
    }

    _cc_inet_ipv4_addr(&sin, nullptr, port);

    if(bind(io_fd, (struct sockaddr *)&sin, sizeof(sin)) == -1){
        fprintf(stderr, "%d bing port error\n", port);
        return false;
    }

    _tprintf(_T("UDSysPLog Listen Port: %d\n"), port);

    _cc_init_event_select(&cycle);

    event = _cc_event_alloc(&cycle,  _CC_EVENT_READABLE_);
    if (event == nullptr) {
        _cc_close_socket(io_fd);
        return false;
    }
    event->callback = udp;
    event->fd = io_fd;

    _cc_set_socket_nonblock(io_fd, 1);

    if (!cycle.attach(&cycle, event)) {
        _cc_logger_debug(_T("thread %d add socket (%d) event fial."), _cc_get_thread_id(nullptr), io_fd);
        _cc_free_event(&cycle, event);
        return false;
    }

    running = true;
    //running = _cc_thread_start(thread_running, "UDPSyslog", &cycle);
    while (running) {
        cycle.wait(&cycle, 100);
    }
    cycle.quit(&cycle);
    return running;
}


_CC_API_PUBLIC(void) stop(void) {
    running = false;
    closeSQLit3();
    _cc_uninstall_socket();
}


int main (int argc, char * const argv[]) {
    char c;
    setlocale( LC_CTYPE, "chs" );
    //SetConsoleOutputCP(65001);

    start(_CC_SYSLOG_PORT_);

    while((c = getchar()) != 'q') {
        if (c == 'c') {
            //system("cls");
        }
        _cc_sleep(1000);
    }
    
    stop();
    return 0;
}

