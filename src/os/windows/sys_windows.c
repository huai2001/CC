#include <libcc/atomic.h>
#include <libcc/dirent.h>
#include <libcc/logger.h>
#include <libcc/rand.h>
#include <libcc/thread.h>
#include <libcc/socket.h>

#ifdef __CC_MSVC__
    #pragma comment(lib, "DbgHelp.lib")
#endif

static HMODULE _kernel32_handle = NULL;

#ifndef _CC_DISABLED_DUMPER_

#include <Dbghelp.h>
#include <Tlhelp32.h>
#include <stdio.h>
#include <shellapi.h>
#include <objbase.h>

typedef LONG(WINAPI *RTLGETVERSION_PTR)(PRTL_OSVERSIONINFOW lpVersionInformation);

typedef BOOL(WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
                                        CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                                        CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                                        CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
static RTLGETVERSION_PTR _call_get_version = NULL;
static tchar_t _minidump_module_path[_CC_MAX_PATH_] = {0};
static tchar_t _minidump_app_name[_CC_MAX_PATH_] = {0};
static HMODULE _dbghelp_handle = NULL;
static HANDLE _current_process = NULL;
MINIDUMPWRITEDUMP _call_minidump_writedump = NULL;
static _cc_dumper_callback_t _dumper_callback = NULL;

static void init_get_version(void) {
    HMODULE ntdll_module = GetModuleHandleW(L"ntdll.dll");
    if (ntdll_module == NULL) {
        _cc_logger_error(_T("GetModuleHandle(ntdll.dll) Error Code:%d."), _cc_last_errno());
    }
	_call_get_version = (RTLGETVERSION_PTR)GetProcAddress(ntdll_module, "RtlGetVersion");
}

/**/
_CC_API_PUBLIC(size_t) _cc_get_device_name(tchar_t *cname, size_t length) {
    if (GetComputerName(cname, (DWORD*)&length)) {
        return length;
    }
    return 0;
}

/**/
_CC_API_PUBLIC(void) _cc_get_os_version(uint32_t *major, uint32_t *minor, uint32_t *build) {
    static _cc_once_t once_get_version = _CC_ONCE_INIT_;
    OSVERSIONINFOW os_info;
    _cc_once(&once_get_version,init_get_version);

    if (!_call_get_version) {
        return;
    }
    
    _call_get_version(&os_info);

    if (major) {
        *major = os_info.dwMajorVersion;
    }

    if (minor) {
        *minor = os_info.dwMajorVersion;
    }

    if (build) {
        *build = os_info.dwBuildNumber;
    }
}

_CC_API_PUBLIC(HMODULE) _cc_load_windows_kernel32() {
    if (_kernel32_handle == NULL) {
        _kernel32_handle = GetModuleHandleW(L"KERNEL32.dll");
        if (_kernel32_handle == NULL) {
            _cc_logger_error(_T("GetModuleHandle(KERNEL32.dll) Error Code:%d."), _cc_last_errno());
            return NULL;
        }
    }
    return _kernel32_handle;
}

_CC_API_PUBLIC(size_t) _cc_get_resolve_symbol(tchar_t *buf, size_t length) {
    DWORD64 displacement = 0;
    PVOID frames[64];
    USHORT i, n;
    size_t r = 0;
    CHAR buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(CHAR)];
    PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    if (_current_process == NULL) {
        _current_process = GetCurrentProcess();
    }

    if (!SymInitialize(_current_process, NULL, TRUE)) {
        _cc_logger_error(_T("SymInitialize failed with error code: %d"), _cc_last_errno());
        return 0;
    }

    n = CaptureStackBackTrace(0, _cc_countof(frames), frames, NULL);
    if (n) {
        for (r = 0, i = 1; i < n; i++) {
            if (SymFromAddr(_current_process, (DWORD64)(uintptr_t)frames[i], &displacement, symbol)) {
                IMAGEHLP_LINE64 line = { sizeof(line) };
                DWORD displacementLine = 0;
                size_t fmt_length = 0;
                size_t remaining = length - r;
                if (SymGetLineFromAddr64(_current_process, (DWORD64)(uintptr_t)frames[i], &displacementLine, &line)) {
                    fmt_length = _sntprintf(buf + r, remaining, _T("%s:%ld\n"), line.FileName, line.LineNumber);
                } else {
                    fmt_length = _sntprintf(buf + r, remaining, _T("%s\n"), symbol->Name);
                }
                if (fmt_length <= 0 || fmt_length >= remaining) {
                    break;
                }
                r += fmt_length;
            }
        }
        if (r > 0) {
            buf[r - 1] = 0;
        }
    }
    SymCleanup(_current_process);
    return r;
}

