#include <libcc/alloc.h>
#include <libcc/os.h>
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