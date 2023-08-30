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
#include <signal.h>
#include <unistd.h>

#include "sys_thread.c.h"

#ifdef __CC_LINUX__
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#endif /* __CC_LINUX__ */

#ifdef __CC_ANDROID__
#include <cc/core/android.h>
#endif

#if defined(__CC_LINUX__) || defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
#include <dlfcn.h>
#ifndef RTLD_DEFAULT
#define RTLD_DEFAULT NULL
#endif
#endif

#ifndef __CC_NACL__
/* List of signals to mask in the subthreads */
static int sig_list[] = {SIGHUP, SIGINT, SIGQUIT, SIGPIPE, SIGALRM, SIGTERM, SIGCHLD, SIGWINCH, SIGVTALRM, SIGPROF, 0};
#endif

static void *RunThread(pvoid_t args) {
#ifdef __CC_ANDROID__
    _cc_jni_setup_thread();
#endif

    /* Call the thread function! */
    _cc_thread_running_function(args);
    //
    pthread_exit(NULL);

    return NULL;
}

#if defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
static bool_t checked_setname = false;
static int (*ppthread_setname_np)(const char *) = NULL;
#elif defined(__CC_LINUX__)
static bool_t checked_setname = false;
static int (*ppthread_setname_np)(pthread_t, const char *) = NULL;
#endif

bool_t _cc_create_sys_thread(_cc_thread_t *thrd, pvoid_t args) {
    pthread_attr_t type;
/* do this here before any threads exist, so there's no race condition. */
#if defined(__CC_LINUX__) || defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    if (!checked_setname) {
        void *fn = dlsym(RTLD_DEFAULT, "pthread_setname_np");
#if defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
        ppthread_setname_np = (int (*)(const char *))fn;
#elif defined(__CC_LINUX__)
        ppthread_setname_np = (int (*)(pthread_t, const char *))fn;
#endif
        checked_setname = true;
    }
#endif

    /* Set the thread attributes */
    if (pthread_attr_init(&type) != 0) {
        _cc_logger_error(_T("Couldn't initialize pthread attributes"));
        return false;
    }
    pthread_attr_setdetachstate(&type, PTHREAD_CREATE_JOINABLE);
    /* Set caller-requested stack size. Otherwise: use the system default. */
    if (thrd->stacksize) {
        pthread_attr_setstacksize(&type, thrd->stacksize);
    }
    /* Create the thread and go! */
    if (pthread_create(&(thrd->handle), &type, RunThread, args) != 0) {
        _cc_logger_error(_T("Not enough resources to create thread"));
        return false;
    }

    return true;
}

void _cc_setup_sys_thread(const tchar_t *name) {
    /* NativeClient does not yet support signals.*/
#if !defined(__CC_NACL__)
    int32_t i;
    sigset_t mask;
    /* Mask asynchronous signals for this thread */
    sigemptyset(&mask);
    for (i = 0; sig_list[i]; ++i) {
        sigaddset(&mask, sig_list[i]);
    }
    pthread_sigmask(SIG_BLOCK, &mask, 0);
#endif /* !__CC_NACL__ */

#if defined(__CC_MACOSX__) || defined(__IPHONEOS__) || defined(__CC_LINUX__)
    if (name != NULL) {
        if (ppthread_setname_np != NULL) {
#if defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
            ppthread_setname_np(name);
#elif defined(__CC_LINUX__)
            ppthread_setname_np(pthread_self(), name);
#endif
        }
    }
#else
    _CC_UNUSED(name);
#endif

#ifdef PTHREAD_CANCEL_ASYNCHRONOUS
    /* Allow ourselves to be asynchronously cancelled */
    {
        int oldstate;
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldstate);
    }
#endif
}

bool_t _cc_set_sys_thread_priority(_CC_THREAD_PRIORITY_EMUM_ priority) {
#if __CC_NACL__
    /* FIXME: Setting thread priority does not seem to be supported in NACL */
    return 0;
#elif __CC_LINUX__
    int value;

    if (priority == _CC_THREAD_PRIORITY_LOW_) {
        value = 19;
    } else if (priority == _CC_THREAD_PRIORITY_HIGH_) {
        value = -20;
    } else {
        value = 0;
    }
    if (setpriority(PRIO_PROCESS, syscall(SYS_gettid), value) < 0) {
        /* Note that this fails if you're trying to set high priority
           and you don't have root permission. BUT DON'T RUN AS ROOT!

           You can grant the ability to increase thread priority by
           running the following command on your application binary:
               sudo setcap 'cap_sys_nice=eip' <application>
         */
        _cc_logger_error(_T("Set Thread Priority Error."));
        return false;
    }
    return true;
#else
    struct sched_param sched;
    int policy;
    pthread_t thread = pthread_self();

    if (pthread_getschedparam(thread, &policy, &sched) < 0) {
        _cc_logger_error(_T("pthread_getschedparam failed."));
        return false;
    }
    if (priority == _CC_THREAD_PRIORITY_LOW_) {
        sched.sched_priority = sched_get_priority_min(policy);
    } else if (priority == _CC_THREAD_PRIORITY_HIGH_) {
        sched.sched_priority = sched_get_priority_max(policy);
    } else {
        int min_priority = sched_get_priority_min(policy);
        int max_priority = sched_get_priority_max(policy);
        sched.sched_priority = (min_priority + (max_priority - min_priority) / 2);
    }
    if (pthread_setschedparam(thread, policy, &sched) < 0) {
        _cc_logger_error(_T("pthread_setschedparam failed."));
        return false;
    }
    return true;
#endif /* __CC_LINUX__ */
}

uint32_t _cc_get_current_sys_thread_id(void) {
    return ((uint32_t)((size_t)pthread_self()));
}

uint32_t _cc_get_sys_thread_id(_cc_thread_t *thrd) {
    uint32_t id;

    if (thrd) {
        id = thrd->thread_id;
    } else {
        id = _cc_get_current_sys_thread_id();
    }
    return (id);
}

void _cc_wait_sys_thread(_cc_thread_t *thrd) {
    pthread_join(thrd->handle, 0);
    pthread_detach(thrd->handle);
}

void _cc_detach_sys_thread(_cc_thread_t *thrd) {
    pthread_detach(thrd->handle);
}
