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

#include <cc/http/http.h>
#include <sys/stat.h>

#define RBUF_LEN(rw) (rw)->r.length
#define RBUF(rw) (rw)->r.buf

#define WBUF_LEN(rw) (rw)->w.length
#define WBUF(rw) (rw)->w.buf

#define METHOD_XX(I, X) \
    { I, sizeof(X), X }
const _cc_http_method_t supported_http_methods[] = {
    METHOD_XX(_CC_HTTP_METHOD_GET_, _T("GET")),
    METHOD_XX(_CC_HTTP_METHOD_POST_, _T("POST")),
    METHOD_XX(_CC_HTTP_METHOD_HEAD_, _T("HEAD")),
    // METHOD_XX(_CC_HTTP_METHOD_PUT_, _T("PUT")),
    METHOD_XX(_CC_HTTP_METHOD_DELETE_, _T("DELETE")),
    METHOD_XX(_CC_HTTP_METHOD_OPTIONS_, _T("OPTIONS")),
    // METHOD_XX(_CC_HTTP_METHOD_TRACE_, _T("TRACE")),
    // METHOD_XX(_CC_HTTP_METHOD_CONNECT_, _T("CONNECT")),
};
#undef METHOD_XX

static const _cc_http_method_t* _http_get_method(tchar_t* method) {
    int32_t i;
    const _cc_http_method_t* p;

    if (method == NULL || method[0] == 0) {
        return NULL;
    }

    for (i = 0; i < _cc_countof(supported_http_methods); i++) {
        p = &(supported_http_methods[i]);
        if (0 == _tcsnicmp(method, p->value, p->method_length)) {
            return p;
        }
    }
    return NULL;
}

static void _http_parsing_header_range(_cc_http_t* res, const tchar_t* range) {
    int x1 = 0, x2 = 0;
    int n = _tscanf(range, _T("bytes=%d-%d"), &x1, &x2);
    if (n == 2 && x1 >= 0 && x2 >= x1) {
        res->request.range_start = x1;
        res->request.range_end = x2;
    } else if (n == 1 && x1 > 0) {
        res->request.range_start = x1;
        res->request.range_end = 0x7fffffff;
    }
}

