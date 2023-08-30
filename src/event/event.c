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
#include <cc/alloc.h>
#include <cc/thread.h>
#include "event.c.h"

#if defined(__CC_LINUX__)
#include <sys/resource.h>
#endif

#define _CC_MAX_CYCLES_ 64
#define _CC_MAX_EVENT_ (1 << 16)
#define _CC_EVENT_INVALID_ID_ 0xffffffff

static struct {
    int32_t r;
    int32_t w;
    int32_t size;
    _cc_atomic32_t round;
    _cc_event_t *storage;
    uint32_t *unused;
    _cc_atomic32_t alloc;
    _cc_atomic32_t ref;
    _cc_spinlock_t elock;
    _cc_array_t cycles;
} g = {0};

static _cc_event_t *_cc_reserve_event(byte_t baseid) {
    uint32_t round;
    uint32_t index;
    _cc_event_t *e;
    do {
        _cc_spin_lock(&g.elock);
        if (g.r == g.w) {
            _cc_spin_unlock(&g.elock);
            return NULL;
        }
        index = g.unused[g.r];
        g.r = (g.r + 1) % g.size;
        _cc_spin_unlock(&g.elock);

        e = &g.storage[index];

        if (_cc_atomic32_cas((_cc_atomic32_t *)&e->ident, _CC_EVENT_INVALID_ID_, 0)) {
            round = _cc_atomic32_inc(&(g.round)) & 0xff;
            round = _CC_BUILD_INT16(round, baseid);
            e->ident = _CC_BUILD_INT32(index, round);

            return e;
        }
        //_cc_logger_warin(_T("The event has been used"));
    } while (1);

    _cc_logger_error(_T("event buffer is filled up!"));
    return NULL;
}

/**/
_cc_event_cycle_t *_cc_get_event_cycle(void) {
    _cc_event_cycle_t *cycle;
    _cc_event_cycle_t *n;
    int32_t i, count;

    cycle = NULL;
    if (g.alloc > 1) {
        i = rand() % g.alloc;
        count = g.alloc + i;
    } else {
        i = 0;
        count = g.alloc;
    }

    for (; i < count; i++) {
        n = (_cc_event_cycle_t *)g.cycles.data[i % g.alloc];
        if (n == NULL || n->running == 0) {
            continue;
        }

        if (cycle == NULL || n->processed < cycle->processed) {
            cycle = n;
        }
    }

    _cc_assert(cycle != NULL);
    return cycle;
}

/**/
_cc_event_t *_cc_get_event_by_id(uint32_t ident) {
    _cc_event_t *e = &g.storage[_CC_LO_UINT16(ident)];

    if (e->ident == ident) {
        return e;
    }
    //_cc_logger_error(_T("event id:%d is deleted"), ident);
    return NULL;
}

/**/
_cc_event_cycle_t *_cc_get_event_cycle_by_id(uint32_t ident) {
    uint16_t index = ((uint16_t)(ident >> 24));
    if (_cc_unlikely(index >= g.cycles.size)) {
        return NULL;
    }
    return (_cc_event_cycle_t *)g.cycles.data[index];
}

/**/
bool_t _cc_valid_event(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    if (_cc_unlikely(e->ident == _CC_EVENT_INVALID_ID_)) {
        return false;
    }

    if (((uint16_t)(e->ident >> 24)) != cycle->ident) {
        return false;
    }

    return true;
}

/**/
uint32_t _cc_valid_connected(_cc_event_t *e, uint32_t events) {
    if (_CC_ISSET_BIT(_CC_EVENT_CONNECT_, events) != 0) {
        _CC_MODIFY_BIT(_CC_EVENT_READABLE_, _CC_EVENT_CONNECT_, e->flags);
        if (!_cc_event_fd_valid(e)) {
            events = _CC_EVENT_DISCONNECT_;
        }
    }
    return events;
}

/**/
bool_t _cc_event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, uint16_t events) {
    /**/
    cycle->processed++;
    
    /**/
    if (e->callback && e->callback(cycle, e, events)) {
        _cc_list_iterator_swap(&cycle->pending, &e->lnk);
        return true;
    } else if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags)) {
        /**/
        if (e->descriptor & (_CC_EVENT_DESC_SOCKET_ | _CC_EVENT_DESC_FILE_)) {
            //_cc_shutdown_socket(e->fd, _CC_SHUT_RD_);
            _CC_MODIFY_BIT(_CC_EVENT_DISCONNECT_, _CC_EVENT_READABLE_, e->flags);
            _cc_list_iterator_swap(&cycle->pending, &e->lnk);
            return true;
        }
    }

    /*force disconnect socket*/
    _cc_cleanup_event(cycle, e);
    return false;
}

