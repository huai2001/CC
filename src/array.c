#include <libcc/alloc.h>
#include <libcc/array.h>

#define _CC_MAX_ARRAY_EXPAND_ 32

typedef struct __array_hdr {
    size_t limit;
    size_t length;
} __array_hdr_t;

/**/
_CC_API_PUBLIC(_cc_array_t) _cc_alloc_array(size_t capacity) {
    __array_hdr_t *hdr = (__array_hdr_t*)_cc_malloc(sizeof(__array_hdr_t) + capacity * sizeof(uintptr_t));
    hdr->limit = capacity;
    hdr->length = 0;
    return (_cc_array_t)(hdr + 1);
}

/**/
_CC_API_PUBLIC(_cc_array_t) _cc_realloc_array(_cc_array_t ctx, size_t capacity) {
    size_t length;
    __array_hdr_t *hdr;
    _cc_assert(ctx != 0);
    hdr = (__array_hdr_t *)((byte_t*)ctx - sizeof(__array_hdr_t));
    if (capacity == hdr->limit) {
        return (_cc_array_t)ctx;
    }
    length = hdr->length;

    hdr = (__array_hdr_t *)_cc_realloc(hdr, sizeof(__array_hdr_t) + capacity * sizeof(uintptr_t));
    hdr->limit = capacity;
    hdr->length = length > capacity ? capacity : length;
    return (_cc_array_t)(hdr + 1);
}

/**/
_CC_API_PUBLIC(size_t) _cc_array_available(const _cc_array_t ctx) {
    __array_hdr_t *hdr;
    _cc_assert(ctx != 0);
    hdr = (__array_hdr_t *)((byte_t *)ctx - sizeof(__array_hdr_t));
    return hdr->limit - hdr->length;
}

/**/
_CC_API_PUBLIC(void) _cc_free_array(_cc_array_t ctx) {
    __array_hdr_t *hdr;
    _cc_assert(ctx != 0);
    hdr = (__array_hdr_t *)((byte_t *)ctx - sizeof(__array_hdr_t));
    _cc_free(hdr);
}

/**/
_CC_API_PUBLIC(size_t) _cc_array_length(const _cc_array_t ctx) {
    _cc_assert(ctx != 0);
    return ((__array_hdr_t *)((byte_t *)ctx - sizeof(__array_hdr_t)))->length;
}

_CC_API_PUBLIC(uintptr_t) _cc_array_get(const _cc_array_t ctx, const size_t index) {
    __array_hdr_t *hdr;
    _cc_assert(ctx != 0);

    hdr = (__array_hdr_t *)((byte_t *)ctx - sizeof(__array_hdr_t));
    if (_cc_unlikely(hdr->length <= index)) {
        _cc_logger_error(_T("Array find: index out of range [%d] with size %d"), index, hdr->length);
        return -1;
    }
    return _cc_array_value(ctx,index);
}

_CC_API_PUBLIC(size_t) _cc_array_push(_cc_array_t *ctx, uintptr_t data) {
    size_t index;
    __array_hdr_t *hdr;
    _cc_array_t ptr = ((_cc_array_t)*ctx);
    _cc_assert(ctx != 0);

    hdr = (__array_hdr_t *)((byte_t *)ptr - sizeof(__array_hdr_t));
    index = hdr->length;
    /*if not enough space,expand first*/
    if (index == hdr->limit) {
        ptr = (uintptr_t)_cc_realloc_array(ptr, index + (size_t)(hdr->limit * 0.72f));
        hdr = (__array_hdr_t *)((byte_t *)ptr - sizeof(__array_hdr_t));
        *ctx = ptr;
    }

    *((uintptr_t*)ptr + index) = data;

    hdr->length++;

    return index;
}

_CC_API_PUBLIC(uintptr_t) _cc_array_pop(_cc_array_t ctx) {
    __array_hdr_t *hdr;
    uintptr_t data;
    _cc_assert(ctx != 0);

    hdr = (__array_hdr_t *)((byte_t *)ctx - sizeof(__array_hdr_t));
    if (hdr->length > 0) {
        /*data is the last element*/
        data = *((uintptr_t *)ctx + hdr->length - 1);
        hdr->length--;
        return data;
    }
    return -1;
}

_CC_API_PUBLIC(void) _cc_array_append(_cc_array_t *ctx, const _cc_array_t append) {
    __array_hdr_t *hdr;
    __array_hdr_t *hdr2;
    _cc_array_t ptr = ((_cc_array_t)*ctx);
    size_t capacity = 0;
    _cc_assert(ctx != 0 && append != 0);

    hdr = (__array_hdr_t *)((byte_t *)ptr - sizeof(__array_hdr_t));
    hdr2 = (__array_hdr_t *)((byte_t *)append - sizeof(__array_hdr_t));
    capacity = hdr->length + hdr2->length;
    /*if not enough space,expand first*/
    if (capacity > hdr->limit) {
        ptr = _cc_realloc_array(ptr, capacity);
        hdr = (__array_hdr_t *)((byte_t *)ptr - sizeof(__array_hdr_t));
        *ctx = ptr;
    }

    memcpy((uintptr_t*)ptr + hdr->length, (uintptr_t*)append, hdr2->length * sizeof(uintptr_t));
    hdr->length = capacity;
}

_CC_API_PUBLIC(bool_t) _cc_array_set(_cc_array_t ctx, const size_t index, uintptr_t data) {
    __array_hdr_t *hdr;
    _cc_assert(ctx != 0);
    hdr = (__array_hdr_t *)((byte_t *)ctx - sizeof(__array_hdr_t));
    if (index >= hdr->length) {
        _cc_logger_error(_T("Array set: index out of range [%d] with length %d"), index, hdr->length);
        return false;
    }

    *((uintptr_t*)ctx + index) = data;

    return true;
}

_CC_API_PUBLIC(uintptr_t) _cc_array_remove(_cc_array_t ctx, const size_t index) {
    uintptr_t data;
    uintptr_t *ptr;
    __array_hdr_t *hdr;
    _cc_assert(ctx != 0);
    hdr = (__array_hdr_t *)((byte_t *)ctx - sizeof(__array_hdr_t));
    
    if (index >= hdr->length) {
        _cc_logger_error(_T("Array remove: index out of range [%d] with length %d"), index, hdr->length);
        return -1;
    } else if ((hdr->length - 1) == index) {
        /*data is the last element*/
        data = *((uintptr_t*)ctx + hdr->length - 1);
        hdr->length--;
    } else {
        size_t n = hdr->length - index - 1;
        ptr = ((uintptr_t*)ctx + index);
        data = *ptr;
        if (n > 0) {
            memmove(ptr, ptr + 1, n * sizeof(uintptr_t));
        }
        hdr->length--;
    }
    return data;
}

_CC_API_PUBLIC(bool_t) _cc_array_cleanup(_cc_array_t ctx) {
    __array_hdr_t *hdr;
    _cc_assert(ctx != 0);
    hdr = (__array_hdr_t *)((byte_t *)ctx - sizeof(__array_hdr_t));
    hdr->length = 0;
    return true;
}

_CC_API_PUBLIC(uintptr_t*) _cc_array_begin(const _cc_array_t ctx) {
    _cc_assert(ctx != 0);
    return (uintptr_t*)ctx;
}

_CC_API_PUBLIC(uintptr_t*) _cc_array_end(const _cc_array_t ctx) {
    __array_hdr_t *hdr;
    _cc_assert(ctx != 0);
    hdr = (__array_hdr_t *)((byte_t *)ctx - sizeof(__array_hdr_t));

    return ((uintptr_t*)ctx + hdr->length - 1);
}
