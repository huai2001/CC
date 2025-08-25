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
#include <libcc/alloc.h>
#include <libcc/thread.h>
#include "event.c.h"

static struct {
    bool_t keep_active;
    int32_t count;
    _cc_async_event_t *async_events;
    _cc_thread_t **threads;
    void (*callback)(_cc_async_event_t*, bool_t);
} g = {false, 0, nullptr, nullptr, nullptr};

/**/
_CC_API_PRIVATE(int32_t) _running(_cc_thread_t *thread, void *args) {
    _cc_async_event_t *async = (_cc_async_event_t *)args;
    
    if (g.callback) {
        g.callback(async, true);
    }

    while (g.keep_active) {
        async->wait(async, 10);
    }

    if (g.callback) {
        g.callback(async, false);
    }

    async->quit(async);
    return 0;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_install_async_event(int32_t count, void (*func)(_cc_async_event_t*,bool_t)) {
    int32_t i;
    _cc_thread_t** threads;
    _cc_async_event_t *async_events;

    if (g.keep_active) {
        return false;
    }

    srand((uint32_t)time(nullptr));
    
    _cc_install_socket();
    
    if (count <= 0) {
        count = _cc_cpu_count();
    }

    async_events = (_cc_async_event_t *)_cc_calloc(count, sizeof(_cc_async_event_t));
    if (async_events == nullptr) {
        return false;
    }

    threads = (_cc_thread_t **)_cc_calloc(count, sizeof(_cc_thread_t *));
    if (threads == nullptr) {
        _cc_free(async_events);
        return false;
    }

    g.keep_active = true;
    g.async_events = async_events;
    g.threads = threads;
    g.callback = func;

    for (i = 0; i < count; ++i) {
        _cc_async_event_t *n = (_cc_async_event_t *)(async_events + i);
        if (_cc_init_event_poller(n) == false) {
            continue;
        }
        n->args = nullptr;
        *(threads + i) = _cc_thread(_running, _T("event lopp"), n);
        g.count++;
    }
    
    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_uninstall_async_event(void) {
    int32_t i;

    if (!g.keep_active) {
        return true;
    }
    
    g.keep_active = false;

    if (g.threads) {
        for (i = 0; i < g.count; ++i) {
            _cc_wait_thread(*(g.threads + i), nullptr);
        }
        _cc_free(g.threads);
        g.threads = nullptr;
    }

    if (g.async_events) {
        _cc_free(g.async_events);
        g.async_events = nullptr;
    }

    _cc_uninstall_socket();
    
    return true;
}

/**/
_CC_API_PUBLIC(void) _cc_async_event_abort(void) {
    g.keep_active = false;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_async_event_is_running(void) {
    return g.keep_active;
}
