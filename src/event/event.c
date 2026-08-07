#include <libcc/alloc.h>
#include <libcc/thread.h>
#include <libcc/queue.h>
#include "event.c.h"

#if defined(__CC_LINUX__)
#include <sys/resource.h>
#endif

/* Event pool initialization and expansion step. */
#ifndef _CC_EVENT_SLOT_STEP_
#define _CC_EVENT_SLOT_STEP_        64
#endif

static struct {
    _cc_atomic32_t async_limit;
    _cc_atomic32_t refcount;
    _cc_atomic32_t slot_refcount;
    int32_t slot_length;
    int32_t slot_limit;
    _cc_queue_t idles;

    _cc_async_event_t **async;
    _cc_event_t **slots;
    _cc_mutex_t *slot_lock;
    _cc_condition_t *slot_cond;
    _cc_atomic32_t slot_wait_ms;
} g_mgr = {0};

_CC_API_PRIVATE(int32_t) _get_max_limit(void) {
#if defined(__CC_LINUX__) || defined(__CC_APPLE__)
    struct rlimit limit;
    //1048576
    if (getrlimit(RLIMIT_NOFILE, &limit) == 0) {
        //printf("rlim_cur =%lld,rlim_max =%lld\n",limit.rlim_cur,limit.rlim_max);
        return (int32_t)(limit.rlim_cur > 0x0FFFFF ? 0x0FFFFF : limit.rlim_cur);
    }
#endif
    return 0x0FFFFF;
}

_CC_API_PRIVATE(_cc_event_t*) _cc_reserve_event(uint16_t baseid) {
    _cc_queue_t *lnk;
    _cc_event_t *e;
    for (;;) {
        lnk = _cc_queue_sync_pop(&g_mgr.idles);

        if (lnk != &g_mgr.idles && lnk != NULL) {
            e = _cc_upcast((_cc_list_t*)lnk, _cc_event_t, lnk);
            break;
        }

        /* try to become the thread that expands slots */
        if (_cc_atomic32_cas(&g_mgr.slot_refcount, 0, 1)) {
            int32_t i,j;
            int32_t expand_length = g_mgr.slot_length + _CC_EVENT_SLOT_STEP_;
            _cc_event_t *data;

            if (g_mgr.slot_limit <= g_mgr.slot_length) {
                _cc_logger_error("The maximum number of event supported by the RLIMIT_NOFILE is %d", g_mgr.slot_limit);
                /* reset flag so others won't deadlock */
                _cc_atomic32_set(&g_mgr.slot_refcount, 0);
                /* wake any waiters */
                if (g_mgr.slot_cond && g_mgr.slot_lock) {
                    _cc_mutex_lock(g_mgr.slot_lock);
                    _cc_condition_broadcast(g_mgr.slot_cond);
                    _cc_mutex_unlock(g_mgr.slot_lock);
                }
                return NULL;
            }

            /* allocate/expand slots */
            g_mgr.slots = (_cc_event_t **)_cc_realloc(g_mgr.slots, sizeof(_cc_event_t*) * expand_length);
            data = (_cc_event_t *)_cc_calloc(sizeof(_cc_event_t), _CC_EVENT_SLOT_STEP_);

            for (i = g_mgr.slot_length, j = 0; j < _CC_EVENT_SLOT_STEP_; ++i,++j) {
                _cc_event_t *event = data + j;
                g_mgr.slots[i] = event;
                event->ident = i;
                _cc_queue_sync_push(&g_mgr.idles, (_cc_queue_t*)(&event->lnk));
            }
            g_mgr.slot_length = expand_length;
            _cc_atomic32_set(&g_mgr.slot_refcount, 0);

            /* notify waiters that slots are available */
            if (g_mgr.slot_cond && g_mgr.slot_lock) {
                _cc_mutex_lock(g_mgr.slot_lock);
                _cc_condition_broadcast(g_mgr.slot_cond);
                _cc_mutex_unlock(g_mgr.slot_lock);
            }
            continue;
        }

        /* wait until someone expands slots or idles become available */
        if (g_mgr.slot_lock && g_mgr.slot_cond) {
            _cc_mutex_lock(g_mgr.slot_lock);
            /* re-check quickly before waiting */
            lnk = _cc_queue_sync_pop(&g_mgr.idles);
            if (lnk != &g_mgr.idles && lnk != NULL) {
                _cc_mutex_unlock(g_mgr.slot_lock);
                e = _cc_upcast((_cc_list_t*)lnk, _cc_event_t, lnk);
                break;
            }
            /* wait with timeout to avoid lost wakeups */
            _cc_condition_wait_timeout(g_mgr.slot_cond, g_mgr.slot_lock, (uint32_t)_cc_atomic32_load(&g_mgr.slot_wait_ms));
            _cc_mutex_unlock(g_mgr.slot_lock);
        } else {
            _cc_sleep(0);
        }
    }
    e->ident = _event_activate_ident(e->ident, baseid);
    return e;
}

