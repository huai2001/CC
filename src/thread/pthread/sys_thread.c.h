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
#ifndef _C_CC_SYS_PTHREAD_C_H_INCLUDED_
#define _C_CC_SYS_PTHREAD_C_H_INCLUDED_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include <libcc/alloc.h>
#include <libcc/thread.h>
#include <libcc/time.h>
#include <semaphore.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
#if !_CC_THREAD_PTHREAD_RECURSIVE_MUTEX_ && !_CC_THREAD_PTHREAD_RECURSIVE_MUTEX_NP_
#define _CC_FAKE_RECURSIVE_MUTEX_ 1
#endif
struct _cc_mutex {
    pthread_mutex_t ident;
#if _CC_FAKE_RECURSIVE_MUTEX_
    int32_t recursive;
    pthread_t owner;
#endif
};

struct _cc_semaphore {
    sem_t sem;
};

struct _cc_condition {
    pthread_cond_t cond_var;
};

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_SYS_PTHREAD_C_H_INCLUDED_*/
