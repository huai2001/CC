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

bool_t _cc_write_TGA(const tchar_t *file_name, _cc_image_t *image) {
    TGAHeader_t imageHeader;
    TGAFooter_t imageFooter;

    void (*color_convert_format)(const pvoid_t sP, int32_t sN, pvoid_t dP) = NULL;
    byte_t *scan_lines = NULL;
    byte_t *row = NULL;
    int32_t y = 0;
    uint32_t row_stride = 0;
    size_t row_size = 0;

    _cc_file_t *wfp = NULL;

    scan_lines = image->data;
    if (scan_lines == NULL) {
        return false;
    }

    bzero(&imageHeader, sizeof(TGAHeader_t));
    // top left of image is the top. the image loader needs to
    // be fixed to only swap/flip
    imageHeader.ImageDescriptor = (1 << 5);

    switch (image->format) {
    case CF_R8G8B8:
        color_convert_format = _cc_convert_R8G8B8_to_R8G8B8;
        imageHeader.PixelDepth = 24;
        imageHeader.ImageDescriptor |= 0;
        break;
    case CF_A8R8G8B8:
        color_convert_format = _cc_convert_A8R8G8B8_to_A8R8G8B8;
        imageHeader.PixelDepth = 32;
        imageHeader.ImageDescriptor |= 8;
        break;
    case CF_A1R5G5B5:
        color_convert_format = _cc_convert_A1R5G5B5_to_A1R5G5B5;
        imageHeader.PixelDepth = 16;
        imageHeader.ImageDescriptor |= 1;
        break;
    case CF_R5G6B5:
        color_convert_format = _cc_convert_R5G6B5_to_A1R5G5B5;
        imageHeader.PixelDepth = 16;
        imageHeader.ImageDescriptor |= 1;
        break;
    }

    // couldn't find a color converter
    if (!color_convert_format)
        return false;

    imageHeader.IdLength = 0;
    imageHeader.ColorMapType = 0;
    imageHeader.ImageType = 2;
    imageHeader.FirstEntryIndex[0] = 0;
    imageHeader.FirstEntryIndex[1] = 0;
    imageHeader.ColorMapLength = 0;
    imageHeader.ColorMapEntrySize = 0;
    imageHeader.XOrigin[0] = 0;
    imageHeader.XOrigin[1] = 0;
    imageHeader.YOrigin[0] = 0;
    imageHeader.YOrigin[1] = 0;
    imageHeader.Width = image->width;
    imageHeader.Height = image->height;


    // size of one pixel in bytes
    // length of one row of the source image in bytes
    row_stride = (image->channel * imageHeader.Width);

    // length of one output row in bytes
    row_size = ((imageHeader.PixelDepth / 8) * imageHeader.Width);

    row = (byte_t*)_cc_malloc(row_size);
    if (row == NULL) {
        return false;
    }

    wfp = _cc_open_file(file_name, _T("wb"));
    if (wfp == NULL) {
        _cc_free(row);
        return false;
    }
    _cc_file_write(wfp, &imageHeader, sizeof(TGAHeader_t), 1);

    // convert the image to 24-bit BGR and flip it over
    for (y = 0; y < imageHeader.Height; ++y) {
        if (image->format == CF_R8G8B8)
            _cc_color_24bit_to_24bit(&scan_lines[y * row_stride], row, imageHeader.Width, 1, 0, false, true);
        else
            // source, length [pixels], destination
            color_convert_format(&scan_lines[y * row_stride], imageHeader.Width, row);

        if (_cc_file_write(wfp, row, sizeof(byte_t), row_size) < row_size) {
            break;
        }
    }
    // clean up our scratch area
    _cc_free(row);

    imageFooter.ExtensionOffset = 0;
    imageFooter.DeveloperOffset = 0;
    strncpy(imageFooter.Signature, "trueVISION-XFILE.", 18);

    if (_cc_file_write(wfp, &imageFooter, sizeof(imageFooter), 1) != 1) {
        _cc_file_close(wfp);
        return false;
    }

    _cc_file_close(wfp);
    return imageHeader.Height <= y;
}






















