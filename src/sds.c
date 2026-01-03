
#include <libcc/alloc.h>
#include <libcc/sds.h>

/* Get SDS header size for a given type */
_CC_FORCE_INLINE_ byte_t sds_hdr(byte_t type) {
    switch(type & _SDS_MASK_) {
        case _SDS_MASK_5_:
            return (byte_t)sizeof(struct _sds_hdr5);
        case _SDS_MASK_8_:
            return (byte_t)sizeof(struct _sds_hdr8);
        case _SDS_MASK_16_:
            return (byte_t)sizeof(struct _sds_hdr16);
        case _SDS_MASK_32_:
            return (byte_t)sizeof(struct _sds_hdr32);
        case _SDS_MASK_64_:
            return (byte_t)sizeof(struct _sds_hdr64);
    }
    return 0;
}

/* Select the appropriate SDS type based on string length */
_CC_FORCE_INLINE_ byte_t sds_rtype(size_t length) {
    if (length < (1 << 5)) {
        return _SDS_MASK_5_;
    }
    if (length < (1 << 8)) {
        return _SDS_MASK_8_;
    }
    if (length < (1 << 16)) {
        return _SDS_MASK_16_;
    }
#if (LONG_MAX == LLONG_MAX)
    if (length < (1ll << 32)) {
        return _SDS_MASK_32_;
    }
    return _SDS_MASK_64_;
#else
    return _SDS_MASK_32_;
#endif
}

_CC_API_PUBLIC(_cc_sds_t) _cc_sds_empty_alloc(size_t size) {
    return _cc_sds_alloc(NULL, size);
}

_CC_API_PUBLIC(_cc_sds_t) _cc_sds_alloc(const tchar_t *s, size_t length) {
    byte_t type;
    byte_t hdr_length;
    byte_t *hdr;
    byte_t *ptr;
    size_t ptr_length = 0;
    
    _cc_assert(s != NULL || length != 0);
    if (s && length == 0) {
        length = _tcslen(s);
    } else if (length == 0) {
        _cc_assert(false);
    }

    type = sds_rtype(length);
    hdr_length = sds_hdr(type);
    hdr = (byte_t*)_cc_malloc(hdr_length + (length + 1) * sizeof(tchar_t));
    ptr = hdr + hdr_length;

    if (s == NULL) {
        *(tchar_t*)(ptr) = '\0';
    } else {
        memcpy(ptr, s, sizeof(tchar_t) * length);
        *(tchar_t*)(ptr + length) = '\0';
        ptr_length = length;
    }

    /* flags pointer. */
    *(byte_t*)(ptr - sizeof(byte_t)) = type;

    switch (type) {
        case _SDS_MASK_5_: {
            struct _sds_hdr5 *h = (struct _sds_hdr5 *)hdr;
            h->flags |= (length << _SDS_BITS_);
            break;
        }
        case _SDS_MASK_8_: {
            struct _sds_hdr8 *h = (struct _sds_hdr8 *)hdr;
            h->length = (uint8_t)ptr_length;
            h->limit = (uint8_t)length;
            break;
        }
        case _SDS_MASK_16_: {
            struct _sds_hdr16 *h = (struct _sds_hdr16 *)hdr;
            h->length = (uint16_t)ptr_length;
            h->limit = (uint16_t)length;
            break;
        }
        case _SDS_MASK_32_: {
            struct _sds_hdr32 *h = (struct _sds_hdr32 *)hdr;
            h->length = (uint32_t)ptr_length;
            h->limit = (uint32_t)length;
            break;
        }
        case _SDS_MASK_64_: {
            struct _sds_hdr64 *h = (struct _sds_hdr64 *)hdr;
            h->length = (uint64_t)ptr_length;
            h->limit = (uint64_t)length;
            break;
        }
    }

    return (_cc_sds_t)ptr;
}

_CC_API_PUBLIC(void) _cc_sds_free(_cc_sds_t s) {
    byte_t *hdr = (byte_t*)s;
    if (hdr == NULL) {
        return;
    }
    _cc_free(hdr - sds_hdr(*(byte_t*)(hdr - 1)));
}

/* Copy a string to SDS */
_CC_API_PUBLIC(_cc_sds_t) _cc_sds_copy(_cc_sds_t sds, const tchar_t *s, size_t size) {
    size_t limit;
    if (sds == NULL) {
        return _cc_sds_alloc(s, size);
    }
    
    limit = _cc_sds_limit(sds);
    if (size > limit) {
        _cc_sds_free(sds);
        sds = _cc_sds_alloc(s, size);
    } else {
        // Copy data to existing SDS
        memcpy(sds, s, size * sizeof(tchar_t));
        _cc_sds_set_length(sds, size);
    }
    return sds;
}

_CC_API_PUBLIC(_cc_sds_t) _cc_sds_vformat(const tchar_t *format, va_list ap) {
    size_t length;
    size_t free_length = 10240;
    tchar_t static_buf[10240];
    tchar_t *ptr = static_buf;
    _cc_sds_t s = NULL;

    if (format == NULL) {
        return NULL;
    }

    /* If the first attempt to format fails, resize the buffer appropriately
     * and try again */
    while (1) {
        length = (size_t)_vsntprintf(ptr, free_length, format, ap);
    #ifdef __CC_WINDOWS__
        if (length == -1) {
            length = _vsntprintf(NULL, 0, format, ap);
        }
    #endif
        if (length <= 0) {
            break;
        }

        /* SUCCESS */
        if (length < free_length) {
            s = _cc_sds_alloc(ptr, length);
            break;
        }

        if (ptr != static_buf) {
            _cc_free(ptr);
        }

        free_length = _cc_aligned_alloc_opt((length + 64) + sizeof(tchar_t), 64);
        ptr = (tchar_t *)_cc_malloc(free_length);
    }

    if (ptr != static_buf) {
        _cc_free(ptr);
    }
    
    return s;
}

_CC_API_PUBLIC(_cc_sds_t) _cc_sds_format(const tchar_t *format, ...){
    va_list ap;
    _cc_sds_t s;

    va_start(ap, format);
    s = _cc_sds_vformat(format, ap);
    va_end(ap);

    return s;
}

_CC_API_PUBLIC(_cc_sds_t) _cc_sds_cat(_cc_sds_t s, const tchar_t *t, size_t length) {
    size_t curlen;
    byte_t flags;

    if (s == NULL) {
        return _cc_sds_alloc(t, length);
    }

    flags = *(((byte_t*)s) - sizeof(byte_t));
    curlen = _cc_sds_length(s);
    if ((flags & _SDS_MASK_) == _SDS_MASK_5_ || _cc_sds_available(s) < length) {
        _cc_sds_t ss = _cc_sds_alloc(NULL, curlen + length);
        memcpy(ss, s, sizeof(tchar_t) * curlen);
        _cc_sds_free(s);
        s = ss;
    }

    memcpy(s + curlen, t, sizeof(tchar_t) * length);
    /* null-terminate */
    s[curlen + length] = '\0';
    _cc_sds_set_length(s, curlen + length);
    return s;
}