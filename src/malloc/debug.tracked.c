#include <libcc/atomic.h>
#include <libcc/list.h>

#ifdef __CC_ANDROID__
#include <libcc/os/android.h>
#endif

#include "debug.tracked.c.h"

#ifdef _CC_USE_DEBUG_MALLOC_

//2020-01-01 00:00:00
#define _START_TIMESTAMP_   1577808000L

static _cc_list_iterator_t tracked_list;
static _cc_atomic_lock_t debug_lock = 0;
static int32_t number_of_alloc = 0;
static int32_t number_of_freed = 0;

pvoid_t _debug_alloc_link(const pvoid_t data, size_t n, const tchar_t *file_name, const int32_t line, byte_t m_type) {
    _cc_debug_alloc_t *debug = (_cc_debug_alloc_t*)data;

    /* Set the data */
    debug->type = m_type;
    debug->size = n;
    debug->line = line;
    debug->create_time = (uint32_t)(time(NULL) - _START_TIMESTAMP_);
    if (file_name) {
        tchar_t *p = _tcsrchr(file_name, _CC_SLASH_C_);
        if (p) {
            debug->file = _tcsdup(p + 1);
        } else {
            debug->file = _tcsdup(file_name);
        }
    } else {
        debug->file = NULL;
    }

    _cc_spin_lock(&debug_lock);
    number_of_alloc++;
    _cc_list_iterator_push(&tracked_list, &debug->lnk);
    _cc_unlock(&debug_lock);

    return (debug + 1);
}

/**/
pvoid_t _debug_alloc_unlink(const pvoid_t data) {
    _cc_debug_alloc_t *debug = (_cc_debug_alloc_t*)((byte_t*)data - sizeof(_cc_debug_alloc_t));
    if (debug->file) {
        free(debug->file);
    }

    _cc_spin_lock(&debug_lock);
    number_of_freed++;
    _cc_list_iterator_remove(&debug->lnk);
    _cc_unlock(&debug_lock);

    return debug;
}

/**/
void _attach_debug_taracked(void) {
    number_of_alloc = number_of_freed = 0;
    _cc_list_iterator_cleanup(&tracked_list);
    _cc_lock_init(&debug_lock);
}

#ifndef __CC_ANDROID__
/**/
_CC_API_PRIVATE(void) _dump_timestamp(time_t debug_time) {
    debug_time += _START_TIMESTAMP_;
    struct tm *t = localtime(&debug_time);

    _tprintf(_T("[%4d-%02d-%02d %02d:%02d:%02d]"), t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour,t->tm_min, t->tm_sec);
}
#endif

/**/
void _detach_debug_taracked(void) {
    int32_t num_leaked;
    size_t tot_leaked;
    static tchar_t *mem_types[4] = {"","_cc_malloc","_cc_calloc","_cc_realloc"};

    num_leaked = 0;
    tot_leaked = 0;

#ifdef __CC_ANDROID__
    __android_log_print(ANDROID_LOG_DEBUG, _CC_ANDROID_TAG_, _T("%d memory allocations, of which %d freed\r\n"), number_of_alloc, number_of_freed);
    _cc_list_iterator_for_each(v, &tracked_list, {
        _cc_debug_alloc_t *debug = _cc_upcast(v, _cc_debug_alloc_t, lnk);
        __android_log_print(ANDROID_LOG_DEBUG, _CC_ANDROID_TAG_, _T("%s (base: $%p, size: %ld) located at:%s(%d)\r\n"), mem_types[debug->type], debug, debug->size, debug->file,
                  debug->line);

        num_leaked++;
        tot_leaked += debug->size;
    });

    if (num_leaked > 0) {
        __android_log_print(ANDROID_LOG_DEBUG, _CC_ANDROID_TAG_, _T("There are %d leaked memory blocks, totalizing %ld bytes\r\n"), num_leaked, tot_leaked);
    } else {
        __android_log_print(ANDROID_LOG_DEBUG, _CC_ANDROID_TAG_, _T("No memory leaks !\r\n"));
    }
    number_of_alloc = number_of_freed = 0;
#else
    _tprintf(_T("%d memory allocations, of which %d freed\r\n"), number_of_alloc, number_of_freed);
    _cc_list_iterator_for_each(v, &tracked_list, {
        _cc_debug_alloc_t *debug = _cc_upcast(v, _cc_debug_alloc_t, lnk);
        _dump_timestamp(debug->create_time);
        _tprintf(_T("%s (base: $%p, size: %ld) located at:%s(%d)\r\n"), mem_types[debug->type], debug, debug->size, debug->file, debug->line);

        num_leaked++;
        tot_leaked += debug->size;
    });

    if (num_leaked > 0) {
        _tprintf(_T("There are %d leaked memory blocks, totalizing %ld bytes\r\n"), num_leaked, tot_leaked);
    } else {
        _tprintf(_T("No memory leaks !\r\n"));
    }
    number_of_alloc = number_of_freed = 0;
#endif
}
#endif