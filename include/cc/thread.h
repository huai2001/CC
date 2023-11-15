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
#ifndef _C_CC_THREAD_H_INCLUDED_
#define _C_CC_THREAD_H_INCLUDED_

#include "atomic.h"
#include "logger.h"

#ifdef __CC_WINDOWS__
#include "thread/windows/sys_thread.h"
#else
#include "thread/pthread/sys_thread.h"
#endif
#include "list.h"
#include "mutex.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    _CC_THREAD_STATE_ALIVE_,
    _CC_THREAD_STATE_DETACHED_,
    _CC_THREAD_STATE_ZOMBIE_,
    _CC_THREAD_STATE_CLEANED_,
} _CC_THREAD_STATE_;

struct _cc_thread {
    int32_t status;
    uint32_t thread_id;
    _cc_thread_handle_t handle;
    _cc_atomic32_t state;
    tchar_t *name;
    /* 0 for default, >0 for user-specified stack size. */
    size_t stacksize;
};

/* The thread priority
 *
 * Note: On many systems you require special privileges to set high priority.
 */
typedef enum {
    _CC_THREAD_PRIORITY_LOW_,
    _CC_THREAD_PRIORITY_NORMAL_,
    _CC_THREAD_PRIORITY_HIGH_
} _CC_THREAD_PRIORITY_EMUM_;

/* This is the function called to run a thread */
extern void _cc_thread_running_function(pvoid_t);

/**/
typedef int32_t (*_cc_thread_callback_t)(_cc_thread_t *, pvoid_t);
/**
 * @brief Create a thread with a default stack size.
 *
 * @param callback Thread  function
 * @param name Thread name
 * @param args Thread user data
 *
 * @return thread handle
 */
_CC_API_PUBLIC(_cc_thread_t *)
_cc_create_thread(_cc_thread_callback_t callback, const tchar_t *name, pvoid_t args);
/**
 * @brief Create a thread
 *
 * @param callback Thread  function
 * @param name Thread name
 * @param stack_size Thread stack size
 * @param args Thread user data
 *
 * @return thread handle
 */
_CC_API_PUBLIC(_cc_thread_t *)
_cc_create_thread_with_stacksize(_cc_thread_callback_t callback, const tchar_t *name, size_t stack_size, pvoid_t args);
/**
 * @brief Create a thread
 *
 * @param callback Thread  function
 * @param name Thread name
 * @param args Thread user data
 *
 * @return true if successful or false on error.
 */
_CC_API_PUBLIC(bool_t)
_cc_thread_start(_cc_thread_callback_t callback, const tchar_t *name, pvoid_t args);
/**
 *  Set the priority for the current thread
 */
_CC_API_PUBLIC(bool_t) _cc_thread_priority(_CC_THREAD_PRIORITY_EMUM_);
/**/
_CC_API_PUBLIC(void) _cc_wait_thread(_cc_thread_t *, int32_t *);
/**/
_CC_API_PUBLIC(void) _cc_detach_thread(_cc_thread_t *);
/**/
_CC_API_PUBLIC(int32_t) _cc_get_thread_id(_cc_thread_t *);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_THREAD_H_INCLUDED_*/
