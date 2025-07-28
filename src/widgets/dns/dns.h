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
#ifndef _C_CC_DNS_H_INCLUDED_
#define _C_CC_DNS_H_INCLUDED_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libcc/alloc.h>
#include <libcc/generic.h>
#include <libcc/thread.h>

#include <libcc/widgets/event.h>
#include <libcc/widgets/dns.h>

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

uint8_t *dns_read_name(uint8_t *, uint8_t *, int *);
int _build_question(uint8_t *buf, const char_t *host, int type);
#pragma pack()
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_DNS_H_INCLUDED_ */