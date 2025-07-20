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
#include <libcc/atomic.h>
#include <libcc/dirent.h>
#include <libcc/logger.h>
#include <libcc/rand.h>
#include <libcc/socket/socket.h>

#ifdef _CC_MSVC_
    #pragma comment(lib, "DbgHelp.lib")
#endif

static HMODULE _kernel32_handle = nullptr;

#ifndef _CC_DISABLED_DUMPER_

#include <Dbghelp.h>
#include <Tlhelp32.h>
#include <stdio.h>
#include <shellapi.h>
#include <objbase.h>

typedef BOOL(WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
                                        CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                                        CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                                        CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

static tchar_t _minidump_module_path[_CC_MAX_PATH_] = {0};
static tchar_t _minidump_app_name[_CC_MAX_PATH_] = {0};
static HMODULE _dbghelp_handle = nullptr;
static HANDLE _current_process = nullptr;
MINIDUMPWRITEDUMP _call_minidump_writedump = nullptr;
static _cc_dumper_callback_t _dumper_callback = nullptr;

_CC_API_PUBLIC(HMODULE) _cc_load_windows_kernel32() {
    if (_kernel32_handle == nullptr) {
        _kernel32_handle = GetModuleHandle(_T("KERNEL32.dll"));
        if (_kernel32_handle == nullptr) {
            _cc_logger_error(_T("GetModuleHandle(KERNEL32.dll) Error Code:%d."), _cc_last_errno());
            return nullptr;
        }
    }
    return _kernel32_handle;
}

_CC_API_PRIVATE(size_t) ResolveSymbol(tchar_t *symbols,size_t size, DWORD64 address) {
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    DWORD64 displacement = 0;
    if (SymFromAddr(_current_process, address, &displacement, symbol)) {
        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD lineDisplacement;
        
        if (SymGetLineFromAddr64(_current_process, address, &lineDisplacement, &line)) {
            return _sntprintf(symbols, size, _T("0x%08lX: %s (File: %s Line: %ld)\n"), (DWORD)address, symbol->Name, line.FileName, line.LineNumber);
        } else {
            return _sntprintf(symbols, size, _T("0x%08lX: %s (Source info unavailable)\n"), (DWORD)address, symbol->Name);
        }
    }
    return 0;
}

_CC_API_PUBLIC(tchar_t**) _cc_get_stack_trace(int *nptr) {
    PVOID frames[64];
    USHORT i,n, frame_count;
    tchar_t **symbols;
    tchar_t *symbols_ptr;
    size_t ptr_array_size;
    size_t ptr_buffer_used = 0;
    size_t limit;
    size_t length;
    static _cc_atomic32_t ref = 0;

    //DWORD options = SymGetOptions();
    //SymSetOptions(options | SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);

    if (_cc_atomic32_inc_ref(&ref)) {
        if (_current_process == nullptr) {
            _current_process = GetCurrentProcess();
        }

        if(!SymInitialize(_current_process, nullptr, TRUE)) {
            _cc_atomic32_dec(&ref);
            fprintf(stderr, "SymInitialize failed: %lu\n", GetLastError());
            return nullptr;
        }
    }

    frame_count = CaptureStackBackTrace(0, _cc_countof(frames), frames, nullptr);
    ptr_array_size = sizeof(tchar_t*) * frame_count;
    limit = _CC_16K_BUFFER_SIZE_;

    symbols = (tchar_t**)malloc(ptr_array_size + _CC_16K_BUFFER_SIZE_);

    symbols_ptr = (tchar_t*)((byte_t*)symbols + ptr_array_size);
    n = 0;
    for ( i = 0; i < frame_count; i++) {
        if ((limit - ptr_buffer_used) < 1024) {
            limit = ptr_buffer_used + _CC_16K_BUFFER_SIZE_;
            symbols = (tchar_t**)realloc(symbols, ptr_array_size + limit);
            symbols_ptr = (tchar_t*)((byte_t*)symbols + ptr_array_size);
        }

        length = ResolveSymbol(symbols_ptr + ptr_buffer_used,
                    limit - ptr_buffer_used, (DWORD64)(uintptr_t)frames[i]);

        if (length > 0) {
            //*(symbols_ptr + ptr_buffer_used + length) = 0;
            symbols[n++] = symbols_ptr + ptr_buffer_used;
            ptr_buffer_used += length;
        }
    }

    if (nptr) {
        *nptr = n;
    }

    if (_cc_atomic32_dec_ref(&ref)) {
        SymCleanup(_current_process);
    }
    return symbols;
}

/**/
_CC_API_PRIVATE(LONG) _exit_proccess(LONG retval) {
    TerminateProcess(_current_process, 0);
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
_CC_API_PRIVATE(LONG WINAPI) _mini_dumper_handler(PEXCEPTION_POINTERS info) {
    LONG retval = EXCEPTION_CONTINUE_SEARCH;
    tchar_t dbghelp_bugreport_path[_CC_MAX_PATH_] = {0};

    BOOL call_success = false;
    HANDLE bugreport_file_handle = nullptr;
    MINIDUMP_EXCEPTION_INFORMATION exc_info = {0};

    time_t timestamp = time(nullptr);

    exc_info.ThreadId = GetCurrentThreadId();
    exc_info.ExceptionPointers = info;
    exc_info.ClientPointers = 0;

    _sntprintf(dbghelp_bugreport_path, _countof(dbghelp_bugreport_path),
               _T("%s\\%s_%d_%03d.dmp"), _minidump_module_path, _minidump_app_name, (int)timestamp, _cc_rand(255) % 100);

    bugreport_file_handle = CreateFile(dbghelp_bugreport_path, GENERIC_WRITE, FILE_SHARE_WRITE, 
                                       nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (bugreport_file_handle == INVALID_HANDLE_VALUE) {
        /* Failed to create dump file" */
        if (_dumper_callback) {
            _dumper_callback(_CC_DUMPER_FAILED_TO_CREATE_DUMP_FILE_, info);
        }

        return _exit_proccess(retval);
    }

    /* write the dump */
    call_success = _call_minidump_writedump(_current_process, GetCurrentProcessId(), bugreport_file_handle,
                                            MiniDumpNormal, &exc_info, nullptr, nullptr);
    if (call_success) {
        retval = EXCEPTION_EXECUTE_HANDLER;
        if (_dumper_callback) {
            _dumper_callback(_CC_DUMPER_SUCCESS_, info);
        }
    } else {
        /*Failed to save dump file to bugreport */
        if (_dumper_callback) {
            _dumper_callback(_CC_DUMPER_FAILED_TO_SAVE_DUMP_FILE_, info);
        }
    }

    CloseHandle(bugreport_file_handle);

    return _exit_proccess(retval);
}

/**/
_CC_API_PUBLIC(bool_t) _cc_install_dumper(_cc_dumper_callback_t callback) {
    int32_t i = 0;
    int32_t rc = 0;
    int32_t x,len = 0;

    if (_call_minidump_writedump && _dbghelp_handle) {
        return true;
    }

    _current_process = GetCurrentProcess();
    _dbghelp_handle = nullptr;
    _dumper_callback = callback;

    rc = (int32_t)GetModuleFileName(nullptr, _minidump_module_path, _cc_countof(_minidump_module_path));
    if (rc <= 0) {
        return false;
    }

    for (i = rc - 1; i >= 0; i--) {
        if (_minidump_module_path[i] == _CC_SLASH_C_) {
            _minidump_module_path[i] = 0;
            i++;
            break;
        }
    }

    x = i;

    if (i > 0 && (rc - i) < _CC_MAX_PATH_) {
        for (; i < rc; i++) {
            _minidump_app_name[len++] = _minidump_module_path[i];
        }
        _minidump_app_name[len] = 0;
    }

    
    _dbghelp_handle = LoadLibrary(_T("DBGHELP.DLL"));
    if (_dbghelp_handle == nullptr) {
        // DBGHELP.DLL not found
        return false;
    }

    _tcscat(_minidump_module_path + x - 1, _T("\\BugReport"));
    _cc_mkdir(_minidump_module_path);

    _call_minidump_writedump = (MINIDUMPWRITEDUMP)GetProcAddress(_dbghelp_handle, "MiniDumpWriteDump");
    if (_call_minidump_writedump == nullptr) {
        // DBGHELP.DLL too old
        FreeLibrary(_dbghelp_handle);
        _dbghelp_handle = nullptr;
        return false;
    }

    SetUnhandledExceptionFilter(_mini_dumper_handler);

    return true;
}

/**/
_CC_API_PUBLIC(void) _cc_uninstall_dumper(void) {
    _dumper_callback = nullptr;
    _call_minidump_writedump = nullptr;

    if (_dbghelp_handle) {
        FreeLibrary(_dbghelp_handle);
    }
}

#endif /*ndef _CC_DISABLED_DUMPER_ */

/**/
_CC_API_PUBLIC(void) _cc_set_last_errno(int32_t _errno) {
    WSASetLastError(_errno);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_last_errno(void) {
    return WSAGetLastError();
}

/**/
_CC_API_PUBLIC(tchar_t *) _cc_last_error(int32_t _errno) {
    static tchar_t sys_error_info[4096];
    tchar_t *p = sys_error_info;

    DWORD c = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, nullptr,
                  _errno, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), sys_error_info, sizeof(sys_error_info), nullptr);
    sys_error_info[c] = 0;
    // kill CR/LF that FormatMessage() sticks at the end
    while (*p) {
        if (*p == '\r') {
            *p = 0;
            break;
        }
        ++p;
    }
    return sys_error_info;
}

_CC_API_PUBLIC(int32_t) _cc_a2w(const char_t *s1, int32_t s1_len, wchar_t *s2, int32_t size) {
    int32_t acp_len = MultiByteToWideChar(CP_ACP, 0, s1, s1_len, nullptr, 0);
    int32_t request_len = 0;
    if (size > acp_len) {
        request_len = (int32_t)MultiByteToWideChar(CP_ACP, 0, s1, s1_len, s2, acp_len);
        s2[request_len] = 0;
    }

    return request_len;
}

_CC_API_PUBLIC(int32_t) _cc_w2a(const wchar_t *s1, int32_t s1_len, char_t *s2, int32_t size) {
    int32_t unicode_len = WideCharToMultiByte(CP_ACP, 0, s1, s1_len, nullptr, 0, nullptr, nullptr);
    int32_t request_len = 0;
    if (size > unicode_len) {
        request_len = WideCharToMultiByte(CP_ACP, 0, s1, s1_len, s2, unicode_len, nullptr, nullptr);
        s2[request_len] = 0;
    }

    return request_len;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_computer_name(tchar_t *name, int32_t maxlen) {
    if (GetComputerName(name,  (LPDWORD)&maxlen)) {
        return 0;
    }
    return 0;
}

HRESULT _CC_CoInitialize(void) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (hr == RPC_E_CHANGED_MODE) {
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    }
    // S_FALSE means success, but someone else already initialized.
    // You still need to call CoUninitialize in this case!
    if (hr == S_FALSE) {
        return S_OK;
    }
    return hr;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_open_url(const tchar_t *url) {
    HINSTANCE rc;

    // MSDN says for safety's sake, make sure COM is initialized.
    const HRESULT hr = _CC_CoInitialize();
    if (FAILED(hr)) {
        _cc_logger_error(_T("CoInitialize failed"));
        return false;
    }

    // Success returns value greater than 32. Less is an error.
    rc = ShellExecute(NULL, _T("open"), url, NULL, NULL, SW_SHOWNORMAL);

    CoUninitialize();
    if (rc <= ((HINSTANCE)32)) {
        _cc_logger_error(_T("Couldn't open given URL."));
        return false;
    }

    return true;
}
