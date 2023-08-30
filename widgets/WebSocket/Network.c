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
#include <stdio.h>
#include "Acceptor.h"

static int32_t threadCount = 0;
static _cc_event_cycle_t* cycles;
static _cc_thread_t** threads;
static bool_t keepActive = true;

/**/
_CC_FORCE_INLINE_ int32_t _Mir2Thread_cc_thread_t *thread, void* args) {
    _cc_event_cycle_t* cycle = (_cc_event_cycle_t*)args;
    while (keepActive) {
        cycle->driver.wait(cycle, 100);
    }
    
    cycle->driver.quit(cycle);
    return 0;
}

/**/
_CC_FORCE_INLINE_ bool_t _Mir2Starting(int32_t threadCount) {
    int i;

    cycles =
        (_cc_event_cycle_t*)_cc_malloc(sizeof(_cc_event_cycle_t) * threadCount);
    if (cycles == NULL) {
        return false;
    }

    threads = (_cc_thread_t**)_cc_malloc(sizeof(_cc_thread_t*) * threadCount);
    if (threads == NULL) {
        _cc_free(cycles);
        return false;
    }

    /*启动线程*/
    for (i = 0; i < threadCount; ++i) {
        _cc_event_cycle_t* n = (_cc_event_cycle_t*)(cycles + i);
        if (_cc_init_event_poller(n) == false) {
            continue;
        }
        *(threads + i) = _cc_create_thread(_Mir2ThreadRunning, NULL, n);
    }
    return true;
}

/**/
_CC_FORCE_INLINE_ void _Mir2Quit(void) {
    int i = 0;
    keepActive = 0;

    if (threads) {
        for (i = 0; i < threadCount; ++i) {
            _cc_wait_thread(*(threads + i), NULL);
        }
        threads = NULL;
    }

    if (cycles) {
        _cc_free(cycles);
        cycles = NULL;
    }
}

/**/
bool_t _Mir2NetworkStartup(int32_t services) {
    _cc_install_socket();
    _Mir2InitAcceptor();
    if (services == 0) {
        services = _cc_cpu_count() * 2;
    }
    
    if (!_Mir2Starting(services)) {
        _cc_logger_error(_T("init service fail."));
        return false;
    }

    _cc_sleep(1000);
    
    return true;
}

/**/
bool_t _Mir2NetworkStop() {
    _Mir2Quit();
    _Mir2QuitAcceptor();
    _cc_uninstall_socket();
    return true;
}

/**/
_cc_event_cycle_t* _Mir2GetCycle() {
    return _cc_get_event_cycle();
}

/**/
bool_t _Mir2GetKeepActive(void) {
    return keepActive;
}

/**/
void _Mir2NetworkBind(_cc_event_t *e,
                      uint8_t classify,
                      pvoid_t args) {
    _Mir2Network_t *network = (_Mir2Network_t*)e->args;
    if (network) {
        network->socketId = e->ident;
        network->classify = classify;
        network->args = args;
    }
}
