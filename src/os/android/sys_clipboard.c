#include <libcc/os/android.h>

/**/
_CC_API_PUBLIC(bool_t) _cc_set_clipboard_text(const tchar_t *str) {
    return Android_JNI_SetClipboardText(str);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_clipboard_text(tchar_t *str, int32_t len) {
    return Android_JNI_GetClipboardText(str, len);
}

/**/
_CC_API_PUBLIC(bool_t) _cc_has_clipboard_text(void) {
    return Android_JNI_HasClipboardText();
}