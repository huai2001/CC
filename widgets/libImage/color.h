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

#ifndef _CC_IMAGE_COLOR_H_INCLUDED_
#define _CC_IMAGE_COLOR_H_INCLUDED_

#include <libcc/core.h>
#include <libcc/math.h>
#include <libcc/alloc.h>
#include <libcc/buf.h>


#ifdef __CC_ANDROID__
#include <android/asset_manager_jni.h>
#endif

#if defined(_CC_IMAGE_EXPORT_SHARED_LIBRARY_)
    #define _CC_IMAGE_API(t) _CC_API_EXPORT_ t
#else
    #define _CC_IMAGE_API(t) t
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t _cc_color_t;

typedef struct _cc_rgb {
    byte_t r;
    byte_t g;
    byte_t b;
}_cc_rgb_t;

typedef struct _cc_rgba {
    byte_t r;
    byte_t g;
    byte_t b;
    byte_t a;
}_cc_rgba_t;

typedef struct _cc_argb {
    byte_t a;
    byte_t r;
    byte_t g;
    byte_t b;
}_cc_argb_t;

typedef enum PIXELFORMAT {
    CF_Automatic    = 0,
    CF_A1R5G5B5     = 1,
    CF_B4G4R4A4,
    CF_B5G5R5A1,
    CF_R5G6B5,
    CF_R8G8B8,
    CF_A8R8G8B8,
    CF_A8B8G8R8,
    CF_R16F,
    CF_G16R16F,
    CF_A16B16G16R16F,
    CF_R32F,
    CF_G32R32F,
    CF_A32B32G32R32F,
    CF_L8,
    CF_A8,
    CF_L8A8,
    CF_RGB_PVRTC2,
    CF_RGB_PVRTC4,
    CF_RGBA_PVRTC2,
    CF_RGBA_PVRTC4
} PIXELFORMAT;

/* Unknown color format:*/
#define CF_UNKNOWN 0xFF

/* maps unsigned 8 bits/channel to D3DCOLOR */
#define _CC_A8R8G8B8(a,r,g,b) \
    ((uint32_t)((((a) & 0xFF) << 24) | \
    (((r) & 0xFF) << 16) | \
    (((g) & 0xFF) << 8) | ((b) & 0xFF)))

#define _CC_R8G8B8A8(a,r,g,b) \
    ((uint32_t)((((r) & 0xFF) << 24) | \
    (((g) & 0xFF) << 16) | \
    (((b) & 0xFF) << 8) | ((a) & 0xFF)))

#define _CC_R8G8B8(r, g, b) _CC_A8R8G8B8(0xFF, r, g, b)
/* defines for converting a byte to a float (0.0-1.0) and vice versa */
#define color_b2f(a) ( ((float)(a)) / 256.0f )
#define color_f2b(a) ( (byte_t)((a) * 255.0f) )

#define colorA(a) ((byte_t)((uint32_t)a >> 24) & 0xFF)
#define colorR(a) ((byte_t)((uint32_t)a >> 16) & 0xFF)
#define colorG(a) ((byte_t)((uint32_t)a >> 8) & 0xFF)
#define colorB(a) ((byte_t)((uint32_t)a & 0xFF))
    
#define colorAf(a) (color_b2f(colorA(a)))
#define colorRf(a) (color_b2f(colorR(a)))
#define colorGf(a) (color_b2f(colorG(a)))
#define colorBf(a) (color_b2f(colorB(a)))

/* maps floating point channel (0.f to 1.f range) to D3DCOLOR */
#define _CC_A8R8G8B8f(a,r,g,b) _CC_A8R8G8B8(\
        (color_f2b(a)),\
        (color_f2b(r)),\
        (color_f2b(g)),\
        (color_f2b(b))\
    )

/* Creates a 16 bit A1R5G5B5 color */
#define _CC_A1R5G5B5(a, r, g, b) \
    (uint16_t)((a & 0x80) << 8 | \
    (r & 0xF8) << 7 | \
    (g & 0xF8) << 2 | \
    (b & 0xF8) >> 3)

/* Creates a 16 bit A1R5G5B5 color */
#define _CC_R5G5B5(r, g, b) _CC_A1R5G5B5(0xFF,r,g,b)

/* Creates a 16bit A1R5G5B5 color, based on 16bit input values */
#define _CC_RGB16FROM16(r, g, b) \
    (0x8000 | (r & 0x1F) << 10 | (g & 0x1F) << 5  | (b & 0x1F))

/* Converts a 32bit (X8R8G8B8) color to a 16bit A1R5G5B5 color
#define _CC_X8R8G8B8_TO_A1R5G5B5(color) \
    (uint16_t)(0x8000 | \
    ( color & 0x00F80000) >> 9 | \
    ( color & 0x0000F800) >> 6 | \
    ( color & 0x000000F8) >> 3)*/
