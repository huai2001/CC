#include <libcc.h>
#include <libcc/event.h>
#include <libcc/http_request.h>

#define ENABLE_SSL 1

#if ENABLE_SSL
_cc_OpenSSL_t *openSSL = NULL;
#endif

typedef struct _http {
    uint8_t state;

    bool_t keep_alive;
    size_t payload;
    _cc_io_buffer_t *io;
    _cc_http_request_header_t *request;
    _cc_buf_t buffer;
    _cc_file_t *file;
} _http_t;

static const tchar_t *root = _T("/var/www/html");
static bool_t onAccept(_cc_async_event_t *async, _cc_event_t *e);
static bool_t onClose(_cc_async_event_t *async, _cc_event_t *e);
static bool_t onRead(_cc_async_event_t *async, _cc_event_t *e);
static bool_t onWrite(_cc_async_event_t *async, _cc_event_t *e);
static bool_t onTimeout(_cc_async_event_t *async, _cc_event_t *e);

static bool_t doEvent(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    if (which & _CC_EVENT_ACCEPT_) {
        onAccept(async,e);
        return true;
    } else if (which & _CC_EVENT_CLOSED_) {
        return onClose(async, e);
    }
    if (which & _CC_EVENT_READABLE_) {
        if (!onRead(async, e)) {
            return false;
        }
    }
    if (which & _CC_EVENT_WRITABLE_) {
        if (!onWrite(async, e)) {
            return false;
        }
    }
    if (which & _CC_EVENT_TIMEOUT_) {
        if (!onTimeout(async, e)) {
            return false;
        }
    }
    return true;
}

static bool_t onAccept(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_socket_t fd;
    _cc_event_t *event;
	_http_t *http;
    struct sockaddr_in remote_addr = {0};
    _cc_socklen_t remote_addr_len = sizeof(struct sockaddr_in);
    _cc_async_event_t *async2 = _cc_get_async_event();

    fd = async->accept(async, e, (_cc_sockaddr_t *)&remote_addr, &remote_addr_len);
    if (fd == _CC_INVALID_SOCKET_) {
        _cc_logger_debug(_T("thread %d accept fail %s."), _cc_get_thread_id(NULL),
                         _cc_last_error(_cc_last_errno()));
        return false;
    }

    event = _cc_alloc_event(async2, _CC_EVENT_TIMEOUT_ | _CC_EVENT_READABLE_);
    if (event == NULL) {
        _cc_close_socket(fd);
        return false;
    }

    _cc_set_socket_nonblock(fd, 1);

    http = (_http_t*)_cc_malloc(sizeof(_http_t));
    http->state = _CC_HTTP_STATE_HEADER_;
    http->request = NULL;
    http->payload = 0;
    http->io = _cc_alloc_io_buffer(_CC_IO_BUFFER_SIZE_);
#if ENABLE_SSL
    http->io->ssl = _SSL_accept(openSSL, fd);
#endif
    _cc_alloc_buf(&http->buffer, _CC_IO_BUFFER_SIZE_);
    event->fd = fd;
    event->callback = e->callback;
    event->timeout = e->timeout;
    event->data = (uintptr_t)http;

    #if ENABLE_SSL
        event->timeout = 100; //wait SSL handshake complete
    #else
        event->timeout = e->timeout;
        _CC_SET_BIT(_CC_EVENT_READABLE_, event->flags);
    #endif

    if (async2->attach(async2, event) == false) {
        _cc_logger_debug(_T("thread %d add socket (%d) event fial."), _cc_get_thread_id(NULL), fd);
        _cc_free_event(async2, event);
        return false;
    }
    _cc_logger_debug(_T("%d accept."), event->ident);
    return true;
}

static bool_t onClose(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_logger_debug(_T("%d onClose."), e->ident);
    if (e->data) {
        _http_t *http = (_http_t*)e->data;
        if (http->io) {
            _cc_free_io_buffer(http->io);
        }
        if (http->request) {
            _cc_http_free_request_header(&http->request);
        }
        if (http->buffer.bytes) {
            _cc_free_buf(&http->buffer);
        }
        if (http->file) {
            _cc_fclose(http->file);
        }
        e->data = 0;
        _cc_free(http);
        return true;
    }
    return false;
}

