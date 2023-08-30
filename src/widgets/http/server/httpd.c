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

#include "SSL.h"
#include "http.h"

_cc_global_http_t global_http = {0};

/**/
const tchar_t* httpd_get_MIME(const tchar_t* name) {
    const tchar_t* value =
        (const tchar_t*)_cc_json_object_find_string(global_http.mime, name);
    if (value == NULL) {
        value = (const tchar_t*)_cc_json_object_find_string(global_http.mime,
                                                            _T("*"));
        if (value == NULL) {
            return _CC_HTTP_DEFAULT_MIME;
        }
    }
    return value;
}

/**/
const tchar_t* httpd_directory_index_access(_cc_http_t* res) {
    tchar_t results[_CC_MAX_PATH_];
    int32_t rlen;
    _cc_http_listener_t* listener = res->listener;
    const tchar_t* document_root = listener->document_root;
    const _cc_rbtree_t* directory_index = listener->directory_index;
/*LOG: is enable virtual-host*/
#ifdef _CC_HTTPD_IS_ENABLE_VHOST_
    if (listener->vhost) {
        _cc_json_t* vhost =
            _cc_json_object_find(listener->vhost, res->request.server_host);
        if (vhost) {
            document_root =
                _cc_json_object_find_string(vhost, _T("document-root"));
            if (document_root == NULL) {
                document_root = listener->document_root;
            }
            directory_index =
                _cc_json_object_find_array(vhost, _T("directory-index"));
            if (directory_index == NULL) {
                directory_index = listener->directory_index;
            }
        }
    }
#endif
    if (directory_index) {
        _cc_rbtree_for_each(v, directory_index, {
            _cc_json_t* j = _cc_upcast(v, _cc_json_t, node);
            const tchar_t* index = (tchar_t*)_cc_json_string(j);
            if (index) {
                _sntprintf(results, _cc_countof(results), _T("%s%s%s"),
                           document_root, res->request.script, index);
                if (_taccess(results, _CC_ACCESS_F_) != -1) {
                    rlen = _sntprintf(results, _cc_countof(results), _T("%s%s"),
                                      res->request.script, index);
                    _cc_free(res->request.script);
                    res->request.script = _cc_tcsndup(results, rlen);
                    return document_root;
                }
            }
        });
    }

    return NULL;
}

