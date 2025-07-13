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

// PNG function for error handling
static void _png_cpexcept_error(png_structp png_ptr, png_const_charp msg) {
	_cc_logger_error(_T("PNG fatal Error:%s"), msg);
	longjmp(png_jmpbuf(png_ptr), 1);
}

// PNG function for warning handling
static void _png_cpexcept_warning(png_structp png_ptr, png_const_charp msg) {
	_cc_logger_error(_T("PNG fatal warning:%s"), msg);
}

// PNG function for file writing
static void PNGAPI _user_write_data_fcn(png_structp png_ptr, png_bytep data, png_size_t length) {
	_cc_file_t *file = (_cc_file_t*)png_get_io_ptr(png_ptr);
	if ((png_size_t)_cc_file_write(file, data, sizeof(byte_t), length) != length) {
		png_error(png_ptr, (png_const_charp)_T("Write Error"));
	}
}
bool_t _cc_write_PNG(const tchar_t *file_name, _cc_image_t *image) {
	png_infop info_ptr;
	_cc_file_t *wfp;
	png_structp png_ptr = nullptr;
	int32_t line_width;
	uint8_t *tmp, *tmp_ptr;
	uint8_t **row_pointers;
	uint32_t i;

	void (*color_convert_format)(const pvoid_t sP, int32_t sN, pvoid_t dP) = nullptr;

	wfp = _cc_open_file(file_name, _T("wb"));
	if (wfp == nullptr) {
		return false;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
	                                  nullptr, (png_error_ptr)_png_cpexcept_error, (png_error_ptr)_png_cpexcept_warning);

	if (png_ptr == nullptr) {
		_cc_file_close(wfp);
		_cc_logger_error(_T("Internal PNG create write struct failure."));
		return false;
	}



	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		_cc_logger_error(_T("Internal PNG create info struct failure."));
		png_destroy_write_struct(&png_ptr, nullptr);
		_cc_file_close(wfp);
		return false;
	}

	// for proper error handling
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		_cc_file_close(wfp);
		return false;
	}

	png_set_write_fn(png_ptr, wfp, _user_write_data_fcn, nullptr);

	// Set info
	switch (image->format) {
	case CF_A8R8G8B8:
	case CF_A1R5G5B5:
		png_set_IHDR(png_ptr, info_ptr, image->width, image->height,
		             8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
		             PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		break;
	default:
		png_set_IHDR(png_ptr, info_ptr, image->width, image->height,
		             8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		             PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	}

	line_width = image->width;
	switch (image->format) {
	case CF_R8G8B8:
	case CF_R5G6B5:
		line_width *= 3;
		break;
	case CF_A8R8G8B8:
	case CF_A1R5G5B5:
		line_width *= 4;
		break;
	// TODO: Error handling in case of unsupported color format
	default:
		break;
	}
	tmp = (byte_t *)_cc_malloc(image->height * line_width);

	if (!tmp) {
		_cc_logger_error(_T("Internal PNG create image struct failure."));
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return false;
	}

	switch (image->format) {
	case CF_R8G8B8:
		color_convert_format = _cc_convert_R8G8B8_to_R8G8B8;
		break;
	case CF_A8R8G8B8:
		color_convert_format = _cc_convert_A8R8G8B8_to_A8R8G8B8;
		break;
	case CF_A1R5G5B5:
		color_convert_format = _cc_convert_A1R5G5B5_to_A8R8G8B8;
		break;
	case CF_R5G6B5:
		color_convert_format = _cc_convert_R5G6B5_to_R8G8B8;
		break;
	}

	if (color_convert_format) {
		color_convert_format(image->data, image->height * image->width, tmp);
	}

	// Create array of pointers to rows in image data

	//Used to point to image rows
	row_pointers = (png_bytep*)_cc_malloc(sizeof(png_bytep) * image->height);
	if (row_pointers == nullptr) {
		_cc_logger_error(_T("Internal PNG create row pointers failure."));
		png_destroy_write_struct(&png_ptr, &info_ptr);
		_cc_free(tmp);
		_cc_file_close(wfp);
		return false;
	}

	tmp_ptr = tmp;
	// Fill array of pointers to rows in image data
	for (i = 0; i < image->height; ++i) {
		row_pointers[i] = tmp_ptr;
		tmp_ptr += line_width;
	}
	// for proper error handling
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		_cc_free(tmp);
		_cc_free(row_pointers);
		_cc_file_close(wfp);
		return false;
	}

	png_set_rows(png_ptr, info_ptr, row_pointers);

	if (image->format == CF_A8R8G8B8 || image->format == CF_A1R5G5B5) {
		png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_BGR, nullptr);
	} else {
		png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);
	}

	_cc_free(tmp);
	_cc_free(row_pointers);
	_cc_file_close(wfp);

	png_destroy_write_struct(&png_ptr, &info_ptr);
	return true;
}
