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
#include "color.h"
#include <string.h>

/* converts a monochrome bitmap to A1R5G5B5 data */
void _cc_color_1bit_to_16bit(const byte_t* in, int16_t* out, int32_t width, int32_t height, int32_t linepad, bool_t flip) {
    int32_t x, y, shift;

    if (!in || !out)
        return;

    if (flip)
        out += width * height;

    for (y = 0; y < height; ++y) {
        shift = 7;
        if (flip)
            out -= width;

        for (x = 0; x < width; ++x) {
            out[x] = *in >> shift & 0x01 ? (int16_t)0xffff : (int16_t)0x8000;

            if ((--shift) < 0) {/* 8 pixel done */
                shift = 7;
                ++in;
            }
        }

        if (shift != 7) /* width did not fill last byte */
            ++in;

        if (!flip)
            out += width;
        in += linepad;
    }
}

/* converts a 4 bit palettized image to A1R5G5B5 */
void _cc_color_4bit_to_16bit(const byte_t* in, int16_t* out, int32_t width, int32_t height, const uint32_t* palette, int32_t linepad, bool_t flip) {
    int32_t x, y, shift;

    if (!in || !out || !palette)
        return;

    if (flip)
        out += width * height;

    for (y = 0; y < height; ++y) {
        shift = 4;
        if (flip)
            out -= width;

        for (x = 0; x < width; ++x) {
            out[x] = _CC_X8R8G8B8_TO_A1R5G5B5(palette[(byte_t)((*in >> shift) & 0xf)]);

            if (shift == 0) {
                shift = 4;
                ++in;
            } else
                shift = 0;
        }

        if (shift == 0) /* odd width */
            ++in;

        if (!flip)
            out += width;
        in += linepad;
    }
}

/* converts a 8 bit palettized image into A1R5G5B5 */
void _cc_color_8bit_to_16bit(const byte_t* in, int16_t* out, int32_t width, int32_t height, const uint32_t* palette, int32_t linepad, bool_t flip) {
    int32_t x, y;

    if (!in || !out || !palette)
        return;

    if (flip)
        out += width * height;

    for (y = 0; y < height; ++y) {
        if (flip)
            out -= width; /* one line back */
        for (x = 0; x < width; ++x) {
            out[x] = _CC_X8R8G8B8_TO_A1R5G5B5(palette[(byte_t)(*in)]);
            ++in;
        }
        if (!flip)
            out += width;
        in += linepad;
    }
}
/* converts a 8 bit palettized or non palettized image (A8) into R8G8B8 */
void _cc_color_8bit_to_24bit(const byte_t* in, byte_t* out, int32_t width, int32_t height, const byte_t* palette, int32_t linepad, bool_t flip) {
    int32_t x, y;
    const int32_t lineWidth = 3 * width;

    if (!in || !out )
        return;

    if (flip)
        out += lineWidth * height;

    for (y = 0; y < height; ++y) {
        if (flip)
            out -= lineWidth; // one line back
        for (x = 0; x < lineWidth; x += 3) {
            if ( palette ) {
#ifdef __BIG_ENDIAN__
                out[x + 0] = palette[ (in[0] << 2 ) + 0];
                out[x + 1] = palette[ (in[0] << 2 ) + 1];
                out[x + 2] = palette[ (in[0] << 2 ) + 2];
#else
                out[x + 0] = palette[ (in[0] << 2 ) + 2];
                out[x + 1] = palette[ (in[0] << 2 ) + 1];
                out[x + 2] = palette[ (in[0] << 2 ) + 0];
#endif
            } else {
                out[x + 0] = in[0];
                out[x + 1] = in[0];
                out[x + 2] = in[0];
            }
            ++in;
        }
        if (!flip)
            out += lineWidth;

        in += linepad;
    }
}

