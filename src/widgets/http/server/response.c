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
#include "http.h"

bool_t _cc_http_insert_header(_cc_http_t* res,
                              const tchar_t* name,
                              const tchar_t* value) {
    if (_tcsrchr(name, '\r') != NULL || _tcsrchr(name, '\n') != NULL) {
        /* drop illegal headers */
        _cc_logger_error(_T("dropping illegal header key!"));
        return false;
    }
    return _cc_insert_kv(&res->response.headers, _cc_tcsdup(name),
                         _cc_tcsdup(value));
}

bool_t _http_response_init_buffer(_cc_http_t* res, int status) {
    res->response.status = status;
    res->response.descriptor = 2;
    if (res->response.content.buf == NULL) {
        res->response.content.buf = _cc_create_buf(_CC_IO_BUFFER_SIZE_);
        if (res->response.content.buf == NULL) {
            return false;
        }
    }
    return true;
}

bool_t _cc_http_write_header(_cc_http_t* res, _cc_event_wbuf_t* w) {
    tchar_t* header = _T("");
    switch (res->response.status) {
        case 100:
            header = _T("Continue\r\n");
            break;
        case 101:
            header = _T("Switching Protocols\r\n");
            break;
        case 102:
            header = _T("Processing\r\n");
            break;
        case 200:
            header = _T("OK\r\n");
            break;
        case 201:
            header = _T("Created\r\n");
            break;
        case 202:
            header = _T("Accepted\r\n");
            break;
        case 203:
            header = _T("Non-Authoritative Information\r\n");
            break;
        case 204:
            header = _T("No Content\r\n");
            break;
        case 205:
            header = _T("Reset Content\r\n");
            break;
        case 206:
            header = _T("Partial Content\r\n");
            break;
        case 207:
            header = _T("Multi-status\r\n");
            break;
        case 300:
            header = _T("Multiple Choices\r\n");
            break;
        case 301:
            header = _T("Moved Permanently\r\n");
            break;
        case 302:
            header = _T("Found\r\n");
            break;
        case 303:
            header = _T("See Other\r\n");
            break;
        case 304:
            header = _T("Not Modified\r\n");
            break;
        case 305:
            header = _T("Use Proxy\r\n");
            break;
        case 306:
            header = _T("(Unused)\r\n");
            break;
        case 307:
            header = _T("Temporary Redirect\r\n");
            break;
        case 308:
            header = _T("Permanent Redirect\r\n");
            break;
        case 400:
            header = _T("Bad Request\r\n");
            break;
        case 401:
            header = _T("Unauthorized\r\n");
            break;
        case 402:
            header = _T("Payment Required\r\n");
            break;
        case 403:
            header = _T("Forbidden\r\n");
            break;
        case 404:
            header = _T("Not Found\r\n");
            break;
        case 405:
            header = _T("Method Not Allowed\r\n");
            break;
        case 406:
            header = _T("Not Acceptable\r\n");
            break;
        case 407:
            header = _T("Proxy Authentication Required\r\n");
            break;
        case 408:
            header = _T("Request Timeout\r\n");
            break;
        case 409:
            header = _T("Conflict\r\n");
            break;
        case 410:
            header = _T("Gone\r\n");
            break;
        case 411:
            header = _T("Length Required\r\n");
            break;
        case 412:
            header = _T("Precondition Failed\r\n");
            break;
        case 413:
            header = _T("Request Entity Too Large\r\n");
            break;
        case 414:
            header = _T("Request-URI Too Long\r\n");
            break;
        case 415:
            header = _T("Unsupported Media Type\r\n");
            break;
        case 416:
            header = _T("Requested Range Not Satisfiable\r\n");
            break;
        case 417:
            header = _T("Expectation Failed\r\n");
            break;
        case 422:
            header = _T("Unprocessable Entity\r\n");
            break;
        case 423:
            header = _T("Locked\r\n");
            break;
        case 424:
            header = _T("Failed Dependency\r\n");
            break;
        case 426:
            header = _T("Upgrade Required\r\n");
            break;
        case 500:
            header = _T("Internal Server Error\r\n");
            break;
        case 501:
            header = _T("Not Implemented\r\n");
            break;
        case 502:
            header = _T("Bad Gateway\r\n");
            break;
        case 503:
            header = _T("Service Not Available\r\n");
            break;
        case 504:
            header = _T("Gateway Timeout\r\n");
            break;
        case 505:
            header = _T("HTTP Version Not Supported\r\n");
            break;
        case 507:
            header = _T("Insufficient Storage\r\n");
            break;
        default:
            header = _T("Unknown\r\n");
            break;
    }

    if (res->response.length > 0) {
        tchar_t buf[32];
        _sntprintf(buf, _cc_countof(buf), _T("%u"),
                   (uint32_t)res->response.length);
        _cc_http_insert_header(res, _T("Content-Length"), buf);
    }

    _cc_spin_lock(&w->lock);
#ifdef _CC_UNICODE_
    {
        uint32_t length;
        wchar_t utf16_buf[10 * 1024];
        bzero(utf16_buf, sizeof(utf16_buf));
        length = _sntprintf(
            (tchar_t*)(utf16_buf), _cc_countof(utf16_buf), _T("%s %d %s"),
            res->request.protocol == NULL ? _T("HTTP/1.1")
                                          : res->request.protocol,
            res->response.status, header);

        _cc_rbtree_for_each(node, &(res->response.headers), {
            _cc_kv_t* item = _cc_upcast(node, _cc_kv_t, node);
            length += _sntprintf((tchar_t*)(utf16_buf + length),
                                 _cc_countof(utf16_buf) - length,
                                 _T("%s: %s\r\n"), item->name, item->value);
        });
        _tcsncpy((tchar_t*)(utf16_buf + length), _T("\r\n"), 2);
        length += 2;

        w->length = _cc_utf16_to_utf8(
            (const uint16_t*)utf16_buf, (const uint16_t*)(utf16_buf + length),
            (uint8_t*)w->buf, (uint8_t*)(w->buf + _CC_IO_BUFFER_SIZE_), false);
    }
#else
    w->length = _sntprintf(
        (tchar_t*)(w->buf + w->length), _cc_countof(w->buf) - w->length,
        _T("%s %d %s"),
        res->request.protocol == NULL ? _T("HTTP/1.1") : res->request.protocol,
        res->response.status, header);

    _cc_rbtree_for_each(node, &(res->response.headers), {
        _cc_kv_t* item = _cc_upcast(node, _cc_kv_t, node);
        w->length += _sntprintf((tchar_t*)(w->buf + w->length),
                                _cc_countof(w->buf) - w->length,
                                _T("%s: %s\r\n"), item->name, item->value);
    });

    _tcsncpy((tchar_t*)(w->buf + w->length), _T("\r\n"), 2);
    w->length += 2;
#endif
    _cc_spin_unlock(&w->lock);

    return true;
}

