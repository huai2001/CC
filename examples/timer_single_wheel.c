#include <libcc.h>
#include <stdio.h>
typedef bool_t (*_timer_callback_t)(uint32_t ident);
#define _CC_TIMEOUT_SLOT_ (1 << 4)
enum {
    _CC_INVALID_TIMEOUT_SLOT_ = -1,
    _CC_TIMEOUT_SLOT_PENDING_ = _CC_TIMEOUT_SLOT_,
    _CC_TIMEOUT_SLOT_NOT_TIMERS_,
    _CC_TIMEOUT_MAX_SLOTS_
};

typedef struct _event_timer {
    uint32_t ident;
    uint32_t flags;
    int16_t from;
    int16_t to;
    int32_t rotation;
    int32_t timeout;
    _timer_callback_t callback;
    _cc_list_iterator_t lnk;
} _event_timer_t;

typedef struct _timer_rotation {
    int32_t refcount;
    int32_t limit;
} _timer_rotation_t;

typedef struct _event_cycle {
    int16_t from;
    int32_t spent;
    _cc_list_iterator_t slots[_CC_TIMEOUT_MAX_SLOTS_];
    _timer_rotation_t rotations[_CC_TIMEOUT_SLOT_];
} _event_cycle_t;

bool_t _cleanup_event_timer(_event_cycle_t* cycle, _event_timer_t* e) {
    _cc_free(e);
    return true;
}

bool_t _reset_event_timeout(_event_cycle_t* cycle,
                            _event_timer_t* e,
                            int32_t timeout) {
    int32_t ts = 1;
    _timer_rotation_t* rotation;

    /*
     * timers->from = _CC_TIMEOUT_SLOT_NOT_TIMERS_ indicates that no update is
     * required without a timer flag, but the timer needs to be updated if
     * _CC_EVENT_TIMEOUT_ is added
     */
    if (e->from == _CC_TIMEOUT_SLOT_NOT_TIMERS_ &&
        _CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, e->flags) == 0) {
        return true;
    }

    if (!_cc_list_iterator_empty(&e->lnk)) {
        _cc_list_iterator_remove(&e->lnk);
        e->from = _CC_INVALID_TIMEOUT_SLOT_;
    }

    /*
     * The timer flag has been cancelled and the timer removed
     */
    if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, e->flags) == 0) {
        _cc_list_iterator_push(&cycle->slots[_CC_TIMEOUT_SLOT_NOT_TIMERS_],
                                 &e->lnk);
        e->from = _CC_TIMEOUT_SLOT_NOT_TIMERS_;
        return true;
    }

    if (e->timeout >= timeout) {
        ts = e->timeout / timeout;
    }

    e->from = (e->to + (ts % _CC_TIMEOUT_SLOT_)) % _CC_TIMEOUT_SLOT_;
    rotation = &cycle->rotations[e->from];
    e->rotation = (ts / _CC_TIMEOUT_SLOT_) + rotation->refcount;

    /*_tprintf(_T("1.timeout:(%d=%d/%d) add timer slot:%d, rotation:%d \n"), ts,
     * e->timers.timeout, timeout, e->timers.from, e->timers.rotation);*/

    if (_cc_list_iterator_empty(&cycle->slots[e->from])) {
        rotation->limit = e->rotation;
    } else if ((rotation->limit - rotation->refcount) > e->rotation) {
        rotation->limit = e->rotation;
    }

    _cc_list_iterator_push(&cycle->slots[e->from], &e->lnk);

    return true;
}

/**/
bool_t _cleanup_event_timeout(_event_cycle_t* cycle, _event_timer_t* e) {
    /*_tprintf(_T("3.delete timer slot:%d, rotation:%d \n"), e->timers.from,
     * e->timers.rotation);*/

    /**/
    if (e->from == _CC_TIMEOUT_SLOT_PENDING_) {
        return true;
    }

    if (!_cc_list_iterator_empty(&e->lnk)) {
        _cc_list_iterator_remove(&e->lnk);
        e->from = _CC_INVALID_TIMEOUT_SLOT_;
    }

    _cc_list_iterator_push(&cycle->slots[_CC_TIMEOUT_SLOT_PENDING_], &e->lnk);
    e->from = _CC_TIMEOUT_SLOT_PENDING_;
    return true;
}

