/*
 * Copyright (c) 2006 - 2018 QIU ZHONG HUAI <huai2011@163.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:

 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes GHO software, freely available from
 *    <https://github.com/huai2001/CC>".
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS`` AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <cc/xxtea.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include "Header.h"

byte_t* keys = (byte_t*)"a3a2836d-0a32-1c";

#define CHUNK_SOURCE 1024
#define CHUNK_DEST 16384

_CC_FORCE_INLINE_ _cc_buf_t* _gzip_def(_cc_buf_t* rbuf, int level) {
    int res, flush;
    z_stream strm;
    _cc_buf_t* buf;
    size_t have, left = 0;

    byte_t dest[CHUNK_DEST];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    // res = deflateInit(&strm, level);
    res = deflateInit2(&strm, level, Z_DEFLATED, MAX_WBITS + 16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (res != Z_OK) {
        return NULL;
    }

    buf = _cc_create_buf(rbuf->length);
    if (buf == NULL) {
        goto DEF_FIAL;
    }
    do {
        have = (rbuf->length - left);

        strm.next_in = (Bytef*)rbuf->bytes + left;
        strm.avail_in = have > CHUNK_SOURCE ? CHUNK_SOURCE : have;
        _cc_assert(strm.avail_in != 0);
        left += strm.avail_in;
        flush = (rbuf->length == left) ? Z_FINISH : Z_NO_FLUSH;

        do {
            strm.avail_out = CHUNK_DEST;
            strm.next_out = (Bytef*)dest;
            /* no bad return value */
            res = deflate(&strm, flush);
            _cc_assert(res != Z_STREAM_ERROR);
            _cc_buf_write(buf, dest, CHUNK_DEST - strm.avail_out);
        } while (strm.avail_out == 0);
        /* all input will be used */
        _cc_assert(strm.avail_in == 0);

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    /* stream will be complete */
    _cc_assert(res == Z_STREAM_END);

    /* clean up and return */
    (void)deflateEnd(&strm);

    return buf;

DEF_FIAL:
    /* clean up and return */
    (void)deflateEnd(&strm);

    if (buf) {
        _cc_destroy_buf(&buf);
    }

    return NULL;
}

_CC_FORCE_INLINE_ int _gzip_inf(_cc_buf_t* buf, byte_t* source, size_t length) {
    int res;
    size_t have, left = 0;
    z_stream strm;
    byte_t out[CHUNK_DEST];

    if (buf == NULL) {
        return Z_STREAM_ERROR;
    }
    _cc_buf_cleanup(buf);

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    res = inflateInit2(&strm, 47);
    if (res != Z_OK) {
        return res;
    }

    /* decompress until deflate stream ends or end of file */
    do {
        have = (length - left);
        strm.next_in = (Bytef*)source + left;
        strm.avail_in = have > CHUNK_SOURCE ? CHUNK_SOURCE : have;
        _cc_assert(strm.avail_in != 0);
        left += strm.avail_in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK_DEST;
            strm.next_out = (Bytef*)out;
            res = inflate(&strm, Z_NO_FLUSH);
            /* state not clobbered */
            _cc_assert(res != Z_STREAM_ERROR);
            switch (res) {
                case Z_NEED_DICT:
                    res = Z_DATA_ERROR; /* and fall through */
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    goto INF_FIAL;
                    break;
            }

            have = CHUNK_DEST - strm.avail_out;
            _cc_buf_write(buf, out, have);
        } while (strm.avail_out == 0);
        /* done when inflate() says it's done */
    } while (res != Z_STREAM_END);

INF_FIAL:
    /* clean up and return */
    (void)inflateEnd(&strm);
    return res == Z_STREAM_END ? Z_OK : res;
}

_cc_buf_t* _xxtea_decrypt_file(const tchar_t* source_path) {
    _cc_buf_t* fdata;
    byte_t* output;
    size_t output_length;

    fdata = _cc_load_buf(source_path);

    if (fdata) {
        output = _cc_xxtea_decrypt(fdata->bytes, fdata->length, keys, &output_length);
        if (output) {
            int res = _gzip_inf(fdata, output, output_length);
            _cc_free(output);
            if (res != Z_OK) {
                printf("_gzip_inf fail. %s\n", source_path);
            } else {
                printf("successfully. %s\n", source_path);
                return fdata;
            }
        } else {
            printf("_cc_xxtea_decrypt fail. %s\n", source_path);
        }
        _cc_destroy_buf(&fdata);
    }
    return NULL;
}

void _xxtea_encrypt_file(_cc_buf_t* buf, const tchar_t* save_path) {
    _cc_buf_t* fdata;
    byte_t* output;
    size_t output_length = 0;

    _cc_file_t* w;
    w = _cc_open_file(save_path, _T("w"));
    if (w == NULL) {
        return;
    }

    fdata = _gzip_def(buf, Z_DEFAULT_COMPRESSION);

    if (fdata) {
        size_t left;
        size_t bytes_write;
        output = _cc_xxtea_encrypt(fdata->bytes, fdata->length, keys, &output_length);
        if (output && output_length > 0) {
            left = 0;
            do {
                bytes_write = _cc_file_write(w, output + left, sizeof(byte_t),
                                             output_length - left);
                if (bytes_write <= 0) {
                    break;
                }
                left += bytes_write;
            } while (output_length != left);

            _cc_file_close(w);
            _cc_free(output);
            if (output_length == left) {
                printf("successfully. %s\n", save_path);
            } else {
                printf("fwrite fail. %s\n", save_path);
            }
        } else {
            printf("_cc_xxtea_encrypt fail. %s\n", save_path);
        }
        _cc_destroy_buf(&fdata);
    } else {
        printf("_gzip_def fail. %s\n", save_path);
    }
}