void _http_badrequest(_cc_http_t* res, _cc_event_wbuf_t* w) {
    _http_response_init_buffer(res, 400);
    if (res->response.content.buf) {
        _cc_buf_putts(
            res->response.content.buf,
            _T("<html>\n<head>\n<meta http-equiv=\"Content-Type\" ")
            _T("content=\"text/html; charset=UTF-8\"/>\n<title>400 Bad ")
            _T("Request</title>\n</head>\n")
            _T("<body>\n<h1>400 Bad Request</h1>\n")
            _T("This server does not understand the requested protocol\n")
            _T("</body>\n</html>\n"));
#ifdef _CC_UNICODE_
        _cc_buf_utf16_to_utf8(res->response.content.buf, 0);
#endif
        res->response.length = res->response.content.buf->length;
    }

    _cc_http_write_header(res, w);
}

void _http_notauthorized(_cc_http_t* res, _cc_event_wbuf_t* w) {
    _http_response_init_buffer(res, 401);
    if (res->response.content.buf) {
        _cc_buf_puttsf(
            res->response.content.buf,
            _T("<html>\n<head>\n<meta http-equiv=\"Content-Type\" ")
            _T("content=\"text/html; charset=UTF-8\"/>\n<title>Not ")
            _T("Authorized</title>\n</head>\n")
            _T("<body>\n<h1>401 Not Authorized</h1>\n")
            _T("A login and password are required for this document %s\n")
            _T("</body>\n</html>\n"),
            res->request.script);
#ifdef _CC_UNICODE_
        _cc_buf_utf16_to_utf8(res->response.content.buf, 0);
#endif
        res->response.length = res->response.content.buf->length;
    }

    _cc_http_write_header(res, w);
}

