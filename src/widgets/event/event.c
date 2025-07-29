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
#include <libcc/queue.h>
#include "event.c.h"

#if defined(__CC_LINUX__)
#include <sys/resource.h>
#endif

#define _CC_MAX_CYCLES_         64
#define _CC_MAX_EVENT_          64   //(1 << 16)
#define _CC_EVENT_INVALID_ID_   0xffffffff

static struct {
    int32_t size;
    _cc_atomic32_t round;
    _cc_atomic32_t alloc;
    _cc_atomic32_t ref;
    _cc_atomic32_t slot_alloc;
    _cc_event_t **slots;
    size_t slot_length;

    _cc_queue_iterator_t idles;
    _cc_array_t cycles;
} g = {0};

_CC_API_PRIVATE(size_t) _event_get_max_limit() {
#if defined(__CC_LINUX__) || defined(__CC_APPLE__)
    struct rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) == 0) {
        //printf("rlim_cur =%lld,rlim_max =%lld\n",limit.rlim_cur,limit.rlim_max);
        return limit.rlim_cur > _CC_MAX_EVENT_ ? _CC_MAX_EVENT_ : limit.rlim_cur;
    }
#endif
    return 1<<16;
}

_CC_API_PRIVATE(_cc_event_t*) _cc_reserve_event(byte_t baseid) {
    uint16_t round;
    _cc_queue_iterator_t *lnk;
    _cc_event_t *e;
    size_t max_limit = _event_get_max_limit();

    do {
        lnk = _cc_queue_sync_pop(&g.idles);

        if (lnk != &g.idles && lnk != nullptr) {
            e = _cc_upcast(lnk, _cc_event_t, lnk);
            break;
        }

        if (_cc_atomic32_cas(&g.slot_alloc, 0, 1)) {
            size_t i,j;
            size_t slot_size = g.slot_length + _CC_MAX_EVENT_;
            _cc_event_t **slots;
            _cc_event_t *data;

            if (max_limit <= g.slot_length) {
                return nullptr;
            }

            slots = (_cc_event_t **)_cc_realloc(g.slots, sizeof(_cc_event_t*) * slot_size);
            bzero(&slots[g.slot_length], _CC_MAX_EVENT_ * sizeof(_cc_event_t*));
            g.slots = slots;

            data = (_cc_event_t *)_cc_calloc(sizeof(_cc_event_t), _CC_MAX_EVENT_);
            i = g.slot_length;
            j = 0;
            do {
                _cc_event_t *e = data + j++;
                g.slots[i] = e;
                e->ident = (int32_t)i++;
                _cc_queue_sync_push(&g.idles, (_cc_queue_iterator_t*)(&e->lnk));
            } while(j < _CC_MAX_EVENT_);
            g.slot_length = slot_size;
            g.slot_alloc = 0;
        } else {
            _cc_sleep(0);
        }
    } while(1);

    round = (uint16_t)(_cc_atomic32_inc(&(g.round)) & 0xff);
    e->round = _CC_BUILD_INT16(round, baseid);

    return e;

}

/**/
_CC_API_PUBLIC(_cc_event_cycle_t*) _cc_get_event_cycle(void) {
    _cc_event_cycle_t *cycle;
    _cc_event_cycle_t *n;
    int32_t i, count;

    cycle = nullptr;
    if (g.alloc > 1) {
        i = rand() % g.alloc;
        count = g.alloc + i;
    } else {
        i = 0;
        count = g.alloc;
    }

    for (; i < count; i++) {
        n = (_cc_event_cycle_t *)g.cycles.data[i % g.alloc];
        if (n == nullptr || n->running == 0) {
            continue;
        }

        if (cycle == nullptr || n->processed < cycle->processed) {
            cycle = n;
        }
    }

    _cc_assert(cycle != nullptr);
    return cycle;
}

/**/
_CC_API_PUBLIC(_cc_event_t*) _cc_get_event_by_id(uint32_t ident) {
    if (g.slot_length <= ident) {
        return nullptr;
    }
    //_cc_logger_error(_T("event id:%d is deleted"), ident);
    return g.slots[ident];
}

