#ifndef _C_CC_EVENT_C_H_INCLUDED_
#define _C_CC_EVENT_C_H_INCLUDED_

#include <libcc/event.h>
#include <libcc/OpenSSL.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Max number of pending change events processed in one poller cycle.
 * Default 64 is suitable for client/small-concurrency workloads.
 * For high-concurrency servers, override at compile time, e.g.:
 *   -D_CC_MAX_CHANGE_EVENTS_=512
 */
#ifndef _CC_MAX_CHANGE_EVENTS_
#define _CC_MAX_CHANGE_EVENTS_      64
#endif

#define _CC_EVENT_SLOT_MASK_        0x0FFFFFU
#define _CC_EVENT_ASYNC_MASK_       0x0FFFU
#define _CC_EVENT_ASYNC_SHIFT_      20U

#define _CC_EVENT_IS_SOCKET(flags)                                                                                     \
    _CC_ISSET_BIT(_CC_EVENT_READABLE_ | _CC_EVENT_WRITABLE_ | _CC_EVENT_ACCEPT_ | _CC_EVENT_CONNECT_, (flags))

#ifdef _CC_EVENT_USE_MUTEX_
#define _event_lock(x)               \
    do {                                \
        if (_cc_likely((x)->lock)) {   \
            _cc_mutex_lock((x)->lock); \
        }                               \
    } while (0)

#define _event_unlock(x)               \
    do {                                  \
        if (_cc_likely((x)->lock)) {     \
            _cc_mutex_unlock((x)->lock); \
        }                                 \
    } while (0)
#else
#define _event_lock(x) _cc_spin_lock(&((x)->lock))
#define _event_unlock(x) _cc_unlock(&((x)->lock))
#endif


_CC_FORCE_INLINE_ uint32_t _event_local_ident(uint64_t ident) {
    return (uint32_t)(ident & 0xFFFFFFFFULL);
}

_CC_FORCE_INLINE_ uint32_t _event_slot_ident(uint64_t ident) {
    return _event_local_ident(ident) & _CC_EVENT_SLOT_MASK_;
}

_CC_FORCE_INLINE_ uint16_t _event_async_ident(uint64_t ident) {
    return (uint16_t)((_event_local_ident(ident) >> _CC_EVENT_ASYNC_SHIFT_) & _CC_EVENT_ASYNC_MASK_);
}

_CC_FORCE_INLINE_ uint64_t _event_activate_ident(uint64_t ident, uint16_t baseid) {
    uint32_t generation = (uint32_t)(ident >> 32);
    uint32_t slotid = _event_slot_ident(ident);
    return ((uint64_t)generation << 32) | ((uint64_t)baseid << _CC_EVENT_ASYNC_SHIFT_) |
           (slotid & _CC_EVENT_SLOT_MASK_);
}

_CC_FORCE_INLINE_ uint64_t _event_retire_ident(uint64_t ident) {
    uint32_t generation = (uint32_t)(ident >> 32) + 1;
    uint32_t slotid = _event_slot_ident(ident);
    if (generation == 0) {
        generation = 1;
    }
    return ((uint64_t)generation << 32) | (slotid & _CC_EVENT_SLOT_MASK_);
}
/*
 */
bool_t _register_async_event(_cc_async_event_t *async);
/*
 */
bool_t _unregister_async_event(_cc_async_event_t *async);
/*
 */
bool_t _event_callback(_cc_async_event_t *async, _cc_event_t *e, uint32_t which);
/*
 */
bool_t _valid_fd(_cc_socket_t fd);
/*
 */
bool_t _disconnect_event(_cc_async_event_t *async, _cc_event_t *e);
/*
 */
bool_t _reset_event(_cc_async_event_t *async, _cc_event_t *e);
/*
 */
void _reset_event_pending(_cc_async_event_t *async, void (*func)(_cc_async_event_t *, _cc_event_t *));
/*
 */
void _add_event_timeout(_cc_async_event_t *async, _cc_event_t *e);
/*
 */
void _reset_event_timeout(_cc_async_event_t *async, _cc_event_t *e);
/*
 */
void _update_event_timeout(_cc_async_event_t *async, uint32_t timeout);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_EVENT_C_H_INCLUDED_*/