void _http_forbidden(_cc_http_t* res, _cc_event_wbuf_t* w) {
    _http_response_init_buffer(res, 403);
    if (res->response.content.buf) {
        _cc_buf_puttsf(
            res->response.content.buf,
            _T("<html>\n<head>\n<meta http-equiv=\"Content-Type\" ")
            _T("content=\"text/html; charset=UTF-8\"/>\n<title>Access ")
            _T("denied</title>\n</head>\n")
            _T("<body>\n<h1>403 Access denied</h1>\n")
            _T("The document %s is access denied\n")
            _T("</body>\n</html>\n"),
            res->request.script);
#ifdef _CC_UNICODE_
        _cc_buf_utf16_to_utf8(res->response.content.buf, 0);
#endif
        res->response.length = res->response.content.buf->length;
    }

    _cc_http_write_header(res, w);
}

void _http_nofound(_cc_http_t* res, _cc_event_wbuf_t* w) {
    _http_response_init_buffer(res, 404);
    if (res->response.content.buf) {
        _cc_buf_puttsf(res->response.content.buf,
                       _T("<html>\n<head>\n<meta http-equiv=\"Content-Type\" ")
                       _T("content=\"text/html; charset=UTF-8\"/>\n<title>Not ")
                       _T("Found</title>\n</head>\n")
                       _T("<body>\n<h1>Document Not Found</h1>\n")
                       _T("The document %s is not available on this server\n")
                       _T("</body>\n</html>\n"),
                       res->request.script);
#ifdef _CC_UNICODE_
        _cc_buf_utf16_to_utf8(res->response.content.buf, 0);
#endif
        res->response.length = res->response.content.buf->length;
    }

    _cc_http_write_header(res, w);
}

void _http_request_too_large(_cc_http_t* res, _cc_event_wbuf_t* w) {
    _http_response_init_buffer(res, 413);
    if (res->response.content.buf) {
        _cc_buf_putts(res->response.content.buf,
                      _T("<html>\n<head>\n<meta http-equiv=\"Content-Type\" ")
                      _T("content=\"text/html; charset=UTF-8\"/>\n<title>413 ")
                      _T("Request Entity Too Large</title>\n</head>\n")
                      _T("<body>\n<h1>413 Request Entity Too Large</h1>\n")
                      _T("Too much POST data\n")
                      _T("</body>\n</html>\n"));
#ifdef _CC_UNICODE_
        _cc_buf_utf16_to_utf8(res->response.content.buf, 0);
#endif
        res->response.length = res->response.content.buf->length;
    }

    _cc_http_write_header(res, w);
}

void _http_unimplemented(_cc_http_t* res, _cc_event_wbuf_t* w) {
    _http_response_init_buffer(res, 501);
    _cc_http_insert_header(res, _T("Allow"), _T("GET,POST,HEAD,OPTIONS"));
    if (res->response.content.buf) {
        _cc_buf_puttsf(res->response.content.buf,
                       _T("<html>\n<head>\n<meta http-equiv=\"Content-Type\" ")
                       _T("content=\"text/html; charset=UTF-8\"/>\n<title>Not ")
                       _T("Implemented</title>\n</head>\n")
                       _T("<body>\n<h1>501 Not Implemented</h1>\n")
                       _T("The %s method is not implemented on this server\n")
                       _T("</body>\n</html>\n"),
                       res->request.method);
#ifdef _CC_UNICODE_
        _cc_buf_utf16_to_utf8(res->response.content.buf, 0);
#endif
        res->response.length = res->response.content.buf->length;
    }

    _cc_http_write_header(res, w);
}

