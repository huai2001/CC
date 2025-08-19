/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
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
#include <libcc/alloc.h>
#include <libcc/widgets/timeout.h>

_CC_API_PRIVATE(void) _timeout_move_list(_cc_async_event_t *async, int level, int n) {
    _cc_list_iterator_t *head = &async->level[level][n];
    _cc_list_iterator_for_each(it, head, { 
        _add_event_timeout(async, _cc_upcast(it, _cc_event_t, lnk)); 
    });
    _cc_list_iterator_cleanup(head);
    //_cc_list_iterator_cleanup(&async->level[level][n]);
}

_CC_API_PRIVATE(void) _timeout_shift(_cc_async_event_t *async) {
    uint32_t t = ++async->timer;
    if (_cc_likely(t == 0)) {
        _timeout_move_list(async, 3, 0);
    } else {
        int i;
        int m = _CC_TIMEOUT_NEAR_;
        uint32_t v = t >> _CC_TIMEOUT_NEAR_SHIFT_;
        for (i = 0; (t & (m - 1)) == 0; i++) {
            int n = v & _CC_TIMEOUT_LEVEL_MASK_;
            if (n != 0) {
                _timeout_move_list(async, i, n);
                break;
            }
            m <<= _CC_TIMEOUT_LEVEL_SHIFT_;
            v >>= _CC_TIMEOUT_LEVEL_SHIFT_;
        }
    }
}

_CC_API_PRIVATE(void) _timeout_execute(_cc_async_event_t *async) {
    int i = async->timer & _CC_TIMEOUT_NEAR_MASK_;

    if (!_cc_list_iterator_empty(&async->nears[i])) {
        _cc_list_iterator_for_each(it, &async->nears[i], {
            _cc_event_t *e = _cc_upcast(it, _cc_event_t, lnk);
            /**/
            if ((e->flags & _CC_EVENT_DISCONNECT_) == 0) {
                _event_callback(async, e, _CC_EVENT_TIMEOUT_);
            } else {
                _cc_free_event(async, e);
            }
        });
        _cc_list_iterator_cleanup(&async->nears[i]);
    }
}

_CC_API_PUBLIC(void) _add_event_timeout(_cc_async_event_t *async, _cc_event_t *e) {
    uint32_t elapsed;
    uint32_t expire;
    uint32_t mask;
    int i;

    expire = e->timer + e->timeout;
    elapsed = async->timer;

    if ((expire | _CC_TIMEOUT_NEAR_MASK_) == (elapsed | _CC_TIMEOUT_NEAR_MASK_)) {
        _cc_list_iterator_swap(&async->nears[expire & _CC_TIMEOUT_NEAR_MASK_], &e->lnk);
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
    _cc_list_iterator_swap(&async->level[i][mask], &e->lnk);
}

_CC_API_PUBLIC(void) _update_event_timeout(_cc_async_event_t *async, uint32_t timeout) {
    uint64_t tick = _cc_get_ticks();

    if (_cc_unlikely(tick < async->tick)) {
        _cc_logger_warin(_T("time diff error: change from %ld to %ld"), tick, async->tick);
        async->tick = tick;
    } else {
        async->diff = (int32_t)(tick - async->tick);

        if (async->diff >= timeout) {
            uint32_t i;

            async->tick = tick;
            for (i = 0; i < async->diff; i++) {
                /*try to dispatch timeout 0*/
                _timeout_execute(async);
                /*shift time first, and then callback timeout*/
                _timeout_shift(async);
                _timeout_execute(async);
            }
            async->diff = 0;
        }
    }
    return;
}

/**/
_CC_API_PRIVATE(void) _reset(_cc_async_event_t *async, _cc_event_t *e) {
    if (_CC_ISSET_BIT(_CC_EVENT_DISCONNECT_, e->flags) == 0 && 
        _CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, e->flags)) {
        e->timer = async->timer;
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
    _cc_assert(async != nullptr);
    return _reset_event(async, e);
}

/**/
_CC_API_PRIVATE(bool_t) _delegate_timeout_quit(_cc_async_event_t *async) {
    _cc_assert(async != nullptr);
    return _async_event_quit(async);
}

_CC_API_PUBLIC(bool_t) _cc_init_event_timeout(_cc_async_event_t *async) {
    _cc_assert(async != nullptr);
    if (!_async_event_init(async)) {
        return false;
    }

    async->priv = nullptr;

    async->reset = _delegate_timeout_reset;
    async->quit = _delegate_timeout_quit;
    async->attach = _delegate_timeout_attach;
    async->wait = _delegate_timeout_wait;

    async->connect = nullptr;
    async->disconnect = nullptr;
    async->accept = nullptr;

    return true;
}

_CC_API_PUBLIC(_cc_event_t*) _cc_add_event_timeout(_cc_async_event_t *async, uint32_t timeout, _cc_event_callback_t callback, pvoid_t args) {
    _cc_event_t *e = _cc_event_alloc(async, _CC_EVENT_TIMEOUT_);
    e->timeout = timeout;
    e->callback = callback;
    e->args = args;

    if (async->attach(async, e)) {
        return e;
    }
    _cc_free_event(async, e);
    return nullptr;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_kill_event_timeout(_cc_async_event_t *async, _cc_event_t *timer) {
    _cc_assert(async != nullptr && timer != nullptr);
    _CC_SET_BIT(_CC_EVENT_DISCONNECT_,timer->flags);
    return async->reset(async, timer);
}
