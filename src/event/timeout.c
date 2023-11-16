/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
 * All rights reserved.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/
#include "event.c.h"
#include <cc/alloc.h>
#include <cc/event/timeout.h>

_CC_API_PRIVATE(void) _timeout_add(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    uint32_t elapsed;
    uint32_t expire;
    uint32_t mask;
    int i;

    expire = e->timers + e->timeout;
    elapsed = cycle->timers;

    if ((expire | _CC_TIMEOUT_NEAR_MASK_) == (elapsed | _CC_TIMEOUT_NEAR_MASK_)) {
        _cc_list_iterator_swap(&cycle->nears[expire & _CC_TIMEOUT_NEAR_MASK_], &e->lnk);
        return;
    }

    mask = _CC_TIMEOUT_NEAR_ << _CC_TIMEOUT_LEVEL_SHIFT_;
    for (i = 0; i < 3; i++) {
        if ((expire | (mask - 1)) == (elapsed | (mask - 1))) {
            break;
        }
        mask <<= _CC_TIMEOUT_LEVEL_SHIFT_;
    }

    mask = ((expire >> (_CC_TIMEOUT_NEAR_SHIFT_ + i * _CC_TIMEOUT_LEVEL_SHIFT_))) & _CC_TIMEOUT_LEVEL_MASK_;
    _cc_list_iterator_swap(&cycle->level[i][mask], &e->lnk);
}

_CC_API_PRIVATE(void) _timeout_move_list(_cc_event_cycle_t *cycle, int level, int ident) {
    _cc_list_iterator_t *head = &cycle->level[level][ident];
    _cc_list_iterator_for_each(it, head, { _timeout_add(cycle, _cc_upcast(it, _cc_event_t, lnk)); });
    _cc_list_iterator_cleanup(&cycle->level[level][ident]);
}

_CC_API_PRIVATE(void) _timeout_shift(_cc_event_cycle_t *cycle) {
    int m;
    int i;
    uint32_t v;
    uint32_t tick;

    tick = ++cycle->timers;
    if (tick == 0) {
        _timeout_move_list(cycle, 3, 0);
        return;
    }

    m = _CC_TIMEOUT_NEAR_;
    v = tick >> _CC_TIMEOUT_NEAR_SHIFT_;
    for (i = 0; (tick & (m - 1)) == 0; i++) {
        int ident = v & _CC_TIMEOUT_LEVEL_MASK_;
        if (ident != 0) {
            _timeout_move_list(cycle, i, ident);
            break;
        }
        m <<= _CC_TIMEOUT_LEVEL_SHIFT_;
        v >>= _CC_TIMEOUT_LEVEL_SHIFT_;
    }
}

_CC_API_PRIVATE(void) _timeout_execute(_cc_event_cycle_t *cycle) {
    int i;
    i = cycle->timers & _CC_TIMEOUT_NEAR_MASK_;

    if (!_cc_list_iterator_empty(&cycle->nears[i])) {
        _cc_list_iterator_for_each(it, &cycle->nears[i], {
            _cc_event_t *e = _cc_upcast(it, _cc_event_t, lnk);
            /**/
            if ((e->flags & _CC_EVENT_DISCONNECT_) == 0) {
                _cc_event_callback(cycle, e, _CC_EVENT_TIMEOUT_);
            } else {
                _cc_cleanup_event(cycle, e);
            }
        });
        _cc_list_iterator_cleanup(&cycle->nears[i]);
    }
}

_CC_API_PUBLIC(void) _cc_update_event_timeout(_cc_event_cycle_t *cycle, uint32_t timeout) {
    uint32_t tick = _cc_get_ticks();
    if (_cc_unlikely(tick < cycle->tick)) {
        //_cc_logger_error(_T("time diff error: change from %ld to %ld"), tick, cycle->tick);
        cycle->tick = tick;
        return;
    }

    cycle->diff = (tick - cycle->tick);

    if (cycle->diff >= timeout) {
        uint32_t i;

        cycle->tick = tick;
        for (i = 0; i < cycle->diff; i++) {
            /*try to dispatch timeout 0*/
            _timeout_execute(cycle);
            /*shift time first, and then callback timeout*/
            _timeout_shift(cycle);
            _timeout_execute(cycle);
        }
        cycle->diff = 0;
    }

    return;
}

_CC_API_PUBLIC(void) _cc_reset_event_timeout(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, e->flags)) {
        e->timers = cycle->timers;
        _timeout_add(cycle, e);
    } else {
        _cc_list_iterator_swap(&cycle->notimer, &e->lnk);
    }
}

/**/
_CC_API_PRIVATE(void) _reset_event(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    if (_CC_ISSET_BIT(_CC_EVENT_DISCONNECT_, e->flags)) {
        /*delete*/
        _cc_cleanup_event(cycle, e);
    } else {
        _cc_reset_event_timeout(cycle, e);
    }
}

/**/
_CC_API_PRIVATE(bool_t) _driver_timeout_wait(_cc_event_cycle_t *cycle, uint32_t timeout) {
    /**/
    _cc_reset_pending_event(cycle, _reset_event);

    _cc_sleep(timeout);
    _cc_update_event_timeout(cycle, timeout);

    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _driver_timeout_attach(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_assert(cycle != NULL);

    if (_cc_event_wait_reset(cycle, e)) {
        return true;
    }

    _cc_cleanup_event(cycle, e);
    return false;
}

/**/
_CC_API_PRIVATE(bool_t) _driver_timeout_quit(_cc_event_cycle_t *cycle) {
    _cc_assert(cycle != NULL);
    return _cc_event_cycle_quit(cycle);
}

_CC_API_PUBLIC(bool_t) _cc_init_event_timeout(_cc_event_cycle_t *cycle) {
    _cc_assert(cycle != NULL);
    if (!_cc_event_cycle_init(cycle)) {
        return false;
    }

    cycle->priv = NULL;

    cycle->driver.quit = _driver_timeout_quit;
    cycle->driver.attach = _driver_timeout_attach;
    cycle->driver.wait = _driver_timeout_wait;

    cycle->driver.connect = NULL;
    cycle->driver.disconnect = NULL;
    cycle->driver.accept = NULL;
    return true;
}

_CC_API_PUBLIC(_cc_event_t*) _cc_add_event_timeout(_cc_event_cycle_t *cycle, uint32_t timeout, _cc_event_callback_t callback, pvoid_t args) {
    _cc_event_t *e = _cc_alloc_event(cycle, _CC_EVENT_TIMEOUT_);
    if (e == NULL) {
        return NULL;
    }
    e->timeout = timeout;
    e->callback = callback;
    e->args = args;

    if (cycle->driver.attach(cycle, e)) {
        return e;
    }
    _cc_free_event(cycle, e);
    return NULL;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_kill_event_timeout(_cc_event_cycle_t *cycle, _cc_event_t *timer) {
    _cc_assert(cycle != NULL && timer != NULL);
    timer->flags = _CC_EVENT_DISCONNECT_;

    return _cc_event_wait_reset(cycle, timer);
}