/**/
_CC_API_PUBLIC(_cc_async_event_t*) _cc_get_async_event(void) {
    _cc_async_event_t *async = NULL;
    static _cc_atomic32_t index = 0;
    int32_t i;
    int32_t limit = (int32_t)_cc_atomic32_load(&g_mgr.async_limit);

    if (limit <= 0) {
        return NULL;
    }

    for (i = 0; i < limit; i++) {
        int32_t started = (int32_t)_cc_atomic32_inc(&index);
        async = g_mgr.async[started % limit];
        if (async && async->running != 0) {
            break;
        }
    }

#if 0
    _cc_async_event_t *n;

    for (; i < count; i++) {
        n = (_cc_async_event_t *)g_mgr.async[i % g_mgr.async_limit];
        if (n == NULL || n->running == 0) {
            continue;
        }

        if (async == NULL || n->processed < async->processed) {
            async = n;
        }
    }
#endif
    _cc_assert(async != NULL);
    return async;
}

/**/
_CC_API_PUBLIC(_cc_event_t*) _cc_get_event_by_id(uint64_t ident) {
    _cc_event_t *e;
    int32_t index = (int32_t)_event_slot_ident(ident);
    if (g_mgr.slot_length <= index) {
        return NULL;
    }

    e = g_mgr.slots[index];
    _cc_assert(e != NULL);
    if (e->ident != ident) {
        _cc_logger_error("event id:%llu is deleted", (unsigned long long)ident);
        return NULL;
    }
    return e;
}

/**/
_CC_API_PUBLIC(_cc_async_event_t*) _cc_get_async_event_by_id(uint64_t ident) {
    int16_t i = _event_async_ident(ident);
    if (g_mgr.async_limit <= i) {
        _cc_logger_error("async_event id:%llu is unregistered!", (unsigned long long)ident);
        return NULL;
    }
    return (_cc_async_event_t *)g_mgr.async[i];
}

/**/
_CC_API_PUBLIC(_cc_event_t*) _cc_alloc_event(_cc_async_event_t *async, const uint32_t flags) {
    _cc_event_t *e = _cc_reserve_event(async->ident);
    if (_cc_unlikely(e == NULL)) {
        return NULL;
    }

    e->filter = _CC_EVENT_UNKNOWN_;
    e->flags = flags;
    e->fd = _CC_INVALID_SOCKET_;
    e->callback = NULL;
    e->expire = 0;
    e->timeout = 0;
    e->data = 0;
#ifdef _CC_EVENT_USE_IOCP_
    e->accept_fd = _CC_INVALID_SOCKET_;
#endif
    if (_CC_EVENT_IS_SOCKET(flags)) {
        e->flags |= _CC_EVENT_SOCKET_;
    }
    
    _cc_list_cleanup(&e->lnk);
    return e;
}
/**/
_CC_API_PUBLIC(void) _cc_free_event(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_socket_t fd;
    
    if (!_cc_list_empty(&e->lnk)) {
        _cc_list_remove(&e->lnk);
    }

    fd = e->fd;

    e->ident = _event_retire_ident(e->ident);
    e->fd = _CC_INVALID_SOCKET_;
    e->flags = _CC_EVENT_UNKNOWN_;
    e->filter = _CC_EVENT_UNKNOWN_;
#ifdef _CC_EVENT_USE_IOCP_
    e->accept_fd = _CC_INVALID_SOCKET_;
#endif
    if (fd != _CC_INVALID_SOCKET_ && fd != 0) {
        _cc_close_socket(fd);
    }
    _cc_queue_sync_push(&g_mgr.idles, (_cc_queue_t*)(&e->lnk));
}