/**/
_CC_API_PUBLIC(_cc_event_cycle_t*) _cc_get_event_cycle_by_id(uint32_t round) {
    return (_cc_event_cycle_t *)g.cycles.data[_CC_HI_UINT8(round)];
}

/**/
_CC_API_PUBLIC(_cc_event_t*) _cc_event_alloc(_cc_event_cycle_t *cycle, const uint32_t flags) {
    _cc_event_t *e;

    e = _cc_reserve_event(cycle->ident);
    if (_cc_unlikely(e == nullptr)) {
        return nullptr;
    }

    e->marks = _CC_EVENT_UNKNOWN_;
    e->flags = (flags & 0xffff);
    e->fd = _CC_INVALID_SOCKET_;
    e->args = nullptr;
    e->callback = nullptr;
    e->buffer = nullptr;
    e->descriptor = (flags >> 16);
    e->timer = cycle->timer;

    if (_CC_EVENT_IS_SOCKET(flags)) {
        e->descriptor |= _CC_EVENT_DESC_SOCKET_;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_BUFFER_, flags)) {
        e->buffer = _cc_alloc_event_buffer();
    }

#ifdef _CC_EVENT_USE_IOCP_
    e->accept_fd = _CC_INVALID_SOCKET_;
#endif

    _cc_list_iterator_cleanup(&e->lnk);
    return e;
}

/**/
_CC_API_PUBLIC(void) _cc_free_event(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_socket_t fd;

    if (e->buffer) {
        _cc_free_event_buffer(e->buffer);
        e->buffer = nullptr;
    }

    if (!_cc_list_iterator_empty(&e->lnk)) {
        _cc_list_iterator_remove(&e->lnk);
    }

    fd = e->fd;

    e->fd = _CC_INVALID_SOCKET_;
    e->round = 0;
    e->flags = _CC_EVENT_UNKNOWN_;
    e->marks = _CC_EVENT_UNKNOWN_;

    if (fd != _CC_INVALID_SOCKET_ && fd != 0) {
        _cc_close_socket(fd);
    }

    _cc_queue_sync_push(&g.idles, (_cc_queue_iterator_t*)(&e->lnk));
}

/*
_CC_API_PUBLIC(void) _cc_print_cycle_processed(void) {
    uint32_t i;
    for (i = 0; i < g.cycles.length; i++) {
        _cc_event_cycle_t *cycle = (_cc_event_cycle_t*)g.cycles.data[i];
        if (cycle) {
            printf("%d: %d, ", i, cycle->processed);
        }
    }
    putchar('\n');
}
*/

/**/
_CC_API_PUBLIC(bool_t) _event_cycle_init(_cc_event_cycle_t *cycle) {
    int i, j;
    _cc_assert(cycle != nullptr);

    if (_cc_atomic32_inc_ref(&g.ref)) {
        _cc_event_t *data;
        _cc_queue_iterator_cleanup(&g.idles);
        g.round = 1;
        g.slots = (_cc_event_t **)_cc_calloc(sizeof(_cc_event_t*), _CC_MAX_EVENT_);
        data = (_cc_event_t *)_cc_calloc(sizeof(_cc_event_t), _CC_MAX_EVENT_);
        i = 0;
        do {
            _cc_event_t *e = (data + i);
            g.slots[i] = e;
            e->ident = i++;
            _cc_queue_iterator_push(&g.idles, (_cc_queue_iterator_t*)(&e->lnk));
        } while(i < _CC_MAX_EVENT_);
        
        g.slot_length = _CC_MAX_EVENT_;
        g.slot_alloc = 0;
        g.alloc = 0;

        _cc_alloc_array(&g.cycles, _CC_MAX_CYCLES_);
    }

#ifdef _CC_EVENT_USE_MUTEX_
    cycle->lock = _cc_alloc_mutex();
#else
    _cc_lock_init(&cycle->lock);
#endif
    cycle->running = 1;
    cycle->timer = 0;
    cycle->diff = 0;
    cycle->tick = _cc_get_ticks();
    cycle->processed = 0;
    cycle->priv = nullptr;
    cycle->ident = _cc_atomic32_inc(&(g.alloc)) & 0xff;
    cycle->attach = nullptr;
    cycle->connect = nullptr;
    cycle->disconnect = nullptr;
    cycle->accept = nullptr;
    cycle->wait = nullptr;
    cycle->quit = nullptr;
    cycle->reset = nullptr;

    if (!_cc_alloc_array(&cycle->changes, _CC_MAX_CHANGE_EVENTS_)) {
        _cc_assert(false);
        return false;
    }

    for (i = 0; i < _CC_TIMEOUT_NEAR_; i++) {
        _cc_list_iterator_cleanup(&cycle->nears[i]);
    }

    for (i = 0; i < _CC_TIMEOUT_MAX_LEVEL_; i++) {
        for (j = 0; j < _CC_TIMEOUT_LEVEL_; j++) {
            _cc_list_iterator_cleanup(&cycle->level[i][j]);
        }
    }

    _cc_list_iterator_cleanup(&cycle->pending);
    _cc_list_iterator_cleanup(&cycle->no_timer);

    _cc_array_insert(&g.cycles, cycle->ident, cycle);
    return true;
}

