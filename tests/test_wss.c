#include <libcc.h>
#include <libcc/http.h>
#include <libcc/event.h>
#include <stdio.h>

_cc_OpenSSL_t *openSSL;

typedef struct _ws {
    uint8_t state;
    uint8_t handshake;
    _cc_io_buffer_t *io;
    _cc_ws_header_t header;
    _cc_http_request_header_t *request;
    int64_t length;
    int64_t payload;
    char_t hash[256];
} _cc_ws_t;

_CC_API_PRIVATE(_cc_ws_t*) _ws_alloc(_cc_socket_t fd) {
    _cc_ws_t *ws = (_cc_ws_t*)_cc_malloc(sizeof(_cc_ws_t));
    ws->handshake = _CC_SSL_HS_SYSCALL_WOULDBLOCK_;
    ws->state = _CC_HTTP_STATUS_HEADER_;
    ws->payload = 0;
    ws->length = 0;
    ws->request = nullptr;
    ws->header.state = WS_DATA_OK;

    ws->io = _cc_alloc_io_buffer(_CC_IO_BUFFER_SIZE_);
    ws->io->ssl = _SSL_accept(openSSL, fd);
    return ws;
}

/**/
_CC_API_PRIVATE(void) _ws_free(_cc_ws_t *ws) {
    if (ws->request) {
        _cc_http_free_request_header(&ws->request);
    }

    if (ws->io) {
        _cc_free_io_buffer(ws->io);
    }

    _cc_free(ws);
}

