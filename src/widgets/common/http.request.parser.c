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
#include "./http.header.c"

/**/
_CC_API_PRIVATE(bool_t) _alloc_request_header(_cc_http_request_header_t **http_header, tchar_t *line, int length) {
    int first = 0, last = 0;
    _cc_http_request_header_t *request = *http_header;

    //printf("%s\n", line);

    if (request == NULL) {
        /* Parse the first line of the HTTP request */
        //if (_tcsnicmp(line, _T("CONNECT"), sizeof(_T("CONNECT")) - 1) != 0 || _tcsnicmp(line, _T("GET"), sizeof(_T("GET")) - 1) != 0) {
            /* LOG: bad protocol in HTTP header */
         //   return false;
        //}

        request = (_cc_http_request_header_t *)_cc_calloc(1, sizeof(_cc_http_request_header_t));
        if (request == NULL) {
            return false;
        }
        request->count = 0;
        *http_header = request;
        _CC_RB_INIT_ROOT(&request->headers);

        /*LOG: HTTP Protocol*/
        /* Find the first non-space letter */
        _cc_first_index_of(first, length, _cc_isspace(line[first]));
        last = first;
        _cc_first_index_of(last, length, !_cc_isspace(line[last]));
        if (first == last) {
            return false;
        }
        request->method = _cc_tcsndup(&line[first], last - first);
        if (request->method == NULL) {
            return false;
        }

        first = last;
        /*LOG: HTTP Script(Host:Port)*/
        _cc_first_index_of(first, length, _cc_isspace(line[first]));
        last = first;
        _cc_first_index_of(last, length, !_cc_isspace(line[last]));
        if (first == last) {
            return false;
        }
        request->script = _cc_tcsndup(&line[first], last - first);
        if (request->script == NULL) {
            return false;
        }

        /*LOG: HTTP Protocol*/
        first = last;
        _cc_first_index_of(first, length, _cc_isspace(line[first]));
        _cc_last_index_of(first, length, _cc_isspace(line[length]));
        request->protocol = _cc_tcsndup(&line[first], length - first);
        if (request->protocol == NULL) {
            return false;
        }
        return true;
    }

    return __http_header_line(&request->headers, line, length);
}

_CC_API_PRIVATE(void) _free_request_header(_cc_http_request_header_t **http_header) {
    _cc_http_request_header_t *res = *http_header;
    _cc_assert(http_header != NULL && res != NULL);
    if (http_header == NULL || res == NULL) {
        return;
    }
    
    (*http_header) = NULL;
    
    if (res->method) {
        _cc_free(res->method);
    }

    if (res->script) {
        _cc_free(res->script);
    }

    if (res->protocol) {
        _cc_free(res->protocol);
    }

    _cc_dict_free(&res->headers);

    _cc_free(res);
}