_CC_API_PRIVATE(void) response_bad_request(_cc_event_t *e, _cc_io_buffer_t *io) {
    _cc_string_t body = _cc_string("<HTML><HEAD><TITLE>BAD REQUEST</TITLE></HEAD><BODY><P>Your browser sent a bad request, such as a POST without a Content-Length.</P></BODY></HTML>");

    io->w.off += snprintf((char*)io->w.bytes + io->w.off, io->w.limit - io->w.off,"HTTP/1.1 400 BAD REQUEST\r\nConnection: close;\r\nContent-type: text/html\r\nContent-Length: %d\r\n\r\n", (int32_t)body.length);
    memcpy(io->w.bytes + io->w.off, body.ptr, body.length * sizeof(char_t));
    io->w.off += (int32_t)body.length * sizeof(char_t);
    _cc_io_buffer_flush(e, io);
}

_CC_API_PRIVATE(void) response_not_found(_cc_event_t *e, _cc_io_buffer_t *io) {
    _cc_string_t body = _cc_string("<HTML><HEAD><TITLE>Not Found</TITLE></HEAD><BODY><p>The server could not find the requested URL.</p></BODY></HTML>");

    io->w.off += snprintf((char*)io->w.bytes + io->w.off, io->w.limit - io->w.off,"HTTP/1.1 404 NOT FOUND\r\nConnection: close;\r\nContent-type: text/html\r\nContent-Length: %d\r\n\r\n", (int32_t)body.length);
    memcpy(io->w.bytes + io->w.off, body.ptr, body.length * sizeof(char_t));
    io->w.off += (int32_t)body.length * sizeof(char_t);
    _cc_io_buffer_flush(e, io);
}
#if 0
_CC_API_PRIVATE(void) response_unimplemented(_cc_event_t *e, _cc_io_buffer_t *io) {
    _cc_string_t body = _cc_string("<HTML><HEAD><TITLE>Method Not Implemented</TITLE></HEAD><BODY><p>HTTP request method not supported.</p></BODY></HTML>");
    
    io->w.off += snprintf((char*)io->w.bytes + io->w.off, io->w.limit - io->w.off,"HTTP/1.1 501 Method Not Implemented\r\nConnection: close;\r\nContent-type: text/html\r\nContent-Length: %ld\r\n\r\n", (int32_t)body.length);
    memcpy(io->w.bytes + io->w.off, body.ptr, body.length * sizeof(char_t));
    io->w.off += (int32_t)body.length * sizeof(char_t);
    _cc_io_buffer_flush(e, io);
}
#endif
_CC_API_PRIVATE(void) response_ok(_cc_event_t *e, _cc_io_buffer_t *io) {
    _cc_string_t body = _cc_string("<HTML><HEAD><TITLE>Welcome to HTTP</TITLE></HEAD><BODY><p>If you see this page, the web server is successfully</p></BODY></HTML>");
    
    io->w.off += snprintf((char*)io->w.bytes + io->w.off, io->w.limit - io->w.off,"HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-type: text/html\r\nContent-Length: %d\r\n\r\n", (int32_t)body.length);
    memcpy(io->w.bytes + io->w.off, body.ptr, body.length * sizeof(char_t));
    io->w.off += (int32_t)body.length * sizeof(char_t);
    _cc_io_buffer_flush(e, io);
}

