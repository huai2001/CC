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
#include <time.h>
#include <libcc/dirent.h>
#include <libcc/core.h>
#include <libcc/alloc.h>
#include <libcc/string.h>
#include <libcc/atomic.h>

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

#ifdef _CC_ENABLE_MEMORY_TRACKED_
static struct {
    _cc_atomic32_t ref;
    tchar_t current_process_path[_CC_MAX_PATH_];
} g = {0};

_CC_API_PRIVATE(void) remove_memory_directory(const tchar_t *directory) {
    tchar_t source_file[_CC_MAX_PATH_ * 2] = {0};
    DIR *dpath = nullptr;
    struct dirent *d;
    struct _stat stat_buf;
    
    if( (dpath = opendir(directory)) == nullptr) {
        return;
    }
    
    while ((d = readdir(dpath)) != nullptr) {
        source_file[0] = 0;
        _tcscat(source_file, directory);
        _tcscat(source_file,_T("/"));
        _tcscat(source_file,d->d_name);
        
        _tstat( source_file, &stat_buf);

        if (S_ISDIR(stat_buf.st_mode) == 0) {
            _cc_unlink(source_file);
        } else {
            remove_memory_directory(source_file);
        }
    }
    closedir(dpath);
}

_CC_API_PUBLIC(void) _cc_enable_tracked_memory(void) {
    if (_cc_atomic32_cas(&g.ref, 0, 1)) {

        tchar_t path[_CC_MAX_PATH_];

        _cc_get_executable_path(path,_CC_MAX_PATH_);
        _sntprintf(g.current_process_path, _cc_countof(g.current_process_path), _T("%s/memory"), path);
        
        _cc_create_directory(g.current_process_path, true);

        remove_memory_directory(g.current_process_path);
    }
}

_CC_API_PRIVATE(void) _write_timestamp(FILE *wfp) {
    time_t now_time = time(nullptr);
    struct tm *t = localtime(&now_time);

    _ftprintf(wfp, _T("%4d-%02d-%02d %02d:%02d:%02d\n"), t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
            t->tm_min, t->tm_sec);
}

_CC_API_PRIVATE(void) __cc_tracked_memory_unlink(pvoid_t ptr) {
    tchar_t _memory_file[_CC_MAX_PATH_];
    _sntprintf(_memory_file, _cc_countof(_memory_file), _T("%s/%p.mem"), g.current_process_path,ptr);
    _cc_unlink(_memory_file);
}

_CC_API_PRIVATE(void) __cc_tracked_memory(pvoid_t ptr, size_t size, const tchar_t *msg) {
    tchar_t _memory_file[_CC_MAX_PATH_];
    FILE *fp;
    _sntprintf(_memory_file, _cc_countof(_memory_file), _T("%s/%p.mem"), g.current_process_path,ptr);
    fp = _tfopen(_memory_file, _T("a"));
    if (fp == nullptr) {
        return;
    }
    
    _write_timestamp(fp);
    _ftprintf(fp, _T("%p %s %lld\n"), ptr, msg, (int64_t)size);
	_cc_dump_stack_trace(fp, 3);
    fclose(fp);
}
#endif

/**/
_CC_API_PRIVATE(pvoid_t) __cc_check_memory(pvoid_t ptr, size_t size, const tchar_t *msg) {
    if (_cc_unlikely(nullptr == ptr)) {
        _cc_logger_error(_T("%s: Out of memory trying to allocate %zu bytes"), msg, size);
        _cc_abort();
    }
#ifdef _CC_ENABLE_MEMORY_TRACKED_
    if (g.ref == 1) {
        __cc_tracked_memory(ptr, size, msg);
    }
#endif
    return ptr;
}

/**/
_CC_API_PUBLIC(pvoid_t) _cc_malloc(size_t n) {
    return __cc_check_memory(malloc(n), n, _T("_cc_malloc"));
}

/**/
_CC_API_PUBLIC(pvoid_t) _cc_calloc(size_t c, size_t n) {
    return __cc_check_memory(calloc(c, n), c * n, _T("_cc_calloc"));
}

/**/
_CC_API_PUBLIC(pvoid_t) _cc_realloc(pvoid_t d, size_t n) {
    if (_cc_unlikely(n <= 0)) {
        _cc_free(d);
        return nullptr;
    }

    if (_cc_unlikely(d == nullptr)) {
        return _cc_malloc(n);
    }
    
#ifdef _CC_ENABLE_MEMORY_TRACKED_
    __cc_tracked_memory_unlink(d);
#endif

    return __cc_check_memory(realloc(d, n), n, _T("realloc"));
}

/**/
_CC_API_PUBLIC(void) _cc_free(pvoid_t p) {
    _cc_assert(p != nullptr);
#ifdef _CC_ENABLE_MEMORY_TRACKED_
    __cc_tracked_memory_unlink(p);
#endif
    free(p);
}

/**/
_CC_API_PUBLIC(wchar_t*) _cc_strdupW(const wchar_t *str) {
    return _cc_strndupW(str, wcslen(str));
}

/**/
_CC_API_PUBLIC(char_t*) _cc_strdupA(const char_t *str) {
    return _cc_strndupA(str, strlen(str));
}

/**/
_CC_API_PUBLIC(wchar_t*) _cc_strndupW(const wchar_t *str, size_t str_len) {
    wchar_t *req_str;

    if (_cc_unlikely(str_len <= 0)) {
        return nullptr;
    }

    req_str = (wchar_t *)_cc_malloc(sizeof(wchar_t) * (str_len + 1));
    if (_cc_likely(req_str)) {
        memcpy(req_str, str, str_len * sizeof(wchar_t));
        req_str[str_len] = 0;
    }

    return req_str;
}

/**/
_CC_API_PUBLIC(char_t*) _cc_strndupA(const char_t *str, size_t str_len) {
    char_t *req_str;

    if (_cc_unlikely(str_len <= 0)) {
        return nullptr;
    }

    req_str = (char_t *)_cc_malloc(sizeof(char_t) * (str_len + 1));
    if (_cc_likely(req_str)) {
        memcpy(req_str, str, str_len * sizeof(char_t));
        req_str[str_len] = 0;
    }

    return req_str;
}
