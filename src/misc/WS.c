#include <string.h>
#include <libcc/WS.h>
#include <libcc/rand.h>

/**/
_CC_API_PUBLIC(bool_t) _cc_ws_mask(byte_t *ptr, int64_t length, byte_t *mask, int64_t offset) {
    int64_t i = offset, n = length + offset;
    if (ptr == NULL || length <= 0 || mask == NULL) {
        return false;
    }
    // Process 4 bytes at a time for better performance
    length = (n / 4) * 4; // Adjust n to ensure we don't go out of bounds
    for (; i < length; i += 4, ptr += 4) {
        ptr[0] ^= mask[i & 0x03];
        ptr[1] ^= mask[(i + 1) & 0x03];
        ptr[2] ^= mask[(i + 2) & 0x03];
        ptr[3] ^= mask[(i + 3) & 0x03];
    }
    // Process remaining bytes
    for (; i < n; i++, ptr++) {
        *ptr ^= mask[i & 0x03];
    }
    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_ws_mask_copy(byte_t *dst, int64_t dst_length, byte_t *src, int64_t src_length, byte_t *mask, int64_t offset) {
    int64_t i = offset, n = src_length + offset;
    if (dst == NULL || src == NULL || mask == NULL) {
        return false;
    }
    if (dst_length < src_length) {
        return false;
    }

    src_length = (n / 4) * 4; // Adjust n to ensure we don't go out of bounds
    for (; i < src_length; i += 4, src += 4, dst += 4) {
        dst[0] = src[0] ^ mask[i & 0x03];
        dst[1] = src[1] ^ mask[(i + 1) & 0x03];
        dst[2] = src[2] ^ mask[(i + 2) & 0x03];
        dst[3] = src[3] ^ mask[(i + 3) & 0x03];
    }

    for (; i < n; i++, src++, dst++) {
        *dst = *src ^ mask[i & 0x03];
    }
    return true;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_ws_reverse_header(byte_t *header, byte_t operation, int64_t length, byte_t *mask) {
    int32_t offset = 0;
    byte_t m = (mask != NULL) ? 0x80 : 0x00;
   
    if (m) {
        header[--offset] = mask[3] = (byte_t)(rand() & 0xff);
        header[--offset] = mask[2] = (byte_t)(rand() & 0xff);
        header[--offset] = mask[1] = (byte_t)(rand() & 0xff);
        header[--offset] = mask[0] = (byte_t)(rand() & 0xff);
    }

    if (length < 126) {
        header[--offset] = (length & 0xFF) | m;
    } else if (length < 0xFFFF) {
        header[--offset] = (byte_t)(length & 0xFF);
        header[--offset] = (byte_t)(length >> 8 & 0xFF);
        header[--offset] = (byte_t)(0x7E | m);
    } else {
        header[--offset] = (byte_t)(length & 0xFF);
        header[--offset] = (byte_t)(length >> 8);
        header[--offset] = (byte_t)(length >> 16);
        header[--offset] = (byte_t)(length >> 24);
        header[--offset] = (byte_t)(length >> 32);
        header[--offset] = (byte_t)(length >> 40);
        header[--offset] = (byte_t)(length >> 48);
        header[--offset] = (byte_t)(length >> 56);
        header[--offset] = (byte_t)(0x7E | m);
    }

    header[--offset] = (operation == 0) ? 0x00 : (0x80 | operation);

    return (-1 * offset);
}
/**/
_CC_API_PUBLIC(int32_t) _cc_ws_header(byte_t *header, byte_t operation, int64_t length, byte_t *mask) {
    int32_t offset = 1;
    byte_t m = (mask != NULL) ? 0x80 : 0x00;
    header[0] = (operation == 0) ? 0x00 : (0x80 | operation);

    if (length < 126) {
        header[1] = (length & 0xFF) | m;
        offset = 2;
    } else if (length < 0xFFFF) {
        header[1] = (byte_t)(0x7E | m);
        header[2] = (byte_t)(length >> 8 & 0xFF);
        header[3] = (byte_t)(length & 0xFF);
        offset = 4;
    } else {
        header[1] = (byte_t)(0x7E | m);
        header[2] = (byte_t)(length >> 56);
        header[3] = (byte_t)(length >> 48);
        header[4] = (byte_t)(length >> 40);
        header[5] = (byte_t)(length >> 32);
        header[6] = (byte_t)(length >> 24);
        header[7] = (byte_t)(length >> 16);
        header[8] = (byte_t)(length >> 8);
        header[9] = (byte_t)(length & 0xFF);

        offset = 10;
    }

    if (m) {
        header[offset++] = mask[0] = (byte_t)(rand() & 0xff);
        header[offset++] = mask[1] = (byte_t)(rand() & 0xff);
        header[offset++] = mask[2] = (byte_t)(rand() & 0xff);
        header[offset++] = mask[3] = (byte_t)(rand() & 0xff);
    }

    return offset;
}

/**/
_CC_API_PUBLIC(int32_t) _cc_ws_header_parser(_cc_ws_header_t *header, byte_t *bytes, int32_t length) {
    int64_t payload;
    int32_t offset = 2;
    //unused flag
    //rsv1 = (bytes[0] & 0x40);
    //rsv2 = (bytes[0] & 0x20);
    //rsv3 = (bytes[0] & 0x10);
    header->fragment = bytes[0];
    header->operation = (bytes[0] & 0x0F);
    header->mask = (bytes[1] & 0x80);
    header->state = WS_HEADER_PARTIAL;
    header->payload = 0;

    payload = (bytes[1] & 0x7F);

    if (payload == 0x7E) {
        offset = 4;
        if (length < offset) {
            return 0;
        }
        payload = (bytes[2] & 0xFF) << 8 | (bytes[3] & 0xFF);
    } else if (payload == 0x7F) {
        offset = 10;
        if (length < offset) {
            return 0;
        }
        payload = ((uint64_t)bytes[2] << 56) |
                  ((uint64_t)bytes[3] << 48) |
                  ((uint64_t)bytes[4] << 40) |
                  ((uint64_t)bytes[5] << 32) |
                  ((uint64_t)bytes[6] << 24) |
                  ((uint64_t)bytes[7] << 16) |
                  ((uint64_t)bytes[8] << 8)  |
                  (uint64_t)bytes[9];
    }

    if (header->mask == 0x80) {
        if ((offset + _WS_MASK_SIZE_) > length) {
            return 0;
        }
        memcpy(header->hash, &bytes[offset], _WS_MASK_SIZE_);
        offset += _WS_MASK_SIZE_;
    }

    header->payload = payload;

    if (((payload + offset) <= (int64_t)length)) {
        header->state = WS_DATA_OK;
    } else {
        header->state = WS_DATA_PARTIAL;
    }

    return offset;
}