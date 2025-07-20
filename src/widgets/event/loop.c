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
    _cc_event_cycle_t *cycles;
    _cc_thread_t **threads;
    void (*callback)(_cc_event_cycle_t*, bool_t);
} g = {false, 0, nullptr, nullptr, nullptr};

/**/
_CC_API_PRIVATE(int32_t) _running(_cc_thread_t *thread, void *args) {
    _cc_event_cycle_t *cycle = (_cc_event_cycle_t *)args;
    if (g.callback) {
        g.callback(cycle, true);
    }

    while (g.keep_active) {
        cycle->wait(cycle, 10);
    }

    if (g.callback) {
        g.callback(cycle, false);
    }

    cycle->quit(cycle);
    return 0;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_event_loop(int32_t count, void (*func)(_cc_event_cycle_t*,bool_t)) {
    int32_t i;
    _cc_thread_t** threads;
    _cc_event_cycle_t *cycles;

    if (g.keep_active) {
        return false;
    }

    srand((uint32_t)time(nullptr));
    
    _cc_install_socket();
    
    if (count <= 0) {
        count = _cc_cpu_count();
    }

    cycles = (_cc_event_cycle_t *)_cc_calloc(count, sizeof(_cc_event_cycle_t));
    if (cycles == nullptr) {
        return false;
    }

    threads = (_cc_thread_t **)_cc_calloc(count, sizeof(_cc_thread_t *));
    if (threads == nullptr) {
        _cc_free(cycles);
        return false;
    }

    g.keep_active = true;
    g.cycles = cycles;
    g.threads = threads;
    g.callback = func;

    for (i = 0; i < count; ++i) {
        _cc_event_cycle_t *n = (_cc_event_cycle_t *)(cycles + i);
        if (_cc_init_event_poller(n) == false) {
            continue;
        }
        n->args = nullptr;
        *(threads + i) = _cc_create_thread(_running, nullptr, n);
        g.count++;
    }
    
    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_quit_event_loop(void) {
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

    if (g.cycles) {
        _cc_free(g.cycles);
        g.cycles = nullptr;
    }

    _cc_uninstall_socket();
    
    return true;
}

/**/
_CC_API_PUBLIC(void) _cc_event_loop_abort() {
    g.keep_active = false;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_event_loop_is_running(void) {
    return g.keep_active;
}