_CC_FORCE_INLINE_ uint16_t _CC_X8R8G8B8_TO_A1R5G5B5(_cc_color_t color) {
    uint8_t* sB = (uint8_t*)&color;
    return (0x8000) | ((uint8_t)(sB[0] >> 3) << 10) | ((uint8_t)(sB[1] >> 3) << 5) | (sB[2] >> 3);
}
/* Converts a 32bit (A8R8G8B8) color to a 16bit A1R5G5B5 color */
_CC_FORCE_INLINE_ uint16_t _CC_A8R8G8B8_TO_A1R5G5B5(_cc_color_t color) {
    uint8_t* sB = (uint8_t*)&color;
    return ((uint8_t)(sB[3] >> 3) << 15) | ((uint8_t)(sB[0] >> 3) << 10) | ((uint8_t)(sB[1] >> 3) << 5) | (sB[2] >> 3);
}

/* Converts a 32bit (A8R8G8B8) color to a 16bit R5G6B5 color */
#define _CC_A8R8G8B8_TO_R5G6B5(color)\
    (uint16_t)(( color & 0x00F80000) >> 8 |\
    ( color & 0x0000FC00) >> 5 |\
    ( color & 0x000000F8) >> 3)

/* Convert A8R8G8B8 Color from A1R5G5B5 color*/
/** build a nicer 32bit Color by extending dest lower bits with source high bits. */
#define _CC_A1R5G5B5_TO_A8R8G8B8(color)\
    (uint32_t)( (( -( (int32_t) color & 0x00008000 ) >> (int32_t) 31 ) & 0xFF000000 ) |\
        (( color & 0x00007C00 ) << 9) | (( color & 0x00007000 ) << 4) |\
        (( color & 0x000003E0 ) << 6) | (( color & 0x00000380 ) << 1) |\
        (( color & 0x0000001F ) << 3) | (( color & 0x0000001C ) >> 2)\
    )

/* Returns A8R8G8B8 Color from R5G6B5 color */
#define _CC_R5G6B5_TO_A8R8G8B8(color)\
        (uint32_t)0xFF000000 |\
        ((color & 0xF800) << 8)|\
        ((color & 0x07E0) << 5)|\
        ((color & 0x001F) << 3)

/* Returns A1R5G5B5 Color from R5G6B5 color */
#define _CC_R5G6B5_TO_A1R5G5B5(color)\
    (uint16_t) 0x8000 | (((color & 0xFFC0) >> 1) | (color & 0x1F))

/* Returns R5G6B5 Color from A1R5G5B5 color */
#define _CC_A1R5G5B5_TO_R5G6B5(color) (uint16_t)(((color & 0x7FE0) << 1) | (color & 0x1F));

/* Returns the alpha component from A1R5G5B5 color */
/** alpha refers to opacity. 
\return The alpha value of the color. 0 is transparent, 1 is opaque. */
#define _CC_GET_ALPHA(color) (uint32_t) (((uint16_t)color >> 15) & 0x1)
#define _CC_SET_ALPHA(color, a) do {\
    ((byte_t*)(uint32_t*)&(color))[3] = a;\
} while (0)

/* Returns the red component from A1R5G5B5 color. */
/** Shift left by 3 to get 8 bit value. */
#define _CC_GET_RED(color) (((uint16_t)color >> 10) & 0x1F)
#define _CC_SET_RED(color, r) do {\
    ((byte_t*)(uint32_t*)&(color))[2] = r;\
} while (0)

/* Returns the green component from A1R5G5B5 color */
/** Shift left by 3 to get 8 bit value. */
#define _CC_GET_GREEN(color) (((uint16_t)color >> 5) & 0x1F)
#define _CC_SET_GREEN(color, g) do {\
    ((byte_t*)(uint32_t*)&(color))[1] = g;\
} while (0)
/* Returns the blue component from A1R5G5B5 color */
/** Shift left by 3 to get 8 bit value. */
#define _CC_GET_BLUE(color) ((uint16_t)color & 0x1F)
#define _CC_SET_BLUE(color, b) do {\
    ((byte_t*)(uint32_t*)&(color))[0] = b;\
} while (0)

/* Returns the average from a 16 bit A1R5G5B5 color */
#define _CC_GET_AVERAGE(color) ((_CC_GET_RED(color) << 3) + (_CC_GET_GREEN(color) << 3) + (_CC_GET_BLUE(color) << 3)) / 3;

