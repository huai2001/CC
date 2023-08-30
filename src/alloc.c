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
#include <time.h>
#include <cc/dirent.h>
#include <cc/core.h>
#include <cc/alloc.h>
#include <cc/string.h>

#ifndef __CC_WINDOWS__
#include <dlfcn.h>
#endif

/* Explicitly override malloc/free etc when using tcmalloc. */
#if defined(__CC_USE_TCMALLOC__)
#define malloc(size) tc_malloc(size)
#define calloc(count, size) tc_calloc(count, size)
#define realloc(ptr, size) tc_realloc(ptr, size)
#define free(ptr) tc_free(ptr)
#elif defined(__CC_USE_JEMALLOC__)
#define malloc(size) je_malloc(size)
#define calloc(count, size) je_calloc(count, size)
#define realloc(ptr, size) je_realloc(ptr, size)
#define free(ptr) je_free(ptr)
#endif

#define LEVEL 5

#ifdef _CC_ENABLE_MEMORY_TRACKED_

static void _write_timestamp(FILE *wfp) {
    time_t now_time = time(NULL);
    struct tm *t = localtime(&now_time);

    fprintf(wfp, "%4d-%02d-%02d %02d:%02d:%02d\n", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
            t->tm_min, t->tm_sec);
}

static void __cc_tracked_memory_unlink(pvoid_t ptr) {
    tchar_t _memory_file[_CC_MAX_PATH_];
    _sntprintf(_memory_file, _cc_countof(_memory_file), _T("./memory/%p.mem"), ptr);
    _cc_unlink(_memory_file);
}

static void __cc_tracked_memory(pvoid_t ptr, size_t size, const tchar_t *msg) {
    tchar_t _memory_file[_CC_MAX_PATH_];
    FILE *fp;
    _sntprintf(_memory_file, _cc_countof(_memory_file), _T("./memory/%p.mem"), ptr);
    fp = _tfopen(_memory_file, _T("a"));
    if (fp == NULL) {
        return;
    }
    
    _write_timestamp(fp);
    _ftprintf(fp, _T("%p %s %ld\n"), ptr, msg, size);
	_cc_print_stack_trace(fp, 3);
    fclose(fp);
}
#endif

/**/
static pvoid_t __cc_check_memory(pvoid_t ptr, size_t size, const tchar_t *msg) {
    if (_cc_unlikely(NULL == ptr)) {
        _cc_logger_error(_T("%s: Out of memory trying to allocate %zu bytes"), msg, size);
        _cc_abort();
    }
    
#ifdef _CC_ENABLE_MEMORY_TRACKED_
    __cc_tracked_memory(ptr, size, msg);
#endif
    return ptr;
}

/**/
pvoid_t _cc_malloc(size_t n) {
    return __cc_check_memory(malloc(n), n, _T("_cc_malloc"));
}

/**/
pvoid_t _cc_calloc(size_t c, size_t n) {
    return __cc_check_memory(calloc(c, n), c * n, _T("_cc_calloc"));
}

/**/
pvoid_t _cc_realloc(void *d, size_t n) {
    if (_cc_unlikely(n <= 0)) {
        _cc_free(d);
        return NULL;
    }

    if (_cc_unlikely(d == NULL)) {
        return _cc_malloc(n);
    }
    
#ifdef _CC_ENABLE_MEMORY_TRACKED_
    __cc_tracked_memory_unlink(d);
#endif

    return __cc_check_memory(realloc(d, n), n, _T("realloc"));
}

/**/
void _cc_free(pvoid_t p) {
    _cc_assert(p != NULL);
    free(p);
#ifdef _CC_ENABLE_MEMORY_TRACKED_
    __cc_tracked_memory_unlink(p);
#endif
}

/**/
wchar_t *_cc_strdupW(const wchar_t *str) {
    return _cc_strndupW(str, wcslen(str));
}

/**/
char_t *_cc_strdupA(const char_t *str) {
    return _cc_strndupA(str, strlen(str));
}

/**/
wchar_t *_cc_strndupW(const wchar_t *str, size_t str_len) {
    wchar_t *req_str;

    if (_cc_unlikely(str_len <= 0)) {
        return NULL;
    }

    req_str = (wchar_t *)_cc_malloc(sizeof(wchar_t) * (str_len + 1));
    if (_cc_likely(req_str)) {
        memcpy(req_str, str, str_len * sizeof(wchar_t));
        req_str[str_len] = 0;
    }

    return req_str;
}

/**/
char_t *_cc_strndupA(const char_t *str, size_t str_len) {
    char_t *req_str;

    if (_cc_unlikely(str_len <= 0)) {
        return NULL;
    }

    req_str = (char_t *)_cc_malloc(sizeof(char_t) * (str_len + 1));
    if (_cc_likely(req_str)) {
        memcpy(req_str, str, str_len * sizeof(char_t));
        req_str[str_len] = 0;
    }

    return req_str;
}
