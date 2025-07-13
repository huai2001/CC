/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#ifndef _C_CC_MUTEX_H_INCLUDED_
#define _C_CC_MUTEX_H_INCLUDED_

#include "types.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
/**
 * Synchronization functions which can time out return this value
 *  if they time out.
 */
#define _CC_MUTEX_TIMEDOUT_ 1

/**
 * This is the timeout value which corresponds to never time out
 */
#define _CC_MUTEX_MAXWAIT_ (~(int32_t)0)

/**
 * The CC mutex structure, defined in mutex.c
 */
struct _cc_mutex;
typedef struct _cc_mutex _cc_mutex_t;

/**
 * Create a mutex, initialized unlocked
 */
_CC_API_PUBLIC(_cc_mutex_t*) _cc_create_mutex(void);
/** Lock the mutex
 *  @return true, or false on error
 */
_CC_API_PUBLIC(bool_t) _cc_mutex_lock(_cc_mutex_t*);

/** Try Lock the mutex
 *  @return 0, _CC_MUTEX_TIMEDOUT_, or -1 on error
 */
_CC_API_PUBLIC(int) _cc_mutex_try_lock(_cc_mutex_t*);
/** Unlock the mutex
 *  @return true, or false on error
 *
 *  It is an error to unlock a mutex that has not been locked by
 *  the current thread, and doing so results in undefined behavior.
 */
_CC_API_PUBLIC(bool_t) _cc_mutex_unlock(_cc_mutex_t*);
/**
 * Destroy a mutex
 */
_CC_API_PUBLIC(void) _cc_destroy_mutex(_cc_mutex_t**);

/**
 * The CC semaphore structure, defined in semaphore.c
 */
typedef struct _cc_semaphore _cc_semaphore_t;

/**
 * Create a semaphore, initialized with value, returns nullptr on failure.
 */
_CC_API_PUBLIC(_cc_semaphore_t*) _cc_create_semaphore(int32_t);

/**
 * Destroy a semaphore
 */
_CC_API_PUBLIC(void) _cc_destroy_semaphore(_cc_semaphore_t**);

/**
 * This function suspends the calling thread until the semaphore pointed
 * to by sem has a positive count. It then atomically decreases the semaphore
 * count.
 */
_CC_API_PUBLIC(int) _cc_semaphore_wait(_cc_semaphore_t*);

/** Non-blocking variant of _cc_semaphore_wait().
 *  @return 0 if the wait succeeds,
 *  _CC_MUTEX_TIMEDOUT_ if the wait would block, and -1 on error.
 */
_CC_API_PUBLIC(int) _cc_semaphore_try_wait(_cc_semaphore_t*);

/** Variant of _cc_semaphore_wait() with a timeout in milliseconds, returns 0 if
 *  the wait succeeds, _CC_MUTEX_TIMEDOUT_ if the wait does not succeed in
 *  the allotted time, and -1 on error.
 *
 *  On some platforms this function is implemented by looping with a delay
 *  of 1 ms, and so should be avoided if possible.
 */
_CC_API_PUBLIC(int) _cc_semaphore_wait_timeout(_cc_semaphore_t*, uint32_t);

/** Atomically increases the semaphore's count (not blocking).
 *  @return true or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_semaphore_post(_cc_semaphore_t*);

/**
 * Returns the current count of the semaphore
 */
_CC_API_PUBLIC(uint32_t) _cc_semaphore_value(_cc_semaphore_t*);

/**
 * The CC condition variable structure, defined in condition.c
 */
typedef struct _cc_condition _cc_condition_t;
/*@}*/

/**
 * Create a condition variable
 */
_CC_API_PUBLIC(_cc_condition_t*) _cc_create_condition(void);

/**
 * Destroy a condition variable
 */
_CC_API_PUBLIC(void) _cc_destroy_condition(_cc_condition_t**);

/** Restart one of the threads that are waiting on the condition variable,
 *  @return true or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_condition_signal(_cc_condition_t*);

/** Restart all threads that are waiting on the condition variable,
 *  @return true or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_condition_broadcast(_cc_condition_t*);

/** Wait on the condition variable, unlocking the provided mutex.
 *  The mutex must be locked before entering this function!
 *  The mutex is re-locked once the condition variable is signaled.
 *  @return true when it is signaled, or false on error.
 */
_CC_API_PUBLIC(bool_t) _cc_condition_wait(_cc_condition_t*, _cc_mutex_t*);

/** Waits for at most 'ms' milliseconds, and returns 0 if the condition
 *  variable is signaled, _CC_MUTEX_TIMEDOUT_ if the condition is not
 *  signaled in the allotted time, and -1 on error.
 *  On some platforms this function is implemented by looping with a delay
 *  of 1 ms, and so should be avoided if possible.
 */
_CC_API_PUBLIC(int)
_cc_condition_wait_timeout(_cc_condition_t*, _cc_mutex_t*, uint32_t);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_MUTEX_H_INCLUDED_*/
