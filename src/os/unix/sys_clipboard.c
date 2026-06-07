#include <libcc/alloc.h>
#include <unistd.h>

#if defined(__CC_FREEBSD__) || defined(__CC_OPENBSD__)
#include <sys/sysctl.h>
#endif

/**/
_CC_API_PUBLIC(bool_t) _cc_set_clipboard_text(const tchar_t *str) {
    return true;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_clipboard_text(tchar_t *str, int32_t len) {
    return 1;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_has_clipboard_text(void) {
    return true;
}