_CC_API_PRIVATE(void) response_options(_cc_event_t *e, _cc_io_buffer_t *io) {
    _cc_string_t http_response_header = _cc_string(
        "HTTP/1.1 200 OK\r\n"\
        "Access-Control-Allow-Origin: *\r\n"\
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"\
        "Access-Control-Allow-Headers: *\r\n"\
        "Access-Control-Allow-Credentials: true\r\n"\
        "Connection: Keep-Alive;\r\n"\
        "Content-type: application/json\r\n"\
        "Content-Length: 0\r\n\r\n");

    if ((io->w.off + http_response_header.length) > io->w.limit) {
        _cc_realloc_write_buffer(io, io->w.off + (int32_t)http_response_header.length);
    }
    memcpy(io->w.bytes + io->w.off, http_response_header.ptr, http_response_header.length * sizeof(char_t));
    io->w.off += (int32_t)http_response_header.length * sizeof(char_t);

    _cc_io_buffer_flush(e, io);
}

_CC_API_PRIVATE(void) response_file(_cc_event_t *e, _cc_io_buffer_t *io, tchar_t *www_file) {
    _http_t *http = (_http_t*)e->data;
    http->file = _cc_fopen(www_file, "rb");
    if (http->file) {
        uint64_t size = _cc_file_size(http->file);
        tchar_t *ext = NULL;
        tchar_t *script_name = _tcsrchr(www_file, '/');
        if (script_name == NULL) {
            script_name = "index.html";
        } else {
            ext = _tcsrchr(script_name, '.');
            script_name++;
        }
        if (ext != NULL) {
            if (_tcsicmp(".exe", ext) == 0) {
                io->w.off += snprintf((char*)io->w.bytes + io->w.off, io->w.limit - io->w.off,"HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-type: Content-Disposition: attachment; filename=\"%s\"\r\nContent-Length: %lld\r\n\r\n", script_name,size);
            } else if (_tcsicmp(".zip", ext) == 0) {
                io->w.off += snprintf((char*)io->w.bytes + io->w.off, io->w.limit - io->w.off,"HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-type: Content-Disposition: attachment; filename=\"%s\"\r\nContent-Length: %lld\r\n\r\n", script_name,size);
            } else {
                io->w.off += snprintf((char*)io->w.bytes + io->w.off, io->w.limit - io->w.off,"HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-type: text/html; charset=utf-8\r\nContent-Length: %lld\r\n\r\n",size);
            }
        } else {
            io->w.off += snprintf((char*)io->w.bytes + io->w.off, io->w.limit - io->w.off,"HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-type: text/html; charset=utf-8\r\nContent-Length: %lld\r\n\r\n",size);
        }
        
        _CC_SET_BIT(_CC_EVENT_WRITABLE_, e->flags);
    } else {
        response_not_found(e, io);
    }
}

_CC_API_PRIVATE(int64_t) _get_content_length(_cc_rbtree_t *headers) {
    const _cc_http_header_t *data = _cc_http_header_find(headers, _T("Content-Length"));
    return (data ? _ttoi(data->value) : 0);
}

_CC_API_PRIVATE(bool_t) _is_keep_alive(_cc_rbtree_t *headers) {
    const _cc_http_header_t *data = _cc_http_header_find(headers, _T("Connection"));
    return (data && _tcsicmp(data->value, _T("keep-alive")) == 0);
}

static bool_t onRead(_cc_async_event_t *async, _cc_event_t *e) {
    _http_t *http = (_http_t*)e->data;
    _cc_io_buffer_t *io = http->io;
    int32_t off = _cc_io_buffer_read(e, io);
    if (off < 0) {
        return false;
    } else if (off == 0) {
        return true;
    }

    if (http->state == _CC_HTTP_STATE_ESTABLISHED_) {
        return false;
    } else  if (http->state == _CC_HTTP_STATE_HEADER_) {
        http->state = _cc_http_header_parser((_cc_http_header_fn_t)_cc_http_alloc_request_header, (pvoid_t *)&http->request, io->r.bytes, &io->r.off);
        /**/
        if (http->state == _CC_HTTP_STATE_HEADER_) {
            return true;
        } else if (http->state != _CC_HTTP_STATE_PAYLOAD_) {
            response_bad_request(e, io);
            return false;
        }
        http->keep_alive = _is_keep_alive(&http->request->headers);
        http->payload = _get_content_length(&http->request->headers);
        if (http->payload == 0) {
            http->state = _CC_HTTP_STATE_ESTABLISHED_;
        }

        if (http->buffer.bytes == NULL && http->payload > 0) {
            _cc_alloc_buf(&http->buffer, (size_t)http->payload);
        }
    } 

    if (http->state == _CC_HTTP_STATE_PAYLOAD_ && io->r.off > 0) {
        _cc_buf_append(&http->buffer, io->r.bytes, io->r.off);
        if (http->buffer.length >= http->payload) {
            http->state = _CC_HTTP_STATE_ESTABLISHED_;
        }
        io->r.off = 0;
    }

    if (http->state == _CC_HTTP_STATE_ESTABLISHED_) {
        if (_tcsicmp(http->request->method, _T("OPTIONS")) == 0) {
            response_options(e, io);
        } else if (http->request->script[0] == '/' && http->request->script[1] == 0) {
            response_ok(e, io);
        } else {
            tchar_t www_file[_CC_MAX_PATH_] = {0};
            _stprintf(www_file, _T("%s%s"), root, http->request->script);
            if (_taccess(www_file, _CC_ACCESS_F_) == 0) {
                //_cc_logger_info("file:%s found.", www_file);
                response_file(e, io, www_file);
            } else {
                response_not_found(e, io);
            }
        }

        _cc_logger_info("http:%s %s %s",http->request->method,http->request->script,http->request->protocol);

        if (_tcsicmp(http->request->method, _T("POST")) == 0) {
            // FILE *fp = fopen("./raw.txt", "wb");
            // if (fp) {
            //     fwrite(http->buffer.bytes, 1, http->buffer.length,fp);
            //     fclose(fp);
            // }
            printf("RAW:%.*s\n",(int)http->buffer.length, http->buffer.bytes);
        }

        http->buffer.length = 0;
        http->state = _CC_HTTP_STATE_HEADER_;
        _cc_http_free_request_header(&http->request);
    }

    return true;
}

static bool_t onWrite(_cc_async_event_t *async, _cc_event_t *e) {
    _http_t *http = (_http_t*)e->data;
    _cc_io_buffer_t *io = http->io;
    _cc_logger_debug(_T("%d onWrite."), e->ident);
    if (io->w.off) {
        if (_cc_io_buffer_flush(e, http->io) < 0) {
            return false;
        }
    } else if (http->file == NULL) {
        _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        return http->keep_alive;
    }

    while (http->file && io->w.limit > io->w.off) {
        int32_t off = (int32_t)_cc_fread(http->file, io->w.bytes + io->w.off, 1, io->w.limit - io->w.off);
        if (off <= 0) {
            _cc_fclose(http->file);
            http->file = NULL;
            if (io->w.off == 0) {
                _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
            }
            break;
        } else {
            io->w.off += off;
            off = _cc_io_buffer_flush(e, http->io);
            if (off == 0) {
                break;
            } else if (off < 0) {
                return false;
            }
        }
    }
    _CC_SET_BIT(_CC_EVENT_WRITABLE_, e->flags);
    return true;
}

static bool_t onTimeout(_cc_async_event_t *async, _cc_event_t *e) {
    _http_t *http = (_http_t*)e->data;
#if ENABLE_SSL
    if (http->io && http->io->ssl) {
        if (!http->io->ssl->is_handshaked) {
            if (_SSL_do_handshake(http->io->ssl) == _CC_SSL_HS_ERROR_) {
                return false;
            }
            if (http->io->ssl->is_handshaked) {
                e->timeout = 60000;
                _CC_SET_BIT(_CC_EVENT_READABLE_, e->flags);
            }
            //wait SSL handshake complete
            return true;
        }
    }
#endif
    _cc_logger_debug(_T("%d onTimeout."), e->ident);
    return false;
}

bool_t addListener(const tchar_t *host, uint16_t port) {
    struct sockaddr_in sa;
    _cc_async_event_t *async = _cc_get_async_event();
    _cc_event_t *event = _cc_alloc_event(async, _CC_EVENT_ACCEPT_);
    _cc_assert(async != NULL);
    _cc_assert(event != NULL);
    if (event == NULL) {
        return false;
    }

    event->timeout = 60000;
    event->callback = doEvent;

    _cc_inet_ipv4_addr(&sa, host, port);
    if (!_cc_tcp_listen(async, event, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
        _cc_free_event(async, event);    
        _cc_assert(FALSE);
        return false;
    }
    return true;
}

int main() {
    int c;
    _cc_alloc_async_event(0, NULL);
#if ENABLE_SSL
    openSSL = _SSL_init(_CC_SSL_DEFAULT_PROTOCOLS_);
    if (openSSL == NULL) {
        return 1;
    }
    _SSL_setup(openSSL, "/var/ssl/ws.libcc.cn_bundle.crt", "/var/ssl/ws.libcc.cn.key",NULL);
#endif
    addListener(NULL, 8080);

    while((c = getchar()) != 'q') {
        _cc_sleep(100);
    }
    _cc_free_async_event();
    return 0;
}