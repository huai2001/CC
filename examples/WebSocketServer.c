#include <libcc.h>
#include <libcc/widgets/widgets.h>
#include <stdio.h>

typedef struct _WebSocket {
    byte_t status;
    _cc_http_request_header_t *request;
    _cc_buf_t buffer;
    int64_t content_length;
    char_t websocket_key[256];
} _WebSocket_t;

/**/
_CC_API_PRIVATE(void) _WebSocketSecKey(const tchar_t *sec_websocket_key, _WebSocket_t *ws) {
    _cc_sha1_t ctx;
    size_t length;
    byte_t results[1024];
    byte_t sha1_results[_CC_SHA1_DIGEST_LENGTH_];

    length = _snprintf((char_t*)results, _cc_countof(results), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", sec_websocket_key);

    _cc_sha1_init(&ctx);
    _cc_sha1_update(&ctx, results, length);
    _cc_sha1_final(&ctx, sha1_results);
    _cc_base64_encode(sha1_results, _CC_SHA1_DIGEST_LENGTH_, ws->websocket_key, _cc_countof(ws->websocket_key));
}

/**/
_CC_API_PRIVATE(bool_t) _WebSocketResponseHeader(_cc_event_t *e, _WebSocket_t *ws) {
    char_t headers[1024];
    size_t length = _snprintf(headers, _cc_countof(headers),
                        "HTTP/1.1 101 Switching Protocols\r\n"
                        "Connection: Upgrade\r\n"
                        "Upgrade: websocket\r\n"
                        "Sec-WebSocket-Protool: echo\r\n"
                        "Sec-WebSocket-Accept: %s\r\n\r\n",
                        ws->websocket_key);

    if (_cc_event_send(e, (byte_t*)headers, sizeof(char_t) * length) == -1) {
        return false;
    }
    //printf(headers);
    return true;
}

_CC_API_PRIVATE(int64_t) _WebSocketGetContentLength(_cc_map_t *headers) {
    const _cc_map_element_t *data = _cc_map_find(headers, _T("Content-Length"));
    return data ? _ttoi(data->element.uni_string) : 0;
}
/**/
_CC_API_PRIVATE(bool_t) _Heartbeat(_cc_event_t *e, byte_t oc) {
    byte_t buf[2];
    buf[0] = 0x80 | oc;
    buf[1] = 0;
    return (_cc_event_send(e, buf, 2) > 0);
}

/**/
_CC_API_PRIVATE(void) _WebSocketFree(_WebSocket_t *ws) {
    _cc_http_free_request_header(&ws->request);
    _cc_free_buf(&ws->buffer);
    _cc_free(ws);
}

/**/
_CC_API_PRIVATE(bool_t) _WebSocketData(_cc_event_t *e) {
    bool_t contice
    _WSHeader_t header = {0};
    _cc_event_rbuf_t *r = &e->buffer->r;

    header.bytes = r->bytes;
    header.length = r->length;
    header.offset = 0;

    do {
        if (_WSRead(&header) == WS_DATA_OK) {
            if (header.payload > 0 && header.mask) {
                //Get the complete packet
                _WSMask(header.bytes + header.offset, header.payload, header.mask);
            }
            //You can handle the packet
            //fn(header, header.bytes + header.offset, header.payload);
            switch (header.opc) {
                case WS_OP_PING:
                    _Heartbeat(e, WS_OP_PONG);
                    break;
                case WS_OP_PONG:
                    break;
                case WS_OP_BINARY:
                case WS_OP_JSON:
                case WS_OP_XML:
                case WS_OP_TXT:
                    _tprintf("WS:%.*s\n",(int)header.payload, header.bytes + header.offset);
                    break;
                case WS_OP_DISCONNECT:
                    return false;
            }
            header.offset += header.payload;
        } else {
            if (header.payload > r->limit) {
                _tprintf("big data fail\n");
                return false;
                /*
                //If you want big data.
                //This is just a simple example, you can optimize it
                _cc_alloc_event_rbuf(r,header.payload + _WS_MAX_HEADER_)
                header.bytes = r->bytes;
                header.length = r->length;
                */
            }
            break;
        }
    } while(header.length > header.offset);

    if (header.offset > 0 && header.length >= header.offset) {
        header.length -= header.offset;
        memmove(r->bytes, r->bytes + header.offset, header.length);
        r->length = header.length;
    }
    return true;
}

static bool_t network_event_callback(_cc_async_event_t *async, _cc_event_t *e, const uint16_t which) {
    _WebSocket_t *ws;
    if (which & _CC_EVENT_ACCEPT_) {
        _cc_event_t *event;
        _cc_async_event_t *cycle_new;
        _cc_socket_t fd;
        struct sockaddr_in remote_addr = {0};
        _cc_socklen_t remote_addr_len = sizeof(struct sockaddr_in);

        fd = _cc_event_accept(async, e, &remote_addr, &remote_addr_len);
        if (fd == _CC_INVALID_SOCKET_) {
            _cc_logger_error(_T("accept fail %s.\n"), _cc_last_error(_cc_last_errno()));
            return true;
        }

        cycle_new = _cc_get_async_event();
        event = _cc_event_alloc(cycle_new, _CC_EVENT_TIMEOUT_ | _CC_EVENT_READABLE_ | _CC_EVENT_BUFFER_);
        if (event == nullptr) {
            _cc_close_socket(fd);
            return true;
        }
        _cc_set_socket_nonblock(fd, 1);

        ws = (_WebSocket_t*)_cc_malloc(sizeof(_WebSocket_t));
        ws->content_length = 0;
        ws->status = _CC_HTTP_STATUS_HEADER_;
        ws->request = nullptr;
        _cc_alloc_buf(&ws->buffer, _CC_16K_BUFFER_SIZE_);

        event->fd = fd;
        event->callback = e->callback;
        event->timeout = e->timeout;
        event->args = ws;

        if (cycle_new->attach(cycle_new, event) == false) {
            _cc_logger_debug(_T("thread %d add socket (%d) event fial."), _cc_get_thread_id(nullptr), fd);
            _cc_free_event(cycle_new, event);
            _WebSocketFree(ws);
            return true;
        }

        {
            struct sockaddr_in *remote_ip = (struct sockaddr_in *)&remote_addr;
            byte_t *ip_addr = (byte_t *)&remote_ip->sin_addr.s_addr;
            _cc_logger_debug(_T("TCP accept [%d,%d,%d,%d] fd:%d"), ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], fd);
        }

        return true;
    }

    if (which & _CC_EVENT_DISCONNECT_) {
        _cc_logger_debug(_T("%d disconnect to client."), e->fd);
        _WebSocketFree((_WebSocket_t*)e->args);
        return false;
    }

    if (which & _CC_EVENT_READABLE_) {
        _cc_event_buffer_t *rw = e->buffer;
        if (e->buffer == nullptr) {
            _WebSocketFree((_WebSocket_t*)e->args);
            return false;
        }

        if (!_cc_event_recv(e)) {
            _WebSocketFree((_WebSocket_t*)e->args);
            return false;
        }

        ws = (_WebSocket_t*)e->args;

        if (ws->status == _CC_HTTP_STATUS_FINISHED_) {
            if(_WebSocketData(e)) {
                return true;
            }
            _WebSocketFree(ws);
            return false;
        }

        if (ws->status == _CC_HTTP_STATUS_HEADER_) {
            ws->status = _cc_http_header_parser((_cc_http_header_fn_t)_cc_http_alloc_request_header, (pvoid_t *)&ws->request, &rw->r);
            /**/
            switch (ws->status) {
            case _CC_HTTP_STATUS_HEADER_:
                return true;
            case _CC_HTTP_STATUS_PAYLOAD_:
                _cc_buf_cleanup(&ws->buffer);
                break;
            default:
                _WebSocketFree((_WebSocket_t*)e->args);
                return false;
            }
            const _cc_map_element_t* connection = _cc_map_find(&ws->request->headers,_T("Connection"));
            const _cc_map_element_t* upgrade = _cc_map_find(&ws->request->headers, _T("Upgrade"));
            const _cc_map_element_t* sec_websocket_key = _cc_map_find(&ws->request->headers, _T("Sec-WebSocket-Key"));
            if (connection == nullptr || upgrade == nullptr || sec_websocket_key == nullptr) {
                _WebSocketFree((_WebSocket_t*)e->args);
                return false;
            }
            
            if (_tcsicmp("Upgrade",connection->element.uni_string) != 0 || _tcsicmp("websocket",upgrade->element.uni_string) != 0) {
                _WebSocketFree((_WebSocket_t*)e->args);
                return false;
            }
            ws->content_length = _WebSocketGetContentLength(&ws->request->headers);
            if (ws->content_length == 0) {
                ws->status = _CC_HTTP_STATUS_FINISHED_;
            }
            _WebSocketSecKey(sec_websocket_key->element.uni_string, ws);
        } 

        if (ws->status == _CC_HTTP_STATUS_PAYLOAD_) {
            _cc_buf_append(&ws->buffer, rw->r.bytes, rw->r.length);
            if (ws->buffer.length >= ws->content_length) {
                ws->status = _CC_HTTP_STATUS_FINISHED_;
            }
            rw->r.length = 0;
        }

        if (ws->status == _CC_HTTP_STATUS_FINISHED_) {
            return _WebSocketResponseHeader(e,ws);
        }

        return true;
    }

    if (which & _CC_EVENT_WRITABLE_) {
        if (e->buffer) {
            if (_cc_event_sendbuf(e) < 0) {
                _WebSocketFree((_WebSocket_t*)e->args);
                return false;
            }
        } else {
            _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        }
    }

    if (which & _CC_EVENT_TIMEOUT_) {
        if (_Heartbeat(e, WS_OP_PONG)) {
            return true;
        }
        _cc_logger_debug(_T("TCP timeout %d\n"), e->fd);
        _WebSocketFree((_WebSocket_t*)e->args);
        return false;
    }

    return true;
}
int main(int argc, char *const argv[]) {
    // char c = 0;
    struct sockaddr_in sa;
    _cc_async_event_t async;
    _cc_event_t *e;

    _cc_install_socket();

    if (_cc_init_event_poller(&async) == false) {
        return 1;
    }
    e = _cc_event_alloc(&async, _CC_EVENT_ACCEPT_);
    if (e == nullptr) {
        async.quit(&async);
        return -1;
    }
    e->callback = network_event_callback;
    e->timeout = 60000;

    _cc_inet_ipv4_addr(&sa, nullptr, 8080);
    _cc_tcp_listen(&async, e, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in));

    while (1) {
        // while((c = getchar()) != 'q') {
        _cc_event_wait(&async, 100);
    }

    // async.quit(&async);
    return 0;
}
