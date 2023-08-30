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
#include <cc/logger.h>

static void normalize_locale_str(tchar_t *dst, tchar_t *str, size_t buflen) {
    tchar_t *ptr;

    /* chop off encoding if specified. */
    ptr = _tcschr(str, '.');
    if (ptr != NULL) {
        *ptr = '\0';
    }

    /* chop off extra bits if specified. */
    ptr = _tcschr(str, '@');
    if (ptr != NULL) {
        *ptr = '\0';
    }

    /* The "C" locale isn't useful for our needs, ignore it if you see it. */
    if ((str[0] == 'C') && (str[1] == '\0')) {
        return;
    }

    if (*str) {
        if (*dst) {
            _tcsncat(dst, ",", buflen);
        }
        _tcsncat(dst, str, buflen);
    }
}

static void normalize_locales(tchar_t *dst, tchar_t *src, size_t buflen) {
    tchar_t *ptr;

    /* entries are separated by colons */
    while ((ptr = _tcschr(src, ':')) != NULL) {
        *ptr = '\0';
        normalize_locale_str(dst, src, buflen);
        src = ptr + 1;
    }
    normalize_locale_str(dst, src, buflen);
}

void _cc_get_preferred_languages(tchar_t *buf, size_t buflen) {
    /* !!! FIXME: should we be using setlocale()? Or some D-Bus thing? */
    const tchar_t *envr;
    tchar_t *tmp = (tchar_t *)_cc_malloc(sizeof(tchar_t) * buflen);
    *tmp = '\0';
    /* LANG is the primary locale (maybe) */
    envr = getenv("LANG");
    if (envr) {
        _tcsncpy(tmp, envr, buflen);
        tmp[buflen - 1] = 0;
    }

    /* fallback languages */
    envr = getenv("LANGUAGE");
    if (envr) {
        if (*tmp) {
            _tcsncpy(tmp, ":", buflen);
        }
        _tcsncpy(tmp, envr, buflen);
    }

    if (*tmp == '\0') {
        _cc_logger_error(_T("LANG environment variable isn't set"));
    } else {
        normalize_locales(buf, tmp, buflen);
    }

    _cc_free(tmp);
}
