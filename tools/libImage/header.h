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

#ifndef _CC_IMAGE_HEADER_H_INCLUDED_
#define _CC_IMAGE_HEADER_H_INCLUDED_

#include <libcc/types.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

typedef struct _PCXHeader {
    byte_t Manufacturer;
    byte_t Version;
    byte_t Encoding;
    byte_t BitsPerPixel;
    int16_t XMin;
    int16_t YMin;
    int16_t XMax;
    int16_t YMax;
    int16_t HorizDPI;
    int16_t VertDPI;
    byte_t Palette[48];
    byte_t Reserved;
    byte_t Planes;
    int16_t BytesPerLine;
    int16_t PaletteType;
    int16_t HScrsize;
    int16_t VScrsize;
    byte_t Filler[54];
} PCXHeader_t;

typedef struct _BMPHeader {
    uint16_t Id;                    //    BM - Windows 3.1x, 95, NT, 98, 2000, ME, XP
    //    BA - OS/2 Bitmap Array
    //    CI - OS/2 Color Icon
    //    CP - OS/2 Color Pointer
    //    IC - OS/2 Icon
    //    PT - OS/2 Pointer
    uint32_t FileSize;
    uint32_t Reserved;
    uint32_t BitmapDataOffset;
    uint32_t BitmapHeaderSize;        // should be 28h for windows bitmaps or
    // 0Ch for OS/2 1.x or F0h for OS/2 2.x
    uint32_t Width;
    uint32_t Height;
    uint16_t Planes;
    uint16_t BPP;                    // 1: Monochrome bitmap
    // 4: 16 color bitmap
    // 8: 256 color bitmap
    // 16: 16bit (high color) bitmap
    // 24: 24bit (true color) bitmap
    // 32: 32bit (true color) bitmap

    uint32_t Compression;            // 0: none (Also identified by BI_RGB)
    // 1: RLE 8-bit / pixel (Also identified by BI_RLE4)
    // 2: RLE 4-bit / pixel (Also identified by BI_RLE8)
    // 3: Bitfields  (Also identified by BI_BITFIELDS)

    uint32_t BitmapDataSize;        // size of the bitmap data in bytes. This number must be rounded to the next 4 byte boundary.
    uint32_t PixelPerMeterX;
    uint32_t PixelPerMeterY;
    uint32_t Colors;
    uint32_t ImportantColors;
} BMPHeader_t;

typedef struct _TGAHeader {
    byte_t IdLength;
    byte_t ColorMapType;
    byte_t ImageType;
    byte_t FirstEntryIndex[2];
    int16_t ColorMapLength;
    byte_t ColorMapEntrySize;
    byte_t XOrigin[2];
    byte_t YOrigin[2];
    int16_t Width;
    int16_t Height;
    byte_t PixelDepth;
    byte_t ImageDescriptor;
} TGAHeader_t;

typedef struct _TGAFooter {
    uint32_t ExtensionOffset;
    uint32_t DeveloperOffset;
    char  Signature[18];
} TGAFooter_t;

typedef struct _PSDHeader {
    uint8_t signature [4];      // Always equal to 8BPS.
    uint16_t version;           // Always equal to 1
    byte_t reserved [6];        // Must be zero
    uint16_t channels;          // Number of any channels inc. alphas
    uint32_t height;            // Rows Height of image in pixel
    uint32_t width;             // Colums Width of image in pixel
    uint16_t depth;             // Bits/channel
    uint16_t mode;              // Color mode of the file (Bitmap/Grayscale..)
} PSDHeader_t;

#pragma pack()

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif



#endif /*_TINYGL_IMAGE_HEADER_H_INCLUDED_*/
