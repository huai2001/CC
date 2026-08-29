#include "event.c.h"
#include <libcc/alloc.h>
#include <libcc/timeout.h>

#define _CC_MAX_TIMEOUT_BATCH_ 64

/**/
_CC_API_PRIVATE(void) _timeout_move_slot(_cc_async_event_t *async, int level, int n) {
    _cc_list_t *head = &async->level[level][n];
    _cc_list_for_each(it, head, { 
        _add_event_timeout(async, _cc_upcast(it, _cc_event_t, lnk)); 
    });
    _cc_list_cleanup(head);
}

/**/
_CC_API_PRIVATE(void) _timeout_shift(_cc_async_event_t *async) {
    uint32_t t = ++async->timer;
    if (_cc_likely(t == 0)) {
        _timeout_move_slot(async, 3, 0);
    } else {
        int i;
        int m = _CC_TIMEOUT_NEAR_;
        uint32_t v = t >> _CC_TIMEOUT_NEAR_SHIFT_;
        for (i = 0; (t & (m - 1)) == 0; i++) {
            int n = v & _CC_TIMEOUT_LEVEL_MASK_;
            if (n != 0) {
                _timeout_move_slot(async, i, n);
                break;
            }
            m <<= _CC_TIMEOUT_LEVEL_SHIFT_;
            v >>= _CC_TIMEOUT_LEVEL_SHIFT_;
        }
    }
}

/**/
_CC_API_DYLIB_PRIVATE(void) _timeout_execute(_cc_async_event_t *async) {
    int i = async->timer & _CC_TIMEOUT_NEAR_MASK_;

    if (!_cc_list_empty(&async->nears[i])) {
        _cc_list_for_each(it, &async->nears[i], {
            _cc_event_t *e = _cc_upcast(it, _cc_event_t, lnk);
            /**/
            if ((e->flags & _CC_EVENT_CLOSED_) == 0) {
                _event_callback(async, e, _CC_EVENT_TIMEOUT_);
            } else {
                _cc_free_event(async, e);
            }
        });
        _cc_list_cleanup(&async->nears[i]);
    }
}

/**/
_CC_API_DYLIB_PRIVATE(void) _add_event_timeout(_cc_async_event_t *async, _cc_event_t *e) {
    uint32_t elapsed = async->timer;
    uint32_t expire = e->expire;
    uint32_t mask;
    int i;

    if ((expire | _CC_TIMEOUT_NEAR_MASK_) == (elapsed | _CC_TIMEOUT_NEAR_MASK_)) {
        _cc_list_swap(&async->nears[expire & _CC_TIMEOUT_NEAR_MASK_], &e->lnk);
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
    _cc_list_swap(&async->level[i][mask], &e->lnk);
}

/**/
_CC_API_DYLIB_PRIVATE(void) _update_event_timeout(_cc_async_event_t *async, uint32_t timeout) {
    uint64_t tick = _cc_get_ticks();

    if (_cc_unlikely(tick < async->tick)) {
        _cc_logger_warin("time diff error: change from %ld to %ld", tick, async->tick);
        async->tick = tick;
    } else {
        async->diff = (int32_t)(tick - async->tick);

        if (async->diff >= timeout) {
            uint32_t i;
            //Limit the number of processes processed at a time to prevent CPU speed racing
            uint32_t limit = async->diff > _CC_MAX_TIMEOUT_BATCH_ ? _CC_MAX_TIMEOUT_BATCH_ : async->diff;
            async->tick = tick;
            for (i = 0; i < limit; i++) {
                /*try to dispatch timeout 0*/
                _timeout_execute(async);
                /*shift time first, and then callback timeout*/
                _timeout_shift(async);
                _timeout_execute(async);
            }
            async->diff -= limit;
        }
    }
}

/**/
_CC_API_PRIVATE(void) _reset(_cc_async_event_t *async, _cc_event_t *e) {
    if (_CC_ISSET_BIT(_CC_EVENT_CLOSED_, e->flags) == 0 && 
        _CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, e->flags)) {
        e->expire = async->timer + e->timeout;
        _add_event_timeout(async, e);
    } else {
        _cc_free_event(async, e);
    }
}

/**/
_CC_API_PRIVATE(bool_t) _delegate_timeout_wait(_cc_async_event_t *async, uint32_t timeout) {
    /**/
    _reset_event_pending(async, _reset);
    _cc_sleep(timeout);
    _update_event_timeout(async, timeout);
    return true;
}

/**/
_CC_API_PRIVATE(bool_t) _delegate_timeout_reset(_cc_async_event_t *async, _cc_event_t *e) {
    return _reset_event(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _delegate_timeout_attach(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_assert(async != NULL);
    return _reset_event(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _delegate_timeout_free(_cc_async_event_t *async) {
    _cc_assert(async != NULL);
    return _unregister_async_event(async);
}

_CC_API_PUBLIC(bool_t) _cc_register_timeout(_cc_async_event_t *async) {
    _cc_assert(async != NULL);
    if (!_register_async_event(async)) {
        return false;
    }

    async->priv = NULL;

    async->reset = _delegate_timeout_reset;
    async->free = _delegate_timeout_free;
    async->attach = _delegate_timeout_attach;
    async->wait = _delegate_timeout_wait;

    async->connect = NULL;
    async->disconnect = NULL;
    async->accept = NULL;

    return true;
}

_CC_API_PUBLIC(_cc_event_t*) _cc_add_event_timeout(_cc_async_event_t *async, uint32_t milliseconds, _cc_event_callback_t callback, uintptr_t data) {
    _cc_event_t *e = _cc_alloc_event(async, _CC_EVENT_TIMEOUT_);
    if (e) {
        e->timeout = milliseconds;
        e->callback = callback;
        e->data = data;

        if (async->attach(async, e)) {
            return e;
        }
        _cc_free_event(async, e);
    }
    return NULL;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_kill_event_timeout(_cc_async_event_t *async, _cc_event_t *timer) {
    _cc_assert(async != NULL && timer != NULL);
    _CC_SET_BIT(_CC_EVENT_CLOSED_,timer->flags);
    return async->reset(async, timer);
}
