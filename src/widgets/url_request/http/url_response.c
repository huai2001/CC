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
#include <cc/alloc.h>
#include <cc/atomic.h>
#include <cc/string.h>
#include <cc/url.h>
#include <cc/widgets/url_request.h>

bool_t _gzip_inf(_cc_url_request_t *request, byte_t *source, size_t length);
/**/
bool_t _cc_url_read_response_body(_cc_url_request_t *request, byte_t *source, size_t length) {
    _cc_http_response_header_t *response = request->response;
    if (response->content_encoding == _CC_URL_CONTENT_ENCODING_GZIP_) {
        return _gzip_inf(request, source, length);
    }

    if (response->content_encoding == _CC_URL_CONTENT_ENCODING_PLAINTEXT_) {
        _cc_buf_write(&request->buffer, source, length);
        return true;
    }
    return false;
}

/**/
static size_t _url_chunked_hex_length(const char_t *p, size_t *length_of_data, size_t length) {
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
bool_t _cc_url_read_response_chunked(_cc_url_request_t *request, _cc_event_rbuf_t *rbuf) {
    /**/
    size_t offset_of_data = 0;
    size_t length_of_data;

    do {
        if (request->response->download_length <= 0) {
            size_t offset =
                _url_chunked_hex_length((const char_t *)(rbuf->buf + offset_of_data), &length_of_data, rbuf->length);
            if (offset < 0) {
                return false;
            } else if (offset == 0) {
                break;
            }

            if (length_of_data == 0) {
                request->status = _CC_HTTP_RESPONSE_SUCCESS_;
                break;
            }
            request->response->download_length = length_of_data;
            offset_of_data += offset;
            rbuf->length -= offset;
        }

        if (request->response->download_length > rbuf->length) {
            length_of_data = rbuf->length;
            request->response->download_length -= length_of_data;
        } else {
            length_of_data = (size_t)request->response->download_length;
            request->response->download_length = 0;
        }

        if (!_cc_url_read_response_body(request, rbuf->buf + offset_of_data, length_of_data)) {
            return false;
        }

        offset_of_data += length_of_data;
        rbuf->length -= length_of_data;
        //\r\n0\r\n
    } while (rbuf->length > 2);

    rbuf->length = 0;

    request->response->length += length_of_data;
    return true;
}