bool_t _init_httpd(int32_t services) {
    /**/
    const _cc_rbtree_t* r;
    _cc_http_listener_t* listener;
    tchar_t path[_CC_MAX_PATH_];

    global_http.conf = NULL;
    global_http.mime = NULL;
    _cc_queue_iterator_cleanup(&global_http.listeners);

    if (_cc_get_module_directory(_T("conf/mime.json"), path, _CC_MAX_PATH_)) {
        global_http.mime = _cc_open_json_file(path);
    }

    //_sntprintf(path, _cc_countof(path), _T("%s/conf/http.json"), path);
    if (_cc_get_module_directory(_T("conf/http.json"), path, _CC_MAX_PATH_)) {
        global_http.conf = _cc_open_json_file(path);
    }

    if (global_http.conf == NULL) {
        _cc_logger_error(_T("http JSON parsing failure: %s\n"),
                         _cc_json_error());
        return false;
    }

    r = _cc_json_array(global_http.conf);
    if (r == NULL) {
        _cc_logger_error(_T("http conf failure"));
        return false;
    }

    _cc_rbtree_for_each(v, r, {
        const tchar_t* str_family;
        _cc_json_t* json_gzip;
        byte_t proto = 0;
        _cc_json_t* root = _cc_upcast(v, _cc_json_t, node);

        listener =
            (_cc_http_listener_t*)_cc_malloc(sizeof(_cc_http_listener_t));
        if (listener == NULL) {
            break;
        }
        bzero(listener, sizeof(_cc_http_listener_t));
        listener->host = _cc_json_object_find_string(root, _T("server-host"));
        listener->port =
            (uint16_t)_cc_json_object_find_number(root, _T("server-port"));
        listener->document_root =
            _cc_json_object_find_string(root, _T("document-root"));
        listener->directory_index =
            _cc_json_object_find_array(root, _T("directory-index"));
        listener->vhost = _cc_json_object_find(root, _T("virtual-host"));

        listener->https = _cc_json_object_find_boolean(root, _T("server-ssl"));
        if (listener->vhost && listener->https) {
            _cc_rbtree_for_each(node, &listener->vhost->object.uni_object, {
                _cc_json_t* m = _cc_upcast(node, _cc_json_t, node);
                _cc_json_t* jsonSSL = _cc_json_object_find(m, _T("SSL"));
                if (jsonSSL) {
                    const tchar_t* cert_key_file = _cc_json_object_find_string(
                        jsonSSL, _T("cert-key-file"));
                    const tchar_t* cert_chain_file =
                        _cc_json_object_find_string(jsonSSL,
                                                    _T("cert-chain-file"));

                    _SSL_init_server(m->unique_index.name, cert_key_file,
                                     cert_chain_file);
                }
            });
        }

        json_gzip = _cc_json_object_find(root, _T("gzip"));
        if (json_gzip) {
            listener->gzip.types =
                _cc_json_object_find_array(json_gzip, _T("types"));
            listener->gzip.min_length = (uint16_t)_cc_json_object_find_number(
                json_gzip, _T("min-length"));
            listener->gzip.level =
                (uint16_t)_cc_json_object_find_number(json_gzip, _T("level"));
        }

        str_family = _cc_json_object_find_string(root, _T("server-family"));
        if (_tcsicmp(str_family, _T("ipv4")) == 0) {
            proto = _CC_NET_PROTO_TCP_;
        } else if (_tcsicmp(str_family, _T("ipv6")) == 0) {
            proto = _CC_NET_PROTO_TCP_V6_;
        }

        _cc_queue_iterator_push(&global_http.listeners, &listener->lnk);

        _cc_http_starting(listener, proto);
    });

    return true;
}

bool_t _cc_httpd_update_conf(const tchar_t* family,
                             const tchar_t* document,
                             const tchar_t* host,
                             uint16_t port) {
    tchar_t path[_CC_MAX_PATH_];
    _cc_json_t* root = NULL;
    _cc_json_t* first;
    _cc_json_t* v;
    _cc_buf_t* buf;
    _cc_file_t* fp;

    if (_cc_get_module_directory(_T("conf/http.json"), path, _CC_MAX_PATH_)) {
        root = _cc_open_json_file(path);
    }

    if (root == NULL) {
        _cc_logger_error(_T("http JSON parsing failure: %s\n"),
                         _cc_json_error());
        return false;
    }
    first = _cc_json_array_find(root, 0);
    if (first == NULL) {
        _cc_logger_error(_T("http conf failure."));
        return false;
    }

    v = _cc_json_object_find(first, _T("server-family"));
    if (v) {
        _cc_free(v->object.uni_string);
        v->object.uni_string = _tcsdup(family);
    }

    v = _cc_json_object_find(first, _T("document-root"));
    if (v) {
        _cc_free(v->object.uni_string);
        v->object.uni_string = _tcsdup(document);
    }

    v = _cc_json_object_find(first, _T("server-host"));
    if (v) {
        _cc_free(v->object.uni_string);
        v->object.uni_string = _tcsdup(host);
    }

    v = _cc_json_object_find(first, _T("server-port"));
    if (v) {
        v->object.uni_int = port;
    }

    fp = _cc_fopen(path, _T("w"));
    if (fp) {
        buf = _cc_print_json(root);
        _cc_fwrite(fp, buf->bytes, 1, buf->length);
        _cc_fclose(fp);
        _cc_destroy_buf(&buf);
        return true;
    }

    return false;
}

bool_t _quit_httpd(void) {
    _cc_queue_iterator_for_each(v, &global_http.listeners, {
        _cc_http_listener_t* listener = _cc_upcast(v, _cc_http_listener_t, lnk);
        _cc_free(listener);
    });

    _cc_destroy_json(&global_http.conf);
    _cc_destroy_json(&global_http.mime);
    return true;
}