/* converts a 8 bit palettized or non palettized image (A8) into R8G8B8 */
void _cc_color_8bit_to_32bit(const byte_t* in, byte_t* out, int32_t width, int32_t height, const byte_t* palette, int32_t linepad, bool_t flip) {

    int32_t x, y;
    uint32_t c;
    const uint32_t lineWidth = 4 * width;

    if (!in || !out )
        return;

    if (flip)
        out += lineWidth * height;

    for (y = 0; y < height; ++y) {
        if (flip)
            out -= lineWidth; // one line back

        if ( palette ) {
            for (x = 0; x < width; x++) {
                c = in[x];
                ((uint32_t*)out)[x] = ((uint32_t*)palette)[ c ];
            }
        } else {
            for (x = 0; x < width; x++) {
                c = in[x];
#ifdef __BIG_ENDIAN__
                ((uint32_t*)out)[x] = c << 24 | c << 16 | c << 8 | 0x000000FF;
#else
                ((uint32_t*)out)[x] = 0xFF000000 | c << 16 | c << 8 | c;
#endif
            }
        }

        if (!flip)
            out += lineWidth;

        in += width + linepad;
    }
}

/* converts 16bit data to 16bit data */
void _cc_color_16bit_to_16bit(const int16_t* in, int16_t* out, int32_t width, int32_t height, int32_t linepad, bool_t flip) {
#ifdef __BIG_ENDIAN__
    int32_t x = 0;
#endif
    int32_t y = 0;

    if (!in || !out)
        return;

    if (flip)
        out += width * height;

    for (y = 0; y < height; ++y) {
        if (flip)
            out -= width;

#ifdef __BIG_ENDIAN__
        for (x = 0; x < width; ++x)
            out[x] = _cc_swap16(in[x]);
#else
        memcpy(out, in, width * sizeof(int16_t));
#endif
        if (!flip)
            out += width;
        in += width;
        in += linepad;
    }
}

/* copies R8G8B8 24bit data to 24bit data */
void _cc_color_24bit_to_24bit(const byte_t* in, byte_t* out, int32_t width, int32_t height, int32_t linepad, bool_t flip, bool_t bgr) {
    const int32_t lineWidth = 3 * width;
    int32_t x = 0, y = 0;

    if (!in || !out)
        return;
    if (flip)
        out += lineWidth * height;

    for (y = 0; y < height; ++y) {
        if (flip)
            out -= lineWidth;
        if (bgr) {
            /*RGB*/
            for (x = 0; x < lineWidth; x += 3) {
                out[x + 0] = in[x + 2];
                out[x + 1] = in[x + 1];
                out[x + 2] = in[x + 0];
            }
        } else {
            /*BGR*/
            memcpy(out, in, lineWidth);
        }
        if (!flip)
            out += lineWidth;

        in += lineWidth;
        in += linepad;
    }
}



/* Resizes the gh to a new size and converts it at the same time
! to an A8R8G8B8 format, returning the pointer to the new buffer.*/
void _cc_color_16bit_to_A8R8G8B8(const int16_t* in, int32_t* out, int32_t newWidth, int32_t newHeight, int32_t currentWidth, int32_t currentHeight) {

    /* note: this is very very slow. (i didn't want to write a fast version.
       but hopefully, nobody wants to convert ghs every frame.*/

    float xr = (float)currentWidth / (float)newWidth;
    float yr = (float)currentHeight / (float)newHeight;
    float sy;
    int32_t x, y, t;
    if (!newWidth || !newHeight)
        return;

    for (x = 0; x < newWidth; ++x) {
        sy = 0.0f;
        for (y = 0; y < newHeight; ++y) {
            t = in[(int32_t)(((int32_t)sy) * currentWidth + x * xr)];
            t = (((t >> 15) & 0x1) << 31) |    (((t >> 10) & 0x1F) << 19) |
                (((t >> 5) & 0x1F) << 11) |    (t & 0x1F) << 3;
            out[(int32_t)(y * newWidth + x)] = t;
            sy += yr;
        }
    }
}

