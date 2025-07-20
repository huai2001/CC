/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
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

static tchar_t *mem_types[4] = {"unknown","_cc_malloc","_cc_calloc","_cc_realloc"};

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

/*
 * Remove a pointer to the rbtree with some key
 */
_CC_API_PUBLIC(void) __cc_tracked_memory_unlink(uintptr_t ptr) {

}

/*
 * Add a pointer to the rbtree with some key
 */
_CC_API_PUBLIC(void) __cc_tracked_memory(uintptr_t ptr, size_t size, const int _type) {

}

/**/
_CC_API_PRIVATE(pvoid_t) __cc_check_memory(pvoid_t ptr, size_t size, const int _type) {
    if (_cc_unlikely(nullptr == ptr)) {
        _cc_logger_error(_T("%s: Out of memory trying to allocate %zu bytes"), mem_types[_type], size);
        _cc_abort();
    }
#ifdef _CC_ENABLE_MEMORY_TRACKED_
    __cc_tracked_memory((uintptr_t)ptr, size, _type);
#endif
    return ptr;
}

/**/
_CC_API_PUBLIC(pvoid_t) _cc_malloc(size_t n) {
    return __cc_check_memory(malloc(n), n, _CC_MEM_MALLOC_);
}

/**/
_CC_API_PUBLIC(pvoid_t) _cc_calloc(size_t c, size_t n) {
    return __cc_check_memory(calloc(c, n), c * n, _CC_MEM_CALLOC_);
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
    __cc_tracked_memory_unlink((uintptr_t)d);
#endif

    return __cc_check_memory(realloc(d, n), n, _CC_MEM_REALLOC_);
}

/**/
_CC_API_PUBLIC(void) _cc_free(pvoid_t p) {
    _cc_assert(p != nullptr);
#ifdef _CC_ENABLE_MEMORY_TRACKED_
    __cc_tracked_memory_unlink((uintptr_t)p);
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