/*
_CC_API_PUBLIC(void) _cc_print_cycle_processed(void) {
    uint32_t i;
    for (i = 0; i < g_mgr.cycles.length; i++) {
        _cc_async_event_t *async = (_cc_async_event_t*)g_mgr.async[i];
        if (async) {
            printf("%d: %d, ", i, async->processed);
        }
    }
    putchar('\n');
}
*/
/**/
bool_t _register_async_event(_cc_async_event_t *async) {
    int32_t i, j;
    int32_t async_limit;
    _cc_assert(async != NULL);

    if (_cc_atomic32_inc_ref(&g_mgr.refcount)) {
        _cc_event_t *data;
        _cc_queue_cleanup(&g_mgr.idles);
        /*If the allocation fails, it directly aborts, so there is no need to check whether the application is successful, which is meaningless.*/
        g_mgr.slots = (_cc_event_t **)_cc_calloc(sizeof(_cc_event_t*), _CC_EVENT_SLOT_STEP_);
        data = (_cc_event_t *)_cc_calloc(sizeof(_cc_event_t), _CC_EVENT_SLOT_STEP_);

        for (i = 0; i < _CC_EVENT_SLOT_STEP_; i++) {
            _cc_event_t *e = (data + i);
            g_mgr.slots[i] = e;
            e->ident = i;
            _cc_queue_push(&g_mgr.idles, (_cc_queue_t*)(&e->lnk));
        }
        
        g_mgr.slot_limit = _get_max_limit();
        g_mgr.slot_length = _CC_EVENT_SLOT_STEP_;
        g_mgr.slot_refcount = 0;
        g_mgr.async_limit = 0;
        g_mgr.async = _cc_calloc(0xFFF, sizeof(_cc_async_event_t*));
        /* initialize slot expansion synchronization */
        g_mgr.slot_lock = _cc_alloc_mutex();
        if (g_mgr.slot_lock == NULL) {
            _cc_logger_warin("failed to allocate slot_lock mutex");
        }
        g_mgr.slot_cond = _cc_alloc_condition();
        if (g_mgr.slot_cond == NULL) {
            _cc_logger_warin("failed to allocate slot_cond condition variable");
        }
        /* default wait timeout (ms) for slot expansion wait */
        _cc_atomic32_set(&g_mgr.slot_wait_ms, 10);
    }

    while (g_mgr.async == NULL) {
        _cc_sleep(10);
    }

    if (g_mgr.async_limit >= 0xFFF) {
        async_limit = 0xFFFF;
        for (i = 0; i < g_mgr.async_limit; i++) {
            if (_cc_atomic_cas((_cc_atomic_t*)&g_mgr.async[i], 0, (intptr_t)async)) {
                async_limit = i;
                break;
            }
        }
        if (async_limit == 0xFFFF) {
            _cc_logger_error("The maximum number of events supported by asynchronous events is %d", g_mgr.async_limit);
            return false;
        }
    } else {
        async_limit = _cc_atomic32_inc(&g_mgr.async_limit);
        g_mgr.async[async_limit] = async;
    }

    async->changes = _cc_alloc_array(_CC_MAX_CHANGE_EVENTS_);
    async->processed = 0;
    async->actives = 0;
    async->running = 0;
    async->timer = 0;
    async->diff = 0;
    async->tick = _cc_get_ticks();
    async->ident = (uint16_t)async_limit & 0xFFF;
    
#ifdef _CC_EVENT_USE_MUTEX_
    async->lock = _cc_alloc_mutex();
#else
    _cc_lock_init(&async->lock);
#endif

    for (i = 0; i < _CC_TIMEOUT_NEAR_; i++) {
        _cc_list_cleanup(&async->nears[i]);
    }

    for (i = 0; i < _CC_TIMEOUT_MAX_LEVEL_; i++) {
        for (j = 0; j < _CC_TIMEOUT_LEVEL_; j++) {
            _cc_list_cleanup(&async->level[i][j]);
        }
    }

    _cc_list_cleanup(&async->pending);
    _cc_list_cleanup(&async->no_timer);

    async->running = 1;
    return true;
}

_CC_API_PRIVATE(void) _event_link_free(_cc_async_event_t *async, _cc_list_t *head) {
    _cc_list_t *next;
    _cc_list_t *curr;
    _cc_event_t *e;

    next = head->next;

    while (_cc_likely(next != head)) {
        curr = next;
        next = next->next;

        e = _cc_upcast(curr, _cc_event_t, lnk);
        if (_CC_ISSET_BIT(_CC_EVENT_CLOSED_, e->flags) == 0 && e->callback) {
            e->callback(async, e, _CC_EVENT_CLOSED_);
        }
        _cc_free_event(async, e);
    }
    _cc_list_cleanup(head);
}

