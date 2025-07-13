/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#include <libcc/core.h>
#include <tlhelp32.h>

static HWND _clipboard_handle = nullptr;

/**/
_CC_API_PUBLIC(bool_t) _cc_set_clipboard_text(const tchar_t *str) {
    if (OpenClipboard(_clipboard_handle)) {
        tchar_t *buf = nullptr;
        size_t len = _tcslen(str) + 1;
        HGLOBAL clipboard_buffer = nullptr;
        HANDLE result;

        if (!EmptyClipboard()) {
            CloseClipboard();
            return false;
        }

        clipboard_buffer = GlobalAlloc(GMEM_DDESHARE, len * sizeof(tchar_t));
        if (clipboard_buffer == nullptr) {
            CloseClipboard();
            return false;
        }

        buf = (tchar_t *)GlobalLock(clipboard_buffer);
        _tcsncpy(buf, str, len);
        buf[len - 1] = 0;
        GlobalUnlock(clipboard_buffer);
#ifdef _CC_UNICODE_
        result = SetClipboardData(CF_UNICODETEXT, buf);
#else
        result = SetClipboardData(CF_TEXT, buf);
#endif
        if (result == nullptr) {
            GlobalFree(clipboard_buffer);
        }
        CloseClipboard();
        return true;
    }
    return false;
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
        if (clipboard_buffer == nullptr) {
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