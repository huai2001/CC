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
#include <cc/loadso.h>
#include "sys_thread.c.h"

#ifndef __CC_WIN32_CE__
/* We'll use the C library from this DLL */
#include <process.h>
#endif

#if defined(__WATCOMC__)

/* This is for Watcom targets except OS2 */
#if __WATCOMC__ < 1240
#define __watcall
#endif

typedef unsigned long(__watcall* _begin_thread_func_t)(
    void*,
    unsigned,
    unsigned(__stdcall* func)(void*),
    void* arg,
    unsigned,
    unsigned* threadID);

typedef void(__watcall* _end_thread_func_t)(unsigned code);

#elif defined(__CC_WIN32_CE__)
typedef void* _begin_thread_func_t;
typedef void* _end_thread_func_t;
#else
typedef uintptr_t(__cdecl* _begin_thread_func_t)(
    void*,
    unsigned,
    unsigned(__stdcall* func)(void*),
    void* arg,
    unsigned,
    unsigned* threadID);
typedef void(__cdecl* _end_thread_func_t)(unsigned code);
#endif

static _begin_thread_func_t pfnBeginThread = NULL;

typedef struct _win_thread_params {
    void* args;
} _win_thread_params_t;

/**/
_CC_API_PRIVATE(DWORD) RunThread(void* args) {
    /* Call the thread function! */
    _cc_thread_running_function(args);
    return 0;
}

/**/
_CC_API_PRIVATE(DWORD WINAPI) RunThreadViaCreateThread(LPVOID data) {
    return RunThread(data);
}

#ifndef STACK_SIZE_PARAM_IS_A_RESERVATION
#define STACK_SIZE_PARAM_IS_A_RESERVATION 0x00010000
#endif
/**/
_CC_API_PUBLIC(bool_t) _cc_create_sys_thread(_cc_thread_t* thrd, pvoid_t args) {
    int flags = thrd->stacksize ? STACK_SIZE_PARAM_IS_A_RESERVATION : 0;
    DWORD thread_id = 0;
    thrd->handle = CreateThread(NULL, thrd->stacksize, RunThreadViaCreateThread,
                                args, flags, &thread_id);
    thrd->thread_id = thread_id;
    if (thrd->handle == NULL) {
        _cc_logger_error(_T("Not enough resources to create thread"));
        return false;
    }
    return true;
}

#pragma pack(push, 8)

typedef struct tagTHREADNAME_INFO {
    DWORD dwType;     /* must be 0x1000 */
    LPCSTR szName;    /* pointer to name (in user addr space) */
    DWORD dwThreadID; /* thread ID (-1=caller thread) */
    DWORD dwFlags;    /* reserved for future use, must be zero */
} THREADNAME_INFO;

#pragma pack(pop)

typedef HRESULT(WINAPI* pfnSetThreadDescription)(HANDLE, PCWSTR);
const DWORD MS_VC_EXCEPTION = 0x406D1388;

/**/
_CC_API_PUBLIC(void) _cc_setup_sys_thread(const tchar_t* name) {
/* Visual Studio 2015, MSVC++ 14.0*/
#if (_MSC_VER >= 1900)
    pfnSetThreadDescription pSetThreadDescription = NULL;
    HMODULE kernel32 = 0;
#ifndef _CC_UNICODE_
    wchar_t buf[256];
#endif
    if (name == NULL) {
        return;
    }

    kernel32 = _cc_load_windows_kernel32();
    if (kernel32) {
        pSetThreadDescription = (pfnSetThreadDescription)_cc_load_function(
            kernel32, "SetThreadDescription");
        if (pSetThreadDescription == NULL) {
            return;
        }
    }

#ifndef _CC_UNICODE_
    _cc_a2w(name, strlen(name), buf, _cc_countof(buf));
    pSetThreadDescription(GetCurrentThread(), buf);
#else
    pSetThreadDescription(GetCurrentThread(), name);
#endif
    /* Presumably some version of Visual Studio will understand
       SetThreadDescription(), but we still need to deal with older OSes and
       debuggers. Set it with the arcane exception magic, too. */

    if (IsDebuggerPresent()) {
        THREADNAME_INFO inf;

        /* C# and friends will try to catch this Exception, let's avoid it. */

        /* This magic tells the debugger to name a thread if it's listening. */
        bzero(&inf, sizeof(THREADNAME_INFO));
        inf.dwType = 0x1000;
        inf.szName = (LPCSTR)name;
        inf.dwThreadID = (DWORD)-1;
        inf.dwFlags = 0;

        /* The debugger catches this, renames the thread, continues on. */
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(inf) / sizeof(ULONG),
                       (const ULONG_PTR*)&inf);
    }
#endif
    return;
}

/**/
_CC_API_PUBLIC(uint32_t) _cc_get_current_sys_thread_id(void) {
    return ((uint32_t)GetCurrentThreadId());
}

/**/
_CC_API_PUBLIC(bool_t) _cc_set_sys_thread_priority(_CC_THREAD_PRIORITY_EMUM_ priority) {
    int value;

    if (priority == _CC_THREAD_PRIORITY_LOW_) {
        value = THREAD_PRIORITY_LOWEST;
    } else if (priority == _CC_THREAD_PRIORITY_HIGH_) {
        value = THREAD_PRIORITY_HIGHEST;
    } else {
        value = THREAD_PRIORITY_NORMAL;
    }
    if (!SetThreadPriority(GetCurrentThread(), value)) {
        _cc_logger_error(_T("Set Thread Priority Error."));
        return false;
    }
    return true;
}

/**/
_CC_API_PUBLIC(uint32_t) _cc_get_sys_thread_id(_cc_thread_t* thrd) {
    uint32_t id;

    if (thrd) {
        id = thrd->thread_id;
    } else {
        id = _cc_get_current_sys_thread_id();
    }
    return (id);
}

/**/
_CC_API_PUBLIC(void) _cc_wait_sys_thread(_cc_thread_t* thrd) {
    if (thrd->handle != NULL) {
        WaitForSingleObject(thrd->handle, INFINITE);
        CloseHandle(thrd->handle);
        thrd->thread_id = 0;
        thrd->handle = NULL;
    }
}

_CC_API_PUBLIC(void) _cc_detach_sys_thread(_cc_thread_t* thrd) {
    CloseHandle(thrd->handle);
    thrd->handle = NULL;
}