#include <libcc/alloc.h>
#include <libcc/http.h>
/**/
_CC_API_PUBLIC(bool_t) _cc_http_alloc_response_header(_cc_http_response_header_t **url_response, tchar_t *line, int32_t length) {
    int32_t first = 0, last = 0;
    _cc_http_response_header_t *response = *url_response;
    //_tprintf(_T("%.*s\n"), length, line);
    if (response == nullptr) {
        /* Parse the first line of the HTTP response */
        if (_tcsnicmp(line, _T("HTTP/"), 5) != 0) {
            /* LOG: bad protocol in HTTP header */
            return false;
        }

        response = (_cc_http_response_header_t *)_cc_calloc(1, sizeof(_cc_http_response_header_t));
        if (response == nullptr) {
            return false;
        }
        
        *url_response = response;
        _cc_rbtree_cleanup(&response->headers);
        
        response->download_length = 0;
        response->content_length = 0;

        /*LOG: HTTP Protocol*/
        /* Find the first non-space letter */
        _cc_first_index_of(first, length, _cc_isspace(line[first]));
        last = first;
        _cc_first_index_of(last, length, !_cc_isspace(line[last]));
        if (first == last) {
            return false;
        }
        response->protocol = _cc_sds_alloc(&line[first], last - first);

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
        response->description = _cc_sds_alloc(&line[first], length - first);

        return true;
    }
    return _cc_http_header_line(&response->headers, line, length);
}

_CC_API_PUBLIC(void) _cc_http_free_response_header(_cc_http_response_header_t **response_header) {
    _cc_http_response_header_t *res = *response_header;
    _cc_assert(response_header != nullptr && res != nullptr);
    if (response_header == nullptr || res == nullptr) {
        return;
    }

    (*response_header) = nullptr;

    if (res->protocol) {
        _cc_sds_free(res->protocol);
    }

    if (res->description) {
        _cc_sds_free(res->description);
    }

    if (res->location) {
        _cc_sds_free(res->location);
    }

    _cc_http_header_destroy(&res->headers);

    _cc_free(res);
}