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
#include <cc/core.h>
#include <cc/core/android.h>
#include <sys/errno.h>

_CC_API_PUBLIC(int32_t) _cc_a2w(const char_t *s1, int32_t s1_len, wchar_t *s2, int32_t size) {
    return _cc_utf8_to_utf16((const uint8_t *)s1, (const uint8_t *)(s1 + s1_len), (uint16_t *)s2,
                             (uint16_t *)(s2 + size), false);
}

_CC_API_PUBLIC(int32_t) _cc_w2a(const wchar_t *s1, int32_t s1_len, char_t *s2, int32_t size) {
    return _cc_utf16_to_utf8((const uint16_t *)s1, (const uint16_t *)(s1 + s1_len), (uint8_t *)s2,
                             (uint8_t *)(s2 + size), false);
    ;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_is_simulator() {
    return _cc_jni_is_simulator();
}

/**/
_CC_API_PUBLIC(int32_t) _cc_set_clipboard_text(const tchar_t *str) {
    return _cc_jni_set_clipboard_text(str);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_clipboard_text(tchar_t *str, int32_t len) {
    return _cc_jni_get_clipboard_text(str, len);
}

/**/
_CC_API_PUBLIC(bool_t) _cc_has_clipboard_text(void) {
    return _cc_jni_has_clipboard_text();
}

_CC_API_PUBLIC(void) _cc_set_last_errno(int32_t _errno) {
    errno = _errno;
}

_CC_API_PUBLIC(int32_t) _cc_last_errno(void) {
    return errno;
}

_CC_API_PUBLIC(tchar_t*) _cc_last_error(int32_t _errno) {
    return strerror(_errno);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_computer_name(tchar_t *name, int32_t maxlen) {
    return 0;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_current_directory(tchar_t *cwd, int32_t maxlen) {
    return _cc_jni_get_cache_directory(cwd, maxlen);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_module_document_directory(tchar_t *cwd, int32_t maxlen) {
    return _cc_jni_get_files_directory(cwd, maxlen);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_module_cache_directory(tchar_t *cwd, int32_t maxlen) {
    return _cc_jni_get_cache_directory(cwd, maxlen);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_current_file(tchar_t *cwd, int32_t maxlen) {
    return _cc_jni_get_files_directory(cwd, maxlen);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_module_file_name(tchar_t *cwd, int32_t maxlen) {
    return _cc_jni_get_package_name(cwd, maxlen);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_executable_directory(tchar_t *cwd, int32_t maxlen) {
    return _cc_jni_get_files_directory(cwd, maxlen);
}

/**/
_CC_API_PUBLIC(int32_t) _cc_get_module_directory(const tchar_t *module, tchar_t *cwd, int32_t maxlen) {
    return _cc_jni_get_files_directory(cwd, maxlen);
}

/**/
_CC_API_PUBLIC(bool_t) _cc_set_current_directory(tchar_t *cwd) {
    if (cwd == NULL) {
        return false;
    }

    return true;
}