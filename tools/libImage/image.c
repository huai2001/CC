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

#include <stdio.h>
#include "image.h"

static const uint16_t _cc_sig_tag[3] = {0x0000,0x0001,0x0002};
static const uint16_t _cc_sig_pcx = 0x050a;
/* file type markers */
static const char _cc_sig_gif[3] = {'G', 'I', 'F'};
static const char _cc_sig_psd[4] = {'8', 'B', 'P', 'S'};
static const char _cc_sig_bmp[2] = {'B', 'M'};
static const char _cc_sig_swf[3] = {'F', 'W', 'S'};
static const char _cc_sig_swc[3] = {'C', 'W', 'S'};
static const char _cc_sig_jpg[3] = {(char) 0xff, (char) 0xd8, (char) 0xff};
static const char _cc_sig_png[8] = {(char) 0x89, (char) 0x50, (char) 0x4e, (char) 0x47,
                                    (char) 0x0d, (char) 0x0a, (char) 0x1a, (char) 0x0a};
static const char _cc_sig_tif_ii[4] = {'I','I', (char)0x2A, (char)0x00};
static const char _cc_sig_tif_mm[4] = {'M','M', (char)0x00, (char)0x2A};
static const char _cc_sig_jpc[3]  = {(char)0xff, (char)0x4f, (char)0xff};
static const char _cc_sig_jp2[12] = {(char)0x00, (char)0x00, (char)0x00, (char)0x0c,
                                     (char)0x6a, (char)0x50, (char)0x20, (char)0x20,
                                     (char)0x0d, (char)0x0a, (char)0x87, (char)0x0a};
static const char _cc_sig_iff[4] = {'F','O','R','M'};
static const char _cc_sig_ico[4] = {(char)0x00, (char)0x00, (char)0x01, (char)0x00};
static const char _cc_sig_riff[4] = {'R', 'I', 'F', 'F'};
static const char _cc_sig_webp[4] = {'W', 'E', 'B', 'P'};
/*!
    Pixel = dest * ( 1 - alpha ) + source * alpha
    alpha [0;256]
*/
 uint32_t _cc_pixel_blend32A( const uint32_t c2, const uint32_t c1, uint32_t alpha ) {
    uint32_t dstRB = c2 & 0x00FF00FF;
    uint32_t dstXG = c2 & 0x0000FF00;

    uint32_t rb = ((c1 & 0x00FF00FF) - dstRB) * alpha;
    uint32_t xg = ((c1 & 0x0000FF00) - dstXG) * alpha;

    return (((rb >> 8) + dstRB) & 0x00FF00FF) | (((xg >> 8) + dstXG) & 0x0000FF00);
}

/*!
    Pixel = dest * ( 1 - alpha ) + source * alpha
    alpha [0;32]
*/
uint16_t _cc_pixel_blend16A ( const uint16_t c2, const uint32_t c1, const uint16_t alpha ) {
    const uint16_t dstRB = c2 & 0x7C1F;
    const uint16_t dstXG = c2 & 0x03E0;

    uint32_t rb = ((c1 & 0x7C1F) - dstRB) * alpha;
    uint32_t xg = ((c1 & 0x03E0) - dstXG) *alpha;

    return (uint16_t)((((rb >> 5) + dstRB) & 0x7C1F) | (((xg >> 5) + dstXG) & 0x03E0));
}

// 1 - Bit Alpha Blending
uint16_t _cc_pixel_blend16 ( const uint16_t c2, const uint16_t c1 ) {
    uint16_t mask = ((c1 & 0x8000) >> 15 ) + 0x7fff;
    return (c2 & mask ) | ( c1 & ~mask );
}

// 1 - Bit Alpha Blending 16Bit SIMD
uint32_t _cc_pixel_blend16_simd ( const uint32_t c2, const uint32_t c1 ) {
    uint32_t mask = ((c1 & 0x80008000) >> 15 ) + 0x7fff7fff;
    return (c2 & mask ) | ( c1 & ~mask );
}