void _http_notmodified(_cc_http_t* res, _cc_event_wbuf_t* w) {
    res->response.status = 304;
    res->response.descriptor = 2;
    if (res->response.content.buf != NULL) {
        _cc_destroy_buf(&res->response.content.buf);
    }
    _cc_http_write_header(res, w);
}
/*
void _http_rpc_service(_cc_http_t *res, _cc_event_wbuf_t *w) {
    res->response.status = 200;
    res->response.descriptor = 2;
    if (res->response.content.buf == NULL) {
        res->response.content.buf = _cc_create_buf(_CC_IO_BUFFER_SIZE_);
    }

    if (res->response.content.buf) {
        _cc_buf_putts(res->response.content.buf,
                    _T("<html>\n<head>\n<meta http-equiv=\"Content-Type\"
content=\"text/html; charset=UTF-8\"/>\n<title>RPC-Service</title>\n</head>\n")
                    _T("<body>\n<h1>RPC-Service</h1>\n")
                    _T("</body>\n</html>\n"));
#ifdef _CC_UNICODE_
        _cc_buf_utf16_to_utf8(res->response.content.buf,0);
#endif
        res->response.length = res->response.content.buf->length;
    }

    _cc_http_write_header(res, w);
}*/
/*
** Compare two ETag values. Return 0 if they match and non-zero if they differ.
**
** The one on the left might be a NULL pointer and it might be quoted.
*/
static int _compare_etags(const tchar_t* zA, const tchar_t* zB) {
    if (zA == 0)
        return 1;
    if (zA[0] == '"') {
        int lenB = (int)_tcslen(zB);
        if (_tcsncmp(zA + 1, zB, lenB) == 0 && zA[lenB + 1] == '"') {
            return 0;
        }
    }
    return _tcscmp(zA, zB);
}

bool_t _cc_http_response_header(_cc_http_t* res) {
    if (res->websocket.status == 1) {
        _cc_http_insert_header(res, _T("Upgrade"), _T("websocket"));
        _cc_http_insert_header(res, _T("Connection"), _T("Upgrade"));
        _cc_http_insert_header(res, _T("Sec-WebSocket-Protool"), _T("echo"));
        _cc_http_insert_header(res, _T("Sec-WebSocket-Accept"),
                               res->websocket.key);
    } else {
        /* Render seconds since 1970 as an RFC822 date string*/
        _cc_http_insert_header(res, _T("Date"),
                               httpd_get_rfc822_date(time(NULL)));
        _cc_http_insert_header(res, _T("Content-Type"), _T("text/html"));
        if (res->request.keep_alive) {
            _cc_http_insert_header(res, _T("Connection"), _T("keep-alive"));
        } else {
            _cc_http_insert_header(res, _T("Connection"), _T("close"));
        }
        _cc_http_insert_header(
            res, _T("Server"),
            _T("http/1.1.0 (") _T(_CC_PLATFORM_NAME_) _T(")"));
    }
    return true;
}

