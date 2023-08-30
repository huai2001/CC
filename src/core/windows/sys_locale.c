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
#include <cc/math.h>

#if __WINRT__
void _cc_get_preferred_languages(tchar_t *buf, size_t buflen) {
    WCHAR wbuffer[128] = L"";
    int res = 0;

    /* !!! FIXME: do we not have GetUserPreferredUILanguages on WinPhone or UWP? */
#ifdef __CC_WINPHONE__
    res = GetLocaleInfoEx(LOCALE_NAME_SYSTEM_DEFAULT, LOCALE_SNAME, wbuffer, _cc_countof(wbuffer));
#else
    res = GetSystemDefaultLocaleName(wbuffer, _cc_countof(wbuffer));
#endif

    if (res > 0) {
        /* Need to convert LPWSTR to LPSTR, that is wide char to char. */
#ifndef _CC_UNICODE_
        _cc_w2a(wbuffer, res, buf, buflen);
#else
        int i;
        if (((size_t)res) >= (buflen - 1)) {
            res = (int)(buflen - 1);
        }

        for (i = 0; i < res; i++) {
            buf[i] = wbuffer[i];
        }
#endif
    }
}

#else

typedef BOOL(WINAPI *pfnGetUserPreferredUILanguages)(DWORD, PULONG, WCHAR *, PULONG);
#ifndef MUI_LANGUAGE_NAME
#define MUI_LANGUAGE_NAME 0x8
#endif

static pfnGetUserPreferredUILanguages pGetUserPreferredUILanguages = NULL;
static HMODULE kernel32 = 0;

/* this is the fallback for WinXP...one language, not a list. */
static void SYS_GetUserPreferredUILanguages_winxp(tchar_t *buf, size_t buflen) {
    tchar_t lang[16];
    tchar_t country[16];

    const int langrc = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, lang, sizeof(lang));
    const int ctryrc = GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, country, sizeof(country));

    /* Win95 systems will fail, because they don't have LOCALE_SISO*NAME ... */
    if (langrc == 0) {
        _cc_logger_error(_T("Couldn't obtain language info"));
    } else {
        _sntprintf(buf, buflen, _T("%s%s%s"), lang, ctryrc ? _T("_") : _T(""), ctryrc ? country : _T(""));
    }
}

static ULONG _cc_ulong_min(ULONG x, ULONG y) {
    return _min(x, y);
}

/* this works on Windows Vista and later. */
static void SYS_GetUserPreferredUILanguages_vista(tchar_t *buf, size_t buflen) {
    ULONG numlangs = 0;
    WCHAR *wbuf = NULL;
    ULONG wbuflen = 0;

    // SDL_assert(pGetUserPreferredUILanguages != NULL);
    pGetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numlangs, NULL, &wbuflen);

    wbuf = (WCHAR *)_cc_malloc(sizeof(WCHAR) * wbuflen);
    if (!pGetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &numlangs, wbuf, &wbuflen)) {
        SYS_GetUserPreferredUILanguages_winxp(buf, buflen);
    } else {
        ULONG str_start = 0;
        ULONG i;
        for (i = 0; i < wbuflen; i++) {
            const WCHAR ch = (WCHAR)wbuf[i];
            /* these should all be low-ASCII, safe to cast */
            if (ch == '\0') {
                /* change null separators to commas */
                wbuf[i] = ',';
                str_start = i;
            }
        }
        wbuf[str_start] = '\0';
#ifndef _CC_UNICODE_
        _cc_w2a(wbuf, wbuflen, buf, (int32_t)buflen);
#else
        _tcsncpy(buf, wbuf, buflen);
        buf[buflen - 1] = 0;
#endif
    }

    _cc_free(wbuf);
}
void _cc_get_preferred_languages(tchar_t *buf, size_t buflen) {
    pGetUserPreferredUILanguages =
        (pfnGetUserPreferredUILanguages)GetProcAddress(_cc_load_windows_kernel32(), "GetUserPreferredUILanguages");
    if (pGetUserPreferredUILanguages == NULL) {
        SYS_GetUserPreferredUILanguages_winxp(buf, buflen);
    } else {
        SYS_GetUserPreferredUILanguages_vista(buf, buflen);
    }
}
#endif
