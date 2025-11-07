#ifndef _C_CC_MEMORY_H_INCLUDED_
#define _C_CC_MEMORY_H_INCLUDED_

#include "logger.h"
#include "list.h"
#include "string.h"

#if defined(_CC_DEBUG_)
#define _CC_USE_DEBUG_MALLOC_ 1
#endif

#ifdef _CC_USE_DEBUG_MALLOC_

#if defined(__CC_USE_TCMALLOC__)
    #include <google/tcmalloc.h>
    #if (TC_VERSION_MAJOR == 1 && TC_VERSION_MINOR >= 6) || (TC_VERSION_MAJOR > 1)
        #define _cc_malloc_size(p) tc_malloc_size(p)
    #else
        #error "Newer version of tcmalloc required"
    #endif
#elif defined(__CC_USE_JEMALLOC__)
    #include <jemalloc/jemalloc.h>
    #if (JEMALLOC_VERSION_MAJOR == 2 && JEMALLOC_VERSION_MINOR >= 1) || (JEMALLOC_VERSION_MAJOR > 2)
        #define _cc_malloc_size(p) je_malloc_usable_size(p)
    #else
        #error "Newer version of jemalloc required"
    #endif
#elif defined(__CC_APPLE__)
    #include <malloc/malloc.h>
    #define _cc_malloc_size(p) malloc_size(p)
#endif

#endif /* _CC_USE_DEBUG_MALLOC_ */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _CC_USE_DEBUG_MALLOC_

/**/
_CC_API_PUBLIC(pvoid_t) _cc_debug_malloc(size_t n, const tchar_t *file, const int line);
/**/
_CC_API_PUBLIC(pvoid_t) _cc_debug_calloc(size_t c, size_t n, const tchar_t *file, const int line);
/**/
_CC_API_PUBLIC(pvoid_t) _cc_debug_realloc(pvoid_t d, size_t n, const tchar_t *file, const int line);
/**/
_CC_API_PUBLIC(void) _cc_debug_free(pvoid_t p);

/**/
#define _cc_malloc(N) _cc_debug_malloc((N), _CC_FILE_, _CC_LINE_)
/**/
#define _cc_calloc(C, N) _cc_debug_calloc((C), (N), _CC_FILE_, _CC_LINE_)
/**/
#define _cc_realloc(P, N) _cc_debug_realloc((P),(N), _CC_FILE_, _CC_LINE_)
/**/
#define _cc_free(P) _cc_debug_free(P)

#else

/**/
_CC_API_PUBLIC(pvoid_t) _cc_malloc(size_t);
/**/
_CC_API_PUBLIC(pvoid_t) _cc_calloc(size_t, size_t);
/**/
_CC_API_PUBLIC(pvoid_t) _cc_realloc(pvoid_t, size_t);
/**/
_CC_API_PUBLIC(void) _cc_free(pvoid_t);

#endif /* _CC_USE_DEBUG_MALLOC_ */

/**/
#define _cc_if_free(d) do {\
    if ((d)) {\
        _cc_free((d));\
        (d) = nullptr;\
    }\
} while (0)

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif 

#endif/*_C_CC_MEMORY_H_INCLUDED_*/
