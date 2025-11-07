#ifndef _C_CC_IPLOCATOR_H_INCLUDED_
#define _C_CC_IPLOCATOR_H_INCLUDED_

#include "types.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cc_ip_locator _cc_ip_locator_t;

struct _cc_ip_locator {
    FILE* fp;
    uint32_t first_index;
    uint32_t last_index;
    uint32_t cur_start_ip;
    uint32_t cur_end_ip;
    uint32_t cur_end_ip_offset;
    uint32_t record_count;

    int32_t (*query)(_cc_ip_locator_t* f, uint32_t ip, byte_t* addr, int32_t len);
    int32_t (*get_version)(_cc_ip_locator_t* f, byte_t* version, int32_t len);
    void (*quit)(_cc_ip_locator_t* f);
};

_CC_API_PUBLIC(bool_t) _cc_init_ip_locator(_cc_ip_locator_t*, const char_t*);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_CRC_H_INCLUDED_*/
