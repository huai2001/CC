#include <libcc/rwbuf.h>
#include <libcc/math.h>

#ifdef _CC_DEBUG_
#define _DEBUG(X) _cc_logger_debug(X ## " fail: %d > %d", ref->off, ref->limit)
#else
#define _DEBUG(X) void(0)
#endif

/**/
void _cc_wbuf_init(_cc_wbuf_t *ref, byte_t*bytes, uint32_t length) {
    ref->off = 0;
    ref->bytes = bytes;
    ref->limit = length;
}

/**/
bool_t _cc_wbuf_int8(_cc_wbuf_t *ref, int8_t x) {
    if (ref->off + sizeof(int8_t) > ref->limit) {
        _DEBUG("write int8");
        return false;
    }
    ref->bytes[ref->off++] = (byte_t)x;
    return true;
}

/**/
bool_t _cc_wbuf_int16(_cc_wbuf_t *ref, int16_t x) {
    if (ref->off + sizeof(int16_t) > ref->limit) {
        _DEBUG("write int16");
        return false;
    }
    ref->bytes[ref->off++] = (uint8_t)(x & 0xff);
    ref->bytes[ref->off++] = (uint8_t)(x >> 8);
    return true;
}

/**/
bool_t _cc_wbuf_int32(_cc_wbuf_t *ref, int32_t x) {
    if (ref->off + sizeof(int32_t) > ref->limit) {
        _DEBUG("write int32");
        return false;
    }
    ref->bytes[ref->off++] = (uint8_t)(x & 0xff);
    ref->bytes[ref->off++] = (uint8_t)(x >> 8);
    ref->bytes[ref->off++] = (uint8_t)(x >> 16);
    ref->bytes[ref->off++] = (uint8_t)(x >> 24);
    return true;
}

/**/
bool_t _cc_wbuf_int64(_cc_wbuf_t *ref, int64_t x) {
    if (ref->off + sizeof(int64_t) > ref->limit) {
        _DEBUG("write int64");
        return false;
    }
    ref->bytes[ref->off++] = (uint8_t)(x & 0xff);
    ref->bytes[ref->off++] = (uint8_t)(x >> 8);
    ref->bytes[ref->off++] = (uint8_t)(x >> 16);
    ref->bytes[ref->off++] = (uint8_t)(x >> 24);
    ref->bytes[ref->off++] = (uint8_t)(x >> 32);
    ref->bytes[ref->off++] = (uint8_t)(x >> 40);
    ref->bytes[ref->off++] = (uint8_t)(x >> 48);
    ref->bytes[ref->off++] = (uint8_t)(x >> 56);
    return true;
}

/**/
bool_t _cc_wbuf_double(_cc_wbuf_t *ref, double v) {
    int64_t value;
    memcpy(&value, &v, sizeof(int64_t));
    return _cc_wbuf_int64(value);
}

/**/
bool_t _cc_wbuf_string(_cc_wbuf_t *ref, const tchar_t* value, int32_t length) {
    if (length == -1) {
        length = (int32_t)_tcslen(value);
    }
    return _cc_wbuf_bytes(ref, (byte_t*)value, length * sizeof(tchar_t));
}

/**/
bool_t _cc_wbuf_bytes(_cc_wbuf_t *ref, const byte_t* value, int32_t length) {
    if (length < 0 || value == NULL) {
        _DEBUG("write bytes length == 0 || value == NULL");
        return false;
    }

    if (length < 0xFE) {
        if ((ref->off + 1 + length) > ref->limit) {
            _DEBUG("write bytes length < 0xFE");
            return false;
        }
        ref->bytes[ref->off++] = (byte_t)length;
    } else {
        if ((ref->off + 3 + length) > ref->limit) {
            _DEBUG("write bytes length > 0xFE");
            return false;
        }
        ref->bytes[ref->off++] = 0xff;
        ref->bytes[ref->off++] = (byte_t)(length & 0xff);
        ref->bytes[ref->off++] = (byte_t)(length >> 8);
    }

    if (length > 0) {
        memcpy(&ref->bytes[ref->off], value, length);
        ref->off += length;
    }
    return true;
}

