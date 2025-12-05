#include <libcc/alloc.h>

#ifndef _CC_USE_DEBUG_MALLOC_

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

/**/
_CC_API_PRIVATE(pvoid_t) _out_of_memory_abort(pvoid_t ptr, size_t size, const tchar_t *tp) {
    if (_cc_unlikely(nullptr == ptr)) {
        _cc_logger_error(_T("%s: Out of memory trying to allocate %zu bytes"), tp, size);
        _cc_abort();
    }
    return ptr;
}

/**/
_CC_API_PUBLIC(pvoid_t) _cc_malloc(size_t n) {
    return _out_of_memory_abort(malloc(n), n, _T("_cc_malloc"));
}

/**/
_CC_API_PUBLIC(pvoid_t) _cc_calloc(size_t c, size_t n) {
    return _out_of_memory_abort(calloc(c, n), c * n, _T("_cc_calloc"));
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
    return _out_of_memory_abort(realloc(d, n), n, _T("_cc_realloc"));
}

/**/
_CC_API_PUBLIC(void) _cc_free(pvoid_t p) {
    _cc_assert(p != nullptr);
    free(p);
}
#else
#include "debug.malloc.c"
#include "debug.tracked.c"
#endif