/*!
    Pixel = dest * ( 1 - SourceAlpha ) + source * SourceAlpha
*/
uint32_t _cc_pixel_blend32 ( const uint32_t c2, const uint32_t c1 ) {
    // alpha test
    uint32_t alpha = c1 & 0xFF000000;
    uint32_t dstRB,dstXG,rb,xg;
    if ( 0 == alpha )
        return c2;

    if ( 0xFF000000 == alpha ) {
        return c1;
    }

    alpha >>= 24;

    // add highbit alpha, if ( alpha > 127 ) alpha += 1;
    alpha += ( alpha >> 7);

    dstRB = c2 & 0x00FF00FF;
    dstXG = c2 & 0x0000FF00;


    rb = ((c1 & 0x00FF00FF) - dstRB) * alpha;
    xg = ((c1 & 0x0000FF00) - dstXG) * alpha;

    return (c1 & 0xFF000000) | ( ((rb >> 8) + dstRB) & 0x00FF00FF) | (((xg >> 8) + dstXG) & 0x0000FF00);
}

//! sets a pixel
void _cc_set_pixel(_cc_image_t *image, uint32_t x, uint32_t y, const _cc_color_t clr, bool_t blend) {
    uint32_t *dest32;
    uint16_t *dest16;
    byte_t *dest8;
    _cc_assert(image != nullptr);
    if(image == nullptr) return ;

    if (x >= image->width || y >= image->height)
        return;

    switch(image->format) {
    case CF_A1R5G5B5: {
            dest16 = (uint16_t*) (image->data + ( y * image->pitch ) + ( x << 1 ));
            *dest16 =  _CC_A8R8G8B8_TO_A1R5G5B5( (uint32_t)clr );
        } break;

    case CF_R5G6B5: {
            dest16 = (uint16_t*) (image->data + ( y * image->pitch ) + ( x << 1 ));
            *dest16 = _CC_A8R8G8B8_TO_R5G6B5( (uint32_t)clr );
        } break;

    case CF_R8G8B8: {
            dest8 = (byte_t*)(image->data + ( y * image->pitch ) + ( x * 3 ));
            dest8[0] = colorB(clr);
            dest8[1] = colorG(clr);
            dest8[2] = colorR(clr);
        } break;
    case CF_A8R8G8B8: {
            dest32 = (uint32_t*) (image->data + ( y * image->pitch ) + ( x << 2 ));
            *dest32 = blend ? _cc_pixel_blend32 ( *dest32, (uint32_t)clr ) : (uint32_t)clr;
        } break;
    }
}

//! returns a pixel
uint32_t _cc_get_pixel(_cc_image_t *image, uint32_t x, uint32_t y) {
    byte_t *dest;
    _cc_assert(image != nullptr);
    if(image == nullptr) return 0;
    if (x >= image->width || y >= image->height)
        return 0;

    switch(image->format) {
    case CF_A1R5G5B5:
        return _CC_A1R5G5B5_TO_A8R8G8B8(((uint16_t*)image->data)[y * image->width + x]);
    case CF_R5G6B5:
        return _CC_R5G6B5_TO_A8R8G8B8(((uint16_t*)image->data)[y * image->width + x]);
    case CF_A8R8G8B8:
        return ((uint32_t*)image->data)[y * image->width + x];
    case CF_R8G8B8: {
            dest = image->data + (y * 3) * image->width + (x * 3);
            return (uint32_t)_CC_R8G8B8(dest[0],dest[1],dest[2]);
        }
    }
    return 0;
}

#define floor_cast(x) ((long)(x))
#define _COLOR_MAX 255.0f
#define _COLOR_MAX_ALPHA 127.0f

static float64_t _get_max(float64_t a, float64_t b) {
    return _max(a, b);
}

