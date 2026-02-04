#ifndef _C_LIBCC_SDS_H_INCLUDED_
#define _C_LIBCC_SDS_H_INCLUDED_

#include <stdarg.h>
#include <stdint.h>
#include "string.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef tchar_t *_cc_sds_t;

#define _SDS_MASK_5_    0x00
#define _SDS_MASK_8_    0x01
#define _SDS_MASK_16_   0x02
#define _SDS_MASK_32_   0x03
#define _SDS_MASK_64_   0x04

#define _SDS_MASK_      0x07
#define _SDS_BITS_      0x03

#pragma pack(push, 1)
struct _sds_hdr5 {
    byte_t flags;
};

struct _sds_hdr8 {
    uint8_t length; 
    uint8_t limit;
    byte_t flags;
};

struct _sds_hdr16 {
    uint16_t length;
    uint16_t limit;
    byte_t flags;
};

struct _sds_hdr32 {
    uint32_t length;
    uint32_t limit;
    byte_t flags;
};

struct _sds_hdr64 {
    uint64_t length;
    uint64_t limit;
    byte_t flags;
};
#pragma pack(pop)

/* Get the length of the SDS string */
_CC_FORCE_INLINE_ size_t _cc_sds_length(const _cc_sds_t s) {
    byte_t *hdr;
    byte_t flags;
    if (s == 0) {
        return 0;
    }
    hdr = (byte_t*)s;
    flags = *(hdr - sizeof(byte_t));
    switch (flags & _SDS_MASK_) {
        case _SDS_MASK_5_:
            return ((struct _sds_hdr5 *)(hdr - sizeof(struct _sds_hdr5)))->flags >> _SDS_BITS_;
        case _SDS_MASK_8_: 
            return (size_t)((struct _sds_hdr8 *)(hdr - sizeof(struct _sds_hdr8)))->length;
        case _SDS_MASK_16_:
            return (size_t)((struct _sds_hdr16 *)(hdr - sizeof(struct _sds_hdr16)))->length;
        case _SDS_MASK_32_:
            return (size_t)((struct _sds_hdr32 *)(hdr - sizeof(struct _sds_hdr32)))->length;
        case _SDS_MASK_64_:
            return (size_t)((struct _sds_hdr64 *)(hdr - sizeof(struct _sds_hdr64)))->length;
    }
    return 0;
}

/* Get the limit of the SDS string */
_CC_FORCE_INLINE_ size_t _cc_sds_limit(const _cc_sds_t s) {
    byte_t *hdr;
    byte_t flags;
    if (s == 0) {
        return 0;
    }
    hdr = (byte_t*)s;
    flags = *(hdr - sizeof(byte_t));
    switch (flags & _SDS_MASK_) {
        case _SDS_MASK_5_: 
            return ((struct _sds_hdr5 *)(hdr - sizeof(struct _sds_hdr5)))->flags >> _SDS_BITS_;
        case _SDS_MASK_8_: 
            return (size_t)((struct _sds_hdr8 *)(hdr - sizeof(struct _sds_hdr8)))->limit;
        case _SDS_MASK_16_:
            return (size_t)((struct _sds_hdr16 *)(hdr - sizeof(struct _sds_hdr16)))->limit;
        case _SDS_MASK_32_:
            return (size_t)((struct _sds_hdr32 *)(hdr - sizeof(struct _sds_hdr32)))->limit;
        case _SDS_MASK_64_:
            return (size_t)((struct _sds_hdr64 *)(hdr - sizeof(struct _sds_hdr64)))->limit;
    }
    return 0;
}
/* Set the length of the SDS string */
_CC_FORCE_INLINE_ void _cc_sds_set_length(_cc_sds_t s, size_t length) {
    byte_t *hdr;
    byte_t flags;
    if (s == 0) {
        return;
    }
    hdr = (byte_t*)s;
    flags = *(hdr - sizeof(byte_t));
    switch (flags & _SDS_MASK_) {
        case _SDS_MASK_5_: {
            struct _sds_hdr5 *h = (struct _sds_hdr5 *)(hdr - sizeof(struct _sds_hdr5));
            _cc_assert(length <= 32);
            if (length <= 32) {
                h->flags = _SDS_MASK_5_ | ((byte_t)length << _SDS_BITS_);
            }
        }
        break;
        case _SDS_MASK_8_: {
            _cc_assert(length <= UCHAR_MAX);
            ((struct _sds_hdr8 *)(hdr - sizeof(struct _sds_hdr8)))->length = (uint8_t)length;
        }
        break;
        case _SDS_MASK_16_: {
            _cc_assert(length <= USHRT_MAX);
            ((struct _sds_hdr16 *)(hdr - sizeof(struct _sds_hdr16)))->length = (uint16_t)length;
        }
        break;
        case _SDS_MASK_32_: {
            _cc_assert(length <= UINT_MAX);
            ((struct _sds_hdr32 *)(hdr - sizeof(struct _sds_hdr32)))->length = (uint32_t)length;
        }
        break;
        case _SDS_MASK_64_: {
            ((struct _sds_hdr64 *)(hdr - sizeof(struct _sds_hdr64)))->length = (uint32_t)length;
        }
        break;
    }
}

