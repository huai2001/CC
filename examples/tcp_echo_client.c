#include <stdio.h>
#include <libcc.h>
#include <locale.h>

static int32_t max_client_count = 0;

static struct {
    const tchar_t *ip;
    uint16_t port;
    _cc_atomic32_t total;
    _cc_atomic32_t live;
    _cc_atomic64_t send_size;
    _cc_atomic64_t revice_size;
}monitoring;

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

static bool_t send_data(_cc_async_event_t *async, _cc_event_t *e) {
static _cc_Strint_t send_str = _cc_String(\
    _T("GET / HTTP/1.0\r\n")\
    _T("Host: 47.96.85.231\r\n")\
    _T("User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.0; Trident/4.0)\r\n")\
    _T("Accept: */*\r\n")\
    _T("Cache-Control: no-cache\r\n")\
    _T("Pragma: no-cache\r\n")\
    _T("Connection: close\r\n")\
    _T("\r\n"));
    int32_t sent = _cc_send(e->fd, (byte_t*)send_str.data, (uint16_t)send_str.length);
    if (sent > 0){
		_cc_atomic64_add(&monitoring.send_size, sent);
        return true;
	}
	_cc_atomic32_dec(&monitoring.live);
    return false;
}

static bool_t network_event_callback(_cc_async_event_t *async, _cc_event_t *e, const uint16_t which) {
    if (which & _CC_EVENT_CONNECTED_) {
        //_cc_logger_debug(_T("%d connect to server."), e->fd);
        _cc_atomic32_inc(&monitoring.live);
        return true;//send_data(async, e);
    }

    if (which & _CC_EVENT_DISCONNECT_) {
        int32_t results = _cc_last_errno();
        _cc_logger_error(_T("Socket(%d) disconnect:%d, %s"), e->fd, results, _cc_last_error(results));
        _cc_atomic32_dec(&monitoring.live);
        return false;
    }
    
    if (which & _CC_EVENT_READABLE_) {
        /*
        _cc_event_buffer_t *rw = e->buffer;
        if (!_cc_event_recv(e)) {
            _cc_logger_debug(_T("%d close to client."), e->fd);
            _cc_atomic32_dec(&monitoring.live);
            return false;
        }

        if (rw->r.length > 0) {
            _cc_atomic64_add(&monitoring.revice_size, rw->r.length);
        }

        _cc_sleep(1);
        rw->r.length = 0;*/
        byte_t buf[1024 * 16];
        int32_t length;

        length = _cc_recv(e->fd, buf, _cc_countof(buf));
        if (length <= 0) {
            //_cc_logger_debug(_T("%d close to client."), e->fd);
            _cc_atomic32_dec(&monitoring.live);
            return false;
        }

        _cc_atomic64_add(&monitoring.revice_size, length);
        return true;
    }

    if (which & _CC_EVENT_WRITABLE_) {
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
    }
    
    if (which & _CC_EVENT_TIMEOUT_) {
        /*
        if (_cc_send(e->fd, (byte_t*)"ping", 5) < 0) {
            _cc_logger_debug(_T("TCP timeout %d"), e->fd);
            _cc_atomic32_dec(&monitoring.live);
            return false;
        }*/
        return send_data(async, e);
    }
    
    return true;
}

uint64_t tick_timer = 0;
void testes_connect(const tchar_t* host, const uint16_t port, int32_t count) {
    int c = 0;
    int r = 0;
    struct sockaddr_in sa;
    _cc_event_t *e;
    for (c = 0; c < count; c++) {
        _cc_async_event_t *async = _cc_get_async_event();
        r = (rand() % 15) + 5;

        e = _cc_event_alloc(async,  _CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_);
        if (e == nullptr) {
            return;
        }
        e->callback = network_event_callback;
        e->timeout = r * 1000;
        _cc_inet_ipv4_addr(&sa, host, port);
        if (!_cc_tcp_connect(async, e, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
            _cc_free_event(async, e);
            return;
        }
        
        monitoring.total++;
        _cc_sleep(1);
    }
}


#define TCP_CLIENT_IP _T("127.0.0.1")
#define TCP_CLIENT_PORT 10000
#define TCP_MAX_CONNECT 100

int32_t i = 0;
tchar_t unit_size[2][1024];

bool_t _timeout_callback(_cc_async_event_t *async, _cc_event_t *e, const uint16_t which) {
    uint64_t t = _cc_get_ticks();
    //
    _cc_logger_debug("total:%d,live:%d,send:%s,revice:%s - %d/s tick_timer:%ld",monitoring.total, monitoring.live,
           get_disk_size_unit(monitoring.send_size,unit_size[0],_cc_countof(unit_size[0])),
           get_disk_size_unit(monitoring.revice_size,unit_size[1],_cc_countof(unit_size[1])), i++,t - tick_timer);

    tick_timer = t;

    if (monitoring.live < max_client_count) {
        int32_t count = max_client_count - monitoring.live;
        testes_connect(monitoring.ip, monitoring.port, _min(count, TCP_MAX_CONNECT));
        //_cc_logger_debug(_T("timer connect: %d"), count);
    }

    return true;
}

int _tmain (int argc, tchar_t * const argv[]) {
    char c = 0;

    srand((uint32_t)time(nullptr));
    _cc_install_async_event(0, nullptr);

    bzero(&monitoring, sizeof(monitoring));

    if (argc < 3) {
        max_client_count = 50000;
        monitoring.ip = TCP_CLIENT_IP;
        monitoring.port = TCP_CLIENT_PORT;
    } else {
        max_client_count = (int32_t)_ttol(argv[3]);
        monitoring.ip = argv[1];
        monitoring.port = _ttoi(argv[2]);
    }

    testes_connect(monitoring.ip, monitoring.port, _min(max_client_count, TCP_MAX_CONNECT));
    tick_timer = _cc_get_ticks();
    _cc_add_event_timeout(_cc_get_async_event(), 5000, _timeout_callback, nullptr);

    while((c = getchar()) != 'q') {
        _cc_sleep(100);
    }
    _cc_uninstall_async_event();
    
    return 0;
}
