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

bool_t _cc_write_BMP(const tchar_t *file_name, _cc_image_t *image) {
    BMPHeader_t imageHeader;
    void (*color_convert_format)(const pvoid_t sP, int32_t sN, pvoid_t dP) = nullptr;
    byte_t *scan_lines = nullptr;
    byte_t *row = nullptr;
    int32_t y = 0;
    uint32_t row_stride = 0;
    size_t row_size = 0;

    _cc_file_t *wfp = nullptr;

    scan_lines = image->data;
    if (scan_lines == nullptr) {
        return false;
    }

    bzero(&imageHeader, sizeof(BMPHeader_t));
    switch (image->format) {
    case CF_R8G8B8:
        color_convert_format = _cc_convert_R8G8B8_to_R8G8B8;
        break;
    case CF_A8R8G8B8:
        color_convert_format = _cc_convert_A8R8G8B8_to_B8G8R8;
        break;
    case CF_A1R5G5B5:
        color_convert_format = _cc_convert_A1R5G5B5_to_R8G8B8;
        break;
    case CF_R5G6B5:
        color_convert_format = _cc_convert_R5G6B5_to_R8G8B8;
        break;
    }

    // couldn't find a color converter
    if (!color_convert_format)
        return false;

    imageHeader.Id = 0x4d42;
    imageHeader.Reserved = 0;
    imageHeader.BitmapDataOffset = sizeof(BMPHeader_t);
    imageHeader.BitmapHeaderSize = 0x28;
    imageHeader.Width = image->width;
    imageHeader.Height = image->height;
    imageHeader.Planes = 1;
    imageHeader.BPP = 24;
    imageHeader.Compression = 0;
    imageHeader.PixelPerMeterX = 0;
    imageHeader.PixelPerMeterY = 0;
    imageHeader.Colors = 0;
    imageHeader.ImportantColors = 0;

    // data size is rounded up to next larger 4 bytes boundary
    imageHeader.BitmapDataSize = imageHeader.Width * imageHeader.BPP / 8;
    imageHeader.BitmapDataSize = (imageHeader.BitmapDataSize + 3) & ~3;
    imageHeader.BitmapDataSize *= imageHeader.Height;

    // file size is data size plus offset to data
    imageHeader.FileSize = imageHeader.BitmapDataOffset + imageHeader.BitmapDataSize;

    // size of one pixel in bytes
    // length of one row of the source image in bytes
    row_stride = (image->channel * imageHeader.Width);

    // length of one row in bytes, rounded up to nearest 4-byte boundary
    row_size = ((3 * imageHeader.Width) + 3) & ~3;

    row = (byte_t*)_cc_malloc(row_size);
    if (row == nullptr) {
        return false;
    }

    wfp = _cc_open_file(file_name, _T("wb"));
    if (wfp == nullptr) {
        _cc_free(row);
        return false;
    }

    _cc_file_write(wfp, &imageHeader, sizeof(BMPHeader_t), 1);

    // convert the image to 24-bit BGR and flip it over
    for (y = imageHeader.Height - 1; 0 <= y; --y) {
        if (image->format == CF_R8G8B8)
            _cc_color_24bit_to_24bit(&scan_lines[y * row_stride], row, imageHeader.Width, 1, 0, false, true);
        else
            // source, length [pixels], destination
            color_convert_format(&scan_lines[y * row_stride], imageHeader.Width, row);

        if (_cc_file_write(wfp, row, sizeof(byte_t), row_size) < row_size)
            break;
    }
    // clean up our scratch area
    _cc_free(row);

    _cc_file_close(wfp);

    return y < 0;
}






