/* Get available capacity in the SDS string */
_CC_FORCE_INLINE_ size_t _cc_sds_available(const _cc_sds_t s) {
    byte_t *hdr = (byte_t*)s;
    byte_t flags = *(hdr - sizeof(byte_t));
    if (s == 0) {
        return 0;
    }
    switch (flags & _SDS_MASK_) {
        case _SDS_MASK_5_: {
            struct _sds_hdr5 *h = (struct _sds_hdr5 *)(hdr - sizeof(struct _sds_hdr5));
            return (1 << 5) - (h->flags >> _SDS_BITS_);
        }
        case _SDS_MASK_8_: {
            struct _sds_hdr8 *h = (struct _sds_hdr8 *)(hdr - sizeof(struct _sds_hdr8));
            return h->limit - h->length;
        }
        case _SDS_MASK_16_: {
            struct _sds_hdr16 *h = (struct _sds_hdr16 *)(hdr - sizeof(struct _sds_hdr16));
            return h->limit - h->length;
        }
        case _SDS_MASK_32_: {
            struct _sds_hdr32 *h = (struct _sds_hdr32 *)(hdr - sizeof(struct _sds_hdr32));
            return h->limit - h->length;
        }
        case _SDS_MASK_64_: {
            struct _sds_hdr64 *h = (struct _sds_hdr64 *)(hdr - sizeof(struct _sds_hdr64));
            return (size_t)(h->limit - h->length);
        }
    }
    return 0;
}

/* Allocate an empty SDS string with specified capacity */
_CC_API_PUBLIC(_cc_sds_t) _cc_sds_empty_alloc(size_t length);
/* Allocate and copy a string to SDS */
_CC_API_PUBLIC(_cc_sds_t) _cc_sds_alloc(const tchar_t *s, size_t size);
/* Copy a string to SDS */
_CC_API_PUBLIC(_cc_sds_t) _cc_sds_copy(_cc_sds_t sds, const tchar_t *s, size_t size);
/* Free an SDS string */
_CC_API_PUBLIC(void) _cc_sds_free(_cc_sds_t s);

/* Format an SDS string using va_list */
_CC_API_PUBLIC(_cc_sds_t) _cc_sds_vformat(const tchar_t *format, va_list ap);
/* Format an SDS string using printf-style arguments */
_CC_API_PUBLIC(_cc_sds_t) _cc_sds_format(const tchar_t *format, ...);
/* Concatenate data to an SDS string */
_CC_API_PUBLIC(_cc_sds_t) _cc_sds_cat(_cc_sds_t s, const tchar_t *t, size_t length);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_LIBCC_SDS_H_INCLUDED_ */