/* copies X8R8G8B8 32 bit data */
void _cc_color_32bit_to_32bit(const int32_t* in, int32_t* out, int32_t width, int32_t height, int32_t linepad, bool_t flip) {
#ifdef __BIG_ENDIAN__
    int32_t x = 0;
#endif
    int32_t y = 0;
    if (!in || !out)
        return;

    if (flip)
        out += width * height;

    for (y = 0; y < height; ++y) {
        if (flip)
            out -= width;

#ifdef __BIG_ENDIAN__
        for (x = 0; x < width; ++x)
            out[x] = _cc_swap32(in[x]);
#else
        memcpy(out, in, width * sizeof(int32_t));
#endif

        if (!flip)
            out += width;

        in += width;
        in += linepad;
    }
}

void _cc_convert_A1R5G5B5_to_R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint16_t* sB = (uint16_t*)sP;
    uint8_t * dB = (uint8_t *)dP;

    for (; x < sN; ++x) {
        dB[2] = (*sB & 0x7c00) >> 7;
        dB[1] = (*sB & 0x03e0) >> 2;
        dB[0] = (*sB & 0x1f) << 3;

        sB += 1;
        dB += 3;
    }
}

void _cc_convert_A1R5G5B5_to_B8G8R8(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint16_t* sB = (uint16_t*)sP;
    uint8_t * dB = (uint8_t *)dP;

    for (; x < sN; ++x) {
        dB[0] = (*sB & 0x7c00) >> 7;
        dB[1] = (*sB & 0x03e0) >> 2;
        dB[2] = (*sB & 0x1f) << 3;

        sB += 1;
        dB += 3;
    }
}

void _cc_convert_A1R5G5B5_to_A8R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint16_t* sB = (uint16_t*)sP;
    uint32_t* dB = (uint32_t*)dP;

    for (; x < sN; ++x) {
        *dB++ = _CC_A1R5G5B5_TO_A8R8G8B8(*sB);
        sB++;
    }
}

void _cc_convert_A1R5G5B5_to_A1R5G5B5(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    memcpy(dP, sP, sN * 2);
}

void _cc_convert_A1R5G5B5_to_R5G6B5(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint16_t* sB = (uint16_t*)sP;
    uint16_t* dB = (uint16_t*)dP;

    for (; x < sN; ++x) {
        *dB++ = _CC_A1R5G5B5_TO_R5G6B5(*sB);
        sB++;
    }
}

void _cc_convert_A8R8G8B8_to_R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint8_t* sB = (uint8_t*)sP;
    uint8_t* dB = (uint8_t*)dP;

    for (; x < sN; ++x) {
        // sB[3] is alpha
        dB[0] = sB[2];
        dB[1] = sB[1];
        dB[2] = sB[0];

        sB += 4;
        dB += 3;
    }
}

void _cc_convert_A8R8G8B8_to_B8G8R8(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint8_t* sB = (uint8_t*)sP;
    uint8_t* dB = (uint8_t*)dP;

    for (; x < sN; ++x) {
        // sB[3] is alpha
        dB[0] = sB[0];
        dB[1] = sB[1];
        dB[2] = sB[2];

        sB += 4;
        dB += 3;
    }
}

void _cc_convert_A8R8G8B8_to_A8R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    memcpy(dP, sP, sN * 4);
}

void _cc_convert_A8R8G8B8_to_A1R5G5B5(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint32_t* sB = (uint32_t*)sP;
    uint16_t* dB = (uint16_t*)dP;

    for (; x < sN; ++x) {

        dB[0] = _CC_A8R8G8B8_TO_A1R5G5B5(*sB);

        dB += 1;
    }
}

void _cc_convert_A8R8G8B8_to_R5G6B5(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint8_t * sB = (uint8_t *)sP;
    uint16_t* dB = (uint16_t*)dP;
    int32_t r, g, b;

    for (; x < sN; ++x) {
        r = sB[2] >> 3;
        g = sB[1] >> 2;
        b = sB[0] >> 3;

        dB[0] = (r << 11) | (g << 5) | (b);

        sB += 4;
        dB += 1;
    }
}