bool_t _http_parsing_header(_cc_http_t* res, _cc_event_buffer_t* rw) {
    char_t* finality;
    tchar_t* v;
    tchar_t* query;
    tchar_t* line;
    char_t* header = (char_t*)RBUF(rw);
#ifdef _CC_UNICODE_
    wchar_t unicode_buf[_CC_IO_BUFFER_SIZE_];
#endif
    int32_t len;
    int32_t size;

    const tchar_t* str_content_length;
    const tchar_t* str_connection;
    const tchar_t* str_host;
    const tchar_t* str_range;
    size_t content_length = 0;

    const _cc_http_method_t* method = NULL;

    finality = strstr(header, "\r\n\r\n");
    if (finality == NULL) {
        return true;
    }
    *finality = 0;
#ifdef _CC_UNICODE_
    _cc_utf8_to_utf16((const uint8_t*)header, (const uint8_t*)finality,
                      (uint16_t*)unicode_buf,
                      (uint16_t*)(unicode_buf + _cc_countof(unicode_buf)),
                      false);
    line = unicode_buf;
#else
    line = header;
#endif
    size = (int32_t)(finality - header) + 4;

    // printf("%line", line);

    /* Parse the first line of the HTTP request */
    v = _http_header_read(line, &line, &len, _cc_isspace);
    if (v) {
        /*GET,POST,PUT,HEAD,DELETE,OPTIONS,TRACE,CONNECT*/
        method = _http_get_method(v);
        res->request.method = _cc_tcsndup(v, len);
        res->request.method_id = method->method;
    }

    if (method == NULL) {
        /* LOG: Unknown request method */
        _http_unimplemented(res, &rw->w);
        return false;
    }

    v = _http_header_read(line, &line, &len, _cc_isspace);
    if (v) {
        if (v[0] != '/') {
            /* LOG: URI does not start with "/" */
            _http_nofound(res, &rw->w);
            return false;
        }

        while (v[1] == '/') {
            v++;
            len--;
        }

        res->request.script = (tchar_t*)_cc_malloc(sizeof(tchar_t) * (len + 1));
        if (res->request.script == NULL) {
            _http_badrequest(res, &rw->w);
            return false;
        }

        len = _cc_url_decode(v, len, res->request.script, len);
        query = _tcschr(res->request.script, _T('?'));
        if (query) {
            *query = 0;
            res->request.query = query + 1;
        }
        /* Do not allow "/." or "/-" to to occur anywhere in the entity name.
        ** This prevents attacks involving ".." and also allows us to create
        ** files and directories whose names begin with "-" or "." which are
        ** invisible to the webserver.
        **
        ** Exception:  Allow the "/.well-known/" prefix in accordance with
        ** RFC-5785.
        */
        v = res->request.script;
        while (*v) {
            if (*v == '/' && (v[1] == '.' || v[1] == '-')) {
                if (_tcsnicmp(v, _T("/.well-known/"), 13) == 0 &&
                    (v[1] != '.' || v[2] != '.')) {
                    v++;
                    /* Exception:  Allow "/." and "/-" for URLs that being with
                    ** "/.well-known/".  But do not allow "/..". */
                    continue;
                }
                /* LOG: Path element begins with "." or "-" */
                _http_nofound(res, &rw->w);
                return false;
            }
            v++;
        }
    }

    v = _http_header_read(line, &line, &len, _cc_isspace);
    if (v) {
        res->request.protocol = _cc_tcsndup(v, len);
    }

    /* LOG: bad protocol in HTTP header */
    if (res->request.protocol == NULL ||
        _tcsnicmp(res->request.protocol, _T("HTTP/"), 5) != 0 ||
        _tcslen(res->request.protocol) != 8) {
        _http_badrequest(res, &rw->w);
        return false;
    }

    while (*line) {
        tchar_t* name = NULL;
        tchar_t* value = NULL;
        tchar_t* str_start = NULL;
        tchar_t* str_end = NULL;
        /* Find the first non-space letter */
        str_start = line = _skip_left_space(line);

        do {
            /* Find the end of the header name */
            if (*line == ':') {
                str_end = line - 1;
                /* skip all trailing space letters */
                while ((str_end > str_start) && _cc_isspace(*str_start)) {
                    str_end--;
                }

                /*Cookie*/
                if (_tcsnicmp(str_start, _T("cookie"), 6) == 0) {
                    name = (tchar_t*)_cc_malloc(sizeof(tchar_t) * 10);
                    _sntprintf(name, 10, _T("Cookie-%d"), res->cookies);
                    res->cookies++;
                } else {
                    name = _cc_tcsndup(str_start, str_end - str_start + 1);
                }
                break;
            }
        } while (++line);

        if (*line == 0) {
            _cc_safe_free(name);
            break;
        }

        if (name == NULL) {
            line = _tcschr(line, _CC_LF_);
            continue;
        }

        /* Skip over colon */
        if (*line) {
            line++;
        }

        /* Find the first non-space letter */
        str_start = line = _skip_left_space(line);
        do {
            if (*line == 0 || (*line == _CC_CR_ && *(line + 1) == _CC_LF_)) {
                str_end = line;
                /* skip all trailing space letters */
                while ((str_end > str_start) && _cc_isspace(*str_start)) {
                    str_end--;
                }

                value = _cc_tcsndup(str_start, str_end - str_start + 1);
                break;
            }
        } while (++line);

        if (value == NULL) {
            _cc_free(name);
            break;
        }

        if (!_cc_insert_kv(&res->request.headers, name, value)) {
            break;
        }

        // printf("%s=%s;\n", name, value);
        if (*line) {
            if (*line == _CC_CR_ && *(line + 1) == _CC_LF_) {
                line = line + 2;
                if (*line && (*line == _CC_CR_ && *(line + 1) == _CC_LF_))
                    break;
            }
        } else {
            break;
        }
    }

    if (method->method == _CC_HTTP_METHOD_GET_) {
        str_range = _cc_find_kv(&res->request.headers, _T("Range"));
        if (str_range) {
            _http_parsing_header_range(res, str_range);
        }
    } else if (method->method == _CC_HTTP_METHOD_DELETE_) {
    }

    str_content_length = _cc_find_kv(&res->request.headers, _T("Content-Length"));
    if (str_content_length) {
        content_length = _ttoi(str_content_length);
        /* LOG: Request too large */
        if (content_length > _CC_HTTP_MAX_CONTENT_LENGTH_) {
            _http_request_too_large(res, &rw->w);
            return false;
        }
    } else {
        content_length = 0;
    }

    str_connection = _cc_find_kv(&res->request.headers, _T("Connection"));
    if (str_connection) {
        if (_tcsicmp(str_connection, _T("upgrade")) == 0) {
            _cc_http_init_websocket(res);
        } else {
            res->request.keep_alive =
                (_tcsicmp(str_connection, _T("keep-alive")) == 0);
        }
    }

    str_host = _cc_find_kv(&res->request.headers, _T("Host"));
    if (str_host) {
        bool_t insquare = false;
        tchar_t* server_port = res->request.server_host = _cc_tcsdup(str_host);
        while ((*server_port != 0) && (*server_port != ':' || insquare)) {
            if (*server_port == '[') {
                insquare = 1;
            } else if (*server_port == ']') {
                insquare = 0;
            }
            server_port++;
        }

        if (*server_port == ':') {
            *server_port = 0;
            server_port++;
            res->request.server_port = _ttoi(server_port);
        } else {
            res->request.server_port = res->listener->port;
        }
    }

    res->request.length = 0;
    res->request.parsing = true;
    res->request.content_length = content_length;

    if (content_length == 0) {
        res->request.finished = true;
        RBUF_LEN(rw) = 0;
        return true;
    }

    if (res->request.body == NULL && content_length > 0) {
        res->request.body = _cc_create_buf((sizeof(tchar_t) * content_length));
        if (res->request.body == NULL) {
            _http_badrequest(res, &rw->w);
            return false;
        }
    } else if (res->request.body) {
        res->request.body->length = 0;
    }

    /**/
    if (res->request.body && RBUF_LEN(rw) > size) {
        content_length = (RBUF_LEN(rw) - size);
        res->request.length += content_length;
        _cc_buf_write(res->request.body, RBUF(rw) + size, content_length);
    }

    RBUF_LEN(rw) = 0;

    return true;
}

#undef RBUF_LEN
#undef WBUF_LEN
#undef RBUF
#undef WBUF
