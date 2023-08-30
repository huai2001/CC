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

bool_t _cc_write_PCX(const tchar_t *file_name, _cc_image_t *image) {
    PCXHeader_t imageHeader;
    byte_t cnt, value;
    uint32_t y = 0, x = 0, k = 0;

    _cc_file_t *wfp = NULL;

    bzero(&imageHeader, sizeof(PCXHeader_t));

    imageHeader.Manufacturer = 10;
    imageHeader.Version = 5;
    imageHeader.Encoding = 1;
    imageHeader.BitsPerPixel = 8;
    imageHeader.XMin = 0;
    imageHeader.YMin = 0;
    imageHeader.Planes = 3;

    imageHeader.BytesPerLine = image->width;
    if (imageHeader.BytesPerLine & 0x0001)
        ++imageHeader.BytesPerLine;

#if __BIG_ENDIAN__
    imageHeader.XMax = _cc_swap16(iimage->width - 1);
    imageHeader.YMax = _cc_swap16(image->height - 1);
    imageHeader.HorizDPI = _cc_swap16(300);
    imageHeader.VertDPI = _cc_swap16(300);
    imageHeader.BytesPerLine = _cc_swap16(imageHeader.BytesPerLine);
    imageHeader.PaletteType = _cc_swap16(1);
    imageHeader.HScrsize = _cc_swap16(800);
    imageHeader.VScrsize = _cc_swap16(600);

#else
    imageHeader.XMax = image->width - 1;
    imageHeader.YMax = image->height - 1;
    imageHeader.HorizDPI = 300;
    imageHeader.VertDPI = 300;
    imageHeader.PaletteType = 1;
    imageHeader.HScrsize = 800;
    imageHeader.VScrsize = 600;
#endif


    wfp = _cc_open_file(file_name, _T("wb"));
    if (wfp == NULL) {
        return false;
    }
    _cc_file_write(wfp, &imageHeader, sizeof(PCXHeader_t), 1);

    // convert the image to 24-bit BGR and flip it over
    for (y = 0; y < image->height; y++) {
        cnt = 0;
        value = 0;

        for (k = 0; k < 3; ++k) {
            for (x = 0; x < image->width; ++x) {
                uint32_t pix = _cc_get_pixel(image, x, y);
                if (cnt != 0 && cnt < 63 && (
                            (k == 0 && (value == colorR(pix))) ||
                            (k == 1 && (value == colorG(pix))) ||
                            (k == 2 && (value == colorB(pix)))
                        )) {
                    ++cnt;
                } else {
                    if (cnt != 0) {
                        if (cnt > 1 || (value & 0xc0) == 0xc0) {
                            cnt |= 0xc0;
                            _cc_file_write(wfp, &cnt, sizeof(byte_t), 1);
                        }
                        _cc_file_write(wfp, &value, sizeof(byte_t), 1);
                    }
                    cnt = 1;
                    if (k == 0)
                        value = (byte_t)colorR(pix);
                    else if (k == 1)
                        value = (byte_t)colorG(pix);
                    else if (k == 2)
                        value = (byte_t)colorB(pix);
                }
            }
        }
        if (cnt > 1 || (value & 0xc0) == 0xc0) {
            cnt |= 0xc0;
            _cc_file_write(wfp, &cnt, sizeof(byte_t), 1);
        }
        _cc_file_write(wfp, &value, sizeof(byte_t), 1);

    }

    return true;
}






