_CC_API_PRIVATE(void) _event_link_free(_cc_event_cycle_t *cycle, _cc_list_iterator_t *head) {
    _cc_list_iterator_t *next;
    _cc_list_iterator_t *curr;
    _cc_event_t *e;

    next = head->next;
    _cc_list_iterator_cleanup(head);

    while (_cc_likely(next != head)) {
        curr = next;
        next = next->next;

        e = _cc_upcast(curr, _cc_event_t, lnk);
        if (e->callback) {
            e->callback(cycle, e, _CC_EVENT_DISCONNECT_);
        }
        _cc_free_event(cycle, e);
    }
}

/**/
_CC_API_PUBLIC(bool_t) _event_cycle_quit(_cc_event_cycle_t *cycle) {
    size_t i, j;
    _cc_assert(cycle != nullptr);

    _event_lock(cycle);
    cycle->running = 0;
    _cc_array_for_each(_cc_event_t, e, i, &cycle->changes, {
        _cc_list_iterator_swap(&cycle->pending, &e->lnk);
    });
    _cc_free_array(&cycle->changes);
    _event_unlock(cycle);
    
    for (i = 0; i < _CC_TIMEOUT_NEAR_; i++) {
        _event_link_free(cycle, &cycle->nears[i]);
    }

    for (i = 0; i < _CC_TIMEOUT_MAX_LEVEL_; i++) {
        for (j = 0; j < _CC_TIMEOUT_LEVEL_; j++) {
            _event_link_free(cycle, &cycle->level[i][j]);
        }
    }
    _event_link_free(cycle, &cycle->no_timer);
    _event_link_free(cycle, &cycle->pending);

    if (_cc_atomic32_dec_ref(&g.ref)) {;
        //
        for (i = 0; i < g.slot_length; i += _CC_MAX_EVENT_) {
            _cc_free(&g.slots[i]);
        }

        g.slots = nullptr;
        g.slot_length = 0;

        _cc_queue_iterator_cleanup(&g.idles);
        _cc_free_array(&g.cycles);
    }
    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _valid_event(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    return (_CC_HI_UINT8(e->round) == cycle->ident);
}

/**/
_CC_API_PUBLIC(uint16_t) _valid_connected(_cc_event_t *e, uint16_t which) {
    if (_valid_event_fd(e)) {
        _CC_MODIFY_BIT(_CC_EVENT_READABLE_, _CC_EVENT_CONNECT_, e->flags);
        return which;
    }
    
    return _CC_EVENT_DISCONNECT_;
}

/**/
_CC_API_PUBLIC(bool_t) _event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, uint16_t which) {
    /**/
    cycle->processed++;
    _cc_list_iterator_swap(&cycle->pending, &e->lnk);
    
    /**/
    if (e->callback) {
        if (e->callback(cycle, e, which)) {
            return true;
        }

        e->callback(cycle, e, _CC_EVENT_DISCONNECT_);
    }


    /*force disconnect socket*/
    _CC_MODIFY_BIT(_CC_EVENT_DISCONNECT_, _CC_EVENT_READABLE_|_CC_EVENT_ACCEPT_, e->flags);
    return false;
}