/**/
_CC_API_PRIVATE(LONG) _exit_process(LONG retval) {
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
    HANDLE bugreport_file_handle = NULL;
    MINIDUMP_EXCEPTION_INFORMATION exc_info = {0};

    time_t timestamp = time(NULL);

    exc_info.ThreadId = GetCurrentThreadId();
    exc_info.ExceptionPointers = info;
    exc_info.ClientPointers = 0;

    _sntprintf(dbghelp_bugreport_path, _countof(dbghelp_bugreport_path),
               _T("%s\\%s_%d_%03d.dmp"), _minidump_module_path, _minidump_app_name, (int)timestamp, _cc_rand(255) % 100);
    dbghelp_bugreport_path[_CC_MAX_PATH_ - 1] = 0;

    bugreport_file_handle = CreateFile(dbghelp_bugreport_path, GENERIC_WRITE, FILE_SHARE_WRITE, 
                                       NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (bugreport_file_handle == INVALID_HANDLE_VALUE) {
        /* Failed to create dump file" */
        if (_dumper_callback) {
            _dumper_callback(_CC_DUMPER_FAILED_TO_CREATE_DUMP_FILE_, info);
        }

        return _exit_process(retval);
    }

    /* write the dump */
    call_success = _call_minidump_writedump(_current_process, GetCurrentProcessId(), bugreport_file_handle,
                                            MiniDumpNormal, &exc_info, NULL, NULL);
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

    return _exit_process(retval);
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
    _dbghelp_handle = NULL;
    _dumper_callback = callback;

    rc = (int32_t)GetModuleFileName(NULL, _minidump_module_path, _cc_countof(_minidump_module_path));
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
    if (_dbghelp_handle == NULL) {
        // DBGHELP.DLL not found
        return false;
    }

    _tcscat(_minidump_module_path + x - 1, _T("\\BugReport"));
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
_CC_API_PUBLIC(void) _cc_uninstall_dumper(void) {
    _dumper_callback = NULL;
    _call_minidump_writedump = NULL;

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
    //MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
    DWORD res = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL,
                  _errno, MAKELANGID(LANG_NEUTRAL, SUBLANG_ENGLISH_US), (LPSTR)sys_error_info, sizeof(sys_error_info), NULL);
    
    if (!res && (GetLastError() == ERROR_MUI_FILE_NOT_FOUND || GetLastError() == ERROR_RESOURCE_TYPE_NOT_FOUND)) {
		res = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, _errno, 0, (LPSTR)sys_error_info, sizeof(sys_error_info), NULL);
    }
    sys_error_info[res] = 0;
    
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

/**/
_CC_API_PUBLIC(const _cc_string_t*) _cc_get_module_file_name(void) {
    static TCHAR dl[64];
    static _cc_string_t path = {0};
    if (path.length == 0) {
        size_t length = 0, i;
        HMODULE hModule = NULL;
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)_cc_get_module_file_name, &hModule);

        length = GetModuleFileName(hModule, dl, _cc_countof(dl));
        if (length <= 0) {
            return &path;
        }
        for (i = length - 1; i > 0; i--) {
            if (dl[i] == _CC_SLASH_C_) {
                break;
            }
        }
        if (i > 0) {
            path.ptr = dl;
            path.length = length - i;
            memmove(dl,dl + i + 1, length - i);
            dl[length - 1] = 0;
        }
    }

    return &path;
}

_CC_API_PUBLIC(int32_t) _cc_a2w(const char_t *s1, int32_t s1_len, wchar_t *s2, int32_t size) {
    int32_t acp_len = MultiByteToWideChar(CP_ACP, 0, s1, s1_len, NULL, 0);
    int32_t request_len = 0;
    if (size > acp_len) {
        request_len = (int32_t)MultiByteToWideChar(CP_ACP, 0, s1, s1_len, s2, acp_len);
        s2[request_len] = 0;
    }

    return request_len;
}

_CC_API_PUBLIC(int32_t) _cc_w2a(const wchar_t *s1, int32_t s1_len, char_t *s2, int32_t size) {
    int32_t unicode_len = WideCharToMultiByte(CP_ACP, 0, s1, s1_len, NULL, 0, NULL, NULL);
    int32_t request_len = 0;
    if (size > unicode_len) {
        request_len = WideCharToMultiByte(CP_ACP, 0, s1, s1_len, s2, unicode_len, NULL, NULL);
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
