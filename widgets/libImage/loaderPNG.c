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
#include "image.h"
#if _CC_USE_SYSTEM_PNG_LIB_
// use system libpng
#include <png.h>
#else
// use 3rd libpng
#include <libpng/png.h>
#endif
#include <setjmp.h>

typedef struct __ImageSource {
    byte_t* data;
    uint32_t size;
    uint32_t offset;
} __ImageSource;

/* PNG function for error handling*/
static void png_cpexcept_error(png_structp png_ptr, png_const_charp msg) {
    _cc_logger_error(_T("PNG FATAL ERROR %s"), msg);
    longjmp(png_jmpbuf(png_ptr), 1);
}

/* PNG function for file reading*/
void PNGAPI user_read_data_fcn(png_structp png_ptr, png_bytep data, png_size_t length) {
    /* changed by zola {*/
    __ImageSource* ImageSource = (__ImageSource*)png_get_io_ptr(png_ptr);
    if (ImageSource == nullptr) {
        _cc_logger_error(_T("Read Error Get IO Ptr failed"));
        return ;
    }

    if (ImageSource->offset + length <= ImageSource->size) {
        memcpy(data, ImageSource->data + ImageSource->offset, length);
        ImageSource->offset += (uint32_t)length;
    } else {
        _cc_logger_error(_T("Read Error offset:%d, size: %d"), ImageSource->offset, ImageSource->size);
    }
    /* }*/
}

_cc_image_t* _cc_load_PNG(const byte_t *image_data, uint32_t image_size) {
    _cc_image_t* image = nullptr;
    __ImageSource ImageSource;
    png_structp png_ptr;
    png_infop info_ptr;
    /*Used to point to image rows*/
    byte_t** row_pointers = 0;
    uint32_t width, height, i;
    /* Use temporary variables to avoid passing casted pointers*/
    png_uint_32 w, h;
    int32_t bit_depth, color_type;
    int intent;
    const double screen_gamma = 2.2;

    ImageSource.data = (byte_t *)(image_data + 8);
    ImageSource.size = (uint32_t)image_size;
    ImageSource.offset = 0;

    _cc_assert(image_data != nullptr);
    if (image_data == nullptr) {
        return nullptr;
    }
    /* check if it really is a PNG file */
    if ( png_sig_cmp((png_bytep)image_data, 0, 8) ) {
        _cc_logger_error(_T("LOAD PNG: not really a png"));
        return nullptr;
    }

    /* allocate the png read struct */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, (png_error_ptr)png_cpexcept_error, nullptr);
    if (!png_ptr) {
        _cc_logger_error(_T("LOAD PNG: Internal PNG create read struct failure"));
        return nullptr;
    }

    /* Allocate the png info struct */
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        _cc_logger_error(_T("LOAD PNG: Internal PNG create info struct failure"));
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        return 0;
    }

    // for proper error handling
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        return nullptr;
    }

    /* changed by zola so we don't need to have public FILE pointers*/
    png_set_read_fn(png_ptr, &ImageSource, user_read_data_fcn);

    png_set_sig_bytes(png_ptr, 8); /* Tell png that we read the signature */

    png_read_info(png_ptr, info_ptr); /* Read the info section of the png file */

    // Extract info
    png_get_IHDR(png_ptr, info_ptr,
                 &w, &h,
                 &bit_depth, &color_type, nullptr, nullptr, nullptr);

    width = w;
    height = h;


    // Convert palette color to true color
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    // Convert low bit colors to 8 bit colors
    if (bit_depth < 8) {
        if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
            //png_set_gray_1_2_4_to_8(png_ptr);
            //png_ptr->transformations |= (0x1000 | 0x2000000L);
            png_set_expand_gray_1_2_4_to_8(png_ptr);
        } else {
            png_set_packing(png_ptr);
        }
    }

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    /* Convert high bit colors to 8 bit colors*/
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);

    /* Convert gray color to true color*/
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);

    if (png_get_sRGB(png_ptr, info_ptr, &intent)) {
        png_set_gamma(png_ptr, screen_gamma, 0.45455);
    } else {
        double image_gamma;
        if (png_get_gAMA(png_ptr, info_ptr, &image_gamma))
            png_set_gamma(png_ptr, screen_gamma, image_gamma);
        else
            png_set_gamma(png_ptr, screen_gamma, 0.45455);
    }
    // Update the changes
    png_read_update_info(png_ptr, info_ptr);

    /* Extract info*/
    png_get_IHDR(png_ptr, info_ptr,
                 &w, &h,
                 &bit_depth, &color_type, nullptr, nullptr, nullptr);

    width = w;
    height = h;

    /* Convert RGBA to BGRA */
//#ifdef _CC_TINYGL_USE_DIRECTX9
    if (color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
#ifdef __BIG_ENDIAN__
        png_set_swap_alpha(png_ptr);
#else
        png_set_bgr(png_ptr);
#endif
    }
//#endif

    /* Create the image structure to be filled by png data */
    if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        image = _cc_init_image(CF_A8R8G8B8, width, height);
    else
        image = _cc_init_image(CF_R8G8B8, width, height);

    if (!image) {
        _cc_logger_error(_T("LOAD PNG: Internal PNG create image struct failure"));
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        return nullptr;
    }

    /* Create array of pointers to rows in image data */
    row_pointers = (byte_t**)_cc_malloc(sizeof(png_bytep) * height);

    if (!row_pointers) {
        _cc_logger_error(_T("LOAD PNG: Internal PNG create row pointers failure"));
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        _cc_destroy_image(&image);
        return nullptr;
    }

    // Fill array of pointers to rows in image data
    {
        byte_t* data = (byte_t*)image->data;
        for (i = 0; i < height; ++i) {
            row_pointers[i] = data;
            data += image->pitch;
        }
    }


    // for proper error handling
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        _cc_free(row_pointers);
        _cc_destroy_image(&image);
        return nullptr;
    }

    /* Read data using the library function that handles all transformations including interlacing*/
    png_read_image(png_ptr, row_pointers);

    png_read_end(png_ptr, nullptr);

    _cc_free(row_pointers);

    png_destroy_read_struct(&png_ptr, &info_ptr, 0); // Clean up memory

    return image;
}
