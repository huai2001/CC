#include <stdio.h>
#include <libcc.h>
#include <cc/widgets/widgets.h>
/*
static char c = 0;

typedef struct timer{
    clock_t tick;
    int32_t idx;
}_timer_t;

static _timer_t timerlist[2];
//static int32_t count = 0;
 */
bool_t _timeout_callback(_cc_event_cycle_t *timer, _cc_event_t *e, const uint16_t events) {
    clock_t tick = _cc_get_ticks();
    //_timer_t *timer = (_timer_t*)e->args;
    time_t t = time(NULL);
    struct tm* local_time = localtime(&t);
    _tprintf(_T("%04d-%02d-%02d %02d:%02d:%02d "),
             local_time->tm_year + 1900,
             local_time->tm_mon + 1,
             local_time->tm_mday,
             local_time->tm_hour,
             local_time->tm_min,
             local_time->tm_sec);
    _tprintf(_T("timer %d callback %ld\n"), e->ident, tick);
    
    return true;
}

int main (int argc, char * const argv[]) {
    //_cc_event_cycle_t timers;
    int32_t tick = _cc_get_ticks();
    time_t t = time(NULL);
    struct tm* local_time = localtime(&t);
    
    //_cc_init_event_poller(&timers);
    //_cc_init_event_timeout(&timers);
    _cc_event_loop(2, NULL);
    _tprintf(_T("add %04d-%02d-%02d %02d:%02d:%02d, %d\n"),
             local_time->tm_year + 1900,
             local_time->tm_mon + 1,
             local_time->tm_mday,
             local_time->tm_hour,
             local_time->tm_min,
             local_time->tm_sec, tick);

    
    _cc_add_event_timeout(_cc_get_event_cycle(), 1000, _timeout_callback, NULL);
    //_cc_add_event_timeout(_cc_get_event_cycle(), 3000, _timeout_callback, NULL);
    while (_cc_event_loop_is_running()) {
        //timers.driver.wait(&timers, 100);
        _cc_sleep(100);
    }
}
