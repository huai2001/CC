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
#ifndef _CC_IMAGE_H_INCLUDED_
#define _CC_IMAGE_H_INCLUDED_

#include "color.h"

#define _CC_3RD_LIBPNG_             1
#define _CC_3RD_LIBJPG_             1

#ifndef __CC_WINDOWS__
#define _CC_USE_SYSTEM_PNG_LIB_     1
#define _CC_USE_SYSTEM_JPEG_LIB_    1
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* {{{ enum image_filetype
   This enum is used to have ext/standard/image.c and ext/exif/exif.c use
   the same constants for file types.
*/
typedef enum {
    _CC_IMAGE_FILETYPE_UNKNOWN_ = 0,
    _CC_IMAGE_FILETYPE_GIF_ = 1,
    _CC_IMAGE_FILETYPE_JPEG_,
    _CC_IMAGE_FILETYPE_PNG_,
    _CC_IMAGE_FILETYPE_SWF_,
    _CC_IMAGE_FILETYPE_PSD_,
    _CC_IMAGE_FILETYPE_BMP_,
    _CC_IMAGE_FILETYPE_TIFF_II_, /* intel */
    _CC_IMAGE_FILETYPE_TIFF_MM_, /* motorola */
    _CC_IMAGE_FILETYPE_JPC_,
    /* _CC_IMAGE_FILETYPE_JPEG2000_ is a userland alias for _CC_IMAGE_FILETYPE_JPC_ */
    _CC_IMAGE_FILETYPE_JP2_,
    _CC_IMAGE_FILETYPE_JPX_,
    _CC_IMAGE_FILETYPE_JB2_,
    _CC_IMAGE_FILETYPE_SWC_,
    _CC_IMAGE_FILETYPE_IFF_,
    _CC_IMAGE_FILETYPE_WBMP_,
    _CC_IMAGE_FILETYPE_XBM_,
    _CC_IMAGE_FILETYPE_ICO_,
    _CC_IMAGE_FILETYPE_WEBP_,
    _CC_IMAGE_FILETYPE_PCX_,
    _CC_IMAGE_FILETYPE_TGA_,
    /* WHEN EXTENDING: PLEASE ALSO REGISTER IN image.c:(_cc_get_imagetypes) */
    _CC_IMAGE_FILETYPE_COUNT_
} _cc_image_filetype_t;
/* }}} */

#ifdef __BIG_ENDIAN__
#define __BYTE_SWAP_16(a,b) do {\
    a = _cc_swap16(*(uint16_t*)b);\
    b += sizeof(uint16_t);\
} while (0)

#define __BYTE_SWAP_32(a,b) do {\
    a = _cc_swap32(*(uint32_t*)b);\
    b += sizeof(uint32_t);\
} while (0)
#else
#define __BYTE_SWAP_16(a,b) do {\
    a = (*(uint16_t*)b);\
    b += sizeof(uint16_t);\
} while (0)

#define __BYTE_SWAP_32(a,b) do {\
    a = (*(uint32_t*)b);\
    b += sizeof(uint32_t);\
} while (0)
#endif

typedef struct _cc_image {
    byte_t *data;
    byte_t format;
    uint32_t width;
    uint32_t height;    
    uint32_t pitch;
    uint32_t channel;
    uint32_t size;
    
    struct {
        uint32_t *data;
        uint32_t size;
    } palette;
    
}_cc_image_t;


_CC_IMAGE_API(uint16_t) _cc_pixel_blend16 ( const uint16_t c2, const uint16_t c1);
_CC_IMAGE_API(uint32_t) _cc_pixel_blend16_simd ( const uint32_t c2, const uint32_t c1 );
_CC_IMAGE_API(uint32_t) _cc_pixel_blend32( const uint32_t c2, const uint32_t c1);

_CC_IMAGE_API(uint16_t) _cc_pixel_blend16A ( const uint16_t c2, const uint32_t c1, const uint16_t alpha );
_CC_IMAGE_API(uint32_t) _cc_pixel_blend32A( const uint32_t c2, const uint32_t c1, uint32_t alpha );

/* load a BMP image */
_CC_IMAGE_API(_cc_image_t*) _cc_load_BMP(const byte_t *data, uint32_t size);
/* write a BMP image */
_CC_IMAGE_API(bool_t) _cc_write_BMP(const tchar_t *file_name, _cc_image_t *image);
/* load a TGA image */
_CC_IMAGE_API(_cc_image_t*) _cc_load_TGA(const byte_t *data, uint32_t size);
/* write a BMP image */
_CC_IMAGE_API(bool_t) _cc_write_TGA(const tchar_t *file_name, _cc_image_t *image);
/* load a PCX image */
_CC_IMAGE_API(_cc_image_t*) _cc_load_PCX(const byte_t *data, uint32_t size);
/* write a BMP image */
_CC_IMAGE_API(bool_t) _cc_write_PCX(const tchar_t *file_name, _cc_image_t *image);
#ifdef _CC_3RD_LIBPNG_
/* load a PNG image */
_CC_IMAGE_API(_cc_image_t*) _cc_load_PNG(const byte_t *data, uint32_t size);
/* write a PNG image */
_CC_IMAGE_API(bool_t) _cc_write_PNG(const tchar_t *file_name, _cc_image_t *image);
#endif
    
#ifdef _CC_3RD_LIBJPG_
/* load a JPG image */
_CC_IMAGE_API(_cc_image_t*) _cc_load_JPG(const byte_t *data, uint32_t size);
/* write a PNG image */
_CC_IMAGE_API(bool_t) _cc_write_JPG(const tchar_t *file_name, _cc_image_t *image, uint32_t quality);
#endif

/**/
_CC_IMAGE_API(_cc_image_filetype_t) _cc_get_imagetypes(byte_t *data, int32_t len);

/**/
_CC_IMAGE_API(_cc_image_t*) _cc_image_from_file(const tchar_t *file_name);

/**/
_CC_IMAGE_API(_cc_image_t*) _cc_init_image(uint32_t format, uint32_t width, uint32_t height);

/**/
_CC_IMAGE_API(_cc_image_t*) _cc_init_image_data(uint32_t format, uint32_t width, uint32_t height, byte_t *data);

/**/
_CC_IMAGE_API(bool_t) _cc_free_image(_cc_image_t* image);

/**/
_CC_IMAGE_API(int32_t) _cc_get_bits_per_pixel_from_format(const byte_t format);

//! returns a pixel
_CC_IMAGE_API(uint32_t) _cc_get_pixel(_cc_image_t *image, uint32_t x, uint32_t y);
//! sets a pixel
_CC_IMAGE_API(void) _cc_set_pixel(_cc_image_t *image, uint32_t x, uint32_t y, const _cc_color_t clr, bool_t blend);

/**/
_CC_IMAGE_API(void) _cc_image_scaling(_cc_image_t *dst, _cc_image_t *src);
/**/
_CC_IMAGE_API(void) _cc_image_resampled (_cc_image_t* dst, _cc_image_t* src,
                                      int32_t dstX, int32_t dstY, int32_t srcX, int32_t srcY,
                                      int32_t dstW, int32_t dstH, int32_t srcW, int32_t srcH);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_CC_IMAGE_H_INCLUDED_*/
