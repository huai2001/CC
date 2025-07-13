#include <stdio.h>
#include <libcc.h>

static struct {
    _cc_atomic32_t total;
    _cc_atomic32_t live;
    _cc_atomic64_t send_size;
    _cc_atomic64_t revice_size;
} monitoring;

static bool_t keep_active = true;
static _cc_event_cycle_t accept_event;

tchar_t* get_disk_size_unit(uint64_t disk_size, tchar_t *buf, int32_t size) {
    if (disk_size < 1024) {
        _sntprintf(buf, size, _T("%lld B"), disk_size);
    } else if (disk_size < 1048576) {
        _sntprintf(buf, size, _T("%lld KB"), disk_size / 1024);
    } else if (disk_size < 1073741824) {
        _sntprintf(buf, size, _T("%lld MB"), disk_size / 1048576);
    } else if (disk_size < 1099511627776) {
        _sntprintf(buf, size, _T("%lld GB"), disk_size / 1073741824);
    } else if (disk_size < 1125899906842624) {
        _sntprintf(buf, size, _T("%lld TB"), disk_size / 1099511627776);
    } else {
        _sntprintf(buf, size, _T("%lld B"), disk_size);
    }

    return buf;
}

/*
static uint32_t getIP(const _cc_event_t* e) {
    struct sockaddr_in addr;
    _cc_socklen_t len = sizeof(struct sockaddr_in);
    if(getpeername(e->fd, (struct sockaddr *)&addr, &len) == -1) {
        int32_t err = _cc_last_errno();
        _cc_ERROR_LOG(_T("discovery client information failed, fd=%d, errno=%d(%#x).\n"), e->fd, err, err);
        return 0;
    }
    return (addr.sin_addr.s_addr);
}*/

static bool_t send_data(_cc_event_cycle_t *cycle, _cc_event_t *e, byte_t *buf, int32_t len) {
    int32_t sent = _cc_send(e->fd, buf, len);//_cc_event_send(e, buf, len);

    if (sent > 0) {
        _cc_atomic64_add(&monitoring.send_size, sent);
        return true;
    }

    _cc_atomic32_dec(&monitoring.live);
	_cc_logger_debug(_T("send len %d fail %d\n"),len, sent);
    return false;
}

static bool_t timeout_event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t which) {
    time_t t = time(nullptr);
    tchar_t unit_size[2][1024];
    struct tm* local_time = localtime(&t);
    _cc_logger_debug(_T("#%04d-%02d-%02d %02d:%02d:%02d delay running %d"),
                local_time->tm_year + 1900,
                local_time->tm_mon + 1,
                local_time->tm_mday,
                local_time->tm_hour,
                local_time->tm_min,
                local_time->tm_sec, 0);

    _cc_logger_debug("total:%d,live:%d,send:%s,revice:%s, %ld/s",monitoring.total, monitoring.live,
        get_disk_size_unit(monitoring.send_size,unit_size[0],_cc_countof(unit_size[0])),
        get_disk_size_unit(monitoring.revice_size,unit_size[1],_cc_countof(unit_size[1])),0
        );
    return true;
}