/**/
void _cc_rbuf_init(_cc_rbuf_t *ref, const byte_t*bytes, uint32_t length) {
    ref->off = 0;
    ref->limit = length;
    ref->bytes = bytes;
}

/**/
int8_t _cc_rbuf_int8(_cc_rbuf_t *ref) {
    if (ref->off + sizeof(int8_t) > ref->limit) {
        _DEBUG("read int8");
        return 0;
    }
    return ref->bytes[ref->off++];
}

/**/
int16_t _cc_rbuf_int16(_cc_rbuf_t *ref) {
    int16_t result;
    if (ref->off + sizeof(int16_t) > ref->limit) {
        _DEBUG("read int16");
        return 0;
    }

    result = ((int16_t)ref->bytes[ref->off]) | 
             ((int16_t)ref->bytes[ref->off + 1] << 8);

    ref->off += sizeof(int16_t);
    return result;
}

/**/
int32_t _cc_rbuf_int32(_cc_rbuf_t *ref) {
    int32_t result;
    if (ref->off + sizeof(int32_t) > ref->limit) {
        _DEBUG("read int32");
        return 0;
    }

    result = ((int32_t)ref->bytes[ref->off]) | 
             ((int32_t)ref->bytes[ref->off + 1] << 8)  |
             ((int32_t)ref->bytes[ref->off + 2] << 16) |
             ((int32_t)ref->bytes[ref->off + 3] << 24);

    ref->off += sizeof(int32_t);

    return result;
}

/**/
int64_t _cc_rbuf_int64(_cc_rbuf_t *ref) {
    int64_t result;
    if (ref->off + sizeof(int64_t) > ref->limit) {
        _DEBUG("read int64");
        return 0;
    }

    result = ((int64_t)ref->bytes[ref->off]) | 
             ((int64_t)ref->bytes[ref->off + 1] << 8)  |
             ((int64_t)ref->bytes[ref->off + 2] << 16) |
             ((int64_t)ref->bytes[ref->off + 3] << 24) |
             ((int64_t)ref->bytes[ref->off + 4] << 32) |
             ((int64_t)ref->bytes[ref->off + 5] << 40) |
             ((int64_t)ref->bytes[ref->off + 6] << 48) |
             ((int64_t)ref->bytes[ref->off + 7] << 56);

    ref->off += sizeof(int64_t);
    return result;
}

/**/
double _cc_rbuf_double(_cc_wbuf_t *ref) {
    double d;
    int64_t v = _cc_rbuf_int64(ref);
    memcpy(&d, &v, sizeof(double));
    return d;
}

/**/
int32_t _cc_rbuf_string(_cc_rbuf_t *ref, tchar_t *value, int32_t length) {
    if (length <= 0 || value == NULL) {
        _DEBUG("read string length <= 0 || value == NULL");
        return 0;
    }

    length = _cc_rbuf_bytes(ref, (byte_t*)value, (length - 1) * sizeof(tchar_t));
    value[length] = 0;
    return length;
}

/**/
int32_t _cc_rbuf_bytes(_cc_rbuf_t *ref, byte_t *value, int32_t length) {
    int32_t result;
    if (length <= 0 || value == NULL) {
        _DEBUG("read bytes length <= 0 || value == NULL");
        return 0;
    }

    result = ref->bytes[ref->off++];
    if (result == 0xFE) {
        if ((ref->off + 2) > ref->limit) {
            _DEBUG("read bytes length == 0xFE");
            return 0;
        }
        result = ref->bytes[ref->off] | (ref->bytes[ref->off + 1] << 8);
        ref->off += 2;
    }

    if (result == 0) {
        return result;
    }
    
    if ((ref->off + result) > ref->limit) {
        _DEBUG("read bytes");
        return 0;
    }

    length = _min(length, result);
    memcpy(value, &ref->bytes[ref->off], length);
    ref->off += result;

    return length;
}
