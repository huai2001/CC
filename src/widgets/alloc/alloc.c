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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "alloc_tracked.c.h"

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
/**/
static byte_t* __cc_abort_out_of_memory(pvoid_t ptr, size_t size, const tchar_t *msg) {
    if (_cc_unlikely(NULL == ptr)) {
        _cc_logger_error(_T("%s: Out of memory trying to allocate %zu bytes"), msg, size);
        _cc_abort();
    }
    return (byte_t*)ptr;
}

/**/
pvoid_t _cc_mem_calloc(size_t data_count, size_t data_size, const tchar_t *file, const int line, const tchar_t *func) {
    byte_t* dat = __cc_abort_out_of_memory(malloc(data_count * data_size + sizeof(_ccmem_element_link_t)), data_size * data_count, _T("_cc_mem_calloc"));
    bzero(dat, data_count * data_size + sizeof(_ccmem_element_link_t));
    
    _ccmem_tracked_insert(dat, data_size * data_count, file, func, line, _CC_MEM_CALLOC);

    return (dat + sizeof(_ccmem_element_link_t));
}

/**/
pvoid_t _cc_mem_malloc(size_t data_size, const tchar_t *file, const int line, const tchar_t *func) {
    byte_t* dat = __cc_abort_out_of_memory(malloc(data_size + sizeof(_ccmem_element_link_t)), data_size, _T("_cc_mem_malloc"));

    _ccmem_tracked_insert(dat, data_size, file, func, line, _CC_MEM_MALLOC);

    return (dat + sizeof(_ccmem_element_link_t));
}

/**/
pvoid_t _cc_mem_realloc(void *dat, size_t data_size, const tchar_t *file, const int line, const tchar_t *func) {
    byte_t* redat;
    

    if (_cc_unlikely(dat == NULL)) {
        dat = _cc_mem_malloc(data_size, file, line, func);
        return dat;
    }

    redat = ((byte_t*)dat - sizeof(_ccmem_element_link_t));
    _ccmem_tracked_remove(redat);

    if (_cc_unlikely(data_size == 0)) {
        _cc_mem_free(dat, file, line, func);
        return NULL;
    }

    redat = __cc_abort_out_of_memory(realloc(redat, data_size + sizeof(_ccmem_element_link_t)), data_size, _T("_cc_mem_realloc"));

    _ccmem_tracked_insert(redat, data_size, file, func, line, _CC_MEM_REALLOC);

    return (redat + sizeof(_ccmem_element_link_t));
}

/**/
void _cc_mem_free(void *dat, const tchar_t *file, const int line, const tchar_t *func) {
    byte_t *r;
    if (_cc_unlikely(dat == NULL)) {
        return;
    }
    
    r = ((byte_t*)dat - sizeof(_ccmem_element_link_t));
    _ccmem_tracked_remove(r);
    free(r);
}

/**/
wchar_t *_cc_mem_strdupW(const wchar_t *str, const tchar_t *file, const int line, const tchar_t *func) {
    return _cc_mem_strndupW(str, wcslen(str), file, line, func);
}

/**/
char_t *_cc_mem_strdupA(const char_t *str, const tchar_t *file, const int line, const tchar_t *func) {
    return _cc_mem_strndupA(str, strlen(str), file, line, func);
}

/**/
wchar_t *_cc_mem_strndupW(const wchar_t *str, size_t str_len, const tchar_t *file, const int line,
                          const tchar_t *func) {
    wchar_t *req_str;
    if (_cc_unlikely(str_len <= 0)) {
        return NULL;
    }

    req_str = (wchar_t *)_cc_mem_malloc(sizeof(wchar_t) * (str_len + 1), file, line, func);
    if (_cc_unlikely(req_str == NULL)) {
        return NULL;
    }

    wcsncpy(req_str, str, str_len);
    req_str[str_len] = 0;

    return req_str;
}

/**/
char_t *_cc_mem_strndupA(const char_t *str, size_t str_len, const tchar_t *file, const int line, const tchar_t *func) {
    char_t *req_str;
    if (_cc_unlikely(str_len <= 0)) {
        return NULL;
    }

    req_str = (char_t *)_cc_mem_malloc(sizeof(char_t) * (str_len + 1), file, line, func);
    if (_cc_unlikely(req_str == NULL)) {
        return NULL;
    }

    strncpy(req_str, str, str_len);
    req_str[str_len] = 0;

    return req_str;
}
#endif