static bool_t network_event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t which) {
    if (which & _CC_EVENT_ACCEPT_) {
        _cc_socket_t fd;
        _cc_event_t *event;
        _cc_sockaddr_t remote_addr = {0};
        _cc_socklen_t remote_addr_len = sizeof(_cc_sockaddr_t);
        _cc_event_cycle_t *cycle_new = _cc_get_event_cycle();
        fd = _cc_event_accept(cycle, e, &remote_addr, &remote_addr_len);
		if (fd == _CC_INVALID_SOCKET_) {
            _cc_logger_debug(_T("thread %d accept fail %s."), _cc_get_thread_id(nullptr), _cc_last_error(_cc_last_errno()));
            return true;
        }
        
        event = _cc_event_alloc(cycle_new, _CC_EVENT_TIMEOUT_ | _CC_EVENT_READABLE_);
        if (event == nullptr) {
            _cc_close_socket(fd);
            return true;
        }
        _cc_set_socket_nonblock(fd, 1);

        event->fd = fd;
        event->callback = e->callback;
        event->timeout = e->timeout;

        if (cycle_new->attach(cycle_new, event) == false) {
            _cc_logger_debug(_T("thread %d add socket (%d) event fial."), _cc_get_thread_id(nullptr), fd);
            _cc_free_event(cycle_new,event);
            return true;
        }
        /*
        {
            struct sockaddr_in* remote_ip = (struct sockaddr_in*)&remote_addr;
			byte_t *ip_addr = (byte_t *)&remote_ip->sin_addr.s_addr;
            _cc_logger_debug(_T("TCP accept [%d,%d,%d,%d] fd:%d"), ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], fd);
        }*/
        _cc_atomic32_inc(&monitoring.live);
        _cc_atomic32_inc(&monitoring.total);

        return true;
    }
    
	if (which & _CC_EVENT_DISCONNECT_) {
        //_cc_logger_debug(_T("%d disconnect to client."), e->fd);
        _cc_atomic32_dec(&monitoring.live);
        return false;
    }
    
    if (which & _CC_EVENT_READABLE_) {
        /*_cc_event_buffer_t *rw = e->buffer;
        if (e->buffer == nullptr) {
            rw = e->buffer = _cc_alloc_event_buffer();
        }
        if (!_cc_event_recv(e)) {
            //_cc_logger_debug(_T("%d close to client."), e->fd);
            _cc_atomic32_dec(&monitoring.live);
            return false;
        }
        if (rw->r.length > 0) {
            _cc_atomic64_add(&monitoring.revice_size, rw->r.length);
            send_data(cycle, e, rw->r.bytes, rw->r.length);
        }

        //_cc_sleep(1);
        rw->r.length = 0;*/
        byte_t buf[1024 * 16];
        int32_t length;

        length = _cc_recv(e->fd, buf, _cc_countof(buf));
        if (length <= 0) {
            //_cc_logger_debug(_T("%d close to client."), e->fd);
            _cc_atomic32_dec(&monitoring.live);
            return false;
        }

        //buf[length] = 0;
        //_cc_logger_debug("-----\n(%d)\n----\n%s\n",e->fd, buf);

        _cc_atomic64_add(&monitoring.revice_size, length);
        //send_data(cycle, e, buf, length);
        _cc_sleep(1);
        return true;
    }
    
    if (which & _CC_EVENT_WRITABLE_) {
        int32_t off = _cc_event_sendbuf(e);
        if (off < 0) {
            return false;
        }

        _cc_atomic64_add(&monitoring.send_size, off);
        _cc_sleep(1);

        if ((e->flags & _CC_EVENT_WRITABLE_) == 0 && e->flags & _CC_EVENT_DISCONNECT_) {
            return false;
        }
    }
    
	if (which & _CC_EVENT_TIMEOUT_) {
        if (_cc_send(e->fd, (byte_t*)_T("ping"), 5) < 0) {
            _cc_atomic32_dec(&monitoring.live);
            return false;
        }
    }
    
    return true;
}

#ifndef __CC_WINDOWS__
void exit_proc(int sig) {
    //kill(0,SIGTERM);
    puts("\n");
    switch (sig) {
        case 1:
            printf("exit_proc -- SIGHUP\n");
            break;
        case 2:
            printf("exit_proc -- SIGINT\n");
            break;
        case 3:
            printf("exit_proc -- SIGQUIT\n");
            break;
        case 15:
            printf("exit_proc -- KILLALL\n");
            break;
        default:
            printf("exit_proc error - %d\n", sig);
            break;
    }
    _cc_quit_event_loop();
    //_cc_sleep(10000);
    exit(0);
}
#endif

int main (int argc, char * const argv[]) {
	//char c = 0;
    int32_t i = 0;
    struct sockaddr_in sa;
    _cc_event_t *e;
    _cc_event_cycle_t *cycle;
    int16_t port = 10000;

    _cc_install_socket();
    _cc_event_loop(0, nullptr);
    bzero(&monitoring, sizeof(monitoring));
    if (_cc_init_event_poller(&accept_event) == false) {
        return 1;
    }
    
    e = _cc_event_alloc(&accept_event, _CC_EVENT_ACCEPT_);
    if (e == nullptr) {
        return - 1;
    }
    e->timeout = 60000;
    e->callback = network_event_callback;

    _cc_inet_ipv4_addr(&sa, nullptr, port);
    if (!_cc_tcp_listen(&accept_event, e, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
        _cc_free_event(&accept_event, e);
        return -1;
    }
    //
    {
        time_t t = time(nullptr);
        struct tm* local_time = localtime(&t);
        _cc_logger_debug(_T("#%04d-%02d-%02d %02d:%02d:%02d delay"),
                    local_time->tm_year + 1900,
                    local_time->tm_mon + 1,
                    local_time->tm_mday,
                    local_time->tm_hour,
                    local_time->tm_min,
                    local_time->tm_sec);
    }
    cycle = _cc_get_event_cycle();

    _cc_add_event_timeout(cycle, 5000, timeout_event_callback, nullptr);
    //_cc_add_event_timeout(_cc_get_event_cycle(), 1000*60, timeout_event_callback, nullptr);
    
#ifndef __CC_WINDOWS__
    signal(SIGHUP,exit_proc);
    signal(SIGINT,exit_proc);
    signal(SIGQUIT,exit_proc);
    signal(SIGTERM,exit_proc);
#endif
    while (1) {
	//while((c = getchar()) != 'q') {
        _cc_event_wait(&accept_event, 100);
    }
#ifdef __CC_WINDOWS__
    _cc_quit_event_loop();
#endif
    //
    return 0;
}
