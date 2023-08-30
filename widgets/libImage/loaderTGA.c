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
#include "header.h"
#include "image.h"


byte_t* loadCompressedTGA( TGAHeader_t* tgaHeader, const byte_t* data) {
    uint32_t pixel = 0, elementCounter = 0;
    int32_t cb = 0, offset = 0, dataSize;
    byte_t chunk = 0, *tgaData = NULL, counter = 0;

    _cc_assert( tgaHeader != NULL && data != NULL);
    if (tgaHeader == NULL || data == NULL)
        return NULL;

    pixel = tgaHeader->PixelDepth / 8;
    dataSize = tgaHeader->Height * tgaHeader->Width * pixel;
    /*Fill in the data*/
    tgaData = _cc_malloc(dataSize * sizeof(byte_t));
    _cc_assert( tgaData != NULL );
    if (!tgaData) {
        return NULL;
    }

    while (cb < dataSize) {
        chunk = 0;
        /*Read The Chunk's Header*/
        memcpy(&chunk, data, sizeof(byte_t));
        data += sizeof(byte_t);

        /*If The Chunk Is A 'RAW' Chunk*/
        if (chunk < 128) {
            /*Add 1 To The Value To Get Total Number Of Raw Pixels*/
            chunk++;
            memcpy(&tgaData[cb], data, pixel * chunk * sizeof(byte_t));
            data += pixel * chunk;
            cb += pixel * chunk;
        } else {
            /*
             thnx to neojzs for some fixes with this code

             If It's An RLE Header
            */
            /*Subtract 127 To Get Rid Of The ID Bit*/
            chunk -= 127;
            offset = cb;
            memcpy(&tgaData[offset], data, pixel * sizeof(byte_t));
            cb += pixel;
            data += pixel;

            for (counter = 1; counter < chunk; counter++) {
                for (elementCounter = 0; elementCounter < pixel; elementCounter++)
                    tgaData[cb + elementCounter] = tgaData[offset + elementCounter];

                cb += pixel;
            }
        }
    }
    return tgaData;
}

