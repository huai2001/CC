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
static bool_t _alloc_response_header(_cc_http_response_header_t **url_response, tchar_t *line, int length) {
    int first = 0, last = 0;
    _cc_http_response_header_t *response = *url_response;
    //_tprintf(_T("%.*s\n"), length, line);
    if (response == NULL) {
        /* Parse the first line of the HTTP response */
        if (_tcsnicmp(line, _T("HTTP/"), 5) != 0) {
            /* LOG: bad protocol in HTTP header */
            return false;
        }

        response = (_cc_http_response_header_t *)_cc_calloc(1, sizeof(_cc_http_response_header_t));
        if (response == NULL) {
            return false;
        }
        *url_response = response;
        _CC_RB_INIT_ROOT(&response->headers);
        
        response->download_length = 0;
        response->length = 0;

        /*LOG: HTTP Protocol*/
        /* Find the first non-space letter */
        _cc_first_index_of(first, length, _cc_isspace(line[first]));
        last = first;
        _cc_first_index_of(last, length, !_cc_isspace(line[last]));
        if (first == last) {
            return false;
        }
        response->protocol = _cc_tcsndup(&line[first], last - first);
        if (response->protocol == NULL) {
            return false;
        }

        first = last;
        /*LOG: HTTP Status*/
        _cc_first_index_of(first, length, _cc_isspace(line[first]));
        last = first;
        _cc_first_index_of(last, length, !_cc_isspace(line[last]));
        if (first == last) {
            return false;
        }
        line[last - first] = 0;
        response->status = _ttoi(&line[first]);

        /*LOG: HTTP Description*/
        first = last;
        _cc_first_index_of(first, length, _cc_isspace(line[first]));
        _cc_last_index_of(first, length, _cc_isspace(line[length]));
        response->description = _cc_tcsndup(&line[first], length - first);
        if (response->description == NULL) {
            return false;
        }
        return true;
    }
    return __http_header_line(&response->headers, line, length);
}

static void _free_response_header(_cc_http_response_header_t **response_header) {
    _cc_http_response_header_t *res = *response_header;
    _cc_assert(response_header != NULL && res != NULL);
    if (response_header == NULL || res == NULL) {
        return;
    }

    (*response_header) = NULL;

    if (res->protocol) {
        _cc_free(res->protocol);
    }

    if (res->description) {
        _cc_free(res->description);
    }

    if (res->location) {
        _cc_free(res->location);
    }

    _cc_dict_free(&res->headers);

    _cc_free(res);
}