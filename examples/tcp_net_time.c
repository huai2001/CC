#include <stdio.h>
#include <libcc.h>
#include <locale.h>

byte_t c = 0;
time_t start_time = 0;
_cc_async_event_t async;

int32_t fn_thread(_cc_thread_t *thrd, void* args) {
    while (c != 'q')
        _cc_event_wait(&async, 100);

    return 1;
}

uint32_t getIP(const _cc_event_t* e) {
    struct sockaddr_in addr;
    _cc_socklen_t len = sizeof(struct sockaddr_in);
    if (getpeername(e->fd, (struct sockaddr *)&addr, &len) == -1) {
        int32_t err = _cc_last_errno();
        _cc_logger_error(_T("discovery client information failed, fd=%d, errno=%d(%#x).\n"), e->fd, err, err);
        return 0;
    }
    return (addr.sin_addr.s_addr);
}

void ConvertTime(time_t ulTime) {
    time_t t = 0;

    struct tm* local_time = nullptr;

    t = _cc_mktime(1900, 1, 1, 0, 0, 0, 0);
    //_tprintf("_cc_mktime:%ld\n",t);
    ulTime += t;

    local_time = localtime((const time_t*)&ulTime);
    if (local_time) {
        _tprintf(_T("%04d-%02d-%02d %02d:%02d:%02d\n"),
                 local_time->tm_year + 1900,
                 local_time->tm_mon + 1,
                 local_time->tm_mday,
                 local_time->tm_hour,
                 local_time->tm_min,
                 local_time->tm_sec);
    }
}

static bool_t network_event_callback(_cc_async_event_t *async, _cc_event_t *ev, const uint16_t which) {
    if (which & _CC_EVENT_CONNECT_) {
        start_time = time(nullptr);
        _tprintf(_T(" connect to server!\n"));

        return true;
    }

    if (which & _CC_EVENT_DISCONNECT_) {
        _tprintf(_T("TCP Close - %d\n"), ev->fd);

        return false;
    }
    /**/
    if (which & _CC_EVENT_READABLE_) {
        //uint32_t IPv4 = getIP(ev);
        time_t ulTime = 0;

        if (_cc_recv(ev->fd, (byte_t*)&ulTime, 4) <= 0) {
            _tprintf(_T("TCP close Data %d\n"), ev->fd);
            return false;

        }
        _cc_loggerA_error("test", "test");
        ConvertTime(ntohl(ulTime));
        return true;//send_data(async, ev);;
    }

    if (which & _CC_EVENT_WRITABLE_) {
        if (_cc_event_send(ev, nullptr, 0) < 0) {
            _tprintf(_T(" Fail to send, error = %d\n"), _cc_last_errno());
            return false;
        }
    }

    if (which & _CC_EVENT_TIMEOUT_) {
        _tprintf(_T("TimeOut - %d\n"), ev->fd);
        return true;
    }

    return true;
}

const tchar_t* webUrl[] = {
    _T("time.nist.gov"),
    _T("time.windows.com"),
    _T("time-nw.nist.gov"),
    _T("time-a.nist.gov"),
    _T("time-b.nist.gov")
};

int main (int argc, char * const argv[]) {
    struct sockaddr_in sa;
    _cc_event_t *e;
    setlocale( LC_CTYPE, "chs" );

    _cc_install_socket();

    if (_cc_init_event_poller(&async) == false) {
        return 0;
    }

    _cc_thread_start(fn_thread, _T("net-time"), &async);

    _cc_inet_ipv4_addr(&sa, webUrl[rand() % _cc_countof(webUrl)], 53);

    e = _cc_event_alloc(&async,  _CC_EVENT_CONNECT_ | _CC_EVENT_TIMEOUT_);
    if (e == nullptr) {
        return;
    }
    e->callback = network_event_callback;
    e->timeout = 10000;
    if (!_cc_tcp_connect(&async, e, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
        _cc_free_event(&async, e);
        return;
    }

    while ((c = getchar()) != 'q') {
        _cc_sleep(100);
    }

    async.quit(&async);

    _cc_uninstall_socket();

    return 0;
}
