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
#include "image.h"

#if _CC_USE_SYSTEM_JPEG_LIB_
// use system jpeglib
#include <jpeglib.h>
#else
// use 3rd jpeglib
#include <jpeglib/jpeglib.h>
#endif

#include <setjmp.h>

// struct for handling jpeg errors
struct __jpeg_error_mgr {
    /* public jpeg error fields */
    struct jpeg_error_mgr err_mgr;

    /* for longjmp, to return to caller on a fatal error */
    jmp_buf setjmp_buffer;
};

static void init_source (struct jpeg_decompress_struct * cinfo) {
    /* DO NOTHING */
}

static boolean fill_input_buffer (struct jpeg_decompress_struct * cinfo) {
    /* DO NOTHING */
    return 1;
}

static void skip_input_data (struct jpeg_decompress_struct * cinfo, long count) {
    struct jpeg_source_mgr * src = (struct jpeg_source_mgr *)cinfo->src;
    if (count > 0) {
        src->bytes_in_buffer -= count;
        src->next_input_byte += count;
    }
}

static void term_source (struct jpeg_decompress_struct * cinfo) {
    /* DO NOTHING */
}

static void _error_exit (struct jpeg_common_struct * cinfo) {
    /* unfortunately we need to use a goto rather than throwing an exception
       as gcc crashes under linux crashes when using throw from within
       extern c code
       cinfo->err really points to a irr_error_mgr struct */
    struct __jpeg_error_mgr *myerr;

    /* Always display the message */
    (*cinfo->err->output_message) (cinfo);

    myerr = (struct __jpeg_error_mgr*) cinfo->err;

    longjmp(myerr->setjmp_buffer, 1);
}


static void _output_message(struct jpeg_common_struct * cinfo) {
    /* display the error message. */
    char_t temp1[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, temp1);
    _cc_logger_error(_T("JPEG LOAD FATAL ERROR %s"), temp1);
}

_cc_image_t* _cc_load_JPG(const byte_t *image_data, uint32_t image_size) {
    /* allocate and initialize JPEG decompression object */
    struct jpeg_decompress_struct cinfo;
    struct __jpeg_error_mgr jerr;
    /* specify data source */
    struct jpeg_source_mgr jsrc;

    /* Used to point to image rows */
    byte_t** row_pointers = 0;
    byte_t* output = 0;
    uint32_t width, height, i, rowsRead;
    int16_t rowspan;
    bool_t use_CMYK = false;

    _cc_assert(image_data != nullptr);
    if (image_data == nullptr) {
        return nullptr;
    }


    cinfo.err = jpeg_std_error(&jerr.err_mgr);
    cinfo.err->error_exit = _error_exit;
    cinfo.err->output_message = _output_message;

    /* compatibility fudge:
       we need to use setjmp/longjmp for error handling as gcc-linux
       crashes when throwing within external c code */
    if (setjmp(jerr.setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error.
           We need to clean up the JPEG object and return. */

        jpeg_destroy_decompress(&cinfo);

        return nullptr;
    }
    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(&cinfo);


    /* Set up data pointer */
    jsrc.bytes_in_buffer = image_size;
    jsrc.next_input_byte = (JOCTET*)image_data;
    cinfo.src = &jsrc;

    jsrc.init_source = init_source;
    jsrc.fill_input_buffer = fill_input_buffer;
    jsrc.skip_input_data = skip_input_data;
    jsrc.resync_to_restart = jpeg_resync_to_restart;
    jsrc.term_source = term_source;

    /* Decodes JPG input from whatever source
       Does everything AFTER jpeg_create_decompress
       and BEFORE jpeg_destroy_decompress
       Caller is responsible for arranging these + setting up cinfo

       read file parameters with jpeg_read_header() */
    jpeg_read_header(&cinfo, true);

    if (cinfo.jpeg_color_space == JCS_CMYK) {
        cinfo.out_color_space = JCS_CMYK;
        cinfo.out_color_components = 4;
        use_CMYK = true;
    } else {
        cinfo.out_color_space = JCS_RGB;
        cinfo.out_color_components = 3;
    }
    cinfo.output_gamma = 2.2f;
    cinfo.do_fancy_upsampling = false;

    /* Start decompressor */
    jpeg_start_decompress(&cinfo);

    /* Get image data */
    rowspan = cinfo.image_width * cinfo.out_color_components;
    width = cinfo.image_width;
    height = cinfo.image_height;

    /* Allocate memory for buffer */
    output = (byte_t*)_cc_malloc(sizeof(byte_t) * height * rowspan);
    /* Here we use the library's state variable cinfo.output_scanline as the
       loop counter, so that we don't have to keep track ourselves.
       Create array of row pointers for lib */
    row_pointers = (byte_t**)_cc_malloc(sizeof(byte_t*) * height);

    for ( i = 0; i < height; i++ )
        row_pointers[i] = &output[ i * rowspan ];

    rowsRead = 0;

    while ( cinfo.output_scanline < cinfo.output_height )
        rowsRead += jpeg_read_scanlines( &cinfo, &row_pointers[rowsRead], cinfo.output_height - rowsRead );

    _cc_free(row_pointers);
    /* Finish decompression */
    jpeg_finish_decompress(&cinfo);

    /* Release JPEG decompression object
       This is an important step since it will release a good deal of memory.*/
    jpeg_destroy_decompress(&cinfo);

    if (use_CMYK == true) {
        uint32_t i = 0, j = 0, size = 0;
        _cc_image_t  *img = _cc_init_image(CF_R8G8B8, width, height);
        size = 3 * width * height;

        if (img == nullptr) {
            _cc_free(output);
            return nullptr;
        }

        if (img->data) {
            for (i = 0, j = 0; i < size; i += 3, j += 4) {
                // Also works without K, but has more contrast with K multiplied in
                //img->data[i + 0] = output[j + 2];
                //img->data[i + 1] = output[j + 1];
                //img->data[i + 2] = output[j + 0];
                img->data[i + 0] = (byte_t)(output[j + 2] * (output[j + 3] / 255.f));
                img->data[i + 1] = (byte_t)(output[j + 1] * (output[j + 3] / 255.f));
                img->data[i + 2] = (byte_t)(output[j + 0] * (output[j + 3] / 255.f));
            }
        }
        _cc_free(output);
        return img;
    }
    return _cc_init_image_data(CF_R8G8B8, width, height, output);
}