void _cc_convert_A8R8G8B8_to_R3G3B2(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint8_t* sB = (uint8_t*)sP;
    uint8_t* dB = (uint8_t*)dP;

    for (; x < sN; ++x) {

        dB[0] = ((sB[2] & 0xe0) | ((sB[1] & 0xe0) >> 3) | ((sB[0] & 0xc0) >> 6));

        sB += 4;
        dB += 1;
    }
}

void _cc_convert_R8G8B8_to_R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    memcpy(dP, sP, sN * 3);
}

void _cc_convert_R8G8B8_to_A8R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint8_t*  sB = (uint8_t* )sP;
    uint32_t* dB = (uint32_t*)dP;

    for (; x < sN; ++x) {
        *dB = 0xff000000 | (sB[0] << 16) | (sB[1] << 8) | sB[2];

        sB += 3;
        ++dB;
    }
}

void _cc_convert_R8G8B8_to_A1R5G5B5(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint8_t * sB = (uint8_t *)sP;
    uint16_t* dB = (uint16_t*)dP;
    int32_t r, g, b;


    for (; x < sN; ++x) {
        r = sB[0] >> 3;
        g = sB[1] >> 3;
        b = sB[2] >> 3;

        dB[0] = (0x8000) | (r << 10) | (g << 5) | (b);

        sB += 3;
        dB += 1;
    }
}

void _cc_convert_B8G8R8_to_A8R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint8_t*  sB = (uint8_t* )sP;
    uint32_t* dB = (uint32_t*)dP;

    for (; x < sN; ++x) {
        *dB = 0xff000000 | (sB[2] << 16) | (sB[1] << 8) | sB[0];

        sB += 3;
        ++dB;
    }
}

void _cc_convert_A8R8G8B8_to_A8B8G8R8(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    const uint32_t* sB = (const uint32_t*)sP;
    uint32_t* dB = (uint32_t*)dP;

    for (x = 0; x < sN; ++x) {
        *dB++ = (*sB & 0xff00ff00) | ((*sB & 0x00ff0000) >> 16) | ((*sB & 0x000000ff) << 16);
        ++sB;
    }
}

void _cc_convert_B8G8R8A8_to_A8R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint8_t* sB = (uint8_t*)sP;
    uint8_t* dB = (uint8_t*)dP;

    for (; x < sN; ++x) {
        dB[0] = sB[3];
        dB[1] = sB[2];
        dB[2] = sB[1];
        dB[3] = sB[0];

        sB += 4;
        dB += 4;
    }

}

void _cc_convert_R8G8B8_to_R5G6B5(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint8_t * sB = (uint8_t *)sP;
    uint16_t* dB = (uint16_t*)dP;

    for (; x < sN; ++x) {
        int32_t r = sB[0] >> 3;
        int32_t g = sB[1] >> 2;
        int32_t b = sB[2] >> 3;

        dB[0] = (r << 11) | (g << 5) | (b);

        sB += 3;
        dB += 1;
    }
}

void _cc_convert_R5G6B5_to_R5G6B5(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    memcpy(dP, sP, sN * 2);
}

void _cc_convert_R5G6B5_to_R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint16_t* sB = (uint16_t*)sP;
    uint8_t * dB = (uint8_t *)dP;

    for (; x < sN; ++x) {
        dB[0] = (*sB & 0xf800) >> 8;
        dB[1] = (*sB & 0x07e0) >> 3;
        dB[2] = (*sB & 0x001f) << 3;

        sB += 1;
        dB += 3;
    }
}

void _cc_convert_R5G6B5_to_B8G8R8(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint16_t* sB = (uint16_t*)sP;
    uint8_t * dB = (uint8_t *)dP;

    for (; x < sN; ++x) {
        dB[2] = (*sB & 0xf800) >> 8;
        dB[1] = (*sB & 0x07e0) >> 3;
        dB[0] = (*sB & 0x001f) << 3;

        sB += 1;
        dB += 3;
    }
}

