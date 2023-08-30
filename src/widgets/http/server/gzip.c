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
#include <cc/http/http.h>
#include "../kv.h"
#include <stdio.h>
#include <zlib.h>

#define CHUNK _CC_IO_BUFFER_SIZE_

/* Compress from file fp to buffer until EOF on fp.
 def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
 allocated for processing, Z_STREAM_ERROR if an invalid compression
 level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
 version of the library linked do not match, or Z_ERRNO if there is
 an error reading or writing the files. */
static _cc_buf_t* _http_gzip_def(const tchar_t* source_file,
                                 int level,
                                 size_t file_size) {
    int res, flush;
    z_stream strm;
    _cc_buf_t* buf;

    byte_t source[CHUNK];
    byte_t dest[CHUNK];

    FILE* fp;

    fp = _tfopen(source_file, _T("rb"));
    if (fp == NULL) {
        return NULL;
    }

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    // res = deflateInit(&strm, level);
    res = deflateInit2(&strm, level, Z_DEFLATED, MAX_WBITS + 16, MAX_MEM_LEVEL,
                       Z_DEFAULT_STRATEGY);
    if (res != Z_OK) {
        return NULL;
    }

    buf = _cc_create_buf(file_size);
    if (buf == NULL) {
        goto DEF_FIAL;
    }
    /* compress until end of file */
    do {
        strm.avail_in = (uint32_t)fread(source, 1, CHUNK, fp);
        if (ferror(fp)) {
            goto DEF_FIAL;
        }

        flush = feof(fp) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = (Bytef*)source;

        do {
            strm.avail_out = CHUNK;
            strm.next_out = (Bytef*)dest;
            /* no bad return value */
            res = deflate(&strm, flush);
            _cc_assert(res != Z_STREAM_ERROR);
            _cc_buf_write(buf, dest, CHUNK - strm.avail_out);
        } while (strm.avail_out == 0);
        /* all input will be used */
        _cc_assert(strm.avail_in == 0);

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    /* stream will be complete */
    _cc_assert(res == Z_STREAM_END);

    /* clean up and return */
    (void)deflateEnd(&strm);
    fclose(fp);

    return buf;

DEF_FIAL:
    /* clean up and return */
    (void)deflateEnd(&strm);
    fclose(fp);

    if (buf) {
        _cc_destroy_buf(&buf);
    }

    return NULL;
}

bool_t _http_gzip_response(const tchar_t* str_content_type,
                           const tchar_t* fp,
                           size_t size,
                           _cc_http_t* res,
                           _cc_event_wbuf_t* w) {
    _cc_http_listener_t* listener = res->listener;
    bool_t gzip = false;
    _cc_buf_t* buf;
    const tchar_t* str_accept_encoding;
    if (str_content_type == NULL) {
        return false;
    }

    if (size < (size_t)listener->gzip.min_length) {
        return false;
    }

    if (listener->gzip.types == NULL) {
        return false;
    }

    str_accept_encoding =
        _cc_find_kv(&res->request.headers, _T("Accept-Encoding"));
    if (!str_accept_encoding ||
        _tcsstr(str_accept_encoding, _T("gzip")) == NULL) {
        return false;
    }

    _cc_rbtree_for_each(v, listener->gzip.types, {
        _cc_json_t* j = _cc_upcast(v, _cc_json_t, node);
        const tchar_t* value = (tchar_t*)_cc_json_string(j);
        if (value && _tcsstr(str_content_type, value) != NULL) {
            gzip = true;
            break;
        }
    });

    if (gzip == false) {
        return false;
    }

    buf = _http_gzip_def(fp, listener->gzip.level, size);
    if (buf == NULL) {
        return false;
    }

    res->response.content.buf = buf;
    res->response.descriptor = 2;
    res->response.status = 200;
    res->response.length = buf->length;

    _cc_http_insert_header(res, _T("Content-Encoding"), _T("gzip"));
    _cc_http_write_header(res, w);
    return true;
}