/**/
_CC_API_PUBLIC(bool_t) _reset_event(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    bool_t results = true;
    if (cycle->running == 0) {
        return false;
    }

    _event_lock(cycle);
    if (_CC_ISSET_BIT(_CC_EVENT_CHANGING_, e->flags) == 0) {
        if (_cc_array_push(&cycle->changes, e) != -1) {
            _CC_SET_BIT(_CC_EVENT_CHANGING_, e->flags);
        } else {
            _cc_logger_debug(_T("_cc_event_reset fail"));
            results = false;
        }
    }
    _event_unlock(cycle);

    return results;
}

/**/
_CC_API_PUBLIC(void) _reset_event_timeout(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, e->flags)) {
        e->timer = cycle->timer;
        _add_event_timeout(cycle, e);
    } else {
        _cc_list_iterator_swap(&cycle->no_timer, &e->lnk);
    }
}

/**/
_CC_API_PUBLIC(void) _reset_event_pending(_cc_event_cycle_t *cycle, void (*_reset)(_cc_event_cycle_t *, _cc_event_t *)) {
    _cc_list_iterator_t *head;
    _cc_list_iterator_t *next;
    _cc_list_iterator_t *curr;

    if (cycle->changes.length > 0) {
        _event_lock(cycle);
        _cc_array_for_each(_cc_event_t, e, i, &cycle->changes, {
            _CC_UNSET_BIT(_CC_EVENT_CHANGING_, e->flags);
            _cc_list_iterator_swap(&cycle->pending, &e->lnk);
        });
        cycle->changes.length = 0;
        _event_unlock(cycle);
    }

    head = &cycle->pending;
    next = head->next;
    _cc_list_iterator_cleanup(&cycle->pending);

    while (_cc_likely(next != head)) {
        curr = next;
        next = next->next;
        _reset(cycle, _cc_upcast(curr, _cc_event_t, lnk));
    }
}

/**/
_CC_API_PUBLIC(bool_t) _disconnect_event(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    /**/
    if (e->descriptor & (_CC_EVENT_DESC_SOCKET_ | _CC_EVENT_DESC_FILE_)) {
        _cc_shutdown_socket(e->fd, _CC_SHUT_RD_);
    }
    _CC_MODIFY_BIT(_CC_EVENT_DISCONNECT_, _CC_EVENT_READABLE_, e->flags);

    return cycle->reset(cycle, e);
}

/**/
_CC_API_PUBLIC(bool_t) _valid_event_fd(_cc_event_t *e) {
    int results = 0;
    socklen_t len = sizeof(results);

    if (_cc_unlikely(e->fd == _CC_INVALID_SOCKET_)) {
        return false;
    }

    if (getsockopt(e->fd, SOL_SOCKET, SO_ERROR, (char *)&results, &len) != 0) {
        results = _cc_last_errno();
        _cc_logger_error(_T("Socket Error:%d, %s"), results, _cc_last_error(results));
        return false;
    }

    if (results != 0) {
        _cc_logger_error(_T("Socket Error:%d, %s"), results, _cc_last_error(results));
        return false;
    }

    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_event_attach(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_assert(cycle != nullptr && cycle->attach != nullptr);
    return cycle->attach(cycle, e);
}

/**/
_CC_API_PUBLIC(bool_t) _cc_event_connect(_cc_event_cycle_t *cycle, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len) {
    _cc_assert(cycle != nullptr && cycle->connect != nullptr && sa != nullptr);
    return cycle->connect(cycle, e, sa, sa_len);
}

/**/
_CC_API_PUBLIC(_cc_socket_t) _cc_event_accept(_cc_event_cycle_t *cycle, _cc_event_t *e, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len) {
    _cc_assert(cycle != nullptr && cycle->accept != nullptr && e != nullptr);
    return cycle->accept(cycle, e, sa, sa_len);
}

/**/
_CC_API_PUBLIC(bool_t) _cc_event_wait(_cc_event_cycle_t *cycle, uint32_t timeout) {
    _cc_assert(cycle != nullptr && cycle->wait != nullptr);
    return cycle->wait(cycle, timeout);
}