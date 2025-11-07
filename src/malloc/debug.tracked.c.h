#ifndef _C_CC_MEMORY_TRACKED_C_H_INCLUDED_
#define _C_CC_MEMORY_TRACKED_C_H_INCLUDED_

#include <libcc/alloc.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _CC_USE_DEBUG_MALLOC_

#define _CC_DEBUG_MALLOC_   0x01
#define _CC_DEBUG_CALLOC_   0x02
#define _CC_DEBUG_REALLOC_  0x03

#pragma pack(push, 1)
typedef struct _cc_debug_alloc {
    /*
    ** 2020-01-01 00:00:00
    ** (create_time + 1577808000)
    */
    uint32_t type;
    uint32_t create_time;
    uint32_t line;
    size_t size;
    tchar_t *file;
    _cc_list_iterator_t lnk;
} _cc_debug_alloc_t;

#pragma pack(pop)
/**/
pvoid_t _debug_alloc_link(const pvoid_t data, size_t n, const tchar_t *file_name, const int32_t line, byte_t m_type);
/**/
pvoid_t _debug_alloc_unlink(const pvoid_t data);

/**/
void _detach_debug_taracked(void);
/**/
void _attach_debug_taracked(void);

#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_MEMORY_TRACKED_C_H_INCLUDED_*/