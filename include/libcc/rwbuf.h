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
_CC_FORCE_INLINE_ int16_t __r_i16(const byte_t* bytes) {
    return (((int16_t)bytes[0]) | 
            ((int16_t)bytes[1] << 8));
}
/**/
_CC_FORCE_INLINE_ int32_t __r_i32(const byte_t* bytes) {
    return (((int32_t)bytes[0]) | 
            ((int32_t)bytes[1] << 8)  |
            ((int32_t)bytes[2] << 16) |
            ((int32_t)bytes[3] << 24));
}
/**/
_CC_FORCE_INLINE_ int64_t __r_i64(const byte_t* bytes) {
    return (((int64_t)bytes[0]) | 
            ((int64_t)bytes[1] << 8)  |
            ((int64_t)bytes[2] << 16) |
            ((int64_t)bytes[3] << 24) |
            ((int64_t)bytes[4] << 32) |
            ((int64_t)bytes[5] << 40) |
            ((int64_t)bytes[6] << 48) |
            ((int64_t)bytes[7] << 56));
}
/** @} */

/** {@ */

/**/
_CC_FORCE_INLINE_ int __w_i16(byte_t* bytes, int16_t x) {
    bytes[0] = (uint8_t)(x & 0xff);
    bytes[1] = (uint8_t)(x >> 8);
    return sizeof(int16_t);
}
/**/
_CC_FORCE_INLINE_ int __w_i32(byte_t* bytes, int32_t x) {
    bytes[0] = (uint8_t)(x & 0xff);
    bytes[1] = (uint8_t)(x >> 8);
    bytes[2] = (uint8_t)(x >> 16);
    bytes[3] = (uint8_t)(x >> 24);
    return sizeof(int32_t);
}
/**/
_CC_FORCE_INLINE_ int __w_i64(byte_t* bytes, int64_t x) {
    bytes[0] = (uint8_t)(x & 0xff);
    bytes[1] = (uint8_t)(x >> 8);
    bytes[2] = (uint8_t)(x >> 16);
    bytes[3] = (uint8_t)(x >> 24);
    bytes[4] = (uint8_t)(x >> 32);
    bytes[5] = (uint8_t)(x >> 40);
    bytes[6] = (uint8_t)(x >> 48);
    bytes[7] = (uint8_t)(x >> 56);
    return sizeof(int64_t);
}
/** @} */

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
double _cc_rbuf_double(_cc_rbuf_t *ref);
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