void _cc_image_resampled (_cc_image_t* dst, _cc_image_t* src,
                                 int32_t dstX, int32_t dstY, int32_t srcX, int32_t srcY,
                                 int32_t dstW, int32_t dstH, int32_t srcW, int32_t srcH) {
    int32_t x, y;
    uint32_t pixel;
    double sy1, sy2, sx1, sx2;
    
    double rx = ((double) srcW / (double) dstW);
    double ry = ((double) srcH / (double) dstH);
    
    struct {
        double x;
        double y;
        double alpha;
        double contribution;
    }portion;
    
    struct {
        double red;
        double green;
        double blue;
        double alpha;
    }colour;
    
    for (y = dstY; (y < dstY + dstH); y++) {
        sy1 = (double)( y - dstY) * ry;
        sy2 = (double)((y + 1.0) - dstY) * ry;
        for (x = dstX; (x < dstX + dstW); x++) {
            double sx, sy;
            double pixel_sum = 0.0;
            double alpha_sum = 0.0;
            
            colour.red = colour.green = colour.blue = colour.alpha = 0;
            sx1 = (double)( x - dstX) * rx;
            sx2 = (double)((x + 1) - dstX) * rx;
            sy = sy1;
            
            do {
                long ycmp[3] = {
                    (long)floor(sy),(long)(sy1),(long)floor(sy2)
                };

                if (ycmp[0] == ycmp[1]) {
                    portion.y = _get_max(1.0 - (sy - ycmp[0]), sy2 - sy1);
                    sy = ycmp[0];
                } else if (sy == ycmp[2]) {
                    portion.y = sy2 - ycmp[2];
                } else {
                    portion.y = 1.0;
                }
                
                sx = sx1;
                
                do {
                    long xcmp[3] = {
                        (long)floor(sx),(long)(sx1),(long)floor(sx2)
                    };
        
                    if (xcmp[0] == xcmp[1]) {
                        portion.x = _get_max(1.0 - (sx - xcmp[0]), sx2 - sx1);
                        sx = xcmp[0];
                    } else if (sx == xcmp[2]) {
                        portion.x = sx2 - xcmp[2];
                    } else {
                        portion.x = 1.0f;
                    }
                    
                    pixel = _cc_get_pixel(src, (int) sx + srcX, (int) sy + srcY);
                    
                    portion.contribution = portion.x * portion.y;
                    portion.alpha = ((_COLOR_MAX_ALPHA - colorA(pixel))) * portion.contribution;
                    
                    colour.red += colorR (pixel) * portion.alpha;
                    colour.green += colorG (pixel) * portion.alpha;
                    colour.blue += colorB (pixel) * portion.alpha;
                    colour.alpha += colorA (pixel) * portion.contribution;
                    
                    alpha_sum += portion.alpha;
                    pixel_sum += portion.contribution;
                    sx += 1.0f;
                } while (sx < sx2);
                
                sy += 1.0f;
            } while (sy < sy2);
            
            if (pixel_sum != 0.0f) {
                colour.red /= pixel_sum;
                colour.green /= pixel_sum;
                colour.blue /= pixel_sum;
                colour.alpha /= pixel_sum;
                colour.alpha += 0.5;
            }
            
            if ( alpha_sum != 0.0f) {
                if( pixel_sum != 0.0f) {
                    alpha_sum /= pixel_sum;
                }
                colour.red /= alpha_sum;
                colour.green /= alpha_sum;
                colour.blue /= alpha_sum;
            }
            
            /* Clamping to allow for rounding errors above */
            colour.red = _min(colour.red, _COLOR_MAX);
            colour.green = _min(colour.green, _COLOR_MAX);
            colour.blue = _min(colour.blue, _COLOR_MAX);
            if (colour.alpha >= _COLOR_MAX_ALPHA) {
                colour.alpha = _COLOR_MAX;
            }

            _cc_set_pixel(dst, x, y, _CC_R8G8B8A8 ((int)colour.red, (int)colour.green, (int)colour.blue, (int)colour.alpha), 0);
        }
    }
}

