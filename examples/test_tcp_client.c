#include <stdio.h>
#include <libcc.h>
#include <locale.h>

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

static bool_t network_event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t which) {
    if (which & _CC_EVENT_CONNECTED_) {
        _cc_logger_debug(_T("%d connect to server."), e->fd);
        return true;
    }

    if (which & _CC_EVENT_DISCONNECT_) {
        int32_t results = _cc_last_errno();
        _cc_logger_error(_T("Socket(%d) disconnect:%d, %s"), e->fd, results, _cc_last_error(results));
        return false;
    }
    
    if (which & _CC_EVENT_READABLE_) {
        /*
        _cc_event_buffer_t *rw = e->buffer;
        if (!_cc_event_recv(e)) {
            _cc_logger_debug(_T("%d close to client."), e->fd);
            return false;
        }
        rw->r.length = 0;*/
        byte_t buf[1024 * 16];
        int32_t length;

        length = _cc_recv(e->fd, buf, _cc_countof(buf));
        if (length <= 0) {
            _cc_logger_debug(_T("%d close to client."), e->fd);
            return false;
        }
        return true;
    }

    if (which & _CC_EVENT_WRITABLE_) {
        if (which & _CC_EVENT_WRITABLE_) {
            int32_t off = _cc_event_sendbuf(e);
            if (off < 0) {
                return false;
            }
            _cc_sleep(1);

            if ((e->flags & _CC_EVENT_WRITABLE_) == 0 && e->flags & _CC_EVENT_DISCONNECT_) {
                return false;
            }
        }
    }
    
    if (which & _CC_EVENT_TIMEOUT_) {
        if (_cc_send(e->fd, (byte_t*)"ping\0", 5) < 0) {
            _cc_logger_debug(_T("TCP timeout %d"), e->fd);
            _cc_atomic32_dec(&monitoring.live);
            return false;
        }
        return true;
    }
    
    return true;
}

void testes_connect(const tchar_t* host, const uint16_t port) {
    int c = 0;
    int r = 0;
    struct sockaddr_in sa;
    _cc_event_t *e;
    _cc_event_cycle_t *cycle = _cc_get_event_cycle();

    e = _cc_event_alloc(cycle,  _CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_);
    if (e == nullptr) {
        return;
    }

    e->callback = network_event_callback;
    e->timeout = r * 1000;

    _cc_inet_ipv4_addr(&sa, host, port);
    if (!_cc_tcp_connect(cycle, e, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
        _cc_free_event(cycle, e);
        return;
    }
}


#define TCP_CLIENT_IP _T("127.0.0.1")

int _tmain (int argc, tchar_t * const argv[]) {
    char c = 0;

    srand((uint32_t)time(nullptr));
    _cc_event_loop(0, nullptr);

    testes_connect(TCP_CLIENT_IP,8080);

    while((c = getchar()) != 'q') {
        _cc_sleep(100);
    }
    _cc_quit_event_loop();
    
    return 0;
}
