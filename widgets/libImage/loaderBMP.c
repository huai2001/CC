/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
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

static void decompress8BitRLE(byte_t** bmpData, int32_t size, int32_t width, int32_t height, int32_t pitch) {
    byte_t* p = (*bmpData);
    byte_t* newBmp = nullptr;
    byte_t* d = nullptr;
    byte_t* destEnd = newBmp + (width + pitch) * height;
    int32_t line = 0;
    byte_t count, readAdditional, i, color;

    newBmp = (byte_t*)_cc_malloc(sizeof(byte_t) * ((width + pitch) * height));
    d = newBmp;

    while ((*bmpData) - p < size && d < destEnd) {
        if (*p == 0) {
            ++p;

            switch (*p) {
            case 0: // end of line
                ++p;
                ++line;
                d = newBmp + (line * (width + pitch));
                break;
            case 1: // end of bmp
                _cc_free((*bmpData));
                (*bmpData) = newBmp;
                return;
            case 2:
                ++p; d += (byte_t) * p; // delta
                ++p; d += ((byte_t) * p) * (width + pitch);
                ++p;
                break;
            default: {
                // absolute mode
                count = (byte_t) * p; ++p;
                readAdditional = ((2 - (count % 2)) % 2);
                for (i = 0; i < count; ++i) {
                    *d = *p;
                    ++p;
                    ++d;
                }

                for (i = 0; i < readAdditional; ++i)
                    ++p;
            }
            }
        } else {
            count = (byte_t) * p; ++p;
            color = *p; ++p;
            for (i = 0; i < count; ++i) {
                *d = color;
                ++d;
            }
        }
    }

    _cc_free((*bmpData));
    (*bmpData) = newBmp;
}


void decompress4BitRLE(byte_t** bmpData, int32_t size, int32_t width, int32_t height, int32_t pitch) {
    int32_t linewidth = (width + 1) / 2 + pitch;
    byte_t* p = *bmpData;
    byte_t* newBmp = nullptr;
    byte_t* d = nullptr;
    byte_t* destEnd = newBmp + linewidth * height;
    byte_t x, y, count, readAdditional, i, color1, color2, readShift, mask, toSet;
    int32_t line = 0, shift = 4;

    newBmp = (byte_t*)_cc_malloc(sizeof(byte_t) * (linewidth * height));
    d = newBmp;

    while ((*bmpData) - p < size && d < destEnd) {
        if (*p == 0) {
            ++p;
            switch (*p) {
            case 0: // end of line
                ++p;
                ++line;
                d = newBmp + (line * linewidth);
                shift = 4;
                break;
            case 1: // end of bmp
                _cc_free((*bmpData));
                (*bmpData) = newBmp;
                return;
            case 2: {
                ++p;
                x = (byte_t) * p; ++p;
                y = (byte_t) * p; ++p;
                d += x / 2 + y * linewidth;
                shift = x % 2 == 0 ? 4 : 0;
            }
            break;
            default: {
                // absolute mode
                count = (byte_t) * p; ++p;
                readAdditional = ((2 - ((count) % 2)) % 2);
                readShift = 4;

                for (i = 0; i < count; ++i) {
                    color1 = (((byte_t) * p) >> readShift) & 0x0f;
                    readShift -= 4;
                    if (readShift < 0) {
                        ++*p;
                        readShift = 4;
                    }

                    mask = 0x0f << shift;
                    *d = (*d & (~mask)) | ((color1 << shift) & mask);

                    shift -= 4;
                    if (shift < 0) {
                        shift = 4;
                        ++d;
                    }

                }

                for (i = 0; i < readAdditional; ++i)
                    ++p;
            }
            }
        } else {
            count = (byte_t) * p;
            ++p;
            color1 = (byte_t) * p & 0x0f;
            color2 = ((byte_t) * p >> 4) & 0x0f;
            ++p;

            for (i = 0; i < count; ++i) {
                mask = 0x0f << shift;
                toSet = (shift == 0 ? color1 : color2) << shift;
                *d = (*d & (~mask)) | (toSet & mask);

                shift -= 4;
                if (shift < 0) {
                    shift = 4;
                    ++d;
                }
            }
        }
    }

    _cc_free((*bmpData));
    (*bmpData) = newBmp;
}

