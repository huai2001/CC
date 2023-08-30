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
#include <cc/atomic.h>
#include <cc/dirent.h>
#include <cc/logger.h>
#include <cc/socket/socket.h>

#pragma comment(lib, "DbgHelp.lib")

static HMODULE hModuleKernel32 = NULL;

#ifndef _CC_DISABLED_DUMPER_

#include <Dbghelp.h>
#include <Tlhelp32.h>
#include <cc/rand.h>
#include <stdio.h>

typedef BOOL(WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
                                        CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                                        CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                                        CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

static PEXCEPTION_POINTERS _dumper_exception_info = {0};
static tchar_t _minidump_module_path[_CC_MAX_PATH_] = {0};
static tchar_t _minidump_app_name[_CC_MAX_PATH_] = {0};
static HMODULE _dbghelp_handle = NULL;
MINIDUMPWRITEDUMP _call_minidump_writedump = NULL;
static _cc_dumper_callback_t _dumper_callback = NULL;

/**/
static LONG _exit_proccess(LONG retval) {
    TerminateProcess(GetCurrentProcess(), 0);
    /*
    // MLM Note: ExitThread will work, and it allows the MiniDumper to kill a crashed thread
    // without affecting the rest of the application. The question of the day:
    //   Is That A Good Idea??? Answer: ABSOLUTELY NOT!!!!!!!
    //
    //ExitThread(0);
    */
    return retval;
}

/**/
static LONG WINAPI _mini_dumper_handler(PEXCEPTION_POINTERS info) {
    SYSTEMTIME st = {0};
    LONG retval = EXCEPTION_CONTINUE_SEARCH;
    tchar_t dbghelp_bugreport_path[_CC_MAX_PATH_] = {0};

    BOOL call_success = false;
    HANDLE bugreport_file_handle = NULL;
    MINIDUMP_EXCEPTION_INFORMATION exc_info = {0};

    GetLocalTime(&st);
    _dumper_exception_info = info;
    exc_info.ThreadId = GetCurrentThreadId();
    exc_info.ExceptionPointers = _dumper_exception_info;
    exc_info.ClientPointers = 0;

    _sntprintf(dbghelp_bugreport_path, _countof(dbghelp_bugreport_path),
               _T("%s\\%s_%d_%02d_%02d_%02d_%02d_%02d_%03d.dmp"), _minidump_module_path, _minidump_app_name, st.wYear,
               st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, _cc_rand() % 100);

    bugreport_file_handle = CreateFile(dbghelp_bugreport_path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (bugreport_file_handle == INVALID_HANDLE_VALUE) {
        /* Failed to create dump file" */
        if (_dumper_callback) {
            _dumper_callback(_CC_DUMPER_FAILED_TO_CREATE_DUMP_FILE_, _dumper_exception_info);
        }

        return _exit_proccess(retval);
    }

    /* write the dump */
    call_success = _call_minidump_writedump(GetCurrentProcess(), GetCurrentProcessId(), bugreport_file_handle,
                                            MiniDumpNormal, &exc_info, NULL, NULL);
    if (call_success) {
        retval = EXCEPTION_EXECUTE_HANDLER;
        if (_dumper_callback) {
            _dumper_callback(_CC_DUMPER_SUCCESS_, _dumper_exception_info);
        }
    } else {
        /*Failed to save dump file to bugreport */
        if (_dumper_callback) {
            _dumper_callback(_CC_DUMPER_FAILED_TO_SAVE_DUMP_FILE_, _dumper_exception_info);
        }
    }

    CloseHandle(bugreport_file_handle);

    return _exit_proccess(retval);
}

/**/
bool_t _cc_install_dumper(_cc_dumper_callback_t callback) {
    int32_t i = 0;
    int32_t rc = 0;
    int32_t len = 0;
    tchar_t dbghelp_module_file[_CC_MAX_PATH_] = {0};

    if (_call_minidump_writedump && _dbghelp_handle) {
        return true;
    }

    _dbghelp_handle = NULL;
    _dumper_callback = callback;

    rc = (int32_t)GetModuleFileName(NULL, _minidump_module_path, _cc_countof(_minidump_module_path));
    if (rc <= 0) {
        return false;
    }

    for (i = rc - 1; i >= 0; i--) {
        if (_minidump_module_path[i] == _CC_T_PATH_SEP_C_) {
            _minidump_module_path[i] = 0;
            i++;
            break;
        }
    }

    if (i > 0 && (rc - i) < _CC_MAX_PATH_) {
        for (; i < rc; i++) {
            _minidump_app_name[len++] = _minidump_module_path[i];
        }
        _minidump_app_name[len] = 0;
    }

    _sntprintf(dbghelp_module_file, _countof(dbghelp_module_file), _T("%s\\DBGHELP.DLL"), _minidump_module_path);
    _dbghelp_handle = LoadLibrary(dbghelp_module_file);
    if (_dbghelp_handle == NULL) {
        _dbghelp_handle = LoadLibrary(_T("DBGHELP.DLL"));
        if (_dbghelp_handle == NULL) {
            // DBGHELP.DLL not found
            return false;
        }
    }

    _tcscat(_minidump_module_path, _T("\\BugReport"));
    _cc_mkdir(_minidump_module_path);

    _call_minidump_writedump = (MINIDUMPWRITEDUMP)GetProcAddress(_dbghelp_handle, "MiniDumpWriteDump");
    if (_call_minidump_writedump == NULL) {
        // DBGHELP.DLL too old
        FreeLibrary(_dbghelp_handle);
        _dbghelp_handle = NULL;
        return false;
    }

    SetUnhandledExceptionFilter(_mini_dumper_handler);

    return true;
}

/**/
void _cc_uninstall_dumper(void) {
    _dumper_callback = NULL;
    _call_minidump_writedump = NULL;

    if (_dbghelp_handle) {
        FreeLibrary(_dbghelp_handle);
    }
}

#endif /*ndef _CC_DISABLED_DUMPER_ */

HMODULE _cc_load_windows_kernel32() {

    if (hModuleKernel32 == NULL) {
        hModuleKernel32 = GetModuleHandle(_T("KERNEL32.dll"));
        if (hModuleKernel32 == NULL) {
            _cc_logger_error(_T("GetModuleHandle(KERNEL32.dll) Error Code:%d."), _cc_last_errno());
            return NULL;
        }
    }
    return hModuleKernel32;
}

void _cc_print_stack_trace(FILE *fp, int i) {
    HANDLE process = GetCurrentProcess();
    pvoid_t frames[1024];
    SYMBOL_INFO_PACKAGE sym;

    int nptrs;

    SymInitialize(process, NULL, TRUE);

    nptrs = CaptureStackBackTrace(0, _cc_countof(frames), frames, NULL);
    sym.si.MaxNameLen = MAX_SYM_NAME;
    sym.si.SizeOfStruct = sizeof(SYMBOL_INFO);
    for (; i < nptrs; i++) {
        DWORD64 address = (DWORD64)(frames[i]);
        DWORD64 displacement = 0;
        if (SymFromAddr(process, address, &displacement, &sym.si)) {
            fprintf(fp, "%s\n", sym.si.Name);
        } else {
            fprintf(fp, "%d ERROR - SymFromAddr()\n", nptrs - i - 1);
        }
    }
    SymCleanup(process);
}