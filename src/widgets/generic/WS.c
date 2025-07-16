/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
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
#include <libcc/widgets/WS.h>

/**/
_CC_API_PUBLIC(void) _WSMask(byte_t *data, int64_t length, byte_t *mask) {
    uint64_t i;
    for (i = 0; i < length; ++i) {
        *(data + i) ^= *(mask + (i & 0x03));
    }
}

/**/
_CC_API_PUBLIC(int32_t) _WSHeader(byte_t *header, byte_t operation, int64_t length, byte_t *mask) {
    int32_t offset = 1;
    byte_t m = (mask != nullptr) ? 0x80 : 0x00;
    header[0] = (operation == 0) ? 0x00 : (0x80 | operation);

    if (length < 126) {
        header[1] = (length & 0xFF) | m;
        offset = 2;
    } else if (length < 0xFFFF) {
        header[1] = 0x7E | m;
        header[2] = (length >> 8 & 0xFF);
        header[3] = (length & 0xFF);
        offset = 4;
    } else {
        header[1] = 0x7F | m;
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
_CC_API_PUBLIC(int) _WSRead(_WSHeader_t *header) {
    byte_t mask;
    byte_t *buf = &header->bytes[header->offset];
    int64_t payload;
    int64_t offset = 2;
    int64_t length = (header->length - header->offset);

    header->payload = 0;
    header->mask = nullptr;

    header->fin = (buf[0] >> 7) & 0x1;
    header->rsv = (buf[0] >> 4) & 0x07;
    header->opc = (buf[0] & 0x0F);

    mask = buf[1];
    payload = mask & 0x7F;

    if (payload == 126) {
        offset = 4;
        if (length < offset) {
            return WS_DATA_PARTIAL;
        }
        payload = (buf[2] & 0xFF) << 8 | (buf[3] & 0xFF);
    } else if (payload == 127) {
        offset = 10;
        if (length < offset) {
            return WS_DATA_PARTIAL;
        }
        payload = ((uint64_t)buf[2] << 56) | 
                  ((uint64_t)buf[3] << 48) |
                  ((uint64_t)buf[4] << 40) | 
                  ((uint64_t)buf[5] << 32) |
                  ((uint64_t)buf[6] << 24) | 
                  ((uint64_t)buf[7] << 16) |
                  ((uint64_t)buf[8] << 8)  | 
                  (uint64_t)buf[9];
    }

    if ((mask & 0x80) == 0x80) {
        header->mask = &buf[offset];
        offset += _WS_MASK_SIZE_;
    }

    header->payload = payload;

    if (((payload + offset) <= length)) {
        header->offset += offset;
        return WS_DATA_OK;
    }

    return WS_DATA_PARTIAL;
}