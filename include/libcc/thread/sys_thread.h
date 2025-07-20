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
#ifndef _C_CC_SYS_THREAD_H_INCLUDED_
#define _C_CC_SYS_THREAD_H_INCLUDED_

#include "../types.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The thread structure, defined in thread.c
 */
typedef struct _cc_thread _cc_thread_t;

/**
 * @brief Create a thread
 *
 * @param thrd Thread handle
 * @param args Thread user data
 *
 * @return true if successful or false on error.
 */
extern bool_t _cc_create_sys_thread(_cc_thread_t* thrd);

/**
 * @brief Get the 32-bit thread identifier for the current thread
 *
 * @return Get current thread id
 */
extern uint32_t _cc_get_current_sys_thread_id(void);

/**
 * @brief Get the 32-bit thread identifier for the specified thread,
 *        equivalent to get_thread_id() if the specified thread is nullptr.
 *
 * @param thrd Thread handle
 *
 * @return Get thread id
 */
extern uint32_t _cc_get_sys_thread_id(_cc_thread_t* thrd);

/**
 * @brief This function does any necessary setup in the child thread
 *
 *  @param name Thread name
 */
extern void _cc_setup_sys_thread(const tchar_t* name);
/**
 * @brief Wait for a thread to finish.
 *        The return code for the thread function is placed in the area
 *        pointed to by 'status', if 'status' is not nullptr.
 *
 * @param thrd Thread handle
 */
extern void _cc_wait_sys_thread(_cc_thread_t* thrd);

/**
 * @brief Mark thread as cleaned up as soon as it exits, without joining.
 *
 * @param thrd Thread handle
 */
extern void _cc_detach_sys_thread(_cc_thread_t* thrd);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_WIN32_THREAD_H_INCLUDED_*/
