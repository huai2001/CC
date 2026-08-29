#include <libcc.h>
#include <libcc/http.h>
#include <libcc/event.h>
#include <stdio.h>
#define _WS_ENABLE_SSL_                 1
#define _WS_SUPPORT_BIG_DATA_           0

#if _WS_ENABLE_SSL_
_cc_OpenSSL_t *WS_SSL = NULL;
#endif

enum {
    WS_DATA_HTTP = 0x01,
    WS_DATA_RING
};

typedef struct _WS_channel {
    uint8_t state;
    uint8_t flag;
    uint8_t classify;
    uint8_t authorized;

#if _WS_SUPPORT_BIG_DATA_
    uint64_t length;
    byte_t* buf;
#endif

    uint64_t identifier;
    //
    uintptr_t data;
    
    _cc_io_buffer_t *io;
    _cc_ws_header_t header;

    struct {
        uint16_t family;
        uint16_t port;
        tchar_t address[24];
    } remote;

    _cc_list_t lnk;
} _WS_channel_t;

/**/
_CC_API_PRIVATE(_WS_channel_t*) _WS_alloc(_cc_socket_t fd) {
    _WS_channel_t *channel = (_WS_channel_t*)_cc_malloc(sizeof(_WS_channel_t));

    channel->state = _CC_HTTP_STATE_HEADER_;
    channel->classify = 0xff;
    channel->authorized = 0;
    channel->remote.port = 0;
    channel->remote.family = 0;
    channel->flag = 0;
    channel->data = 0;
    channel->header.state = WS_DATA_OK;

#if _WS_SUPPORT_BIG_DATA_
    channel->length = 0;
    channel->buf = NULL;
#endif

#if _WS_ENABLE_SSL_
    channel->io = _cc_alloc_io_buffer(_CC_IO_BUFFER_SIZE_, _SSL_accept(WS_SSL, fd));
#else
    channel->io = _cc_alloc_io_buffer(_CC_IO_BUFFER_SIZE_, NULL);
#endif

    _cc_list_cleanup(&channel->lnk);
    return channel;
}

/**/
_CC_API_PRIVATE(void) _WS_free(_WS_channel_t *channel) {
    if (channel->data) {
        switch (channel->flag) {
            case WS_DATA_HTTP:
                _cc_http_free_request_header((_cc_http_request_header_t**)&channel->data);
            break;
            case WS_DATA_RING:
                channel->data = 0;
            break;
        }
    }

    if (channel->io) {
        _cc_free_io_buffer(channel->io);
    }
#if _WS_SUPPORT_BIG_DATA_
    _cc_if_free(channel->buf);
#endif
    _cc_free(channel);
}

_CC_API_PRIVATE(void) bad_request(_cc_event_t *e, _cc_io_buffer_t *io) {
    struct {const tchar_t *ptr; size_t length;} body = {_CC_STRING("<HTML><HEAD><TITLE>BAD REQUEST</TITLE></HEAD><BODY><P>Your browser sent a bad request, such as a POST without a Content-Length.</P></BODY></HTML>")};
    io->w.off = snprintf((char*)io->w.bytes, io->w.limit,"HTTP/1.1 400 BAD REQUEST\r\nConnection: close;\r\nContent-type: text/html\r\nContent-Length: %ld\r\n\r\n", body.length);
    memcpy(io->w.bytes + io->w.off, body.ptr, body.length * sizeof(char_t));
    io->w.off += (int32_t)body.length * sizeof(char_t);
    _cc_io_buffer_flush(e, io);
}

