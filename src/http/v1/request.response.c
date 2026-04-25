#include <libcc/http_request.h>
#include <libcc/gzip.h>
#include <libcc/event.h>

/**/
_CC_API_PUBLIC(bool_t) _cc_http_response_body(_cc_http_request_t *request, byte_t *source, size_t length) {
    _cc_http_response_header_t *response = request->response;

    if (response->content_encoding == _CC_URL_CONTENT_ENCODING_GZIP_) {
        return _gzip(request->gzip, source, length, &request->buffer, false);
    }

    if (response->content_encoding == _CC_URL_CONTENT_ENCODING_PLAINTEXT_) {
        _cc_buf_append(&request->buffer, source, length);
        return true;
    }

    return false;
}

/**/
_CC_API_PRIVATE(size_t) _url_chunked_hex_length(const char_t *p, size_t *length_of_data, size_t length) {
    size_t offset = 0;

    *length_of_data = 0;
    if (*p == _CC_CR_ && *(p + 1) == _CC_LF_) {
        offset = 2;
        p += 2;
    }

    for (; offset < length; offset++, p++) {
        if ((*p) >= '0' && (*p) <= '9') {
            *length_of_data = (*length_of_data << 4) + (*p & 0x0F);
        } else if (((*p) >= 'a' && (*p) <= 'f') || ((*p) >= 'A' && (*p) <= 'F')) {
            *length_of_data = (*length_of_data << 4) + ((*p & 0x0F) + 0x09);
        } else {
            if ((length - offset) < 2) {
                break;
            }
            if (*p == _CC_CR_ && *(p + 1) == _CC_LF_) {
                return offset + 2;
            }
            return -1;
        }
    }

    return 0;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_http_response_chunked(_cc_http_request_t *request, _cc_io_buffer_t *io) {
    /**/
    size_t offset_of_data = 0;
    size_t length_of_data;
    _cc_http_response_header_t *response = request->response;

    do {
        if (response->content_length <= 0) {
            size_t offset = _url_chunked_hex_length((const char_t *)(io->r.bytes + offset_of_data), &length_of_data, io->r.off);
            if (offset < 0) {
                return false;
            } else if (offset == 0) {
                break;
            }

            if (length_of_data == 0) {
                request->state = _CC_HTTP_STATE_ESTABLISHED_;
                break;
            }
            response->content_length = length_of_data;
            offset_of_data += offset;
            io->r.off -= (uint16_t)offset;
        }

        if (response->content_length > io->r.off) {
            length_of_data = io->r.off;
            response->content_length -= length_of_data;
        } else {
            length_of_data = (size_t)response->content_length;
            response->content_length = 0;
        }

        if (!_cc_http_response_body(request, io->r.bytes + offset_of_data, length_of_data)) {
            return false;
        }

        offset_of_data += length_of_data;
        io->r.off -= (uint16_t)length_of_data;
        //\r\n0\r\n\r\n
    } while (io->r.off >= 2);

    io->r.off = 0;
    /**/
    response->download_length += length_of_data;

    return true;
}