bool_t _cc_http_response(_cc_http_t* res, _cc_event_wbuf_t* w) {
    if (res->websocket.status == 1) {
        w->length = 0;
        res->response.length = 0;
        res->response.status = 101;
        res->response.descriptor = 0;

        _cc_http_write_header(res, w);

        return true;
    } else if (res->websocket.status == 2) {
        return true;
    }

    if (res->response.descriptor == 0) {
        const tchar_t* str_if_none_match;
        const tchar_t* str_if_modified_since;
        const tchar_t* str_content_type = NULL;
        tchar_t hv[128];
        /* Information about the file to be retrieved */
        struct _stat statbuf;
        const tchar_t* p = NULL;
        const tchar_t* document_root = res->listener->document_root;
        int32_t len = 0;
        int32_t i = 0;
        tchar_t rpath[_CC_MAX_PATH_];
        res->response.content.buf = NULL;
        w->length = 0;

        p = res->request.script + 1;

        /*
        if (*p && _tcsicmp(p, _T("rpc-service")) == 0) {
            _http_rpc_service(res, w);
            return true;
        }*/

        len = (int32_t)_tcslen(res->request.script);
        if (res->request.script[len - 1] == _T('/')) {
            document_root = httpd_directory_index_access(res);
            if (document_root == NULL) {
                _http_nofound(res, w);
                return true;
            }
        }

        _sntprintf(rpath, _cc_countof(rpath), _T("%s%s"), document_root,
                         res->request.script);
        len = _cc_realpath(rpath);
        for (i = (len - 1); i > 0; i--) {
            if (rpath[i] == _T('.') || rpath[i] == _T('/')) {
                p = &rpath[i];
                break;
            }
        }

        if (p && *p == _T('.')) {
            str_content_type = httpd_get_MIME(p);
            _cc_http_insert_header(res, _T("Content-Type"), str_content_type);
        }

        if (_tstat(rpath, &statbuf) == 0) {
            res->response.descriptor = 1;
            _sntprintf(hv, _cc_countof(hv), _T("m%xs%x"), (int)statbuf.st_mtime,
                       (int)statbuf.st_size);

            _cc_http_insert_header(res, _T("Last-Modified"),
                                   httpd_get_rfc822_date(statbuf.st_mtime));
            _cc_http_insert_header(res, _T("ETag"), hv);
            /* Cache-control max-age */
            _cc_http_insert_header(res, _T("Cache-Control"), _T("max-age=120"));

            str_if_none_match =
                _cc_find_kv(&res->request.headers, _T("If-None-Match"));
            str_if_modified_since =
                _cc_find_kv(&res->request.headers, _T("If-Modified-Since"));

            if (_compare_etags(str_if_none_match, hv) == 0 ||
                (httpd_get_rfc822_time(str_if_modified_since) >=
                 statbuf.st_mtime)) {
                res->response.status = 304;
                res->response.length = 0;
                _cc_http_write_header(res, w);
                return true;
            }

            if (res->request.method_id == _CC_HTTP_METHOD_HEAD_ ||
                res->request.method_id == _CC_HTTP_METHOD_DELETE_) {
                res->response.status = 200;
                res->response.length = 0;
                res->response.descriptor = 255;
                _cc_http_write_header(res, w);
                return true;
            }

            if (res->request.method_id == _CC_HTTP_METHOD_OPTIONS_) {
                res->response.status = 200;
                res->response.length = 0;
                res->response.descriptor = 255;

                _cc_http_insert_header(res, _T("Allow"),
                                       _T("GET,POST,HEAD,OPTIONS"));

                _cc_http_write_header(res, w);
                return true;
            }

            if (_http_gzip_response(str_content_type, rpath, statbuf.st_size,
                                    res, w)) {
                return true;
            }

            res->response.content.rfp = _cc_fopen(rpath, _T("rb"));
            if (res->response.content.rfp) {
                res->response.status = 200;
                res->response.length = statbuf.st_size;

                if (res->request.range_start < res->response.length &&
                    res->request.range_end > 0 &&
                    res->request.range_end > res->request.range_start) {
                    if (res->request.range_end > res->response.length) {
                        res->request.range_end = res->response.length - 1;
                    }
                    _cc_fseek(res->response.content.rfp,
                              res->request.range_start, _CC_FILE_SEEK_SET_);

                    _sntprintf(hv, _cc_countof(hv), _T("bytes %zu-%zu/%zu"),
                               res->request.range_start, res->request.range_end,
                               res->response.length);
                    _cc_http_insert_header(res, _T("Content-Range"), hv);
                    res->response.length =
                        res->request.range_end + 1 - res->request.range_start;
                }

                _cc_http_write_header(res, w);
            } else {
                /* LOG: fopen() failed for static content */
                _http_nofound(res, w);
            }
        } else {
            _http_nofound(res, w);
        }
    } else if (res->response.length > 0) {
        size_t r = _CC_IO_BUFFER_SIZE_ - w->length;
        if (r <= 0) {
            return true;
        }

        if (res->response.descriptor == 1) {
            r = (size_t)_cc_file_read(res->response.content.rfp,
                                      &w->buf[w->length], sizeof(byte_t), r);
            if (r > 0) {
                res->response.length -= r;
                w->length += r;
            }
        } else if (res->response.descriptor == 2 && res->response.content.buf) {
            _cc_buf_t* b = res->response.content.buf;
            size_t offset = (size_t)(b->length - res->response.length);
            r = _min(r, res->response.length);
            memcpy(&w->buf[w->length], b->bytes + offset, r);
            res->response.length -= r;
            w->length += r;
        }
    }

    return true;
}