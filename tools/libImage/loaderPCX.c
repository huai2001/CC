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

_cc_image_t* _cc_load_PCX(const byte_t *data, uint32_t size) {
    _cc_image_t* image = nullptr;
    byte_t *dataPtr = (byte_t*)data, *tempPalette;
    uint32_t *PaletteData = nullptr;
    int32_t PaletteSize = 0;
    int32_t i = 0 , width = 0, height = 0, dataSize = 0;
    byte_t *pcxData = nullptr;
    PCXHeader_t pcxHeader;
    byte_t cnt, value;
    int32_t offset = 0, lineoffset = 0, linestart = 0, nextmode = 1, pad = 0;

    _cc_assert(data != nullptr);
    if (data == nullptr) {
        return nullptr;
    }

    pcxHeader.Manufacturer = *(byte_t*)dataPtr++;
    pcxHeader.Version = *(byte_t*)dataPtr++;
    pcxHeader.Encoding = *(byte_t*)dataPtr++;
    pcxHeader.BitsPerPixel = *(byte_t*)dataPtr++;

    __BYTE_SWAP_16(pcxHeader.XMin, dataPtr);
    __BYTE_SWAP_16(pcxHeader.YMin, dataPtr);
    __BYTE_SWAP_16(pcxHeader.XMax, dataPtr);
    __BYTE_SWAP_16(pcxHeader.YMax, dataPtr);
    __BYTE_SWAP_16(pcxHeader.HorizDPI, dataPtr);
    __BYTE_SWAP_16(pcxHeader.VertDPI, dataPtr);

    memcpy(pcxHeader.Palette, dataPtr, sizeof(pcxHeader.Palette));
    dataPtr += sizeof(pcxHeader.Palette);

    pcxHeader.Reserved = *(byte_t*)dataPtr++;
    pcxHeader.Planes = *(byte_t*)dataPtr++;

    __BYTE_SWAP_16(pcxHeader.BytesPerLine, dataPtr);
    __BYTE_SWAP_16(pcxHeader.PaletteType, dataPtr);
    __BYTE_SWAP_16(pcxHeader.HScrsize, dataPtr);
    __BYTE_SWAP_16(pcxHeader.VScrsize, dataPtr);

    memcpy(pcxHeader.Filler, dataPtr, sizeof(pcxHeader.Filler));
    dataPtr += sizeof(pcxHeader.Filler);

    //! return if the header is wrong
    if (pcxHeader.Manufacturer != 0x0a && pcxHeader.Encoding != 0x01) {
        return nullptr;
    }
    // return if this isn't a supported type
    if ((pcxHeader.BitsPerPixel != 8) && (pcxHeader.BitsPerPixel != 4) && (pcxHeader.BitsPerPixel != 1)) {
        _cc_logger_error(_T("Unsupported bits per pixel in PCX file."));
        return nullptr;
    }

    // read palette
    if ( (pcxHeader.BitsPerPixel == 8) && (pcxHeader.Planes == 1) ) {
        // the palette indicator (usually a 0x0c is found infront of the actual palette data)
        // is ignored because some exporters seem to forget to write it. This would result in
        // no image loaded before, now only wrong colors will be set.
        tempPalette = (byte_t *)_cc_malloc(sizeof(byte_t) * 768);
        PaletteSize = 256;
        PaletteData = (uint32_t *)_cc_malloc(sizeof(uint32_t) * PaletteSize);
        bzero(PaletteData, PaletteSize * sizeof(int32_t));
        memcpy(tempPalette, data + ((size - 768)), 768);
        for ( i = 0; i < 256;  i++ ) {
            PaletteData[i] = ((tempPalette[i * 3 + 0] << 16) |
                              (tempPalette[i * 3 + 1] << 8) |
                              (tempPalette[i * 3 + 2] ));
        }

        _cc_free(tempPalette);
    } else if ( pcxHeader.BitsPerPixel == 4 ) {
        PaletteSize = 16;
        PaletteData = (uint32_t *)_cc_malloc(sizeof(uint32_t) * PaletteSize);
        bzero(PaletteData, PaletteSize * sizeof(int32_t));
        for ( i = 0;  i < 16; i++ ) {
            PaletteData[i] = ((pcxHeader.Palette[i * 3 + 0] << 16) |
                              (pcxHeader.Palette[i * 3 + 1] << 8) |
                              (pcxHeader.Palette[i * 3 + 2]));
        }
    }
    // read image data
    width = pcxHeader.XMax - pcxHeader.XMin + 1;
    height = pcxHeader.YMax - pcxHeader.YMin + 1;
    dataSize = pcxHeader.BytesPerLine * pcxHeader.Planes * pcxHeader.BitsPerPixel * height / 8;
    pcxData = (byte_t *)_cc_malloc(sizeof(byte_t) * dataSize);
    if (pcxData == nullptr) {
        _cc_free(PaletteData);
        return nullptr;
    }


    for (offset = 0, lineoffset = 0, linestart = 0, nextmode = 1; offset < dataSize; offset += cnt) {
        cnt = *dataPtr++;
        if ( !((cnt & 0xc0) == 0xc0) ) {
            value = cnt;
            cnt = 1;
        } else {
            cnt &= 0x3f;
            value = *dataPtr++;
        }
        if (pcxHeader.Planes == 1)
            memset(pcxData + offset, value, cnt);
        else {
            for (i = 0; i < cnt; ++i) {
                pcxData[linestart + lineoffset] = value;
                lineoffset += 3;
                if (lineoffset >= (3 * pcxHeader.BytesPerLine)) {
                    lineoffset = nextmode;
                    if (++nextmode == 3)
                        nextmode = 0;
                    if (lineoffset == 0)
                        linestart += 3 * pcxHeader.BytesPerLine;
                }
            }
        }
    }

    pad = (pcxHeader.BytesPerLine - width * pcxHeader.BitsPerPixel / 8) * pcxHeader.Planes;

    if (pad < 0)
        pad = -pad;

    if (pcxHeader.BitsPerPixel == 8) {
        // TODO: Other formats
        switch (pcxHeader.Planes) {
        case 1:
            image = _cc_init_image(CF_A1R5G5B5, width, height);
            if (image == nullptr) {
                _cc_free(pcxData);
                _cc_free(PaletteData);
                return nullptr;
            }
            _cc_color_8bit_to_16bit((byte_t*)pcxData, (int16_t*)image->data, width, height, PaletteData, pad, false);
            break;
        case 3:
            image = _cc_init_image(CF_R8G8B8, width, height);
            if (image == nullptr) {
                _cc_free(pcxData);
                _cc_free(PaletteData);
                return nullptr;
            }
            _cc_color_24bit_to_24bit(pcxData, image->data, width, height, pad, false, false);
            break;
        }
    } else if (pcxHeader.BitsPerPixel == 4) {
        if (pcxHeader.Planes == 1) {
            image = _cc_init_image(CF_A1R5G5B5, width, height);
            if (image == nullptr) {
                _cc_free(pcxData);
                _cc_free(PaletteData);
                return nullptr;
            }
            _cc_color_4bit_to_16bit((byte_t*)pcxData, (int16_t*)image->data, width, height, PaletteData, pad, false);
        }
    } else if (pcxHeader.BitsPerPixel == 1) {
        if (pcxHeader.Planes == 4) {
            image = _cc_init_image(CF_A1R5G5B5, width, height);
            if (image == nullptr) {
                _cc_free(pcxData);
                _cc_free(PaletteData);
                return nullptr;
            }
            _cc_color_4bit_to_16bit((byte_t*)pcxData, (int16_t*)image->data, width, height, PaletteData, pad, false);
        } else if (pcxHeader.Planes == 1) {
            image = _cc_init_image(CF_A1R5G5B5, width, height);
            if (image == nullptr) {
                _cc_free(pcxData);
                _cc_free(PaletteData);
                return nullptr;
            }
            _cc_color_1bit_to_16bit((byte_t*)pcxData, (int16_t*)image->data, width, height, pad, false);
        }
    }

    image->palette.data = PaletteData;
    image->palette.size = PaletteSize;
    _cc_safe_free(pcxData);
    return image;
}