/**/
_CC_API_PRIVATE(bool_t) _ws_response_header(_cc_event_t *e, _cc_ws_t *ws) {
    _cc_hash_t c;
    int32_t length;
    char_t results[1024];
    byte_t digest[_CC_SHA1_DIGEST_LENGTH_];

    const _cc_http_header_t *ws_protool = _cc_http_header_find(&ws->request->headers, _T("Sec-WebSocket-Protool"));
    const _cc_http_header_t *ws_key = _cc_http_header_find(&ws->request->headers, _T("Sec-WebSocket-Key"));
    _cc_sds_t protool = ws_protool ? ws_protool->value : _T("JSON");
    _cc_io_buffer_t *io = (_cc_io_buffer_t*)ws->io;

    length = (int32_t)_snprintf(results, _cc_countof(results), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", ws_key->value);
    //_cc_logger_debug("Sec-WebSocket-Key: %s",ws_key->value);

    _cc_sha1_init(&c);
    c.update(&c, (byte_t*)results, length);
    c.final(&c, digest, &length);
    c.free(&c);

    _cc_base64_encode(digest, length, results, _cc_countof(results));

    io->w.off = _snprintf((char*)io->w.bytes, io->w.limit,
                        "HTTP/1.1 101 Switching Protocols\r\n"
                        "Connection: Upgrade\r\n"
                        "Upgrade: websocket\r\n"
                        "Sec-WebSocket-Protool: %s\r\n"
                        "Sec-WebSocket-Accept: %s\r\n\r\n",
                        protool, results);
    //printf("send: %.*s\n",io->w.off, io->w.bytes);
    return _cc_io_buffer_flush(e, io);
}

_CC_API_PRIVATE(void)  _ws_send(_cc_io_buffer_t *io, byte_t *data, int64_t length) {
    _cc_spin_lock(&io->lock_of_writable);
    io->w.off += _cc_ws_header(io->w.bytes + io->w.off, WS_OP_TEXT, length, nullptr);
    memcpy(io->w.bytes + io->w.off, data, length);
    io->w.off += length;
    _cc_unlock(&io->lock_of_writable);
}

_CC_API_PRIVATE(int64_t) _ws_get_content_length(_cc_rbtree_t *headers) {
    const _cc_http_header_t *data = _cc_http_header_find(headers, _T("Content-Length"));
    return data ? _ttoi(data->value) : 0;
}

/**/
_CC_API_PRIVATE(bool_t) _ws_heartbeat(_cc_event_t *e, byte_t oc) {
    _cc_ws_t *ws = (_cc_ws_t*)e->data;
    _cc_io_buffer_t *io = (_cc_io_buffer_t*)ws->io;
    byte_t buf[2];
    buf[0] = 0x80 | oc;
    buf[1] = 0;
    return (_cc_io_buffer_send(e, io, buf, 2) > 0);
}

/**/
_CC_API_PRIVATE(bool_t) _ws_unpack(_cc_event_t *e) {
    _cc_ws_t *ws = (_cc_ws_t*)e->data;
    int32_t off = 0;
    _cc_io_buffer_t *io = (_cc_io_buffer_t*)ws->io;

    while (io->r.off > off) {
        if (ws->header.state == WS_DATA_OK || ws->header.state == WS_HEADER_PARTIAL) {
            off += _cc_ws_header_parser(&ws->header,io->r.bytes + off, io->r.off - off);
            if (ws->header.state == WS_HEADER_PARTIAL) {
                break;
            }
            //You can handle the packet
            switch (ws->header.operation) {
                case WS_OP_PING:
                case WS_OP_PONG:
                    if (ws->header.payload > (int64_t)io->r.limit) {
                        _cc_logger_debug(_T("big data fail. operation 0x%x"), ws->header.operation);
                        return false;
                    }
                    break;
                case WS_OP_CONTINUATION:
                case WS_OP_TEXT:
                case WS_OP_BINARY:
                case WS_OP_JSON:
                case WS_OP_XML:
                    break;
                case WS_OP_DISCONNECT:
                    return false;
                default:
                    /* not handled or failed */
                    _cc_logger_debug(_T("Unhandled ext operation 0x%x"), ws->header.operation);
                    return false;
            }
        }

        if (ws->header.state == WS_DATA_PARTIAL) {
            int64_t length = (io->r.off - off);
            if (ws->header.payload > (int64_t)io->r.limit) {
                int64_t remaining = ws->header.payload - ws->length;
                //_cc_logger_debug(_T("big data. operation 0x%x"), ws->header.operation);
                if (remaining <= length) {
                    //copy data
                    //memcpy(ws->buf + ws->length, io->r.bytes + off, remaining);
                    if (ws->header.mask == 0x80) {
                        _cc_ws_mask(io->r.bytes + off, remaining, ws->header.hash, ws->length);
                    }
                    _tprintf("%.*s\n",(int)remaining, io->r.bytes + off);
                    ws->length = 0;
                    ws->header.state = WS_HEADER_PARTIAL;
                    
                    //discard it directly without any treatment
                    off += remaining;
                    //There is still data. Keep processing
                    if (io->r.off > off) {
                        continue;
                    }
                } else {
                    //copy data
                    //memcpy(ws->buf + ws->length, io->r.bytes + off, length);
                    //discard it directly without any treatment
                    if (ws->header.mask == 0x80) {
                        _cc_ws_mask(io->r.bytes + off, length, ws->header.hash, ws->length);
                    }
                    _tprintf("%.*s",(int)length, io->r.bytes + off);
                    ws->length += length;
                }
                off = 0;
                io->r.off = 0;
                break;
            } else if (ws->header.payload > length) {
                // The data is incomplete. wait
                break;
            } else {
                ws->header.state = WS_DATA_OK;
            }
        }

        if (ws->header.state == WS_DATA_OK) {
            if (ws->header.payload > 0) {
                //Get the complete packet
                if (ws->header.mask == 0x80) {
                    _cc_ws_mask(io->r.bytes + off, ws->header.payload, ws->header.hash, 0);
                }
                _tprintf("WS:%.*s\n",(int)ws->header.payload, io->r.bytes + off);
                _ws_send(io, io->r.bytes + off, ws->header.payload);
                _CC_SET_BIT(_CC_EVENT_WRITABLE_, e->flags);
                off += ws->header.payload;
            }
            if (WS_OP_PING == ws->header.operation) {
                _ws_heartbeat(e, WS_OP_PONG);
            }
        }
    }

    if (off > 0) {
        io->r.off -= off;
        if (io->r.off > 0) {
            memmove(io->r.bytes, io->r.bytes + off, io->r.off);
        }
    }

    return true;
}

static void bad_request(_cc_event_t *e, _cc_io_buffer_t *io) {
    _cc_string_t body = _cc_string("<HTML><HEAD><TITLE>BAD REQUEST</TITLE></HEAD><BODY><P>Your browser sent a bad request, such as a POST without a Content-Length.</P></BODY></HTML>");

    io->w.off = snprintf((char*)io->w.bytes, io->w.limit,"HTTP/1.1 400 BAD REQUEST\r\nConnection: close;\r\nContent-type: text/html\r\nContent-Length: %ld\r\n\r\n", body.length);
    memcpy(io->w.bytes + io->w.off, body.ptr, body.length * sizeof(char_t));
    io->w.off += (int32_t)body.length * sizeof(char_t);
    _cc_io_buffer_flush(e, io);
}

static bool_t ws_http_handler(_cc_event_t *e, _cc_ws_t *ws) {
    _cc_io_buffer_t *io = (_cc_io_buffer_t*)ws->io;
    if (ws->state == _CC_HTTP_STATUS_HEADER_) {
        const _cc_http_header_t *connection, *upgrade;
        ws->state = _cc_http_header_parser((_cc_http_header_fn_t)_cc_http_alloc_request_header, (pvoid_t *)&ws->request, io->r.bytes, &io->r.off);
        /**/
        if (ws->state != _CC_HTTP_STATUS_PAYLOAD_) {
            return ws->state == _CC_HTTP_STATUS_HEADER_;
        }

        ws->payload = _ws_get_content_length(&ws->request->headers);
        if (ws->payload == 0) {
            ws->state = _CC_HTTP_STATUS_ESTABLISHED_;
        }

        connection = _cc_http_header_find(&ws->request->headers,_T("Connection"));
        upgrade = _cc_http_header_find(&ws->request->headers, _T("Upgrade"));
        if (connection == nullptr || upgrade == nullptr) {
            bad_request(e, io);
            return false;
        } else if (_tcsicmp("Upgrade",connection->value) != 0 || _tcsicmp("websocket",upgrade->value) != 0) {
            bad_request(e, io);
            return false;
        }
    } 

    if (ws->state == _CC_HTTP_STATUS_PAYLOAD_) {
        _cc_logger_debug(_T("%.*s."), io->r.off,io->r.bytes);
        if (ws->length >= ws->payload) {
            ws->state = _CC_HTTP_STATUS_ESTABLISHED_;
            ws->length = 0;
        } else {
            ws->length += io->r.off;
        }
        io->r.off = 0;
    }

    if (ws->state == _CC_HTTP_STATUS_ESTABLISHED_) {
        return _ws_response_header(e,ws);
    }
    return true;
}

static bool_t network_event_callback(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    _cc_ws_t *ws = (_cc_ws_t*)e->data;
    if (which & _CC_EVENT_ACCEPT_) {
        _cc_event_t *event;
        _cc_socket_t fd;
        struct sockaddr_in remote_addr = {0};
        _cc_socklen_t remote_addr_len = sizeof(struct sockaddr_in);

        fd = async->accept(async, e, (_cc_sockaddr_t*)&remote_addr, &remote_addr_len);
        if (fd == _CC_INVALID_SOCKET_) {
            _cc_logger_error(_T("accept fail %s."), _cc_last_error(_cc_last_errno()));
            return true;
        }

        _cc_set_socket_nonblock(fd, 1);

        event = _cc_alloc_event(async, _CC_EVENT_TIMEOUT_);
        if (event == nullptr) {
            _cc_close_socket(fd);
            _ws_free(ws);
            return true;
        }

        event->fd = fd;
        event->callback = e->callback;
        event->timeout = 100; //wait SSL handshake complete
        event->data = (uintptr_t)_ws_alloc(fd);

        if (async->attach(async, event) == false) {
            _cc_logger_debug(_T("thread %d add socket (%d) event fial."), _cc_get_thread_id(nullptr), fd);
            _cc_free_event(async, event);
            _ws_free(ws);
            return true;
        }

        {
            struct sockaddr_in *remote_ip = (struct sockaddr_in *)&remote_addr;
            byte_t *ip_addr = (byte_t *)&remote_ip->sin_addr.s_addr;
            _cc_logger_debug(_T("TCP accept [%d,%d,%d,%d] fd:%d"), ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], fd);
        }

        return true;
    } else if (which & _CC_EVENT_TIMEOUT_) {
        if (ws->handshake != _CC_SSL_HS_ESTABLISHED_) {
            ws->handshake = _SSL_do_handshake(ws->io->ssl);
            if (ws->handshake == _CC_SSL_HS_ESTABLISHED_) {
                e->timeout = 60000;
                _CC_SET_BIT(_CC_EVENT_READABLE_, e->flags);
                return true;
            } else if (ws->handshake == _CC_SSL_HS_ERROR_) {
                return false;
            }
            //wait SSL handshake complete
            return true;
        }
        _cc_logger_debug(_T("TCP timeout %d"), e->fd);
        if (_ws_heartbeat(e, WS_OP_PONG)) {
            return true;
        }
        return false;
    } else if (which & _CC_EVENT_CLOSED_) {
        _cc_logger_debug(_T("%d disconnect to client."), e->fd);
        if (e->data) {
            _ws_free((_cc_ws_t*)e->data);
        }
        return false;
    }

    if (which & _CC_EVENT_READABLE_) {
        _cc_io_buffer_t *io = (_cc_io_buffer_t*)ws->io;
        do {
            int32_t off = _cc_io_buffer_read(e, io);
            if (off < 0) {
                _cc_logger_debug(_T("read fail %s."), _cc_last_error(_cc_last_errno()));
                return false;
            } else if (off == 0) {
                break;
            }
            
            if (ws->state == _CC_HTTP_STATUS_ESTABLISHED_) {
                if (!_ws_unpack(e)) {
                    return false;
                }
            } else if (!ws_http_handler(e, ws)) {
                return false;
            }
        } while(true);
    }
 
    if (which & _CC_EVENT_WRITABLE_) {
        if (ws->io) {
            return _cc_io_buffer_flush(e, ws->io);
        } else {
            _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        }
    }

    return true;
}

int main(int argc, char *const argv[]) {
    // char c = 0;
    struct sockaddr_in sa;
    _cc_async_event_t async;
    _cc_event_t *e;
    uint16_t port = 5500;

    _cc_install_socket();
    openSSL = _SSL_init(_CC_SSL_DEFAULT_PROTOCOLS_);
    if (openSSL == nullptr) {
        return 1;
    }

    _SSL_setup(openSSL, "/var/ssl/ws.libcc.cn_bundle.crt", "/var/ssl/ws.libcc.cn.key",nullptr);

    if (_cc_register_poller(&async) == false) {
        return 1;
    }
    e = _cc_alloc_event(&async, _CC_EVENT_ACCEPT_);
    if (e == nullptr) {
        async.free(&async);
        return -1;
    }
    e->callback = network_event_callback;
    e->timeout = 60000;

    _cc_inet_ipv4_addr(&sa, nullptr, port);
    _cc_tcp_listen(&async, e, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in));
    _cc_logger_debug(_T("listen port: %d"), port);

    while (1) {
        // while((c = getchar()) != 'q') {
        async.wait(&async, 100);
    }

    async.free(&async);
    return 0;
}