/**/
bool_t _unregister_async_event(_cc_async_event_t *async) {
    int32_t i, j;
    _cc_assert(async != NULL);

    _event_lock(async);
    async->running = 0;

    _cc_array_for_each(_cc_event_t, e, i, async->changes, {
        _cc_list_swap(&async->pending, &e->lnk);
    });

    _cc_free_array(async->changes);
    _event_unlock(async);
    
    for (i = 0; i < _CC_TIMEOUT_NEAR_; i++) {
        _event_link_free(async, &async->nears[i]);
    }

    for (i = 0; i < _CC_TIMEOUT_MAX_LEVEL_; i++) {
        for (j = 0; j < _CC_TIMEOUT_LEVEL_; j++) {
            _event_link_free(async, &async->level[i][j]);
        }
    }
    _event_link_free(async, &async->no_timer);
    _event_link_free(async, &async->pending);

    if (_cc_atomic32_dec_ref(&g_mgr.refcount)) {;
        //
        for (i = 0; i < g_mgr.slot_length; i += _CC_EVENT_SLOT_STEP_) {
            _cc_free(g_mgr.slots[i]);
        }

        _cc_free(g_mgr.slots);
        _cc_free(g_mgr.async);
        if (g_mgr.slot_lock) {
            _cc_free_mutex(g_mgr.slot_lock);
            g_mgr.slot_lock = NULL;
        }
        if (g_mgr.slot_cond) {
            _cc_free_condition(g_mgr.slot_cond);
            g_mgr.slot_cond = NULL;
        }
        _cc_queue_cleanup(&g_mgr.idles);
        g_mgr.slot_length = 0;
        g_mgr.slots = NULL;
        g_mgr.async = NULL;
    } else {
        g_mgr.async[async->ident] = 0;
    }
    return true;
}

/**/
bool_t _valid_event(_cc_async_event_t *async, _cc_event_t *e) {
    return (_event_async_ident(e->ident) == async->ident);
}

_CC_API_PUBLIC(void) _cc_event_set_slot_wait(uint32_t ms) {
    if (ms == 0) ms = 1; /* minimum 1ms */
    _cc_atomic32_set(&g_mgr.slot_wait_ms, (int32_t)ms);
}

/**/
bool_t _valid_fd(_cc_socket_t fd) {
    int r = 0;
    socklen_t length = sizeof(r);

    if (_cc_unlikely(fd == _CC_INVALID_SOCKET_)) {
        return false;
    }

    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&r, &length) != 0) {
        r = _cc_last_errno();
        _cc_logger_error("Socket Error:%d, %s", r, _cc_last_error(r));
        return false;
    }

    if (r != 0) {
        _cc_logger_error("Socket Error:%d, %s", r, _cc_last_error(r));
        return false;
    }
    return true;
}

/**/
bool_t _event_callback(_cc_async_event_t *async, _cc_event_t *e, uint32_t which) {
    /**/
    async->processed++;
    _cc_list_swap(&async->pending, &e->lnk);
    /**/
    _cc_assert(e->callback != NULL);

    if (e->callback(async, e, which)) {
        if ((e->flags & _CC_EVENT_CLOSED_) == 0) {
            return true;
        }
    }

    /**/
    if (_CC_ISSET_BIT(_CC_EVENT_CLOSED_, which) == 0 && 
        _CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags) == 0) {
        e->callback(async, e, _CC_EVENT_CLOSED_);
    }

    /*force disconnect*/
    _CC_MODIFY_BIT(_CC_EVENT_CLOSED_, _CC_EVENT_READABLE_|_CC_EVENT_ACCEPT_, e->flags);
    return false;
}

/**/
bool_t _reset_event(_cc_async_event_t *async, _cc_event_t *e) {
    if (async->running == 0) {
        return false;
    }
    _event_lock(async);
    _cc_array_push(&async->changes, (uintptr_t)e);
    _event_unlock(async);

    return true;
}

/**/
void _reset_event_timeout(_cc_async_event_t *async, _cc_event_t *e) {
    if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, e->flags)) {
        e->expire = async->timer + e->timeout;
        _add_event_timeout(async, e);
    } else {
        _cc_list_swap(&async->no_timer, &e->lnk);
    }
}

/**/
void _reset_event_pending(_cc_async_event_t *async, void (*_reset)(_cc_async_event_t *, _cc_event_t *)) {
    _cc_list_t *head;
    _cc_list_t *next;
    _cc_list_t *curr;
    size_t length = _cc_array_length(async->changes);

    if (length > 0) {
        size_t i;
        _cc_event_t *e;
        _event_lock(async);
        for (i = 0; i < length; i++) {
            e = ((_cc_event_t*)*((uintptr_t*)(async->changes) + i));
            _cc_list_swap(&async->pending, &e->lnk);
        }
        _cc_array_clear(async->changes);
        _event_unlock(async);
    }

    head = &async->pending;
    next = head->next;
    _cc_list_cleanup(&async->pending);

    while (_cc_likely(next != head)) {
        curr = next;
        next = next->next;
        _reset(async, _cc_upcast(curr, _cc_event_t, lnk));
    }
}

/**/
bool_t _disconnect_event(_cc_async_event_t *async, _cc_event_t *e) {
    /**/
    if (e->flags & (_CC_EVENT_SOCKET_ | _CC_EVENT_FILE_)) {
        _cc_shutdown_socket(e->fd, _CC_SHUT_RD_);
    }
    
    _CC_MODIFY_BIT(_CC_EVENT_CLOSED_, _CC_EVENT_READABLE_, e->flags);

    return async->reset(async, e);
}