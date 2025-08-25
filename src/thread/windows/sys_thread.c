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
#include <libcc/alloc.h>
#include <libcc/loadso.h>
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
typedef void (__cdecl * _end_thread_func_t) (unsigned retval);
typedef uintptr_t (__cdecl * _begin_thread_func_t)
                   (void *security, unsigned stacksize, unsigned (__stdcall *startaddr)(void *),
                    void * arglist, unsigned initflag, unsigned *threadaddr);
#endif

static _begin_thread_func_t pfnBeginThread = _beginthreadex;
static _end_thread_func_t pfnEndThread = _endthreadex;

typedef struct {
    _cc_once_callback_t callback;
} _os_once_data_t;

static BOOL WINAPI _os_once_inner(INIT_ONCE *once, void* args, void** context) {
    _os_once_data_t* data = args;
    data->callback();
    return TRUE;
}

_CC_API_PUBLIC(void) _cc_once(_cc_once_t* guard, _cc_once_callback_t callback) {
    _os_once_data_t data = { 
        .callback = callback
    };
    InitOnceExecuteOnce(&guard->init_once, _os_once_inner, (void*)&data, NULL);
}

static DWORD RunThread(LPVOID args) {
    _cc_thread_running_function(args);

    if (pfnEndThread) {
        pfnEndThread(0);
    }
    return 0;
}

static DWORD WINAPI MINGW32_FORCEALIGN RunThreadViaCreateThread(LPVOID args) {
   return RunThread(args);
}

static unsigned __stdcall MINGW32_FORCEALIGN RunThreadViaBeginThreadEx(LPVOID args) {
    return (unsigned)RunThread(args);
}

#ifndef STACK_SIZE_PARAM_IS_A_RESERVATION
#define STACK_SIZE_PARAM_IS_A_RESERVATION 0x00010000
#endif
/**/
_CC_API_PUBLIC(bool_t) _cc_create_sys_thread(_cc_thread_t* self) {
    int flags = self->stacksize ? STACK_SIZE_PARAM_IS_A_RESERVATION : 0;
    DWORD thread_id = 0;
    // self->stacksize == 0 means "system default", same as win32 expects
    if (pfnBeginThread) {
        self->handle = (_cc_thread_handle_t)((size_t)pfnBeginThread(nullptr, (unsigned int)self->stacksize,
                                                                   (_beginthreadex_proc_type)RunThreadViaBeginThreadEx, self, flags, (unsigned*)&thread_id));
    } else {
        self->handle = CreateThread(nullptr, self->stacksize, RunThreadViaCreateThread, self, flags, &thread_id);
        pfnEndThread = nullptr;
    }
    
    if (self->handle == nullptr) {
        _cc_logger_error(_T("Not enough resources to create thread"));
        return false;
    }
    self->thread_id = thread_id;
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
const DWORD CC_DEBUGGER_NAME_EXCEPTION_CODE = 0x406D1388;
static LONG NTAPI EmptyVectoredExceptionHandler(EXCEPTION_POINTERS *info) {
    if (info != nullptr && info->ExceptionRecord != nullptr && info->ExceptionRecord->ExceptionCode == CC_DEBUGGER_NAME_EXCEPTION_CODE) {
        return EXCEPTION_CONTINUE_EXECUTION;
    } else {
        return EXCEPTION_CONTINUE_SEARCH;
    }
}
/**/
_CC_API_PUBLIC(void) _cc_setup_sys_thread(const tchar_t* name) {
/* Visual Studio 2015, MSVC++ 14.0*/
//#if (_CC_MSVC_ >= 1900) || (defined(__GNUC__) && defined(__i386__))
    PVOID exceptionHandlerHandle;
    pfnSetThreadDescription pSetThreadDescription = nullptr;
    HMODULE kernel32 = 0;
#ifndef _CC_UNICODE_
    wchar_t buf[512];
#endif
    if (name == nullptr) {
        return;
    }

    kernel32 = _cc_load_windows_kernel32();
    if (kernel32) {
        pSetThreadDescription = (pfnSetThreadDescription)_cc_load_function(kernel32, "SetThreadDescription");
        if (pSetThreadDescription == nullptr) {
            return;
        }
    }
    if (pSetThreadDescription) {
    #ifndef _CC_UNICODE_
        _cc_a2w(name, (int32_t)strlen(name), buf, _cc_countof(buf));
        pSetThreadDescription(GetCurrentThread(), buf);
    #else
        pSetThreadDescription(GetCurrentThread(), name);
    #endif
    }
    /* Presumably some version of Visual Studio will understand
       SetThreadDescription(), but we still need to deal with older OSes and
       debuggers. Set it with the arcane exception magic, too. */
    exceptionHandlerHandle = AddVectoredExceptionHandler(1, EmptyVectoredExceptionHandler);
    if (exceptionHandlerHandle) {
        THREADNAME_INFO inf;
        // This magic tells the debugger to name a thread if it's listening.
        bzero(&inf, sizeof(THREADNAME_INFO));
        inf.dwType = 0x1000;
        inf.szName = (LPCSTR)name;
        inf.dwThreadID = (DWORD)-1;
        inf.dwFlags = 0;

        // The debugger catches this, renames the thread, continues on.
        RaiseException(CC_DEBUGGER_NAME_EXCEPTION_CODE, 0, sizeof(inf) / sizeof(ULONG_PTR), (const ULONG_PTR *)&inf);
        RemoveVectoredExceptionHandler(exceptionHandlerHandle);
    }
//#endif
    return;
}

/**/
_CC_API_PUBLIC(size_t) _cc_get_current_sys_thread_id(void) {
    return ((size_t)GetCurrentThreadId());
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
_CC_API_PUBLIC(size_t) _cc_get_sys_thread_id(_cc_thread_t* self) {
    size_t id;

    if (self) {
        id = self->thread_id;
    } else {
        id = _cc_get_current_sys_thread_id();
    }
    return (id);
}

/**/
_CC_API_PUBLIC(void) _cc_wait_sys_thread(_cc_thread_t* self) {
    if (self->handle != nullptr) {
        WaitForSingleObject(self->handle, INFINITE);
        CloseHandle(self->handle);
        self->thread_id = 0;
        self->handle = nullptr;
    }
}

_CC_API_PUBLIC(void) _cc_detach_sys_thread(_cc_thread_t* self) {
    CloseHandle(self->handle);
    self->handle = nullptr;
}