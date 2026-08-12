#ifndef _C_CC_RWBUF_H_INCLUDED_
#define _C_CC_RWBUF_H_INCLUDED_

#include "types.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

//Adopt the TL (Type-Length) binary serialization format

typedef struct _cc_rbuf {
    uint32_t off;
    uint32_t limit;
    const byte_t* bytes;
} _cc_rbuf_t;

typedef struct _cc_wbuf {
    uint32_t off;
    uint32_t limit;
    byte_t* bytes;
} _cc_wbuf_t;

/** {@ */
/**/
void _cc_wbuf_init(_cc_wbuf_t *ref, byte_t *bytes, uint32_t length);
/**/
bool_t _cc_wbuf_int8(_cc_wbuf_t *ref, int8_t x);
/**/
bool_t _cc_wbuf_int16(_cc_wbuf_t *ref, int16_t x);
/**/
bool_t _cc_wbuf_int32(_cc_wbuf_t *ref, int32_t x);
/**/
bool_t _cc_wbuf_int64(_cc_wbuf_t *ref, int64_t x);
/**/
bool_t _cc_wbuf_double(_cc_wbuf_t *ref, double v);
/**/
bool_t _cc_wbuf_string(_cc_wbuf_t *ref, const tchar_t* value, int32_t length);
/**/
bool_t _cc_wbuf_bytes(_cc_wbuf_t *ref, const byte_t* value, int32_t length);

/**/
void _cc_rbuf_init(_cc_rbuf_t *ref, const byte_t *bytes, uint32_t length);
/**/
int8_t _cc_rbuf_int8(_cc_rbuf_t *ref);
/**/
int16_t _cc_rbuf_int16(_cc_rbuf_t *ref);
/**/
int32_t _cc_rbuf_int32(_cc_rbuf_t *ref);
/**/
int64_t _cc_rbuf_int64(_cc_rbuf_t *ref);
/**/
double _cc_rbuf_double(_cc_wbuf_t *ref);
/**/
int32_t _cc_rbuf_string(_cc_rbuf_t *ref, tchar_t* value, int32_t length);
/**/
int32_t _cc_rbuf_bytes(_cc_rbuf_t *ref, byte_t* value, int32_t length);


/** @} */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_RWBUF_H_INCLUDED_*/