_cc_image_t* _cc_load_BMP(const byte_t *data, uint32_t size) {
    int32_t w, p;
    float t;
    byte_t *bmpData = nullptr, *dataPtr = (byte_t*)data;
    BMPHeader_t bmpHeader;

    int32_t PaletteSize, i;
    uint32_t *PaletteData = nullptr;

    _cc_image_t* image = nullptr;
    _cc_assert( data != nullptr );
    if (data == nullptr) {
        return nullptr;
    }

    bzero(&bmpHeader, sizeof(bmpHeader));
    __BYTE_SWAP_16(bmpHeader.Id, dataPtr);
    __BYTE_SWAP_32(bmpHeader.FileSize, dataPtr);
    __BYTE_SWAP_32(bmpHeader.Reserved, dataPtr);
    __BYTE_SWAP_32(bmpHeader.BitmapDataOffset, dataPtr);
    __BYTE_SWAP_32(bmpHeader.BitmapHeaderSize, dataPtr);
    __BYTE_SWAP_32(bmpHeader.Width, dataPtr);
    __BYTE_SWAP_32(bmpHeader.Height, dataPtr);
    __BYTE_SWAP_16(bmpHeader.Planes, dataPtr);
    __BYTE_SWAP_16(bmpHeader.BPP, dataPtr);
    __BYTE_SWAP_32(bmpHeader.Compression, dataPtr);
    __BYTE_SWAP_32(bmpHeader.BitmapDataSize, dataPtr);
    __BYTE_SWAP_32(bmpHeader.PixelPerMeterX, dataPtr);
    __BYTE_SWAP_32(bmpHeader.PixelPerMeterY, dataPtr);
    __BYTE_SWAP_32(bmpHeader.Colors, dataPtr);
    __BYTE_SWAP_32(bmpHeader.ImportantColors, dataPtr);

    if ( bmpHeader.Id != 0x4d42 ) {
        _cc_logger_error(_T("LoadBMP: only Windows-style BMP files supported"));
        return nullptr;
    }
    if ( bmpHeader.FileSize != size ) {
        _cc_logger_error(_T("LoadBMP: header size does not match file size"));
        return nullptr;
    }
    if ( bmpHeader.Compression > 2 ) {
        _cc_logger_error(_T("LoadBMP: only uncompressed BMP files supported"));
        return nullptr;
    }

    // adjust bitmap data size to dword boundary
    bmpHeader.BitmapDataSize += (4 - (bmpHeader.BitmapDataSize % 4)) % 4;
    /*
    sizeof(BMPHeader_t) - 2
    BMP files 54-byte header
    */
    PaletteSize = (bmpHeader.BitmapDataOffset - (sizeof(BMPHeader_t) - 2)) / 4;
    if (PaletteSize) {
        PaletteData = (uint32_t*)_cc_malloc(sizeof(uint32_t) * PaletteSize);
        _cc_assert( PaletteData != nullptr );
        if (PaletteData == nullptr) {
            return nullptr;
        }

        for (i = 0; i < PaletteSize; ++i) {
#ifdef __BIG_ENDIAN__
            PaletteData[i] = _cc_swap32(*(uint32_t *)dataPtr);
#else
            PaletteData[i] = (*(uint32_t *)dataPtr);
#endif
            dataPtr++;
        }
        dataPtr = (byte_t*)(data + bmpHeader.BitmapDataOffset);
    }

    if (!bmpHeader.BitmapDataSize) {
        bmpHeader.BitmapDataSize = (uint32_t)size - bmpHeader.BitmapDataOffset;
    }

    t = (float)(bmpHeader.Width * (bmpHeader.BPP / 8.0f));
    w = (int32_t)t;
    t -= w;
    if (t != 0.0f)
        ++w;

    p = ((4 - (w % 4)) % 4);
    bmpData = (byte_t *)_cc_malloc(sizeof(byte_t) * bmpHeader.BitmapDataSize);
    if (bmpData == nullptr) {
        return nullptr;
    }
    memcpy(bmpData, dataPtr, sizeof(byte_t) * bmpHeader.BitmapDataSize);

    switch (bmpHeader.Compression) {
    case 1:
        decompress8BitRLE(&bmpData, bmpHeader.BitmapDataSize, bmpHeader.Width, bmpHeader.Height, p);
        break;
    case 2:
        decompress4BitRLE(&bmpData, bmpHeader.BitmapDataSize, bmpHeader.Width, bmpHeader.Height, p);
        break;
    }

    switch (bmpHeader.BPP) {
    case 1:
        image = _cc_init_image(CF_A1R5G5B5, bmpHeader.Width, bmpHeader.Height);
        if (image == nullptr) {
            _cc_free(bmpData);
            return nullptr;
        }
        _cc_color_1bit_to_16bit(bmpData, (int16_t*)image->data, image->width, image->height, p, true);
        break;
    case 4:
        image = _cc_init_image(CF_A1R5G5B5, bmpHeader.Width, bmpHeader.Height);
        if (image == nullptr) {
            _cc_free(bmpData);
            return nullptr;
        }
        _cc_color_4bit_to_16bit(bmpData, (int16_t*)image->data, image->width, image->height, PaletteData, p, true);
        break;
    case 8:
        image = _cc_init_image(CF_A1R5G5B5, bmpHeader.Width, bmpHeader.Height);
        if (image == nullptr) {
            _cc_free(bmpData);
            return nullptr;
        }
        _cc_color_8bit_to_16bit(bmpData, (int16_t*)image->data, image->width, image->height, PaletteData, p, true);
        break;
    case 16:
        image = _cc_init_image(CF_A1R5G5B5, bmpHeader.Width, bmpHeader.Height);
        if (image == nullptr) {
            _cc_free(bmpData);
            return nullptr;
        }
        _cc_color_16bit_to_16bit((int16_t*)bmpData, (int16_t*)image->data, image->width, image->height, p, true);
        break;
    case 24:
        image = _cc_init_image(CF_R8G8B8, bmpHeader.Width, bmpHeader.Height);
        if (image == nullptr) {
            _cc_free(bmpData);
            return nullptr;
        }
        _cc_color_24bit_to_24bit(bmpData, image->data, image->width, image->height, p, true, true);
        break;
    case 32: // thx to Reinhard Ostermeier
        image = _cc_init_image(CF_A8R8G8B8, bmpHeader.Width, bmpHeader.Height);
        if (image == nullptr) {
            _cc_free(bmpData);
            return nullptr;
        }
        _cc_color_32bit_to_32bit((int32_t*)bmpData, (int32_t*)image->data, image->width, image->height, p, true);
        break;
    };

    if (image) {
        image->palette.data = PaletteData;
        image->palette.size = PaletteSize;
    }
    _cc_free(bmpData);

    return image;
}
