#include <stdio.h>
#include <locale.h>
#include <libcc/timeout.h>

//A simple example program which connects to a server

static bool_t timeout_handler(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {  
    tchar_t syslog_timestamp[64];
    struct tm tm_now;
    time_t now = time(NULL);

    _cc_localtime(&now, &tm_now);
    _tcsftime(syslog_timestamp, _cc_countof(syslog_timestamp), _T("%b %d %H:%M:%S"), &tm_now);
    if (which & _CC_EVENT_TIMEOUT_) {
        printf(_T("%llu timer timeout. arg:%ld : %s\n"), e->ident, e->data, syslog_timestamp);
    } else if (which & _CC_EVENT_CLOSED_) {
        printf(_T("%llu destroy timeout. arg:%ld %s\n"), e->ident, e->data, syslog_timestamp);
        return false;
    }
    // 返回 true 继续定时触发，false 停止计时器并释放
    return true;
}


int main (int argc, char * const argv[]) {
    int c;
    _cc_event_t *event;
    _cc_async_event_t async;
    _cc_register_timeout(&async);

    //创建一个定时器
    _cc_add_event_timeout(&async, 1000, timeout_handler, 1);
    //
    event = _cc_add_event_timeout(&async, 20000, timeout_handler, 2);
    //删除定时器
    _cc_kill_event_timeout(&async, event);
    
    // Event loop
    while((async.flag & 0x0F) == 1) {
        async.wait(&async, 100/* 0.1 sec */);
    }

    //关闭所有连接，并释放所有资源。
    async.free(&async);
    return 0;
}
