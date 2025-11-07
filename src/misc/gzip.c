#include <zlib.h>
#include <libcc/alloc.h>
#include <libcc/gzip.h>

#define CHUNK_SOURCE        (1024 * 8)
#define CHUNK_DEST          (1024 * 32)

struct _gzip {
    z_stream strm;
    int (*cb)(z_streamp, int);
};

_CC_API_PUBLIC(void) _gzip_free(_gzip_t *gzip) {
    /* clean up */
    (void)inflateEnd((z_stream*)&gzip->strm);
    _cc_free(gzip);
}

_CC_API_PUBLIC(void) _gzip_reset(_gzip_t *gzip) {
    /*  */
    (void)inflateReset((z_stream*)&gzip->strm);
}

_CC_API_PUBLIC(_gzip_t*) _gzip_alloc(byte_t m) {
    _gzip_t *gzip = (_gzip_t*)_cc_malloc(sizeof(_gzip_t));
    /* allocate inflate state */
    gzip->strm.zalloc = Z_NULL;
    gzip->strm.zfree = Z_NULL;
    gzip->strm.opaque = Z_NULL;
    gzip->strm.avail_in = 0;
    gzip->strm.next_in = Z_NULL;
    if (m == _GZIP_INF_) {
        if (inflateInit2(&gzip->strm, MAX_WBITS + 32) != Z_OK) {
            _cc_free(gzip);
            return nullptr;
        }
        gzip->cb = inflate;
    } else {
        if (deflateInit2(&gzip->strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK) {
            _cc_free(gzip);
            return nullptr;
        }
        gzip->cb = deflate;
    }
    return gzip;
}

_CC_API_PUBLIC(bool_t) _gzip(_gzip_t *gzip, byte_t *source, size_t length, _cc_buf_t *buffer, bool_t finish) {
    int res = Z_STREAM_ERROR, flush;
    size_t have, left = 0;
    byte_t out[CHUNK_DEST];
    z_stream *strm = (z_stream*)&gzip->strm;

    /* decompress until deflate stream ends or end of file */
    do {
        have = (length - left);
        if (have == 0) {
            break;
        }
        strm->next_in = (Bytef *)source + left;
        strm->avail_in = (have > CHUNK_SOURCE) ? CHUNK_SOURCE : (uInt)have;

        left += strm->avail_in;
        flush = (finish && (length == left)) ? Z_FINISH : Z_NO_FLUSH;
        /* run inflate() on input until output buffer not full */
        do {
            strm->avail_out = CHUNK_DEST;
            strm->next_out = (Bytef *)out;
            res = gzip->cb(strm, flush);
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