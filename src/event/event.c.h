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
#ifndef _C_CC_EVENT_C_H_INCLUDED_
#define _C_CC_EVENT_C_H_INCLUDED_

#include <cc/event/event.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_MAX_CHANGE_EVENTS_                64

#define _CC_EVENT_IS_SOCKET(flags)                                                                                     \
    _CC_ISSET_BIT(_CC_EVENT_READABLE_ | _CC_EVENT_WRITABLE_ | _CC_EVENT_ACCEPT_ | _CC_EVENT_CONNECT_, (flags))


#ifdef _CC_EVENT_USE_MUTEX_
#define _cc_event_lock(x)               \
    do {                                \
        if (_cc_likely((x)->elock)) {   \
            _cc_mutex_lock((x)->elock); \
        }                               \
    } while (0)

#define _cc_event_unlock(x)               \
    do {                                  \
        if (_cc_likely((x)->elock)) {     \
            _cc_mutex_unlock((x)->elock); \
        }                                 \
    } while (0)
#else
#define _cc_event_lock(x) _cc_spin_lock(&((x)->elock))
#define _cc_event_unlock(x) _cc_spin_unlock(&((x)->elock))
#endif

/*
 * @brief Initializes an event cycle class
 *
 * @param cycle _cc_event_cycle_t structure
 *
 * @return true if successful or false on error.
 */
bool_t _cc_event_cycle_init(_cc_event_cycle_t *cycle);

/*
 * @brief Free an event cycle class
 *
 * @param cycle _cc_event_cycle_t structure
 *
 * @return true if successful or false on error.
 */
bool_t _cc_event_cycle_quit(_cc_event_cycle_t *cycle);

/**
 * @brief Calls back an event function
 *
 * @param cycle _cc_event_cycle_t structure
 * @param e _cc_event_t structure
 * @param events The state of an event
 *
 * @return true if successful or false on error.
 */
bool_t _cc_event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, uint16_t events);

/**
 * @brief Cleanup event
 *
 * @param cycle _cc_event_cycle_t structure
 * @param e _cc_event_t structure
 */
void _cc_cleanup_event(_cc_event_cycle_t *cycle, _cc_event_t *e);

/**
 * @brief Check valid event
 *
 * @param cycle _cc_event_cycle_t structure
 * @param e _cc_event_t structure
 *
 * @return true if successful or false on error.
 */
bool_t _cc_valid_event(_cc_event_cycle_t *cycle, _cc_event_t *e);

/**
 * @brief Check valid socket connected event
 *
 * @param cycle _cc_event_cycle_t structure
 * @param flags current event flag
 *
 * @return Returns an updated event flag
 */
uint32_t _cc_valid_connected(_cc_event_t *e, uint32_t flags);

/**
 * @brief Reset timeout events
 *
 * @param cycle _cc_event_cycle_t structure
 * @param e _cc_event_t structure
 *
 */
void _cc_reset_event_timeout(_cc_event_cycle_t *cycle, _cc_event_t *e);
/**
 * @brief Reset pending events
 *
 * @param cycle _cc_event_cycle_t structure
 * @param func callback
 *
 */
void _cc_reset_pending_event(_cc_event_cycle_t *cycle, void (*func)(_cc_event_cycle_t *, _cc_event_t *));

/**
 * @brief Run timeout events
 *
 * @param cycle _cc_event_cycle_t structure
 * @param func callback function
 *
 * @return true if successful or false on error.
 */
void _cc_update_event_timeout(_cc_event_cycle_t *cycle, uint32_t timeout);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_EVENT_C_H_INCLUDED_*/