_cc_image_t* _cc_load_TGA(const byte_t *data, uint32_t size) {
    _cc_image_t* image = NULL;
    byte_t *data_ptr = (byte_t *)data;
    int32_t ColorMapsize = 0, dataSize = 0;
    uint32_t *PaletteData = NULL;
    byte_t *ColorMap = NULL, *tgaData = NULL;

    TGAHeader_t tgaHeader;
    _cc_assert(data != NULL);
    if (data == NULL) {
        return NULL;
    }

    tgaHeader.IdLength = *(byte_t*)data_ptr++;
    tgaHeader.ColorMapType = *(byte_t*)data_ptr++;
    tgaHeader.ImageType = *(byte_t*)data_ptr++;
    tgaHeader.FirstEntryIndex[0] = *(byte_t*)data_ptr++;
    tgaHeader.FirstEntryIndex[1] = *(byte_t*)data_ptr++;
    __BYTE_SWAP_16(tgaHeader.ColorMapLength, data_ptr);
    tgaHeader.ColorMapEntrySize = *(byte_t*)data_ptr++;
    tgaHeader.XOrigin[0] = *(byte_t*)data_ptr++;
    tgaHeader.XOrigin[1] = *(byte_t*)data_ptr++;
    tgaHeader.YOrigin[0] = *(byte_t*)data_ptr++;
    tgaHeader.YOrigin[1] = *(byte_t*)data_ptr++;
    __BYTE_SWAP_16(tgaHeader.Width, data_ptr);
    __BYTE_SWAP_16(tgaHeader.Height, data_ptr);
    tgaHeader.PixelDepth = *(byte_t*)data_ptr++;
    tgaHeader.ImageDescriptor = *(byte_t*)data_ptr++;

    /*skip image identification field*/
    if (tgaHeader.IdLength)
        data_ptr += tgaHeader.IdLength;

    if (tgaHeader.ColorMapType) {
        PaletteData = (uint32_t*)_cc_malloc(sizeof(uint32_t) * tgaHeader.ColorMapLength);
        _cc_assert( PaletteData != NULL );
        if (PaletteData == NULL) {
            return NULL;
        }
        ColorMapsize = sizeof(byte_t) * (tgaHeader.ColorMapEntrySize / 8 * tgaHeader.ColorMapLength);
        ColorMap = (byte_t*)_cc_malloc(ColorMapsize);
        _cc_assert( ColorMap != NULL );
        if (ColorMap == NULL) {
            return NULL;
        }
        memcpy(ColorMap, data_ptr, ColorMapsize);
        data_ptr += ColorMapsize;

        // convert to 32-bit palette
        switch ( tgaHeader.ColorMapEntrySize ) {
        case 16:
            _cc_convert_A1R5G5B5_to_A8R8G8B8(ColorMap, tgaHeader.ColorMapLength, PaletteData);
            break;
        case 24:
            _cc_convert_B8G8R8_to_A8R8G8B8(ColorMap, tgaHeader.ColorMapLength, PaletteData);
            break;
        case 32:
            _cc_convert_B8G8R8A8_to_A8R8G8B8(ColorMap, tgaHeader.ColorMapLength, PaletteData);
            break;
        }
        _cc_free(ColorMap);
    }
    if (    tgaHeader.ImageType == 1 || // Uncompressed, color-mapped images.
            tgaHeader.ImageType == 2 || // Uncompressed, RGB images
            tgaHeader.ImageType == 3 // Uncompressed, black and white images
       ) {
        dataSize = tgaHeader.Height * tgaHeader.Width * tgaHeader.PixelDepth / 8;
        tgaData = (byte_t*)_cc_malloc(dataSize * sizeof(byte_t));

        if (tgaData == NULL)
            return NULL;

        memcpy(tgaData, data_ptr, dataSize * sizeof(byte_t));
        data_ptr += dataSize;

    } else if (tgaHeader.ImageType == 10) {
        // Runlength encoded RGB images
        tgaData = loadCompressedTGA(&tgaHeader, data_ptr);
    } else {
        _cc_logger_error(_T("loadTGA: Unsupported TGA file type"));
        _cc_free(PaletteData);
        return NULL;
    }

    switch (tgaHeader.PixelDepth) {
    case 8:
        if (tgaHeader.ImageType == 3) {
            image = _cc_init_image(CF_R8G8B8, tgaHeader.Width, tgaHeader.Height);
            if (image == NULL) {
                _cc_free(tgaData);
                _cc_free(PaletteData);
                return NULL;
            }
            _cc_color_8bit_to_24bit(tgaData, image->data, tgaHeader.Width, tgaHeader.Height,
                                    0, false, (tgaHeader.ImageDescriptor & 0x20) == 0);
        } else {
            image = _cc_init_image(CF_A1R5G5B5, tgaHeader.Width, tgaHeader.Height);
            if (image == NULL) {
                _cc_free(tgaData);
                _cc_free(PaletteData);
                return NULL;
            }
            _cc_color_8bit_to_16bit(tgaData, (int16_t*)image->data, tgaHeader.Width, tgaHeader.Height,
                                    PaletteData, false, (tgaHeader.ImageDescriptor & 0x20) == 0);
        }

        break;
    case 16:
        image = _cc_init_image(CF_A1R5G5B5, tgaHeader.Width, tgaHeader.Height);
        if (image == NULL) {
            _cc_free(tgaData);
            _cc_free(PaletteData);
            return NULL;
        }
        _cc_color_16bit_to_16bit((int16_t*)tgaData, (int16_t*)image->data, tgaHeader.Width, tgaHeader.Height,
                                 false, (bool_t)(tgaHeader.ImageDescriptor & 0x20) == 0);
        break;
    case 24:
        image = _cc_init_image(CF_R8G8B8, tgaHeader.Width, tgaHeader.Height);
        if (image == NULL) {
            _cc_free(tgaData);
            _cc_free(PaletteData);
            return NULL;
        }
        _cc_color_24bit_to_24bit(tgaData, image->data, tgaHeader.Width, tgaHeader.Height, 0,
                                 (tgaHeader.ImageDescriptor & 0x20) == 0, true);
        break;
    case 32:
        image = _cc_init_image(CF_A8R8G8B8, tgaHeader.Width, tgaHeader.Height);
        if (image == NULL) {
            _cc_free(tgaData);
            _cc_free(PaletteData);
            return NULL;
        }
        _cc_color_32bit_to_32bit((int32_t*)tgaData, (int32_t*)image->data, tgaHeader.Width, tgaHeader.Height,
                                 false, (bool_t)(tgaHeader.ImageDescriptor & 0x20) == 0);
        break;
    default:
        _cc_logger_error(_T("loadTGA: Unsupported TGA format"));
        break;
    }

    _cc_safe_free(tgaData);

    image->palette.data = PaletteData;
    image->palette.size = tgaHeader.ColorMapLength;

    return image;
}
