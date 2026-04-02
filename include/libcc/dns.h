
#ifndef _C_CC_LIBDNS_H_INCLUDED_
#define _C_CC_LIBDNS_H_INCLUDED_

#include "rbtree.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// DNS header structure
typedef struct _cc_dns_header {
    uint16_t ident;     /* identification number */
    uint16_t flags;     /* Flags word */
    uint16_t quests;    /* # of questions */
    uint16_t answer;    /* # of answer RRs */
    uint16_t author;    /* # of authority RRs */
    uint16_t addition;  /* # of additional RRs */
} _cc_dns_header_t;

typedef struct _cc_dns_question {
    char_t* name;
    uint16_t name_length;
    uint16_t type;
    uint16_t classes;
} _cc_dns_question_t;

typedef struct _cc_dns_record {
    char_t* name;
    uint16_t name_length;
    uint16_t type;
    uint16_t classes;
    uint16_t length;
    uint32_t ttl;
    byte_t* rdata;
} _cc_dns_record_t;

typedef struct _cc_dns {
    uint16_t error_code;
    _cc_dns_header_t header;
    _cc_dns_record_t *answers;
    _cc_dns_record_t *authorities;
    _cc_dns_record_t *additional;

    _cc_rb_t lnk;
} _cc_dns_t;

/* DNS Query types */
#define _CC_DNS_T_A_              1        /* IPv4 Address record */
#define _CC_DNS_T_NS_             2        /* Name server */
#define _CC_DNS_T_CNAME_          5        /* Canonical name (IP-hostname) */
#define _CC_DNS_T_SOA_            6        /* Start of zone-of-authority */
#define _CC_DNS_T_WKS_            11       /* Well Known Service description */
#define _CC_DNS_T_PTR_            12       /* Pointer record */
#define _CC_DNS_T_HINFO_          13       /* Host information */
#define _CC_DNS_T_MINFO_          14       /* Mail{box,list} information */
#define _CC_DNS_T_MX_             15       /* Mail exchange record */
#define _CC_DNS_T_TXT_            16       /* Text strings */
#define _CC_DNS_T_RP_             17       /* Responsible Person */
#define _CC_DNS_T_AFSDB_          18       /* AFS cell database server */
#define _CC_DNS_T_RT_             21       /* Route-through record */
#define _CC_DNS_T_AAAA_           28       /* IPv6 address record (RFC-1886) */
#define _CC_DNS_T_LOC_            29       /* Location information (RFC-1876) */
#define _CC_DNS_T_SRV_            33       /* Service information (RFC-2782) */
#define _CC_DNS_T_A6_             38       /* IPv6 A6 address record (RFC-2874) */
/* the following appear in query records only! */
#define _CC_DNS_T_AXFR_           252      /* Zone transfer */
#define _CC_DNS_T_ANY_            255        /* All records */


#define _CC_DNS_OP_QUERY_         0
#define _CC_DNS_OP_IQUERY_        1
#define _CC_DNS_OP_STATUS_        2
#define _CC_DNS_OP_NOTIFY_        4
#define _CC_DNS_OP_UPDATE_        5

/* 
 * #define's for Libdns error codes
 */
#define _CC_DNS_ERR_FORMAT_ERROR_            1  /* Format error of request string */
#define _CC_DNS_ERR_TOKEN_TOO_LONG_          2  /* A token was longer than 63 */
#define _CC_DNS_ERR_SEE_ERRNO_               3  /* Look at value of errno */
#define _CC_DNS_ERR_NULL_PARAM_              4  /* A NULL parameter was given */
#define _CC_DNS_ERR_INVALID_QUERY_           5  /* The given _cc_dns_t is invalid */
#define _CC_DNS_ERR_PARAM_ERROR_             6  /* Function given invalid param */
#define _CC_DNS_ERR_QUERY_TOO_LONG_          7  /* The whole query is too big
                                                 * (> 512 for UDP) */
#define _CC_DNS_ERR_REPLY_TRUNCATED_         8  /* The response was truncated */
#define _CC_DNS_ERR_NO_SUCH_NAME_            9  /* requested name doesn't exist */
#define _CC_DNS_ERR_QUERY_REFUSED_          10  /* Server refused to handle query */
#define _CC_DNS_ERR_BAD_FORMAT_             11  /* sent query was badly formatted */
#define _CC_DNS_ERR_SERVER_FAILURE_         12  /* Server failure */
#define _CC_DNS_ERR_NOT_IMPLEMENTED_        13  /* Query type not supported */
#define _CC_DNS_ERR_TIMEDOUT_               14  /* Timed out waiting for reply */
#define _CC_DNS_ERR_ENOMEM_                 15  /* Out of memory*/

/**/
_CC_API_PUBLIC(int) _cc_dns_lookup(_cc_dns_t* dns, const char_t* host, int type);
/**/
_CC_API_PUBLIC(void) _cc_dns_free(_cc_dns_t* dns);
/**/
_CC_API_PUBLIC(void) _cc_dns_servers(const tchar_t* servers[], int count);
/**/
_CC_API_PUBLIC(bool_t) _cc_dns_listen(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_LIBDNS_H_INCLUDED_ */
