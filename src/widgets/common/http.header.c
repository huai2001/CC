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

static bool_t __http_header_line(_cc_dict_t *headers, tchar_t *line, int length) {
    int first = 0, last = 0, i = 0;
    tchar_t *name;
    tchar_t *value;

    /* Find the first non-space letter */
    _cc_first_index_of(first, length, _cc_isspace(line[first]));
    last = first;
    
    if (line[last] == ':') {
        last++;
    }

    _cc_first_index_of(last, length, line[last] != ':');
    if (line[last] != ':') {
        return false;
    }

    i = last;
    /*Find the last non-space letter*/
    _cc_last_index_of(first, i, _cc_isspace(line[i]));
    if (i == first) {
        return false;
    }
    
    name = _cc_tcsndup(&line[first], i);
    if (name == NULL) {
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

    value = _cc_tcsndup(&line[last], length - last);
    if (value == NULL) {
        _cc_free(name);
        return false;
    }
    
    return _cc_dict_insert(headers, name, value);
}

/**/
static int _cc_http_header_parser(_cc_http_header_fn_t fn, pvoid_t *arg, _cc_event_rbuf_t* r) {
    int i = 0;
    byte_t *n;
    byte_t *start = (byte_t*)r->buf;
#ifdef _CC_UNICODE_
    wchar_t buf[1024];
#endif
    int result = _CC_HTTP_RESPONSE_HEADER_;
    while (true) {
        n = memchr(start, '\n', r->length - (start - r->buf));
        if (n == NULL) {
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
                result = _CC_HTTP_RESPONSE_BODY_;
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

    if (start == r->buf) {
        return result;
    }

    i = (int)(r->length - (start - r->buf));
    if (i > 0) {
        memmove(r->buf, start, i);
    }

    r->length = i;
    return result;
}
