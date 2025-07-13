/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#include "header.h"
#include "image.h"

_CC_FORCE_INLINE_ int16_t get_shift_from_channel(byte_t channel_nr, uint16_t header_channels)  {
    switch (channel_nr) {
    case 0:
        return 16;  // red
    case 1:
        return 8;   // green
    case 2:
        return 0;   // blue
    case 3:
        return header_channels == 4 ? 24 : -1;    // ?
    case 4:
        return 24;  // alpha
    default:
        return -1;
    }
}

_CC_FORCE_INLINE_ bool_t readRaw(const byte_t *dataPtr, PSDHeader_t *header, byte_t *pixels) {
    int16_t shift;
    int32_t n24;
    byte_t c;
    int32_t channel;
    for (channel = 0; channel < header->channels && channel < 3; ++channel) {
        shift = get_shift_from_channel(channel, header->channels);
        if (shift != -1) {
            uint32_t x, y;
            uint32_t mask = 0xff << shift;

            for (x = 0; x < header->width; ++x) {
                for (y = 0; y < header->height; ++y) {
                    n24 = x + y * header->width;
                    c = *(dataPtr + n24);
                    pixels[n24] = ~(~c | mask);
                    pixels[n24] |= *(dataPtr + n24) << shift;
                }
            }
        } else {
            dataPtr += (header->width * header->height);
        }
    }
    return true;
}

_CC_FORCE_INLINE_ bool_t readRLE(const byte_t *dataPtr, PSDHeader_t *header, byte_t *pixels) {
    uint32_t y;
    uint16_t c;
    uint16_t *rle;
    size_t size;
    int32_t channel;
    byte_t *tmp;
    byte_t *dst;
    int8_t rh;
    int16_t shift;
    uint16_t *rle_ref;

    tmp = _cc_malloc(sizeof(uint8_t) * header->width * header->height);
    if (tmp == nullptr) {
        return false;
    }
    rle = _cc_malloc(sizeof(uint16_t) * header->height * header->channels);
    if (rle == nullptr) {
        _cc_free(tmp);
        return false;
    }
    size = 0;
    for (y = 0; y < header->height * header->channels; ++y) {
        __BYTE_SWAP_16(c, dataPtr);
        rle[y] = c;
        size += c;
    }

    rle_ref = rle;
    for (channel = 0; channel < header->channels; channel++) {
        for (y = 0; y < header->height; ++y, ++rle_ref) {
            c = 0;
            dst = &tmp[y * header->width];
            while (c < *rle_ref) {
                rh = *dataPtr++;
                if (rh >= 0) {
                    ++rh;
                    while (rh--) {
                        *dst = *dataPtr++;
                        ++dst;
                    }
                } else if (rh > -128) {
                    rh = -rh + 1;
                    while (rh--) {
                        *dst = *dataPtr;
                        ++dst;
                    }
                    ++dataPtr;
                    ++c;
                }
            }
        }
        shift = get_shift_from_channel(channel, header->channels);
        if (shift != -1) {
            uint32_t x;
            int32_t n24;
            uint32_t mask = 0xff << shift;

            for (x = 0; x < header->width; ++x) {
                for (y = 0; y < header->height; ++y) {
                    n24 = x + y * header->width;
                    c = *(tmp + n24);
                    pixels[n24] = ~(~c | mask);
                    pixels[n24] |= *(tmp + n24) << shift;
                }
            }
        }
    }
    _cc_free(tmp);
    _cc_free(rle);
    return true;
}

_cc_image_t* _cc_load_PSD(const byte_t *image_data, uint32_t image_size) {
    const byte_t *dataPtr;
    byte_t *pixels;
    PSDHeader_t header;
    bool_t res;
    uint32_t skip;
    uint16_t compression_type;
    _cc_assert(image_data != nullptr);
    if (image_data == nullptr) {
        return nullptr;
    }
    dataPtr = image_data;
    if (*dataPtr       != '8' ||
            *(dataPtr + 1) != 'B' ||
            *(dataPtr + 2) != 'P' ||
            *(dataPtr + 3) != 'S') {
        _cc_logger_error(_T("LoadPSD: Unsupported file"));
        return nullptr;
    }

    dataPtr += 4;
    __BYTE_SWAP_16(header.version, dataPtr);
    dataPtr += 6;
    __BYTE_SWAP_16(header.channels, dataPtr);
    __BYTE_SWAP_32(header.height, dataPtr);
    __BYTE_SWAP_32(header.width, image_data);
    __BYTE_SWAP_16(header.depth, image_data);
    __BYTE_SWAP_16(header.mode, image_data);

    if (header.version != 1) {
        _cc_logger_error(_T("LoadPSD: Unsupported PSD file version"));
        return nullptr;
    }

    if (header.mode != 3 || header.depth != 8) {
        _cc_logger_error(_T("Unsupported PSD color mode or depth"));
        return nullptr;
    }

    /*skip color mode data*/
    __BYTE_SWAP_32(skip, dataPtr);

    dataPtr += skip;
    if (dataPtr > (image_data + image_size)) {
        _cc_logger_error(_T("Error seeking file pos to image resources"));
        return nullptr;
    }

    /*skip image resources*/
    __BYTE_SWAP_32(skip, dataPtr);

    dataPtr += skip;
    if (dataPtr > (image_data + image_size)) {
        _cc_logger_error(_T("Error seeking file pos to layer and mask"));
        return nullptr;
    }

    /*skip layer & mask*/
    __BYTE_SWAP_32(skip, dataPtr);

    dataPtr += skip;
    if (dataPtr > (image_data + image_size)) {
        _cc_logger_error(_T("Error seeking file pos to image data section"));
        return nullptr;
    }
    /**/
    __BYTE_SWAP_32(compression_type, dataPtr);
    if (compression_type != 1 && compression_type != 0) {
        _cc_logger_error(_T("Unsupported psd compression mode"));
        return nullptr;
    }

    pixels = _cc_malloc(sizeof(uint32_t) * header.width * header.height);
    if (pixels == nullptr) {
        return nullptr;
    }

    if (compression_type == 0) {
        res = readRaw(dataPtr, &header, pixels);
    } else {
        res = readRLE(dataPtr, &header, pixels);
    }

    if (res) {
        return _cc_init_image_data(CF_A8R8G8B8, header.width, header.height, pixels);
    }

    return nullptr;
}
