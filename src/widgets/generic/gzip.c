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
#include <zlib.h>
#include <libcc/widgets/gzip.h>

#define CHUNK_SOURCE        (1024 * 8)
#define CHUNK_DEST          (1024 * 32)

_CC_API_PUBLIC(void) _gzip_clean(pvoid_t gzip) {
    /* clean up */
    (void)inflateEnd((z_stream*)gzip);
    _cc_free(gzip);
}

_CC_API_PUBLIC(void) _gzip_reset(pvoid_t gzip) {
    /*  */
    (void)inflateReset((z_stream*)gzip);
}

_CC_API_PUBLIC(pvoid_t) _gzip_inf_init(void) {
    z_stream *strm = (z_stream*)_cc_malloc(sizeof(z_stream));
    /* allocate inflate state */
    strm->zalloc = Z_NULL;
    strm->zfree = Z_NULL;
    strm->opaque = Z_NULL;
    strm->avail_in = 0;
    strm->next_in = Z_NULL;

    if (inflateInit2(strm, MAX_WBITS + 32) != Z_OK) {
        _cc_free(strm);
        return nullptr;
    }
    return strm;
}

_CC_API_PUBLIC(bool_t) _gzip_inf(pvoid_t gzip, byte_t *source, size_t length, _cc_buf_t *buffer) {
    int res = Z_STREAM_ERROR;
    size_t have, left = 0;
    byte_t out[CHUNK_DEST];
    z_stream *strm = (z_stream*)gzip;

    /* decompress until deflate stream ends or end of file */
    do {
        have = (length - left);
        if (have == 0) {
            break;
        }
        strm->next_in = (Bytef *)source + left;
        strm->avail_in = (have > CHUNK_SOURCE) ? CHUNK_SOURCE : (uInt)have;

        left += strm->avail_in;
        /* run inflate() on input until output buffer not full */
        do {
            strm->avail_out = CHUNK_DEST;
            strm->next_out = (Bytef *)out;
            res = inflate(strm, Z_NO_FLUSH);
            switch (res) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                goto INF_FIAL;
                break;
            }
            have = CHUNK_DEST - strm->avail_out;
            if (_cc_likely(have > 0)) {
                _cc_buf_append(buffer, out, have);
            }

        } while (strm->avail_out == 0);
        /* done when inflate() says it's done */
    } while (res != Z_STREAM_END);

    return (res == Z_OK || res == Z_STREAM_END);

INF_FIAL:
    /* clean up and return */
    inflateReset(strm);
    return false;
}