/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
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

/**/
_CC_API_PUBLIC(bool_t) _cc_http_alloc_request_header(_cc_http_request_header_t **http_header, tchar_t *line, int length) {
    int first = 0, last = 0;
    _cc_http_request_header_t *request = *http_header;

    //printf("%s\n", line);

    if (request == nullptr) {
        /* Parse the first line of the HTTP request */
        //if (_tcsnicmp(line, _T("CONNECT"), sizeof(_T("CONNECT")) - 1) != 0 || _tcsnicmp(line, _T("GET"), sizeof(_T("GET")) - 1) != 0) {
            /* LOG: bad protocol in HTTP header */
         //   return false;
        //}

        request = (_cc_http_request_header_t *)_cc_calloc(1, sizeof(_cc_http_request_header_t));
        if (request == nullptr) {
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
        if (request->method == nullptr) {
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
        if (request->script == nullptr) {
            return false;
        }

        /*LOG: HTTP Protocol*/
        first = last;
        _cc_first_index_of(first, length, _cc_isspace(line[first]));
        _cc_last_index_of(first, length, _cc_isspace(line[length]));
        request->protocol = _cc_tcsndup(&line[first], length - first);
        if (request->protocol == nullptr) {
            return false;
        }
        return true;
    }

    return _cc_http_header_line(&request->headers, line, length);
}

/**/
_CC_API_PUBLIC(void) _cc_http_free_request_header(_cc_http_request_header_t **http_header) {
    _cc_http_request_header_t *res = *http_header;
    _cc_assert(http_header != nullptr && res != nullptr);
    if (http_header == nullptr || res == nullptr) {
        return;
    }
    
    (*http_header) = nullptr;
    
    if (res->method) {
        _cc_free(res->method);
    }

    if (res->script) {
        _cc_free(res->script);
    }

    if (res->protocol) {
        _cc_free(res->protocol);
    }

    _cc_map_free(&res->headers);

    _cc_free(res);
}