void _cc_convert_R5G6B5_to_A8R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint16_t* sB = (uint16_t*)sP;
    uint32_t* dB = (uint32_t*)dP;

    for (; x < sN; ++x) {
        *dB++ = _CC_R5G6B5_TO_A8R8G8B8(*sB);
        sB++;
    }
}

void _cc_convert_R5G6B5_to_A1R5G5B5(const pvoid_t sP, int32_t sN, pvoid_t dP) {
    int32_t x = 0;
    uint16_t* sB = (uint16_t*)sP;
    uint16_t* dB = (uint16_t*)dP;

    for (; x < sN; ++x) {
        *dB++ = _CC_R5G6B5_TO_A1R5G5B5(*sB);
        sB++;
    }
}


void _cc_color_convert_via_format(const pvoid_t sp, uint32_t sformat, int32_t sn,
                                  pvoid_t dp, uint32_t dformat) {
    switch (sformat) {
    case CF_A1R5G5B5:
        switch (dformat) {
        case CF_A1R5G5B5:
            _cc_convert_A1R5G5B5_to_A1R5G5B5(sp, sn, dp);
            break;
        case CF_R5G6B5:
            _cc_convert_A1R5G5B5_to_R5G6B5(sp, sn, dp);
            break;
        case CF_A8R8G8B8:
            _cc_convert_A1R5G5B5_to_A8R8G8B8(sp, sn, dp);
            break;
        case CF_R8G8B8:
            _cc_convert_A1R5G5B5_to_R8G8B8(sp, sn, dp);
            break;
#ifndef _DEBUG
        default:
            break;
#endif
        }
        break;
    case CF_R5G6B5:
        switch (dformat) {
        case CF_A1R5G5B5:
            _cc_convert_R5G6B5_to_A1R5G5B5(sp, sn, dp);
            break;
        case CF_R5G6B5:
            _cc_convert_R5G6B5_to_R5G6B5(sp, sn, dp);
            break;
        case CF_A8R8G8B8:
            _cc_convert_R5G6B5_to_A8R8G8B8(sp, sn, dp);
            break;
        case CF_R8G8B8:
            _cc_convert_R5G6B5_to_R8G8B8(sp, sn, dp);
            break;
#ifndef _DEBUG
        default:
            break;
#endif
        }
        break;
    case CF_A8R8G8B8:
        switch (dformat) {
        case CF_A1R5G5B5:
            _cc_convert_A8R8G8B8_to_A1R5G5B5(sp, sn, dp);
            break;
        case CF_R5G6B5:
            _cc_convert_A8R8G8B8_to_R5G6B5(sp, sn, dp);
            break;
        case CF_A8B8G8R8:
            _cc_convert_A8R8G8B8_to_A8B8G8R8(sp, sn, dp);
            break;
        case CF_A8R8G8B8:
            _cc_convert_A8R8G8B8_to_A8R8G8B8(sp, sn, dp);
            break;
        case CF_R8G8B8:
            _cc_convert_A8R8G8B8_to_R8G8B8(sp, sn, dp);
            break;
#ifndef _DEBUG
        default:
            break;
#endif
        }
        break;
    case CF_R8G8B8:
        switch (dformat) {
        case CF_A1R5G5B5:
            _cc_convert_R8G8B8_to_A1R5G5B5(sp, sn, dp);
            break;
        case CF_R5G6B5:
            _cc_convert_R8G8B8_to_R5G6B5(sp, sn, dp);
            break;
        case CF_A8R8G8B8:
            _cc_convert_R8G8B8_to_A8R8G8B8(sp, sn, dp);
            break;
        case CF_R8G8B8:
            _cc_convert_R8G8B8_to_R8G8B8(sp, sn, dp);
            break;
#ifndef _DEBUG
        default:
            break;
#endif
        }
        break;
    }
}
