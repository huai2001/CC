#include <libcc/alloc.h>
#include <libcc/logger.h>

_CC_API_PRIVATE(void) normalize_locale_str(tchar_t *dst, tchar_t *str, size_t length) {
    tchar_t *ptr;

    /* chop off encoding if specified. */
    ptr = _tcschr(str, '.');
    if (ptr != nullptr) {
        *ptr = '\0';
    }

    /* chop off extra bits if specified. */
    ptr = _tcschr(str, '@');
    if (ptr != nullptr) {
        *ptr = '\0';
    }

    /* The "C" locale isn't useful for our needs, ignore it if you see it. */
    if ((str[0] == 'C') && (str[1] == '\0')) {
        return;
    }

    if (*str) {
        if (*dst) {
            _tcsncat(dst, ",", length);
        }
        _tcsncat(dst, str, length);
    }
}

_CC_API_PRIVATE(void) normalize_locales(tchar_t *dst, tchar_t *src, size_t length) {
    tchar_t *ptr;

    /* entries are separated by colons */
    while ((ptr = _tcschr(src, ':')) != nullptr) {
        *ptr = '\0';
        normalize_locale_str(dst, src, length);
        src = ptr + 1;
    }
    normalize_locale_str(dst, src, length);
}

_CC_API_PUBLIC(void) _cc_get_preferred_languages(tchar_t *buf, size_t length) {
    /* !!! FIXME: should we be using setlocale()? Or some D-Bus thing? */
    const tchar_t *envr;
    tchar_t *tmp = (tchar_t *)_cc_malloc(sizeof(tchar_t) * length);
    *tmp = '\0';
    /* LANG is the primary locale (maybe) */
    envr = getenv("LANG");
    if (envr) {
        _tcsncpy(tmp, envr, length);
        tmp[length - 1] = 0;
    }

    /* fallback languages */
    envr = getenv("LANGUAGE");
    if (envr) {
        if (*tmp) {
            _tcsncpy(tmp, ":", length);
        }
        _tcsncpy(tmp, envr, length);
    }

    if (*tmp == '\0') {
        _cc_logger_error(_T("LANG environment variable isn't set"));
    } else {
        normalize_locales(buf, tmp, length);
    }

    _cc_free(tmp);
}
