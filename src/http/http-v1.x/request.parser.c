#include <libcc/alloc.h>
#include <libcc/url.h>
#include <libcc/http.h>

/**/
_CC_API_PUBLIC(bool_t) _cc_http_alloc_request_header(_cc_http_request_header_t **http_header, tchar_t *line, int32_t length) {
    int32_t first = 0, last = 0;
    _cc_http_request_header_t *request = *http_header;

    //printf("%s\n", line);

    if (request == NULL) {
        tchar_t script[_CC_MAX_PATH_];
        /* Parse the first line of the HTTP request */
        //if (_tcsnicmp(line, _T("CONNECT"), sizeof(_T("CONNECT")) - 1) != 0 || _tcsnicmp(line, _T("GET"), sizeof(_T("GET")) - 1) != 0) {
            /* LOG: bad protocol in HTTP header */
         //   return false;
        //}

        request = (_cc_http_request_header_t *)_cc_calloc(1, sizeof(_cc_http_request_header_t));
        if (request == NULL) {
            return false;
        }
        
        *http_header = request;
        _cc_rbtree_cleanup(&request->headers);

        /*LOG: HTTP Protocol*/
        /* CONNECT www.baidu.com:443 HTTP/1.1 */
        /* GET / HTTP/1.1 */
        /* Find the first non-space letter */
        _cc_find_first_of(first, length, _CC_ISSPACE(line[first]));
        last = first;
        _cc_find_first_of(last, length, !_CC_ISSPACE(line[last]));
        if (first == last) {
            return false;
        }
        request->method = _cc_sds_alloc(&line[first], last - first);

        first = last;
        /*LOG: HTTP Script(Host:Port)*/
        _cc_find_first_of(first, length, _CC_ISSPACE(line[first]));
        last = first;
        _cc_find_first_of(last, length, !_CC_ISSPACE(line[last]));
        if (first == last) {
            return false;
        }

        first = _cc_url_decode(&line[first], last - first, script, _CC_MAX_PATH_);
        request->script = _cc_sds_alloc(script, first);

        /*LOG: HTTP Protocol*/
        first = last;
        _cc_find_first_of(first, length, _cc_isspace(line[first]));
        _cc_find_last_of(first, length, _cc_isspace(line[length]));
        request->protocol = _cc_sds_alloc(&line[first], length - first);
        return true;
    }

    return _cc_http_header_line(&request->headers, line, length);
}

/**/
_CC_API_PUBLIC(void) _cc_http_free_request_header(_cc_http_request_header_t **http_header) {
    _cc_http_request_header_t *res = *http_header;
    _cc_assert(http_header != NULL && res != NULL);
    if (http_header == NULL || res == NULL) {
        return;
    }
    
    (*http_header) = NULL;
    
    if (res->method) {
        _cc_sds_free(res->method);
    }

    if (res->script) {
        _cc_sds_free(res->script);
    }

    if (res->protocol) {
        _cc_sds_free(res->protocol);
    }

    _cc_http_header_destroy(&res->headers);

    _cc_free(res);
}