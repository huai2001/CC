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
#include <cc/crc.h>
#include <cc/list.h>
#include <cc/mutex.h>
#include <cc/rbtree.h>
#include <cc/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _CC_ENABLE_MEMORY_TRACKED_

#include "alloc_tracked.c.h"

#define INITIAL_SIZE (256)
#define MAX_CHAIN_LENGTH (8)

//2020-01-01 00:00:00
#define _START_TIMESTAMP_   1577808000L

/* We need to keep keys and values */
struct _ccmem_element {
    byte_t type;
    /*
    ** 2020-01-01 00:00:00
    ** (create_time + 1577808000)
    */
    uint32_t create_time;
    uint32_t line;
    size_t size;
    tchar_t *func;
    tchar_t *file;
};

/* A hashmap has some maximum size and current size,
 * as well as the data to hold. */
static _cc_list_iterator_t tracked_list;
static _cc_mutex_t *mem_lock = NULL;
static int32_t num_alloc = 0;
static int32_t num_free = 0;
static int32_t num_realloc = 0; /* counts only `real' reallocations
                             (i.e., an existing buffer will be resized
                             to a value larger than zero */
static int32_t fail_alloc = 0;
static int32_t fail_realloc = 0;

/*
 * Add a pointer to the hashmap with some key
 */
_CC_API_PRIVATE(bool_t) _insert(const pvoid_t data, size_t data_size, const tchar_t *file_name, const tchar_t *func_name,
                              const int32_t file_line, byte_t mem_type) {
    _ccmem_element_link_t *element_link = (_ccmem_element_link_t*)data;
    _ccmem_element_t *element = (_ccmem_element_t *)malloc(sizeof(_ccmem_element_t));

    /* Set the data */
    if (element == NULL) {
        return false;
    }
    
    element->type = mem_type;
    element->size = data_size;
    element->line = file_line;
    element->create_time = (uint32_t)(time(NULL) - _START_TIMESTAMP_);

    element->file = _tcsdup(file_name);
    element->func = _tcsdup(func_name);
    
    element_link->element = element;
    
    _cc_mutex_lock(mem_lock);
    _cc_list_iterator_push(&tracked_list, &element_link->lnk);
    _cc_mutex_unlock(mem_lock);
    return true;
}

/*
 * Add a pointer to the hashmap with some key
 */
bool_t _ccmem_tracked_insert(const pvoid_t data, size_t data_size, const tchar_t *file_name, const tchar_t *func_name,
                             const int32_t file_line, byte_t mem_type) {
    if (mem_lock == NULL) {
        return false;
    }

    if (data == NULL) {
        switch (mem_type) {
        case _CC_MEM_CALLOC:
        case _CC_MEM_MALLOC:
            fail_alloc++;
            break;
        case _CC_MEM_REALLOC:
            fail_realloc++;
            break;
        }
    } else {
        if (_insert(data, data_size, file_name, func_name, file_line, mem_type)) {
            switch (mem_type) {
            case _CC_MEM_CALLOC:
            case _CC_MEM_MALLOC:
                num_alloc++;
                break;
            case _CC_MEM_REALLOC:
                num_realloc++;
                break;
            }
        }
    }

    return true;
}

/*
 * Remove an element with that key from the map
 */
pvoid_t _ccmem_tracked_remove(const pvoid_t data) {
    _ccmem_element_link_t *element_link = (_ccmem_element_link_t*)data;
    _ccmem_element_t *element = element_link->element;

    if (mem_lock == NULL || element == NULL) {
        return data;
    }

    num_free++;
    
    element = element_link->element;
    
    _cc_mutex_lock(mem_lock);
    _cc_list_iterator_remove(&element_link->lnk);
    _cc_mutex_unlock(mem_lock);
    
    free(element->file);
    free(element->func);
    free(element);
    return data;
}

/**/
void _cc_install_memory_tracked(void) {
    if (mem_lock) {
        return;
    }

    num_alloc = num_realloc = num_free = 0;
    fail_alloc = fail_realloc = 0;
    _cc_list_iterator_cleanup(&tracked_list);
    
    mem_lock = _cc_create_mutex();
}

_CC_API_PRIVATE(void) _print_timestamp(FILE *wfp, uint32_t create_time) {
    time_t now_time = create_time + _START_TIMESTAMP_;
    struct tm *t = localtime(&now_time);

    fprintf(wfp, "[%4d-%02d-%02d %02d:%02d:%02d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,
            t->tm_min, t->tm_sec);
}

/**/
void _cc_uninstall_memory_tracked(void) {
    int32_t num_leaked;
    size_t tot_leaked;
    int32_t len = 0;
    FILE *wfp = NULL;
    tchar_t cwd[_CC_MAX_PATH_];
    static tchar_t *mem_types[4] = {"","_cc_malloc","_cc_calloc","_cc_realloc"};
    
    _ccmem_element_t *m = NULL;
    _ccmem_element_link_t *mlink = NULL;
    if (mem_lock == NULL) {
        return;
    }
    _cc_destroy_mutex(&mem_lock);

    len = _cc_get_current_file(cwd, _CC_MAX_PATH_);
    if (len) {
        _tcscat(cwd + len - 1, "-alloc.log");
        len += 1;
        wfp = _tfopen(cwd, _T("wb"));
    }

    if (wfp == NULL) {
        wfp = stderr;
        len = 0;
    }
    
    num_leaked = 0;
    tot_leaked = 0;


    _ftprintf(wfp, _T("%d memory allocations, of which %d failed\r\n"), num_alloc, fail_alloc);

    _ftprintf(wfp, _T("%d memory reallocations, of which %d failed\r\n"), num_realloc, fail_realloc);

    _ftprintf(wfp, _T("%d memory frees, of which %d failed\r\n"), num_free, num_leaked);

    if (num_leaked > 0) {
        _ftprintf(wfp, _T("There are %d leaked memory blocks, totalizing %ld bytes\r\n"), num_leaked, tot_leaked);
    } else {
        _ftprintf(wfp, _T("No memory leaks !\r\n"));
    }

    _cc_list_iterator_for_each(v, &tracked_list, {
        mlink = _cc_upcast(v, _ccmem_element_link_t, lnk);
        m = mlink->element;
        _print_timestamp(wfp, m->create_time);
        _ftprintf(wfp, _T("%s (base: $%p, size: %ld) file:%s(%d) _%s\r\n"), mem_types[m->type], mlink + 1, m->size, m->file,
                  m->line, m->func);

        num_leaked++;
        tot_leaked += m->size;
    });

    if (len) {
        fclose(wfp);
    }

    num_alloc = num_realloc = num_free = 0;
    fail_alloc = fail_realloc = 0;

    return;
}
#endif
