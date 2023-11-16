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
#include <WinSock2.h>
#include <cc/alloc.h>
#include <cc/core.h>

static HWND _clipboard_handle = NULL;

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

#ifdef __CC_WIN32_CE__
/*-- Called from fileio.c */
_CC_API_PUBLIC(int) unlink(const tchar_t *filename) {
    /* Called to delete files before an extract overwrite. */
    return (DeleteFile(filename) ? 0 : -1);
}

#if __CC_WIN32_CE__ < 211
/*
// Old versions of Win CE prior to 2.11 do not support stdio library functions.
// We provide simplyfied replacements that are more or less copies of the
// UNIX style low level I/O API functions. Only unbuffered I/O in binary mode
// is supported.
//-- Called from fileio.c
*/
_CC_API_PUBLIC(int) fflush(FILE *stream) {
    return (FlushFileBuffers((HANDLE)stream) ? 0 : EOF);
}
#endif
#endif

/**/
_CC_API_PUBLIC(int32_t) _cc_set_clipboard_text(const tchar_t *str) {
    if (OpenClipboard(_clipboard_handle)) {
        tchar_t *buf = NULL;
        int32_t len = (int32_t)_tcslen(str);
        HGLOBAL clipboard_buffer = NULL;

        if (!EmptyClipboard()) {
            CloseClipboard();
            return 2;
        }

        clipboard_buffer = GlobalAlloc(GMEM_DDESHARE, (len + 1) * sizeof(tchar_t));
        if (clipboard_buffer == NULL) {
            CloseClipboard();
            return 3;
        }

        buf = (tchar_t *)GlobalLock(clipboard_buffer);
        _tcsncpy(buf, str, len);
        buf[len] = 0;

        GlobalUnlock(clipboard_buffer);
#ifdef _CC_UNICODE_
        SetClipboardData(CF_UNICODETEXT, buf);
#else
        SetClipboardData(CF_TEXT, buf);
#endif

        CloseClipboard();
        return 0;
    }
    return 1;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_clipboard_text(tchar_t *str, int32_t len) {
#ifdef _CC_UNICODE_
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        return 4;
    }
#else
    if (!IsClipboardFormatAvailable(CF_TEXT)) {
        return 4;
    }
#endif
    if (OpenClipboard(_clipboard_handle)) {
#ifdef _CC_UNICODE_
        HGLOBAL clipboard_buffer = GetClipboardData(CF_UNICODETEXT);
#else
        HGLOBAL clipboard_buffer = GetClipboardData(CF_TEXT);
#endif
        if (clipboard_buffer == NULL) {
            CloseClipboard();
            return 3;
        }

        _tcsncpy(str, (LPTSTR)GlobalLock(clipboard_buffer), len);
        *(str + len - 1) = 0;

        GlobalUnlock(clipboard_buffer);
        CloseClipboard();

        return 0;
    }

    return 1;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_has_clipboard_text(void) {
#ifdef _CC_UNICODE_
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        return false;
    }
#else
    if (!IsClipboardFormatAvailable(CF_TEXT)) {
        return false;
    }
#endif
    return true;
}

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

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL,
                  _errno, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), sys_error_info, sizeof(sys_error_info), NULL);
    return sys_error_info;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_computer_name(tchar_t *name, int32_t maxlen) {
    if (GetComputerName(name, &maxlen)) {
        return 0;
    }
    return 0;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_current_directory(tchar_t *cwd, int32_t maxlen) {
    return GetCurrentDirectory(maxlen, (LPTSTR)cwd);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_current_file(tchar_t *cwd, int32_t maxlen) {
    int32_t rc = (int32_t)GetModuleFileName(NULL, cwd, maxlen);
    if (rc <= 0) {
        return 0;
    }
    return rc;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_module_file_name(tchar_t *cwd, int32_t maxlen) {
    int32_t i = 0;
    int32_t len = 0;
    int32_t rc = 0;
    tchar_t path[_CC_MAX_PATH_];

    if (maxlen <= 0) {
        return 0;
    }

    rc = (int32_t)GetModuleFileName(NULL, path, _CC_MAX_PATH_);
    if (rc <= 0) {
        return 0;
    }

    for (i = rc - 1; i >= 0; i--) {
        if (path[i] == _CC_T_PATH_SEP_C_) {
            i++;
            break;
        }
    }

    if (i > 0 && (rc - i) < maxlen) {
        for (; i < rc; i++) {
            cwd[len++] = path[i];
        }
        cwd[len] = 0;
    }

    return len;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_module_document_directory(tchar_t *cwd, int32_t maxlen) {
    /*
    if (!SHGetSpecialFolderPath(NULL, cwd, CSIDL_PERSONAL, false)) {
        _cc_logger_error(_T("SHGetSpecialFolderPath CSIDL_PERSONAL fail (%ld)"),
    CSIDL_PERSONAL, GetLastError());
    }*/
    return _cc_get_module_directory(_T("documents"), cwd, maxlen);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_module_cache_directory(tchar_t *cwd, int32_t maxlen) {
    /*
    if (!SHGetSpecialFolderPath(NULL, cwd, CSIDL_LOCAL_APPDATA, false)) {
        _cc_logger_error(_T("SHGetSpecialFolderPath CSIDL_LOCAL_APPDATA fail
    (%ld)"), CSIDL_LOCAL_APPDATA, GetLastError());
    }*/
    return _cc_get_module_directory(_T("cache"), cwd, maxlen);
}

/**/
int32_t _get_executable_directory(tchar_t *cwd, int32_t maxlen) {
    return (int32_t)GetModuleFileName(NULL, cwd, maxlen);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_module_directory(const tchar_t *module, tchar_t *cwd, int32_t maxlen) {
    int32_t i = 0;
    int32_t rc = (int32_t)GetModuleFileName(NULL, cwd, maxlen);

    for (i = rc - 1; i >= 0; i--) {
        if (cwd[i] == _CC_T_PATH_SEP_C_) {
            cwd[i++] = 0;
            break;
        }
    }

    if (module && i < maxlen) {
        tchar_t *p = (tchar_t *)module;
        cwd[i - 1] = _CC_T_PATH_SEP_C_;
        while (i < maxlen) {
            cwd[i++] = *p++;
            if (*p == 0) {
                break;
            }
        }
        cwd[i++] = 0;
    }

    return i;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_set_current_directory(tchar_t *cwd) {
    if (cwd == NULL) {
        return false;
    }

    SetCurrentDirectory(cwd);

    return true;
}
