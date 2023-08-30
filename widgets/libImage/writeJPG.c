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
#include "image.h"

#include <setjmp.h>

#if _CC_USE_SYSTEM_JPEG_LIB_
#include <jpeglib.h>
#include <jerror.h>
#else
// use <3rd> jpeglib
#include <jpeglib/jpeglib.h>
#include <jpeglib/jerror.h>
#endif

// The writer uses a 4k buffer and flushes to disk each time it's filled
#define _OUTPUT_BUF_SIZE_ 4096

typedef struct {
	struct jpeg_destination_mgr pub;	/* public fields */
	_cc_file_t* file;					/* target file */
	JOCTET buffer[_OUTPUT_BUF_SIZE_];	/* image buffer */
} __jpg_destination_mgr_t;

// struct for handling jpeg errors
struct __jpeg_error_mgr {
	/* public jpeg error fields */
	struct jpeg_error_mgr err_mgr;

	/* for longjmp, to return to caller on a fatal error */
	jmp_buf setjmp_buffer;
};

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

// init
static void _jpeg_init_destination(j_compress_ptr cinfo) {
	__jpg_destination_mgr_t* dest = (__jpg_destination_mgr_t*) cinfo->dest;
	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = _OUTPUT_BUF_SIZE_;
}


// flush to disk and reset buffer
static boolean _jpeg_empty_output_buffer(j_compress_ptr cinfo) {
	__jpg_destination_mgr_t* dest = (__jpg_destination_mgr_t*) cinfo->dest;

	// for now just exit upon file error
	if (_cc_file_write(dest->file, dest->buffer, sizeof(JOCTET), _OUTPUT_BUF_SIZE_) != _OUTPUT_BUF_SIZE_) {
		ERREXIT (cinfo, JERR_FILE_WRITE);
	}

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = _OUTPUT_BUF_SIZE_;

	return true;
}


static void _jpeg_term_destination(j_compress_ptr cinfo) {
	__jpg_destination_mgr_t* dest = (__jpg_destination_mgr_t*) cinfo->dest;
	const size_t datacount = _OUTPUT_BUF_SIZE_ - dest->pub.free_in_buffer;
	// for now just exit upon file error
	if (_cc_file_write(dest->file, dest->buffer, sizeof(JOCTET), datacount) != datacount) {
		ERREXIT (cinfo, JERR_FILE_WRITE);
	}
}


// set up buffer data
static void _jpeg_file_dest(j_compress_ptr cinfo, _cc_file_t *file) {
	__jpg_destination_mgr_t* dest;
	if (cinfo->dest == NULL) {
		/* first time for this JPEG object? */
		cinfo->dest = (struct jpeg_destination_mgr *)
		              (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo,
		                      JPOOL_PERMANENT,
		                      sizeof(__jpg_destination_mgr_t));
	}

	dest = (__jpg_destination_mgr_t*) cinfo->dest;  /* for casting */

	/* Initialize method pointers */
	dest->pub.init_destination = _jpeg_init_destination;
	dest->pub.empty_output_buffer = _jpeg_empty_output_buffer;
	dest->pub.term_destination = _jpeg_term_destination;

	/* Initialize private member */
	dest->file = file;
}

/* write_JPEG_memory: store JPEG compressed image into memory.
*/
bool_t _cc_write_JPG(const tchar_t *file_name, _cc_image_t *image, uint32_t quality) {
	uint8_t *dest;
	_cc_file_t *wfp;

	struct __jpeg_error_mgr jerr;
	struct jpeg_compress_struct cinfo;

	void (*color_convert_format)(const pvoid_t sP, int32_t sN, pvoid_t dP) = NULL;

	wfp = _cc_open_file(file_name, _T("wb"));
	if (wfp == NULL) {
		return false;
	}

	switch ( image->format ) {
	case CF_R8G8B8:
		color_convert_format = _cc_convert_R8G8B8_to_R8G8B8;
		break;
	case CF_A8R8G8B8:
		color_convert_format = _cc_convert_A8R8G8B8_to_R8G8B8;
		break;
	case CF_A1R5G5B5:
		color_convert_format = _cc_convert_A1R5G5B5_to_R8G8B8;
		break;
	case CF_R5G6B5:
		color_convert_format = _cc_convert_R5G6B5_to_R8G8B8;
		break;
	}

	// couldn't find a color converter
	if ( NULL == color_convert_format ) {
		_cc_file_close(wfp);
		return false;
	}

	cinfo.err = jpeg_std_error(&jerr.err_mgr);
	cinfo.err->error_exit = _error_exit;
	cinfo.err->output_message = _output_message;
	/* compatibility fudge:
	   we need to use setjmp/longjmp for error handling as gcc-linux
	   crashes when throwing within external c code */
	if (setjmp(jerr.setjmp_buffer)) {
		_cc_file_close(wfp);
		return false;
	}

	jpeg_create_compress(&cinfo);
	_jpeg_file_dest(&cinfo, wfp);
	cinfo.image_width = image->width;
	cinfo.image_height = image->height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);

	if ( 0 == quality ) {
		quality = 75;
	}

	jpeg_set_quality(&cinfo, quality, true);
	jpeg_start_compress(&cinfo, true);

	dest = (uint8_t*)_cc_malloc(image->width * 3);

	if (dest) {
		const uint32_t pitch = image->pitch;
		byte_t* src = image->data;
		/* pointer to JSAMPLE row[s] */
		JSAMPROW row_pointer[1];
		row_pointer[0] = dest;

		while (cinfo.next_scanline < cinfo.image_height) {
			// convert next line
			color_convert_format( src, image->width, dest );
			src += pitch;
			jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}

		_cc_free(dest);

		/* Step 6: Finish compression */
		jpeg_finish_compress(&cinfo);
	}

	/* Step 7: Destroy */
	jpeg_destroy_compress(&cinfo);

	_cc_file_close(wfp);

	return (dest != 0);
}