/**/
_cc_event_t *_cc_alloc_event(_cc_event_cycle_t *cycle, const uint32_t flags) {
    _cc_event_t *e;

    e = _cc_reserve_event(cycle->ident);
    if (_cc_unlikely(e == NULL)) {
        return NULL;
    }

    e->marks = _CC_EVENT_UNKNOWN_;
    e->flags = (flags & 0xffff);
    e->fd = _CC_INVALID_SOCKET_;
    e->args = NULL;
    e->callback = NULL;
    e->descriptor = (flags >> 16);
    e->timers = cycle->timers;

    if (_CC_EVENT_IS_SOCKET(flags)) {
        e->descriptor |= _CC_EVENT_DESC_SOCKET_;
    } else if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, flags)) {
        e->descriptor |= _CC_EVENT_DESC_TIMER_;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_BUFFER_, flags)) {
        _cc_bind_event_buffer(cycle, &e->buffer);
    } else {
        e->buffer = NULL;
    }

#ifdef _CC_EVENT_USE_IOCP_
    e->accept_fd = _CC_INVALID_SOCKET_;
#endif

    e->lnk.next = NULL;
    e->lnk.prev = NULL;
    return e;
}

/**/
bool_t _cc_event_wait_reset(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    bool_t results = true;
    if (cycle->running == 0) {
        return false;
    }

    _cc_event_lock(cycle);
    if (_CC_ISSET_BIT(_CC_EVENT_CHANGING_, e->flags) == 0) {
        if (_cc_array_push(&cycle->changes, e) != -1) {
            _CC_SET_BIT(_CC_EVENT_CHANGING_, e->flags);
        } else {
            _cc_logger_error(_T("_cc_event_wait_reset fail"));
            results = false;
        }
    }

    _cc_event_unlock(cycle);

    return results;
}

/**/
void _cc_free_event(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_socket_t fd;
    int32_t w;
    int32_t index = _CC_LO_UINT16(e->ident);

    if (e->buffer) {
        _cc_unbind_event_buffer(cycle, &e->buffer);
    }

    if (!_cc_list_iterator_empty(&e->lnk)) {
        _cc_list_iterator_remove(&e->lnk);
    }

    fd = e->fd;

    e->fd = _CC_INVALID_SOCKET_;
    e->ident = _CC_EVENT_INVALID_ID_;
    e->flags = _CC_EVENT_UNKNOWN_;
    e->marks = _CC_EVENT_UNKNOWN_;

    if (fd != _CC_INVALID_SOCKET_ && fd != 0) {
        _cc_close_socket(fd);
    }

    _cc_spin_lock(&g.elock);
    w = (g.w + 1) % g.size;
    if (g.r != w) {
        g.unused[g.w] = (index % g.size);
        g.w = w;
    }
    _cc_spin_unlock(&g.elock);
}

/**/
void _cc_cleanup_event(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    e->callback(cycle, e, _CC_EVENT_DELETED_);

    if (cycle->cleanup) {
        cycle->cleanup(cycle, e);
    }
    
    _cc_free_event(cycle, e);
}

/**/
void _cc_reset_pending_event(_cc_event_cycle_t *cycle, void (*_reset_event)(_cc_event_cycle_t *, _cc_event_t *)) {
    _cc_list_iterator_t *head;
    _cc_list_iterator_t *next;
    _cc_list_iterator_t *curr;

    head = &cycle->pending;
    next = head->next;
    _cc_list_iterator_cleanup(&cycle->pending);

    while (_cc_likely(next != head)) {
        curr = next;
        next = next->next;
        _reset_event(cycle, _cc_upcast(curr, _cc_event_t, lnk));
    }

    if (cycle->changes.length > 0) {
        _cc_event_lock(cycle);
        _cc_array_for_each(_cc_event_t, e, key, &cycle->changes, {
            if (_cc_valid_event(cycle, e)) {
                _reset_event(cycle, e);
            }
            _CC_UNSET_BIT(_CC_EVENT_CHANGING_, e->flags);
        });
        
        cycle->changes.length = 0;
        _cc_event_unlock(cycle);
    }
}

