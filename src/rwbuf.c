#include <libcc/rwbuf.h>
#include <libcc/math.h>
/**/
void _cc_wbuf_init(_cc_wbuf_t *buffer, byte_t*bytes, uint32_t length) {
    buffer->off = 0;
    buffer->bytes = bytes;
    buffer->limit = length;
}

/**/
bool_t _cc_wbuf_int8(_cc_wbuf_t *buffer, int8_t x) {
    if (buffer->off + sizeof(int8_t) > buffer->limit) {
        return false;
    }
    buffer->bytes[buffer->off++] = (byte_t)x;
    return true;
}

/**/
bool_t _cc_wbuf_int16(_cc_wbuf_t *buffer, int16_t x) {
    if (buffer->off + sizeof(int16_t) > buffer->limit) {
        return false;
    }
    buffer->bytes[buffer->off++] = (uint8_t)(x & 0xff);
    buffer->bytes[buffer->off++] = (uint8_t)(x >> 8);
    return true;
}

/**/
bool_t _cc_wbuf_int32(_cc_wbuf_t *buffer, int32_t x) {
    if (buffer->off + sizeof(int32_t) > buffer->limit) {
        return false;
    }
    buffer->bytes[buffer->off++] = (uint8_t)(x & 0xff);
    buffer->bytes[buffer->off++] = (uint8_t)(x >> 8);
    buffer->bytes[buffer->off++] = (uint8_t)(x >> 16);
    buffer->bytes[buffer->off++] = (uint8_t)(x >> 24);
    return true;
}

/**/
bool_t _cc_wbuf_int64(_cc_wbuf_t *buffer, int64_t x) {
    if (buffer->off + sizeof(int64_t) > buffer->limit) {
        return false;
    }
    buffer->bytes[buffer->off++] = (uint8_t)(x & 0xff);
    buffer->bytes[buffer->off++] = (uint8_t)(x >> 8);
    buffer->bytes[buffer->off++] = (uint8_t)(x >> 16);
    buffer->bytes[buffer->off++] = (uint8_t)(x >> 24);
    buffer->bytes[buffer->off++] = (uint8_t)(x >> 32);
    buffer->bytes[buffer->off++] = (uint8_t)(x >> 40);
    buffer->bytes[buffer->off++] = (uint8_t)(x >> 48);
    buffer->bytes[buffer->off++] = (uint8_t)(x >> 56);
    return true;
}

/**/
bool_t _cc_wbuf_string(_cc_wbuf_t *buffer, const tchar_t* value, int32_t length) {
    if (length == -1) {
        length = (int32_t)_tcslen(value);
    }
    return _cc_wbuf_bytes(buffer, (byte_t*)value, length * sizeof(tchar_t));
}

/**/
bool_t _cc_wbuf_bytes(_cc_wbuf_t *buffer, const byte_t* value, int32_t length) {
    if (length < 0 || value == NULL) {
        return false;
    }

    if (length < 255) {
        if ((buffer->off + 1 + length) > buffer->limit) {
            return false;
        }
        buffer->bytes[buffer->off++] = (byte_t)length;
    } else {
        if ((buffer->off + 3 + length) > buffer->limit) {
            return false;
        }
        buffer->bytes[buffer->off++] = 0xff;
        buffer->bytes[buffer->off++] = (byte_t)(length & 0xff);
        buffer->bytes[buffer->off++] = (byte_t)(length >> 8);
    }

    if (length > 0) {
        memcpy(&buffer->bytes[buffer->off], value, length);
        buffer->off += length;
    }
    return true;
}

/**/
void _cc_rbuf_init(_cc_rbuf_t *buffer, const byte_t*bytes, uint32_t length) {
    buffer->off = 0;
    buffer->limit = length;
    buffer->bytes = bytes;
}

/**/
int8_t _cc_rbuf_int8(_cc_rbuf_t *buffer) {
    if (buffer->off + sizeof(int8_t) > buffer->limit) {
        return -1;
    }
    return buffer->bytes[buffer->off++];
}

/**/
int16_t _cc_rbuf_int16(_cc_rbuf_t *buffer) {
    int16_t result;
    if (buffer->off + sizeof(int16_t) > buffer->limit) {
        return -1;
    }

    result = ((int16_t)buffer->bytes[buffer->off]) | 
             ((int16_t)buffer->bytes[buffer->off + 1] << 8);

    buffer->off += sizeof(int16_t);
    return result;
}

/**/
int32_t _cc_rbuf_int32(_cc_rbuf_t *buffer) {
    int32_t result;
    if (buffer->off + sizeof(int32_t) > buffer->limit) {
        return -1;
    }

    result = ((int32_t)buffer->bytes[buffer->off]) | 
             ((int32_t)buffer->bytes[buffer->off + 1] << 8)  |
             ((int32_t)buffer->bytes[buffer->off + 2] << 16) |
             ((int32_t)buffer->bytes[buffer->off + 3] << 24);

    buffer->off += sizeof(int32_t);

    return result;
}

/**/
int64_t _cc_rbuf_int64(_cc_rbuf_t *buffer) {
    int64_t result;
    if (buffer->off + sizeof(int64_t) > buffer->limit) {
        return -1;
    }

    result = ((int64_t)buffer->bytes[buffer->off]) | 
             ((int64_t)buffer->bytes[buffer->off + 1] << 8)  |
             ((int64_t)buffer->bytes[buffer->off + 2] << 16) |
             ((int64_t)buffer->bytes[buffer->off + 3] << 24) |
             ((int64_t)buffer->bytes[buffer->off + 4] << 32) |
             ((int64_t)buffer->bytes[buffer->off + 5] << 40) |
             ((int64_t)buffer->bytes[buffer->off + 6] << 48) |
             ((int64_t)buffer->bytes[buffer->off + 7] << 56);

    buffer->off += sizeof(int64_t);
    return result;
}

/**/
int32_t _cc_rbuf_string(_cc_rbuf_t *buffer, tchar_t *value, int32_t length) {
    if (length <= 0 || value == NULL) {
        return 0;
    }

    length = _cc_rbuf_bytes(buffer, (byte_t*)value, (length - 1) * sizeof(tchar_t));
    value[length] = 0;
    return length;
}

/**/
int32_t _cc_rbuf_bytes(_cc_rbuf_t *buffer, byte_t *value, int32_t length) {
    int32_t result;
    if (length <= 0 || value == NULL) {
        return 0;
    }

    result = buffer->bytes[buffer->off++];
    if (result == 0xff) {
        if ((buffer->off + 2) > buffer->limit) {
            return 0;
        }
        result = buffer->bytes[buffer->off] | (buffer->bytes[buffer->off + 1] << 8);
        buffer->off += 2;
    }

    if (result == 0) {
        return result;
    }
    
    if ((buffer->off + result) > buffer->limit) {
        return 0;
    }

    length = _min(length, result);
    memcpy(value, &buffer->bytes[buffer->off], length);
    buffer->off += result;

    return length;
}
