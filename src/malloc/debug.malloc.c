#include <libcc/alloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "debug.tracked.c.h"

#ifdef _CC_USE_DEBUG_MALLOC_

/**/
_CC_API_PRIVATE(pvoid_t) _out_of_memory_abort(pvoid_t ptr, size_t size, byte_t m_type, const tchar_t *file, const int line) {
    if (_cc_unlikely(NULL == ptr)) {
        printf(_T("%s(%d):Out of memory trying to allocate %zu bytes"), file, line, size);
        abort();
    }
    return _debug_alloc_link(ptr, size, file, line, m_type);
}

/**/
_CC_API_PUBLIC(pvoid_t) _cc_debug_malloc(size_t n, const tchar_t *file, const int line) {
    return _out_of_memory_abort(malloc(n + sizeof(_cc_debug_alloc_t)), n, _CC_DEBUG_MALLOC_, file, line);
}

/**/
_CC_API_PUBLIC(pvoid_t) _cc_debug_calloc(size_t c, size_t n, const tchar_t *file, const int line) {
    pvoid_t ptr;
    n = c * n;
    ptr = _out_of_memory_abort(malloc(n + sizeof(_cc_debug_alloc_t)), n, _CC_DEBUG_CALLOC_, file, line);
    bzero(ptr, n);
    return ptr;
}

/**/
_CC_API_PUBLIC(pvoid_t) _cc_debug_realloc(pvoid_t d, size_t n, const tchar_t *file, const int line) {
    if (_cc_unlikely(n <= 0)) {
        _cc_debug_free(d);
        return NULL;
    }

    if (_cc_unlikely(d == NULL)) {
        return _cc_debug_malloc(n, file, line);
    }
    return _out_of_memory_abort(realloc(_debug_alloc_unlink(d), n + sizeof(_cc_debug_alloc_t)), n, _CC_DEBUG_REALLOC_, file, line);
}

/**/
_CC_API_PUBLIC(void) _cc_debug_free(pvoid_t p) {
    _cc_assert(p != NULL);
    free(_debug_alloc_unlink(p));
}
#endif