bool_t _running_event_timeout(_event_cycle_t* cycle, int32_t timeout) {
    int16_t from = cycle->from;
    int32_t limit = 0;
    _timer_rotation_t* rotation = &cycle->rotations[from];

    if (_cc_list_iterator_empty(&cycle->slots[from])) {
        goto TIMEOUT_RUNNING_END;
    }

    rotation->refcount++;

    if (rotation->limit < rotation->refcount) {
        _cc_list_iterator_for_each(val, &cycle->slots[from], {
            _event_timer_t* e = _cc_upcast(val, _event_timer_t, lnk);
            e->rotation -= rotation->limit;
            if (e->rotation > 0) {
                if (limit == 0 || limit > e->rotation) {
                    limit = e->rotation;
                }
                continue;
            }

            /**/
            if ((e->flags & _CC_EVENT_DISCONNECT_) == 0) {
                /*_tprintf(_T("2.timeout:(%d=%d/%d) add timer slot:%d,
                   rotation:%d\n"), e->fd, e->timers.timeout, timeout,
                   e->timers.from, e->timers.rotation);*/
                //_cc_event_callback(cycle, e, _CC_EVENT_TIMEOUT_);
                if (e->callback) {
                    if (e->callback(e->ident)) {
                        _cleanup_event_timeout(cycle, e);
                    } else {
                        _cleanup_event_timer(cycle, e);
                    }
                }
            } else {
                _cleanup_event_timer(cycle, e);
            }
        });

        rotation->limit = limit;
        rotation->refcount = 0;
    }

TIMEOUT_RUNNING_END:
    cycle->from = (from + 1) % _CC_TIMEOUT_SLOT_;

    return true;
}

_CC_FORCE_INLINE_ void _reset_event(_event_cycle_t* cycle,
                                    _event_timer_t* e,
                                    int32_t timeout) {
    if (_CC_ISSET_BIT(_CC_EVENT_DISCONNECT_, e->flags)) {
        /*delete*/
        _cleanup_event_timer(cycle, e);
    } else {
        _reset_event_timeout(cycle, e, timeout);
    }
}

_CC_FORCE_INLINE_ void _reset(_event_cycle_t* cycle,
                              int32_t timeout,
                              _cc_list_iterator_t* slot) {
    _cc_list_iterator_for_each(v, slot, {
        _reset_event(cycle, _cc_upcast(v, _event_timer_t, lnk), timeout);
    });
}

/**/
_CC_FORCE_INLINE_ bool_t _timeout_wait(_event_cycle_t* cycle, int32_t timeout) {
    uint32_t started_tick = 0;
    int32_t spent = 0;
    int32_t event_spent_time = 0;

    started_tick = _cc_get_ticks();

    _reset(cycle, timeout, &cycle->slots[_CC_TIMEOUT_SLOT_PENDING_]);
    _cc_list_iterator_cleanup(&cycle->slots[_CC_TIMEOUT_SLOT_PENDING_]);

    spent = timeout - cycle->spent;
    if (spent > 0) {
        _cc_sleep(spent);
    }

    event_spent_time = (int32_t)(_cc_get_ticks() - started_tick);
    spent = cycle->spent + event_spent_time;
    if (spent >= timeout) {
        _running_event_timeout(cycle, timeout);
        cycle->spent =
            ((cycle->spent + (int32_t)(_cc_get_ticks() - started_tick)) -
             timeout);
    } else {
        cycle->spent += event_spent_time;
    }

    return true;
}

bool_t _timeout_callback(uint32_t ident) {
    time_t t = time(NULL);
    struct tm* local_time = localtime(&t);
    _tprintf(_T("%04d-%02d-%02d %02d:%02d:%02d %d\n"),
             local_time->tm_year + 1900, local_time->tm_mon + 1,
             local_time->tm_mday, local_time->tm_hour, local_time->tm_min,
             local_time->tm_sec, ident);
    return true;
}

void add_event_timeout(_event_cycle_t* cycle,
                       uint32_t ident,
                       uint32_t timeout,
                       _timer_callback_t callback,
                       pvoid_t args) {
    _event_timer_t* e = (_event_timer_t*)_cc_malloc(sizeof(_event_timer_t));
    bzero(e, sizeof(_event_timer_t));
    e->callback = callback;
    e->timeout = timeout;
    e->flags = _CC_EVENT_TIMEOUT_;
    e->ident = ident;

    _cc_list_iterator_push(&cycle->slots[_CC_TIMEOUT_SLOT_PENDING_], &e->lnk);
    e->from = _CC_TIMEOUT_SLOT_PENDING_;
    e->to = cycle->from;
}

int main(int argc, char* const argv[]) {
    _event_cycle_t timers;
    int i;
    time_t t = time(NULL);
    struct tm* local_time = localtime(&t);

    bzero(&timers, sizeof(_event_cycle_t));
    timers.from = 0;
    timers.spent = 0;

    for (i = 0; i < _cc_countof(timers.slots); i++) {
        _cc_list_iterator_cleanup(&timers.slots[i]);
    }
    bzero(timers.rotations, sizeof(timers.rotations));

    _tprintf(_T("add %04d-%02d-%02d %02d:%02d:%02d \n"),
             local_time->tm_year + 1900, local_time->tm_mon + 1,
             local_time->tm_mday, local_time->tm_hour, local_time->tm_min,
             local_time->tm_sec);

    add_event_timeout(&timers, 1, 1000, _timeout_callback, NULL);
    add_event_timeout(&timers, 2, 10000, _timeout_callback, NULL);

    while (1) {
        _timeout_wait(&timers, 100);
    }
}
