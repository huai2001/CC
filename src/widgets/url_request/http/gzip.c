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
#include <cc/widgets/url_request.h>
#include <stdio.h>
#include <zlib.h>

#define CHUNK_SOURCE 1024
#define CHUNK_DEST 16384

bool_t _gzip_inf(_cc_url_request_t *request, byte_t *source, size_t length) {
    int res;
    size_t have, left = 0;
    z_stream strm;
    byte_t out[CHUNK_DEST];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    res = inflateInit2(&strm, MAX_WBITS + 32);
    if (res != Z_OK) {
        return res;
    }

    /* decompress until deflate stream ends or end of file */
    do {
        have = (length - left);
        strm.next_in = (Bytef *)source + left;
        strm.avail_in = (have > CHUNK_SOURCE) ? CHUNK_SOURCE : (uInt)have;

        left += strm.avail_in;
        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK_DEST;
            strm.next_out = (Bytef *)out;
            res = inflate(&strm, Z_NO_FLUSH);
            switch (res) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                goto INF_FIAL;
                break;
            }

            if (_cc_likely(strm.avail_out > 0 && CHUNK_DEST > strm.avail_out)) {
                _cc_buf_write(&request->buffer, out, CHUNK_DEST - strm.avail_out);
            }

        } while (strm.avail_out == 0);
        /* done when inflate() says it's done */
    } while (res != Z_STREAM_END);

INF_FIAL:
    /* clean up and return */
    (void)inflateEnd(&strm);
    return res == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}