/* converts a monochrome bitmap to A1R5G5B5 data elf1BitTo16Bit */
_CC_IMAGE_API(void) _cc_color_1bit_to_16bit(const byte_t* in, int16_t* out, int32_t width, int32_t height, int32_t linepad, bool_t flip);
/* converts a 4 bit palettized image to A1R5G5B5 */
_CC_IMAGE_API(void) _cc_color_4bit_to_16bit(const byte_t* in, int16_t* out, int32_t width, int32_t height, const uint32_t* palette, int32_t linepad, bool_t flip);
/* converts a 8 bit palettized image into A1R5G5B5 */
_CC_IMAGE_API(void) _cc_color_8bit_to_16bit(const byte_t* in, int16_t* out, int32_t width, int32_t height, const uint32_t* palette, int32_t linepad, bool_t flip);
/* converts a 8 bit palettized or non palettized image (A8) into R8G8B8 */
_CC_IMAGE_API(void) _cc_color_8bit_to_24bit(const byte_t* in, byte_t* out, int32_t width, int32_t height, const byte_t* palette, int32_t linepad, bool_t flip);
/* converts a 8 bit palettized or non palettized image (A8) into R8G8B8 */
_CC_IMAGE_API(void)_cc_color_8bit_to_32bit(const byte_t* in, byte_t* out, int32_t width, int32_t height, const byte_t* palette, int32_t linepad, bool_t flip);
/* converts 16bit data to 16bit data */
_CC_IMAGE_API(void) _cc_color_16bit_to_16bit(const int16_t* in, int16_t* out_data, int32_t width, int32_t height, int32_t linepad, bool_t flip);
/* copies R8G8B8 24bit data to 24bit data */
_CC_IMAGE_API(void) _cc_color_24bit_to_24bit(const byte_t* in, byte_t* out_data, int32_t width, int32_t height, int32_t linepad, bool_t flip, bool_t bgr);
/* Resizes the gh to a new size and converts it at the same time */
/* to an A8R8G8B8 format, returning the pointer to the new buffer. */
_CC_IMAGE_API(void) _cc_color_16bit_to_A8R8G8B8(const int16_t* in, int32_t* out, int32_t newWidth, int32_t newHeight, int32_t currentWidth, int32_t currentHeight);
/* copies X8R8G8B8 32 bit data */
_CC_IMAGE_API(void) _cc_color_32bit_to_32bit(const int32_t* in, int32_t* out, int32_t width, int32_t height, int32_t linepad, bool_t flip);

//! functions for converting one image format to another efficiently
//! and hopefully correctly.
//!
//! \param sP pointer to source pixel data
//! \param sN number of source pixels to copy
//! \param dP pointer to destination data buffer. must be big enough
//! to hold sN pixels in the output format.
_CC_IMAGE_API(void) _cc_convert_A1R5G5B5_to_R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_A1R5G5B5_to_B8G8R8(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_A1R5G5B5_to_A8R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_A1R5G5B5_to_A1R5G5B5(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_A1R5G5B5_to_R5G6B5(const pvoid_t sP, int32_t sN, pvoid_t dP);

_CC_IMAGE_API(void) _cc_convert_A8R8G8B8_to_R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_A8R8G8B8_to_B8G8R8(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_A8R8G8B8_to_A8B8G8R8(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_A8R8G8B8_to_A8R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_A8R8G8B8_to_A1R5G5B5(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_A8R8G8B8_to_R5G6B5(const pvoid_t sP, int32_t sN, pvoid_t dP);

_CC_IMAGE_API(void) _cc_convert_A8R8G8B8_to_R3G3B2(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_R8G8B8_to_R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_R8G8B8_to_A8R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_R8G8B8_to_A1R5G5B5(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_R8G8B8_to_R5G6B5(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_B8G8R8_to_A8R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_B8G8R8A8_to_A8R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP);

_CC_IMAGE_API(void) _cc_convert_R5G6B5_to_R5G6B5(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_R5G6B5_to_R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_R5G6B5_to_B8G8R8(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_R5G6B5_to_A8R8G8B8(const pvoid_t sP, int32_t sN, pvoid_t dP);
_CC_IMAGE_API(void) _cc_convert_R5G6B5_to_A1R5G5B5(const pvoid_t sP, int32_t sN, pvoid_t dP);

_CC_IMAGE_API(void) _cc_color_convert_via_format(const pvoid_t sp, uint32_t sformat, int32_t sn,
    pvoid_t dp, uint32_t dformat);

#define _CC_COLOR_ARGBf(ct, ca, cr, cg, cb)\
do {\
    ct = _CC_A8R8G8B8(color_b2f(ca), color_b2f(cr), color_b2f(cg), color_b2f(cb)); \
} while (0)

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_CC_IMAGE_COLOR_H_INCLUDED_*/