//! copies this gh into another, scaling it to the target image size
// note: this is very very slow.
void _cc_image_scaling(_cc_image_t *dst, _cc_image_t *src) {
    uint32_t bpp,bw,rest,y;
    byte_t *dstpos, *srcpos;
    _cc_assert(dst->data != nullptr && src->data != nullptr);

    if(dst->data == nullptr || src->data == nullptr)
        return;

    if (!dst->width || !dst->height)
        return;

    bpp = _cc_get_bits_per_pixel_from_format(dst->format) / 8;
    if (0 == dst->pitch)
        dst->pitch = src->width * bpp;

    if (src->format == dst->format && src->width == dst->width && src->height == dst->height) {
        if (dst->pitch == src->pitch) {
            memcpy(dst->data, src->data, dst->height * dst->pitch);
            return;
        } else {
            dstpos = dst->data;
            srcpos = src->data;
            bw = dst->width * bpp;
            rest = dst->pitch - bw;
            for (y = 0;  y < dst->height; ++y) {
                // copy scanline
                memcpy(dstpos, srcpos, bw);
                // clear pitch
                memset(dstpos + bw, 0, rest);
                dstpos += dst->pitch;
                srcpos += src->pitch;
            }
            return;
        }
    }

    _cc_image_resampled(dst, src, 0, 0, 0, 0, dst->width, dst->height, src->width, src->height);
}

int32_t _cc_get_bits_per_pixel_from_format(const byte_t format) {
    switch(format) {
    case CF_A1R5G5B5:
    case CF_R5G6B5:
    case CF_R16F:
        return 16;
    case CF_R8G8B8:
        return 24;
    case CF_A8R8G8B8:
    case CF_G16R16F:
    case CF_R32F:
        return 32;
    case CF_A16B16G16R16F:
    case CF_G32R32F:
        return 64;
    case CF_A32B32G32R32F:
        return 128;
    default:
        return 0;
    }
}

_cc_image_t* _cc_init_image_data(uint32_t format, uint32_t width, uint32_t height, byte_t *data) {
    _cc_image_t  *image = (_cc_image_t  *)_cc_malloc(sizeof(_cc_image_t ));
    if(image == nullptr) {
        return nullptr;
    }
    image->format = format;
    image->width = width;
    image->height = height;
    image->channel = _cc_get_bits_per_pixel_from_format(format) / 8;
    image->pitch = (image->channel * image->width);
    image->size = height * image->pitch;
    
    image->palette.data = nullptr;
    image->palette.size = 0;

    if(data == nullptr) {
        image->data = (byte_t *)_cc_malloc(sizeof(byte_t) * (uint32_t)image->size);
        if(image->data == nullptr) {
            _cc_free(image);
            return nullptr;
        }
        memset(image->data, 0xff, sizeof(byte_t) * image->size);
    } else {
        image->data = data;
    }
    return image;
}