/**/
_CC_API_PRIVATE(bool_t) _ws_response_header(_cc_event_t *e, _WS_channel_t *channel) {
    _cc_hasher_t c;
    int32_t length;
    char_t results[1024];
    byte_t digest[_CC_SHA1_DIGEST_LENGTH_];

    const _cc_http_request_header_t *request = (_cc_http_request_header_t *)channel->data;
    const _cc_http_header_t *ws_protool = _cc_http_header_find(&request->headers, _T("Sec-WebSocket-Protocol"));
    const _cc_http_header_t *ws_key = _cc_http_header_find(&request->headers, _T("Sec-WebSocket-Key"));
    const _cc_http_header_t *ws_version = _cc_http_header_find(&request->headers, _T("Sec-WebSocket-Version"));
    _cc_io_buffer_t *io = (_cc_io_buffer_t*)channel->io;

    //RFC 6455
    if (ws_version && _tcsicmp(ws_version->value, "13") != 0) {
        struct {const tchar_t *ptr; size_t length;} body = {_CC_STRING("HTTP/1.1 426 Upgrade Required\r\nSec-WebSocket-Version: 13\r\nConnection: close\r\nContent-Length: 0\r\n\r\n")};
        memcpy(io->w.bytes + io->w.off, body.ptr, body.length * sizeof(char_t));
        io->w.off += (int32_t)body.length * sizeof(char_t);
    } else {
        length = (int32_t)_snprintf(results, _cc_countof(results), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", ws_key->value);
        //_cc_logger_debug("Sec-WebSocket-Key: %s",ws_key->value);

        _cc_sha1_init(&c);
        c.update(&c, (byte_t*)results, length);
        c.final(&c, digest, &length);
        c.free(&c);

        length = _cc_base64_encode(digest, length, results, _cc_countof(results));

        io->w.off = _snprintf((char*)io->w.bytes, io->w.limit,
                            "HTTP/1.1 101 Switching Protocols\r\n"
                            "Connection: Upgrade\r\n"
                            "Upgrade: websocket\r\n"
                            "Sec-WebSocket-Accept: %.*s\r\n"
                            "Sec-WebSocket-Version: 13\r\n",
                            length, results);
        if (ws_protool) {
            _cc_sds_t protool = ws_protool->value;
            int length = _cc_sds_length(protool);
            const char_t* token = strchr(protool, ',');
            if (token) {
                length = (int)(token - protool);
            }
            io->w.off += _snprintf((char*)io->w.bytes + io->w.off, io->w.limit - io->w.off, "Sec-WebSocket-Protocol: %.*s\r\n\r\n", length, protool);
        } else {
            io->w.bytes[io->w.off] = '\r';
            io->w.bytes[io->w.off + 1] = '\n';
            io->w.off += 2;
        }
    }

    _cc_http_free_request_header((_cc_http_request_header_t**)&channel->data);

    //完成握手

    //printf("send: %.*s\n",io->w.off, io->w.bytes);
    return _cc_io_buffer_flush(e, io) >= 0;
}

/**/
bool_t _WS_heartbeat(_cc_event_t *e, byte_t oc) {
    _WS_channel_t *channel = (_WS_channel_t*)e->data;
    _cc_io_buffer_t *io = (_cc_io_buffer_t*)channel->io;
    byte_t buf[2];
    buf[0] = 0x80 | oc;
    buf[1] = 0;
    return (_cc_io_buffer_send(e, io, buf, 2) > 0);
}

/**/
bool_t _WS_unpack(_cc_event_t *e) {
    _WS_channel_t *channel = (_WS_channel_t*)e->data;
    int32_t off = 0;
    _cc_io_buffer_t *io = (_cc_io_buffer_t*)channel->io;

    while (io->r.off > off) {
        if (channel->header.state == WS_DATA_OK || channel->header.state == WS_HEADER_PARTIAL) {
            off += _cc_ws_header_parser(&channel->header,io->r.bytes + off, io->r.off - off);
            if (channel->header.state == WS_HEADER_PARTIAL) {
                break;
            }
            //You can handle the packet
            switch (channel->header.operation) {
                case WS_OP_PING:
                case WS_OP_PONG:
                    if (channel->header.payload > (int64_t)io->r.limit) {
                        printf("big data fail. operation 0x%x", channel->header.operation);
                        return false;
                    }
                    break;
                case WS_OP_CONTINUATION:
                    printf("WS_OP_CONTINUATION operation 0x%x", channel->header.operation);
                    break;
                case WS_OP_TEXT:
                case WS_OP_BINARY:
                case WS_OP_JSON:
                case WS_OP_XML:
#if _WS_SUPPORT_BIG_DATA_
                    if (channel->header.payload > (int64_t)io->r.limit) {
                        printf("%ld, big data. operation 0x%x", channel->header.payload, channel->header.operation);
                        if (channel->buf) {
                            _cc_free(channel->buf);
                        }
                        channel->buf = _cc_malloc(channel->header.payload);
                    }
#endif
                    break;
                case WS_OP_DISCONNECT:
                    return false;
                default:
                    /* not handled or failed */
                    printf("Unhandled ext operation 0x%x", channel->header.operation);
                    return false;
            }
        }

        if (channel->header.state == WS_DATA_PARTIAL) {
            int64_t length = (io->r.off - off);
            if (channel->header.payload > (int64_t)io->r.limit) {
#if _WS_SUPPORT_BIG_DATA_
                int64_t remaining = channel->header.payload - channel->length;
                if (remaining <= length) {
                    if (channel->header.mask == 0x80) {
                        _cc_ws_mask_copy(channel->buf + channel->length, channel->header.payload - channel->length, io->r.bytes + off, remaining, channel->header.hash, channel->length);
                    } else {
                        // copy data
                        memcpy(channel->buf + channel->length, io->r.bytes + off, remaining);
                    }

                    _tprintf("WS:%.*s\n",(int)channel->length, channel->buf);

                    channel->length = 0;
                    channel->header.state = WS_HEADER_PARTIAL;//

                    off += remaining;
                    _cc_if_free(channel->buf);
                    // There is still data. Keep processing
                    if (io->r.off > off) {
                        continue;
                    }
                } else {
                    // copy data
                    if (channel->header.mask == 0x80) {
                        _cc_ws_mask_copy(channel->buf + channel->length, channel->header.payload - channel->length, io->r.bytes + off, length, channel->header.hash, channel->length);
                    } else {
                        // copy data
                        memcpy(channel->buf + channel->length, io->r.bytes + off, length);
                    }
                    channel->length += length;
                }
                off = 0;
                io->r.off = 0;
                break;
#else
                //discard it directly without any treatment
                return false;
#endif
            } else if (channel->header.payload > length) {
                // The data is incomplete. wait
                break;
            } else {
                channel->header.state = WS_DATA_OK;
            }
        }

        if (channel->header.state == WS_DATA_OK) {
            if (WS_OP_PING == channel->header.operation) {
                _WS_heartbeat(e, WS_OP_PONG);
            } else if (channel->header.payload > 0) {
                //Get the complete packet
                if (channel->header.mask == 0x80) {
                    _cc_ws_mask(io->r.bytes + off, channel->header.payload, channel->header.hash, 0);
                }

                _tprintf("WS:%.*s\n",(int)channel->header.payload, io->r.bytes + off);
            }
            off += channel->header.payload;
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

_CC_API_PRIVATE(bool_t) _ws_http_handler(_cc_event_t *e, _WS_channel_t *channel) {
    _cc_io_buffer_t *io = (_cc_io_buffer_t*)channel->io;
    
    if (channel->state == _CC_HTTP_STATE_HEADER_) {
        const _cc_rbtree_t *headers;
        const _cc_http_header_t *connection, *upgrade;
        channel->flag = WS_DATA_HTTP;
        channel->state = _cc_http_header_parser((_cc_http_header_fn_t)_cc_http_alloc_request_header, (pvoid_t *)&channel->data, io->r.bytes, (int32_t *)&io->r.off);
        
        /**/
        if (channel->state != _CC_HTTP_STATE_PAYLOAD_) {
            return channel->state == _CC_HTTP_STATE_HEADER_;
        }

        headers = &((_cc_http_request_header_t *)channel->data)->headers;
        connection = _cc_http_header_find(headers,_T("Connection"));
        upgrade = _cc_http_header_find(headers, _T("Upgrade"));
        if (connection == NULL || upgrade == NULL) {
            bad_request(e, io);
            return false;
        } else if (_tcsicmp("Upgrade",connection->value) != 0 || _tcsicmp("websocket",upgrade->value) != 0) {
            bad_request(e, io);
            return false;
        }
        channel->state = _CC_HTTP_STATE_ESTABLISHED_;
        /*
        _cc_rb_t *node;
        _cc_rbtree_for(node, headers) {
            _cc_http_header_t *header = _cc_upcast(node, _cc_http_header_t, lnk);
            printf("header:%s=%s\n", header->keyword, header->value);
        }*/
    }

    if (io->r.off != 0) {
        //直接丢弃
        io->r.off = 0;
    }

    if (channel->state == _CC_HTTP_STATE_ESTABLISHED_ && channel->flag == WS_DATA_HTTP) {
        return _ws_response_header(e,channel);
    }

    return true;
}

_CC_API_PRIVATE(bool_t) _ws_handler(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    _WS_channel_t *channel = (_WS_channel_t*)e->data;
    if (which & _CC_EVENT_ACCEPT_) {
        _cc_event_t *event;
        _cc_socket_t fd;
        _cc_async_event_t *async_event = _cc_get_async_event();
        struct sockaddr_storage remote_addr = {0};
        //struct sockaddr_in remote_addr = {0};
        _cc_socklen_t remote_addr_len = sizeof(struct sockaddr_storage);

        fd = async->accept(async, e, (_cc_sockaddr_t*)&remote_addr, &remote_addr_len);
        if (fd == _CC_INVALID_SOCKET_) {
            printf("accept fail %s.", _cc_last_error(_cc_last_errno()));
            return true;
        }

        _cc_set_socket_nonblock(fd, 1);

        event = _cc_alloc_event(async_event, _CC_EVENT_TIMEOUT_);
        if (event == NULL) {
            _cc_close_socket(fd);
            return true;
        }

        event->fd = fd;
        event->callback = e->callback;
    #if _WS_ENABLE_SSL_
        event->timeout = 100; //wait SSL handshake complete
    #else
        event->timeout = e->timeout;
        _CC_SET_BIT(_CC_EVENT_READABLE_, event->flags);
    #endif

        channel = _WS_alloc(fd);
        channel->identifier = event->ident;
        event->data = (uintptr_t)channel;

        if (async_event->attach(async_event, event) == false) {
            printf("thread %ld add socket (%d) event fial.", (intptr_t)_cc_get_thread_id(NULL), (int)fd);
            _cc_free_event(async_event, event);
            _WS_free(channel);
            return true;
        }

        if (remote_addr.ss_family == AF_INET) {
            struct sockaddr_in *a = (struct sockaddr_in *)&remote_addr;
            channel->remote.family = a->sin_family;
            _cc_inet_ntop(a->sin_family, &(a->sin_addr), channel->remote.address, _cc_countof(channel->remote.address));
        } else if (remote_addr.ss_family == AF_INET6) {
            struct sockaddr_in6 *a = (struct sockaddr_in6 *)&remote_addr;
            channel->remote.family = a->sin6_family;
            _cc_inet_ntop(a->sin6_family, &(a->sin6_addr), channel->remote.address, _cc_countof(channel->remote.address));
        }
        
        return true;
    } else if (which & _CC_EVENT_TIMEOUT_) {
#if _WS_ENABLE_SSL_
        _cc_io_buffer_t *io = (_cc_io_buffer_t*)channel->io;
        if (io && io->ssl && io->ssl->is_handshaked == false) {
            if (_SSL_do_handshake(io->ssl) == _CC_SSL_HS_ERROR_) {
                return false;
            }
            if (io->ssl->is_handshaked) {
                e->timeout = 60000;
                _CC_SET_BIT(_CC_EVENT_READABLE_, e->flags);
            }
            //wait SSL handshake complete
            return true;
        }
#endif
        printf("TCP timeout %d", e->fd);
        if (_WS_heartbeat(e, WS_OP_PONG)) {
            return true;
        }

        return false;
    } else if (which & _CC_EVENT_CLOSED_) {
        printf("%d disconnect to client.", e->fd);
        if (e->data) {
            _WS_free(channel);
            e->data = 0;
        }
        return false;
    }

    if (which & _CC_EVENT_READABLE_) {
        _cc_io_buffer_t *io = (_cc_io_buffer_t*)channel->io;
        do {
            int32_t off = _cc_io_buffer_read(e, io);
            if (off < 0) {
                //printf("read fail %s.", _cc_last_error(_cc_last_errno()));
                return false;
            } else if (off == 0) {
                break;
            }
            
            if (channel->state == _CC_HTTP_STATE_ESTABLISHED_) {
                if (!_WS_unpack(e)) {
                    return false;
                }
            } else if (!_ws_http_handler(e, channel)) {
                return false;
            }
        } while(true);
    }
 
    if (which & _CC_EVENT_WRITABLE_) {
        _cc_io_buffer_t *io = (_cc_io_buffer_t*)channel->io;
        if (io) {
            if (_cc_io_buffer_flush(e, io) < 0) {
                return false;
            }
        }

        if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags)) {
            _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags); 
        }
    }

    return true;
}

bool_t _WS_listener(uint16_t port) {
    struct sockaddr_in sa;
    _WS_channel_t *channel;
    _cc_async_event_t *async = _cc_get_async_event();
    _cc_event_t *event = _cc_alloc_event(async, _CC_EVENT_ACCEPT_);

    _cc_assert(async != NULL);
    _cc_assert(event != NULL);

    if (event == NULL) {
        return false;
    }

    channel = (_WS_channel_t*)_cc_malloc(sizeof(_WS_channel_t));
    channel->state = _CC_HTTP_STATE_ESTABLISHED_;
    channel->authorized = 0;
    channel->flag = 0;
    channel->data = 0;
    channel->header.state = WS_DATA_OK;

#if _WS_SUPPORT_BIG_DATA_
    channel->length = 0;
    channel->buf = NULL;
#endif

    channel->io = NULL;
    _cc_list_cleanup(&channel->lnk);

    event->timeout = 60000;
    event->callback = _ws_handler;
    event->data = (uintptr_t)channel;

    _cc_inet_ipv4_addr(&sa, NULL, port);

    channel->remote.port = port;
    channel->remote.family = sa.sin_family;
    channel->remote.address[0] = 0;

    if (!_cc_tcp_listen(async, event, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
        _cc_free_event(async, event);
        _cc_assert(false);
        return false;
    }
    
    return true;
}

int main(int argc, char *const argv[]) {
    int c;
    _cc_alloc_async_event(0, NULL);
#if _WS_ENABLE_SSL_
    WS_SSL = _SSL_init(_CC_SSL_DEFAULT_PROTOCOLS_);
    if (WS_SSL == NULL) {
        return 1;
    }
    _SSL_setup(WS_SSL, "/opt/www/ssl/ws.libcc.cn_bundle.crt", "/opt/www/ssl/ws.libcc.cn.key",NULL);
#endif

    _WS_listener(5500);

    while((c = getchar()) != 'q') {
        _cc_sleep(100);
    }
    _cc_free_async_event();
    return 0;
}