/**/
bool_t _cc_event_change_flag(_cc_event_cycle_t *cycle, _cc_event_t *e, uint16_t flags) {
    if (cycle == NULL) {
        cycle = _cc_get_event_cycle_by_id(e->ident);
        if (cycle == NULL) {
            return false;
        }
    }

    if (_CC_ISSET_BIT(e->flags, flags) == flags) {
        return false;
    }

    _CC_SET_BIT(flags, e->flags);

    return _cc_event_wait_reset(cycle, e);
}
/*
void _cc_print_cycle_processed(void) {
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

static int32_t _event_get_max_limit() {
#if defined(__CC_LINUX__) || defined(__CC_APPLE__)
    struct rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) == 0) {
        //printf("rlim_cur =%lld,rlim_max =%lld\n",limit.rlim_cur,limit.rlim_max);
        return limit.rlim_cur > _CC_MAX_EVENT_ ? _CC_MAX_EVENT_ : (int32_t)limit.rlim_cur;
    }
#endif
    return _CC_MAX_EVENT_;
}

/**/
bool_t _cc_event_cycle_init(_cc_event_cycle_t *cycle) {
    int i, j;
    _cc_assert(cycle != NULL);

    if (_cc_atomic32_inc_ref(&g.ref)) {
        g.round = 1;
        g.w = 0;
        g.r = 0;
        g.size = _event_get_max_limit();
        g.storage = _cc_calloc(g.size, sizeof(_cc_event_t));
        g.unused = _cc_calloc(g.size, sizeof(uint32_t));
        _cc_assert(g.storage != NULL);
        _cc_assert(g.unused != NULL);
        _cc_spin_lock_init(&g.elock);

        for (i = 0; i < g.size; i++) {
            g.storage[i].ident = _CC_EVENT_INVALID_ID_;
            g.unused[i] = i;
        }
        g.w = i;

        g.alloc = 0;
        _cc_array_alloc(&g.cycles, _CC_MAX_CYCLES_);
    }

#ifdef _CC_EVENT_USE_MUTEX_
    cycle->elock = _cc_create_mutex();
#else
    _cc_spin_lock_init(&cycle->elock);
#endif
    cycle->timers = 0;
    cycle->diff = 0;
    cycle->tick = _cc_get_ticks();
    cycle->processed = 0;
    cycle->priv = NULL;
    cycle->ident = _cc_atomic32_inc(&(g.alloc)) & 0xff;
    cycle->cleanup = NULL;
    cycle->running = 1;

    if (!_cc_array_alloc(&cycle->changes, _CC_MAX_CHANGE_EVENTS_)) {
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
    _cc_list_iterator_cleanup(&cycle->notimer);

    _cc_array_insert(&g.cycles, cycle->ident, cycle);
    return true;
}

static void _event_link_free(_cc_event_cycle_t *cycle, _cc_list_iterator_t *head) {
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
            e->callback(cycle, e, _CC_EVENT_DELETED_);
        }

        _cc_free_event(cycle, e);
    }
}

/**/
bool_t _cc_event_cycle_quit(_cc_event_cycle_t *cycle) {
    int i, j, c;
    _cc_event_t *e;
    _cc_assert(cycle != NULL);

    _cc_event_lock(cycle);
    cycle->running = 0;
    c = _cc_array_length(&cycle->changes);
    for (i = 0; i < c; i++) {
        e = (_cc_event_t *)cycle->changes.data[i];

        if (!_cc_list_iterator_empty(&e->lnk)) {
            continue;
        }
        if (e->callback) {
            e->callback(cycle, e, _CC_EVENT_DELETED_);
        }
        _cc_free_event(cycle, e);
    }
    _cc_array_free(&cycle->changes);
    _cc_event_unlock(cycle);
    
    for (i = 0; i < _CC_TIMEOUT_NEAR_; i++) {
        _event_link_free(cycle, &cycle->nears[i]);
    }

    for (i = 0; i < _CC_TIMEOUT_MAX_LEVEL_; i++) {
        for (j = 0; j < _CC_TIMEOUT_LEVEL_; j++) {
            _event_link_free(cycle, &cycle->level[i][j]);
        }
    }
    _event_link_free(cycle, &cycle->notimer);
    _event_link_free(cycle, &cycle->pending);

#ifdef _CC_EVENT_USE_MUTEX_
    _cc_destroy_mutex(&cycle->elock);
#endif

    cycle->cleanup = NULL;
    cycle->driver.attach = NULL;
    cycle->driver.connect = NULL;
    cycle->driver.disconnect = NULL;
    cycle->driver.accept = NULL;
    cycle->driver.wait = NULL;
    cycle->driver.quit = NULL;

    if (_cc_atomic32_dec_ref(&g.ref)) {
        _cc_safe_free(g.storage);
        _cc_safe_free(g.unused);
        _cc_array_free(&g.cycles);
    }
    return true;
}

/**/
bool_t _cc_event_fd_valid(_cc_event_t *e) {
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
bool_t _cc_event_attach(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_assert(cycle != NULL && cycle->driver.attach != NULL);
    return cycle->driver.attach(cycle, e);
}

/**/
bool_t _cc_event_connect(_cc_event_cycle_t *cycle, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len) {
    _cc_assert(cycle != NULL && cycle->driver.connect != NULL && sa != NULL);
    return cycle->driver.connect(cycle, e, sa, sa_len);
}

/**/
_cc_socket_t _cc_event_accept(_cc_event_cycle_t *cycle, _cc_event_t *e, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len) {
    _cc_assert(cycle != NULL && cycle->driver.accept != NULL && e != NULL);
    return cycle->driver.accept(cycle, e, sa, sa_len);
}

/**/
bool_t _cc_event_disconnect(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    /**/
    if (e->descriptor & (_CC_EVENT_DESC_SOCKET_ | _CC_EVENT_DESC_FILE_)) {
        _cc_shutdown_socket(e->fd, _CC_SHUT_RD_);
    }
    _CC_MODIFY_BIT(_CC_EVENT_DISCONNECT_, _CC_EVENT_READABLE_, e->flags);

    return _cc_event_wait_reset(cycle, e);
}

/**/
bool_t _cc_event_wait(_cc_event_cycle_t *cycle, uint32_t timeout) {
    _cc_assert(cycle != NULL && cycle->driver.wait != NULL);
    return cycle->driver.wait(cycle, timeout);
}