_cc_image_filetype_t _cc_get_imagetypes(byte_t *data, int32_t len) {
    uint16_t *sig = (uint16_t*)data;
    if (*sig == _cc_sig_pcx) {
        return _CC_IMAGE_FILETYPE_PCX_;
    } else if (*(sig) == _cc_sig_tag[0] && *(sig + 1) == _cc_sig_tag[1] && *(sig + 2) == _cc_sig_tag[2]) {
        return _CC_IMAGE_FILETYPE_TGA_;
    }

    if (len < 3) {
        return _CC_IMAGE_FILETYPE_UNKNOWN_;
    }

    if (!memcmp(data, _cc_sig_gif, 3)) {
        return _CC_IMAGE_FILETYPE_GIF_;
    } else if (!memcmp(data, _cc_sig_jpg, 3)) {
        return _CC_IMAGE_FILETYPE_JPEG_;
    } else if (len >= 8 && !memcmp(data, _cc_sig_png, 8)) {
        return _CC_IMAGE_FILETYPE_PNG_;
    } else if (!memcmp(data, _cc_sig_swf, 3)) {
        return _CC_IMAGE_FILETYPE_SWF_;
    } else if (!memcmp(data, _cc_sig_swc, 3)) {
        return _CC_IMAGE_FILETYPE_SWC_;
    } else if (!memcmp(data, _cc_sig_psd, 3)) {
        return _CC_IMAGE_FILETYPE_PSD_;
    } else if (!memcmp(data, _cc_sig_bmp, 2)) {
        return _CC_IMAGE_FILETYPE_BMP_;
    } else if (!memcmp(data, _cc_sig_jpc, 3)) {
        return _CC_IMAGE_FILETYPE_JPC_;
    } else if (!memcmp(data, _cc_sig_riff, 3)) {
        if (len >= 12 && !memcmp(data, _cc_sig_webp, 4)) {
            return _CC_IMAGE_FILETYPE_WEBP_;
        }
        return _CC_IMAGE_FILETYPE_UNKNOWN_;
    } else if (len < 4) {
        return _CC_IMAGE_FILETYPE_UNKNOWN_;
    }

    if (!memcmp(data, _cc_sig_tif_ii, 4)) {
        return _CC_IMAGE_FILETYPE_TIFF_II_;
    } else if (!memcmp(data, _cc_sig_tif_mm, 4)) {
        return _CC_IMAGE_FILETYPE_TIFF_MM_;
    } else if (!memcmp(data, _cc_sig_iff, 4)) {
        return _CC_IMAGE_FILETYPE_IFF_;
    } else if (!memcmp(data, _cc_sig_ico, 4)) {
        return _CC_IMAGE_FILETYPE_ICO_;
    } else if (len < 12) {
        return _CC_IMAGE_FILETYPE_UNKNOWN_;
    }

    if (!memcmp(data, _cc_sig_jp2, 12)) {
        return _CC_IMAGE_FILETYPE_JP2_;
    }

    return _CC_IMAGE_FILETYPE_UNKNOWN_;
}

_cc_image_t* _cc_init_image(uint32_t format, uint32_t width, uint32_t height) {
    return _cc_init_image_data(format,width,height,nullptr);
}


_cc_image_t * _cc_image_from_file(const tchar_t *file_name) {
    _cc_image_t *img = nullptr;
    _cc_buf_t buf;
    _cc_image_filetype_t filetype;

    if (!_cc_buf_from_file(&buf,file_name)) {
        return nullptr;
    }

    filetype = _cc_get_imagetypes( buf.bytes, (int32_t)buf.length );
    
    if (filetype == _CC_IMAGE_FILETYPE_UNKNOWN_) {
        _cc_free_buf(&buf);
        _cc_logger_error(_T("Image file type(%d) unknown :%s"), filetype, file_name);

        return nullptr;
    }

    switch(filetype) {
        case _CC_IMAGE_FILETYPE_BMP_:
            img = _cc_load_BMP( buf.bytes, (int32_t)buf.length );
            break;
#ifdef _CC_3RD_LIBPNG_
        case _CC_IMAGE_FILETYPE_PNG_:
            img = _cc_load_PNG( buf.bytes, (int32_t)buf.length );
            break;
#endif
#ifdef _CC_3RD_LIBJPG_
        case _CC_IMAGE_FILETYPE_JPEG_:
        case _CC_IMAGE_FILETYPE_JP2_:
            img = _cc_load_JPG( buf.bytes, (int32_t)buf.length );
            break;
#endif
        case _CC_IMAGE_FILETYPE_PCX_:
            img = _cc_load_PCX( buf.bytes, (int32_t)buf.length );
            break;
        case _CC_IMAGE_FILETYPE_TGA_:
            img = _cc_load_TGA( buf.bytes, (int32_t)buf.length );
            break;
        default:
            _cc_logger_error(_T("Unable to load image file:%s"), file_name);
            break;
    }

    _cc_free_buf(&buf);

    return img;
}

bool_t _cc_free_image( _cc_image_t* image ) {
    if ( image == nullptr) {
        return false;
    }

    _cc_safe_free(image->data);

    if (image->palette.data && image->palette.size > 0) {
        _cc_free(image->palette.data);
        image->palette.data = nullptr;
        image->palette.size = 0;
    }

    _cc_free(image);
    return true;
}


