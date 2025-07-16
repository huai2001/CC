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
#include <libcc/widgets/http.h>

_CC_API_PUBLIC(bool_t) _cc_http_header_line(_cc_map_t *headers, tchar_t *line, int length) {
    int first = 0, last = 0, i = 0;
    _cc_map_element_t *m = (_cc_map_element_t*)_cc_malloc(sizeof(_cc_map_element_t));

    /* Find the first non-space letter */
    _cc_first_index_of(first, length, _cc_isspace(line[first]));
    last = first;
    
    if (line[last] == ':') {
        last++;
    }

    _cc_first_index_of(last, length, line[last] != ':');
    if (line[last] != ':') {
        _cc_free(m);
        return false;
    }

    i = last;
    /*Find the last non-space letter*/
    _cc_last_index_of(first, i, _cc_isspace(line[i]));
    if (i == first) {
        _cc_free(m);
        return false;
    }
    
    m->name = _cc_tcsndup(&line[first], i);
    if (m->name == nullptr) {
        _cc_free(m);
        return false;
    }

    last += 1;
    /* Find the first non-space letter */
    _cc_first_index_of(last, length, _cc_isspace(line[last]));
    /*Find the last non-space letter*/
    _cc_last_index_of(last, length, _cc_isspace(line[length]));
    if (last == length) {
        return false;
    }

    m->type = _CC_MAP_STRING_;
    m->element.uni_string = _cc_tcsndup(&line[last], length - last);
    if (m->element.uni_string == nullptr) {
        _cc_map_element_free(m);
        return false;
    }
    return _cc_map_push(headers, m);
}

/**/
_CC_API_PUBLIC(int) _cc_http_header_parser(_cc_http_header_fn_t fn, pvoid_t *arg, _cc_event_rbuf_t* r) {
    int i = 0;
    byte_t *n;
    byte_t *start = (byte_t*)r->bytes;
#ifdef _CC_UNICODE_
    wchar_t buf[1024];
#endif
    int result = _CC_HTTP_STATUS_HEADER_;
    while (true) {
        n = memchr(start, '\n', r->length - (start - r->bytes));
        if (n == nullptr) {
            break;
        }

        if (*(n - 1) == '\r') {
            if ((size_t)(n - start) > 1024) {
                _cc_logger_error(_T("size of header is too bigger"));
                return _CC_HTTP_ERROR_TOOLARGE_;
            }
            /*If we received just a CR LF on a line, the headers are finished*/
            if ((n - 1) == start) {
                start = n + 1;
                result = _CC_HTTP_STATUS_BODY_;
                break;
            }
#ifdef _CC_UNICODE_
            i = _cc_utf8_to_utf16((const uint8_t *)start, (const uint8_t *)n, (uint16_t *)buf, (uint16_t *)&buf[_cc_countof(buf)], false);
            if (!fn(arg, (wchar_t*)buf, i - 1)) {
#else
            if (!fn(arg, (tchar_t*)start, (int)(n - start - 1))) {
#endif
                return _CC_HTTP_ERROR_BADREQUEST_;
            }
            start = n + 1;
        } else {
            return _CC_HTTP_ERROR_BADREQUEST_;
        }
    }

    if (start == r->bytes) {
        return result;
    }

    i = (int)(r->length - (start - r->bytes));
    if (i > 0) {
        memmove(r->bytes, start, i);
    }

    r->length = i;
    return result;
}
