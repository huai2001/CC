#ifndef _C_CC_DNS_H_INCLUDED_
#define _C_CC_DNS_H_INCLUDED_

#include <libcc/dns.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define DNS_SERVERS_MCOUNT 10
#define DNS_BUFFER_SIZE 65536

/* DNS Query classes */
#define DNS_CLASS_INET 1 /* Inet address */
#define DNS_CLASS_CSNET 2
#define DNS_CLASS_CHAOS 3 /* Chaos system */
#define DNS_CLASS_HESIOD 4
#define DNS_CLASS_ANY 255

/* DNS Query reply codes */
#define DNS_R_NO_ERROR 0        /* No error */
#define DNS_R_FORMAT_ERROR 1    /* Format error with query */
#define DNS_R_SERVER_FAILURE 2  /* Server failure */
#define DNS_R_NAME_ERROR 3      /* Name error */
#define DNS_R_NOT_IMPLEMENTED 4 /* Query type not supported */
#define DNS_R_REFUSED 5         /* Server refused to handle query */

// Constant sized fields of the resource record structure

#pragma pack(1)

// Types of DNS resource records :)

// Constant sized fields of query structure
struct QUESTION {
    uint16_t type;
    uint16_t classes;
};

struct R_DATA {
    uint16_t type;
    uint16_t classes;
    uint32_t ttl;
    uint16_t length;
};

#pragma pack()
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_DNS_H_INCLUDED_ */
