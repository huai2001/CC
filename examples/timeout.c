#include <stdio.h>
#include <libcc.h>
#include <libcc/widgets/widgets.h>
/*
static char c = 0;
 */
typedef struct timer{
    clock_t tick;
    int32_t ident;
}_timer_t;

static _timer_t timerlist[10];
//static int32_t count = 0;

bool_t _timeout_callback(_cc_async_event_t *async, _cc_event_t *e, const uint16_t which) {
    uint64_t tick = _cc_get_ticks();
    _timer_t *timer = (_timer_t*)e->args;
    time_t t = time(nullptr);
    struct tm* local_time = localtime(&t);
    _tprintf(_T("%04d-%02d-%02d %02d:%02d:%02d "),
             local_time->tm_year + 1900,
             local_time->tm_mon + 1,
             local_time->tm_mday,
             local_time->tm_hour,
             local_time->tm_min,
             local_time->tm_sec);
    _tprintf(_T("timer %d callback %lld\n"), timer->ident, tick - timer->tick);
    
    timer->tick = tick;
    return true;
}

int main (int argc, char * const argv[]) {
    //_cc_async_event_t timers;
    int i;
    uint64_t tick = _cc_get_ticks();

    //_cc_init_event_poller(&timers);
    //_cc_init_event_timeout(&timers);
    _cc_install_async_event(0, nullptr);

    for(i = 1; i < _cc_countof(timerlist); i++) {
        timerlist[i].ident = i + 1000;
        timerlist[i].tick = tick;
        _cc_add_event_timeout(_cc_get_async_event(), 1000 * (i + 1), _timeout_callback, &timerlist[i]);
    }
    while (_cc_async_event_is_running()) {
        //timers.wait(&timers, 100);
        _cc_sleep(100);
    }
}
