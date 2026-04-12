#include <libcc/http_request.h>
#include <libcc/http2.h>

static _cc_OpenSSL_t *openSSL = NULL;
static bool_t url_request(const tchar_t *url, pvoid_t args);
static bool_t url_request_connect(_cc_http_request_t *request);

typedef struct _cc_http2_client {
    uint32_t connection_window_size;
    uint32_t stream_window_size;
    uint32_t stream_id;
    uint8_t enable_push;
    uint32_t header_table_size;
    uint32_t max_frame_size;
    uint32_t max_concurrent_streams;
    uint32_t max_header_list_size;
    bool_t settings_acked;
    bool_t request_sent;
} _cc_http2_client_t;

static _cc_http2_client_t http_clients[128];

static _cc_http2_client_t* _cc_http2_get_client(void) {
    return &http_clients[0];
}

static void _cc_http2_init_client(_cc_http2_client_t *client) {
    memset(client, 0, sizeof(*client));
    client->connection_window_size = _CC_HTTP2_INITIAL_WINDOW_SIZE_;
    client->stream_window_size = _CC_HTTP2_INITIAL_WINDOW_SIZE_;
    client->max_frame_size = 16384;
    client->stream_id = 1;
}

static int32_t _cc_hpack_encode_integer(byte_t *buffer, size_t size, uint32_t value, uint8_t prefix_bits, uint8_t prefix_mask) {
    uint32_t prefix_max;
    int32_t offset = 0;

    if (size == 0 || prefix_bits == 0 || prefix_bits > 8) {
        return -1;
    }

    prefix_max = (1u << prefix_bits) - 1u;
    if (value < prefix_max) {
        buffer[offset++] = (byte_t)(prefix_mask | value);
        return offset;
    }

    buffer[offset++] = (byte_t)(prefix_mask | prefix_max);
    value -= prefix_max;

    while (value >= 128) {
        if ((size_t)offset >= size) {
            return -1;
        }
        buffer[offset++] = (byte_t)((value & 0x7f) | 0x80);
        value >>= 7;
    }

    if ((size_t)offset >= size) {
        return -1;
    }

    buffer[offset++] = (byte_t)value;
    return offset;
}

static int32_t _cc_hpack_encode_string(byte_t *buffer, size_t size, const byte_t *value, size_t length) {
    int32_t bytes = _cc_hpack_encode_integer(buffer, size, (uint32_t)length, 7, 0x00);
    if (bytes < 0 || (size_t)(bytes + length) > size) {
        return -1;
    }

    if (length > 0) {
        memcpy(buffer + bytes, value, length);
    }

    return bytes + (int32_t)length;
}

static int32_t _cc_hpack_encode_indexed(byte_t *buffer, size_t size, uint32_t index) {
    return _cc_hpack_encode_integer(buffer, size, index, 7, 0x80);
}

static int32_t _cc_hpack_encode_literal_indexed_name(byte_t *buffer, size_t size, uint32_t name_index, const byte_t *value, size_t length) {
    int32_t bytes = _cc_hpack_encode_integer(buffer, size, name_index, 4, 0x00);
    int32_t total;

    if (bytes < 0) {
        return -1;
    }

    total = _cc_hpack_encode_string(buffer + bytes, size - (size_t)bytes, value, length);
    if (total < 0) {
        return -1;
    }

    return bytes + total;
}

static int32_t _cc_http2_send_frame(_cc_event_t *e, _cc_io_buffer_t *io, uint8_t type, uint8_t flags, uint32_t stream_id, const byte_t *payload, uint32_t length) {
    byte_t header[_CC_HTTP2_FRAME_HEADER_SIZE_];

    if (_cc_http2_frame_header(header, type, flags, stream_id, length) != _CC_HTTP2_FRAME_HEADER_SIZE_) {
        return -1;
    }

    if (_cc_io_buffer_send(e, io, header, _CC_HTTP2_FRAME_HEADER_SIZE_) < 0) {
        return -1;
    }

    if (length > 0 && _cc_io_buffer_send(e, io, payload, (int32_t)length) < 0) {
        return -1;
    }

    return 0;
}

static int32_t _cc_http2_send_settings(_cc_event_t *e, _cc_io_buffer_t *io, bool_t ack) {
    if (ack) {
        return _cc_http2_send_frame(e, io, _CC_HTTP2_FRAME_TYPE_SETTINGS_, _CC_HTTP2_FRAME_FLAG_ACK_, 0, NULL, 0);
    } else {
        byte_t payload[6];
        payload[0] = 0x00;
        payload[1] = _CC_HTTP2_SETTINGS_ENABLE_PUSH_;
        payload[2] = 0x00;
        payload[3] = 0x00;
        payload[4] = 0x00;
        payload[5] = 0x00;
        return _cc_http2_send_frame(e, io, _CC_HTTP2_FRAME_TYPE_SETTINGS_, _CC_HTTP2_FRAME_FLAG_NO_, 0, payload, sizeof(payload));
    }
}

static int32_t _cc_http2_send_ping(_cc_event_t *e, _cc_io_buffer_t *io, uint8_t flags, const byte_t *opaque_data) {
    return _cc_http2_send_frame(e, io, _CC_HTTP2_FRAME_TYPE_PING_, flags, 0, opaque_data, 8);
}

static int32_t _authority(const _cc_url_t *url, char *buffer, size_t size) {
    bool_t include_port = false;
    int32_t length = (int32_t)_cc_sds_length(url->host);

    if (url->scheme.ident == _CC_SCHEME_HTTPS_ && url->port != _CC_PORT_HTTPS_) {
        include_port = true;
    } else if (url->scheme.ident == _CC_SCHEME_HTTP_ && url->port != _CC_PORT_HTTP_) {
        include_port = true;
    }

    if (include_port) {
        return snprintf(buffer, size, "%.*s:%u", length, url->host, url->port);
    }

    memcpy(buffer, url->host, length);
    buffer[length] = '\0';
    return length;
}

static int32_t _build_path(const _cc_url_t *url, char *buffer, size_t size) {
    if (url->query && _cc_sds_length(url->query) > 0) {
        return snprintf(buffer, size, "%s?%s", url->path, url->query);
    }

    if (url->path && _cc_sds_length(url->path) > 0) {
        return snprintf(buffer, size, "%s", url->path);
    }

    buffer[0] = '/';
    buffer[1] = '\0';
    return 1;
}

static void _cc_http2_close_connection(_cc_http2_client_t *client, uint32_t stream_id) {
    _CC_UNUSED(client);
    _CC_UNUSED(stream_id);
}

static bool_t url_request_header(_cc_http_request_t *request, _cc_event_t *e) {
    _cc_url_t *u = &request->url;
    _cc_io_buffer_t *io = request->io;
    _cc_http2_client_t *client = _cc_http2_get_client();
    byte_t header_block[1024];
    const char preface[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
    char authority[512];
    char path[1024];
    int32_t length;
    int32_t offset;

    if (client->request_sent) {
        return true;
    }

    length = _authority(u, authority, sizeof(authority));
    if (length <= 0 || (size_t)length >= sizeof(authority)) {
        return false;
    }

    length = _build_path(u, path, sizeof(path));
    if (length <= 0 || (size_t)length >= sizeof(path)) {
        return false;
    }

    if (_cc_io_buffer_send(e, io, (const byte_t *)preface, (int32_t)(sizeof(preface) - 1)) < 0) {
        return false;
    }

    if (_cc_http2_send_settings(e, io, false) < 0) {
        return false;
    }

    offset = _cc_hpack_encode_indexed(header_block, sizeof(header_block), _CC_HTTP2_INDEXED_METHOD_GET_);
    if (offset < 0) {
        return false;
    }

    if (u->scheme.ident == _CC_SCHEME_HTTP_) {
        length = _cc_hpack_encode_indexed(header_block + offset, sizeof(header_block) - (size_t)offset, _CC_HTTP2_INDEXED_SCHEME_HTTP_);
    } else {
        length = _cc_hpack_encode_indexed(header_block + offset, sizeof(header_block) - (size_t)offset, _CC_HTTP2_INDEXED_SCHEME_HTTPS_);
    }
    if (length < 0) {
        return false;
    }
    offset += length;

    if (path[0] == '/' && path[1] == '\0') {
        length = _cc_hpack_encode_indexed(header_block + offset, sizeof(header_block) - (size_t)offset, _CC_HTTP2_INDEXED_PATH_);
    } else {
        length = _cc_hpack_encode_literal_indexed_name(header_block + offset, sizeof(header_block) - (size_t)offset, _CC_HTTP2_INDEXED_PATH_, (const byte_t *)path, strlen(path));
    }
    if (length < 0) {
        return false;
    }
    offset += length;

    length = _cc_hpack_encode_literal_indexed_name(header_block + offset, sizeof(header_block) - (size_t)offset, _CC_HTTP2_INDEXED_AUTHORITY_, (const byte_t *)authority, strlen(authority));
    if (length < 0) {
        return false;
    }
    offset += length;

    length = _cc_hpack_encode_literal_indexed_name(header_block + offset, sizeof(header_block) - (size_t)offset, _CC_HTTP2_INDEXED_USER_AGENT_, (const byte_t *)"libcc-http2-client", sizeof("libcc-http2-client") - 1);
    if (length < 0) {
        return false;
    }
    offset += length;

    if (_cc_http2_send_frame(e, io, _CC_HTTP2_FRAME_TYPE_HEADERS_, _CC_HTTP2_FRAME_FLAG_END_HEADERS_ | _CC_HTTP2_FRAME_FLAG_END_STREAM_, client->stream_id, header_block, (uint32_t)offset) < 0) {
        return false;
    }

    client->request_sent = true;
    return _cc_io_buffer_flush(e, io) >= 0;
}

static bool_t _handshaking(_cc_event_t *e, _cc_http_request_t *request) {
    if (_SSL_do_handshake(request->io->ssl) == _CC_SSL_HS_ERROR_) {
        return false;
    }
    if (request->io->ssl->is_handshaked) {
        e->timeout = 60000;
        _CC_SET_BIT(_CC_EVENT_READABLE_, e->flags);
        return url_request_header(request, e);
    }
    return true;
}

static bool_t _http_request_callback(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    _cc_http_request_t *request = (_cc_http_request_t *)e->data;
    _cc_http2_client_t *client = _cc_http2_get_client();

    _CC_UNUSED(async);

    if (_CC_ISSET_BIT(_CC_EVENT_CLOSED_, which)) {
        _cc_logger_warin("_cc_http_request_ _CC_EVENT_CLOSED_ %d", e->ident);
        _cc_free_http_request(request);
        return false;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, which)) {
        if (request->url.scheme.ident == _CC_SCHEME_HTTPS_ && request->io && request->io->ssl && !request->io->ssl->is_handshaked) {
            return _handshaking(e, request);
        }
        return false;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_CONNECT_, which)) {
        _cc_logger_info("url_request connected,%s", request->url.host);
        if (request->url.scheme.ident == _CC_SCHEME_HTTPS_) {
            return _handshaking(e, request);
        }
        _CC_SET_BIT(_CC_EVENT_READABLE_, e->flags);
        return url_request_header(request, e);
    }

    if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_, which)) {
        return _cc_io_buffer_flush(e, request->io) >= 0;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_READABLE_, which)) {
        int32_t bytes = _cc_io_buffer_read(e, request->io);
        int32_t offset = 0;

        if (bytes < 0) {
            return false;
        }
        if (bytes == 0) {
            return true;
        }

        while ((request->io->r.off - offset) >= _CC_HTTP2_FRAME_HEADER_SIZE_) {
            byte_t *frame = request->io->r.bytes + offset;
            byte_t *payload;
            _cc_http2_frame_header_t header;
            int32_t frame_size;

            header.length = ((uint32_t)frame[0] << 16) | ((uint32_t)frame[1] << 8) | (uint32_t)frame[2];
            header.type = frame[3];
            header.flags = frame[4];
            header.stream_id = ((uint32_t)(frame[5] & 0x7f) << 24) | ((uint32_t)frame[6] << 16) | ((uint32_t)frame[7] << 8) | (uint32_t)frame[8];
            frame_size = _CC_HTTP2_FRAME_HEADER_SIZE_ + (int32_t)header.length;
            if ((request->io->r.off - offset) < frame_size) {
                break;
            }

            payload = frame + _CC_HTTP2_FRAME_HEADER_SIZE_;

            printf("Received frame: type=0x%02x, length=%u, stream_id=%u\n", header.type, header.length, header.stream_id);

            switch (header.type) {
            case _CC_HTTP2_FRAME_TYPE_DATA_:
                if (header.length > 0) {
                    printf("Response data: %.*s\n", (int)header.length, (char *)payload);
                }
                break;
            case _CC_HTTP2_FRAME_TYPE_HEADERS_:
                printf("Received HEADERS frame: stream_id=%u, flags=0x%02x\n", header.stream_id, header.flags);
                break;
            case _CC_HTTP2_FRAME_TYPE_PRIORITY_:
                if (header.length != 5) {
                    _cc_logger_error("Invalid PRIORITY frame length: %u", header.length);
                    return false;
                }
                break;
            case _CC_HTTP2_FRAME_TYPE_RST_STREAM_:
                if (header.length != 4) {
                    _cc_logger_error("Invalid RST_STREAM frame length: %u", header.length);
                    return false;
                }
                printf("Received RST_STREAM frame: stream_id=%u\n", header.stream_id);
                break;
            case _CC_HTTP2_FRAME_TYPE_SETTINGS_: {
                int32_t i;
                if ((header.flags & _CC_HTTP2_FRAME_FLAG_ACK_) != 0) {
                    client->settings_acked = true;
                    break;
                }
                if (header.length % 6 != 0) {
                    _cc_logger_error("Invalid SETTINGS frame length: %u", header.length);
                    return false;
                }
                for (i = 0; i < (int32_t)header.length; i += 6) {
                    uint16_t settings_id = (uint16_t)(((uint16_t)payload[i] << 8) | (uint16_t)payload[i + 1]);
                    uint32_t value = ((uint32_t)payload[i + 2] << 24) | ((uint32_t)payload[i + 3] << 16) | ((uint32_t)payload[i + 4] << 8) | (uint32_t)payload[i + 5];
                    printf("Received SETTINGS frame: id=%u, value=%u\n", settings_id, value);
                    switch (settings_id) {
                    case _CC_HTTP2_SETTINGS_HEADER_TABLE_SIZE_:
                        client->header_table_size = value;
                        break;
                    case _CC_HTTP2_SETTINGS_ENABLE_PUSH_:
                        client->enable_push = (uint8_t)value;
                        break;
                    case _CC_HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS_:
                        client->max_concurrent_streams = value;
                        break;
                    case _CC_HTTP2_SETTINGS_INITIAL_WINDOW_SIZE_:
                        client->stream_window_size = value;
                        break;
                    case _CC_HTTP2_SETTINGS_MAX_FRAME_SIZE_:
                        client->max_frame_size = value;
                        break;
                    case _CC_HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE_:
                        client->max_header_list_size = value;
                        break;
                    default:
                        break;
                    }
                }
                if (_cc_http2_send_settings(e, request->io, true) < 0) {
                    return false;
                }
                if (_cc_io_buffer_flush(e, request->io) < 0) {
                    return false;
                }
                break;
            }
            case _CC_HTTP2_FRAME_TYPE_PUSH_PROMISE_:
                printf("Received PUSH_PROMISE frame: stream_id=%u\n", header.stream_id);
                break;
            case _CC_HTTP2_FRAME_TYPE_PING_:
                if ((header.flags & _CC_HTTP2_FRAME_FLAG_ACK_) == 0) {
                    if (_cc_http2_send_ping(e, request->io, _CC_HTTP2_FRAME_FLAG_ACK_, payload) < 0) {
                        return false;
                    }
                    if (_cc_io_buffer_flush(e, request->io) < 0) {
                        return false;
                    }
                }
                printf("Received PING frame\n");
                break;
            case _CC_HTTP2_FRAME_TYPE_GOAWAY_: {
                uint32_t last_stream_id;
                uint32_t error_code;
                if (header.length < 8) {
                    _cc_logger_error("Invalid GOAWAY frame length: %u", header.length);
                    return false;
                }
                last_stream_id = ((uint32_t)(payload[0] & 0x7f) << 24) | ((uint32_t)payload[1] << 16) | ((uint32_t)payload[2] << 8) | (uint32_t)payload[3];
                error_code = ((uint32_t)payload[4] << 24) | ((uint32_t)payload[5] << 16) | ((uint32_t)payload[6] << 8) | (uint32_t)payload[7];
                printf("Received GOAWAY frame: last_stream_id=%u, error_code=%u\n", last_stream_id, error_code);
                _cc_http2_close_connection(client, last_stream_id);
                return false;
            }
            case _CC_HTTP2_FRAME_TYPE_WINDOW_UPDATE_: {
                uint32_t window_size;
                if (header.length != 4) {
                    _cc_logger_error("Invalid WINDOW_UPDATE frame length: %u", header.length);
                    return false;
                }
                window_size = ((uint32_t)(payload[0] & 0x7f) << 24) | ((uint32_t)payload[1] << 16) | ((uint32_t)payload[2] << 8) | (uint32_t)payload[3];
                printf("Received WINDOW_UPDATE frame: stream_id=%u, increment=%u\n", header.stream_id, window_size);
                if (header.stream_id == 0) {
                    client->connection_window_size += window_size;
                } else if (header.stream_id == client->stream_id) {
                    client->stream_window_size += window_size;
                }
                break;
            }
            case _CC_HTTP2_FRAME_TYPE_CONTINUATION_:
                printf("Received CONTINUATION frame: stream_id=%u\n", header.stream_id);
                break;
            default:
                printf("Ignoring frame type: 0x%02x\n", header.type);
                break;
            }

            offset += frame_size;
        }

        if (offset > 0) {
            request->io->r.off -= offset;
            if (request->io->r.off > 0) {
                memmove(request->io->r.bytes, request->io->r.bytes + offset, request->io->r.off);
            }
        }
    }
    return true;
}

static bool_t url_request(const tchar_t *url, pvoid_t args) {
    _cc_http_request_t *request = _cc_http_request(url, args);

    if (request == NULL) {
        return false;
    }

    _cc_http2_init_client(_cc_http2_get_client());
    if (!url_request_connect(request)) {
        _cc_free_http_request(request);
        return false;
    }

    return true;
}

static bool_t url_request_connect(_cc_http_request_t *request) {
    struct sockaddr_in sa;
    _cc_socket_t fd;
    _cc_event_t *e;
    _cc_async_event_t *async = _cc_get_async_event();
    if (request == NULL) {
        return false;
    }

    /*Open then socket*/
    fd = _cc_socket(AF_INET, _CC_SOCK_NONBLOCK_ | _CC_SOCK_CLOEXEC_ | SOCK_STREAM, 0);
    if (fd == -1) {
        _cc_logger_error("socket fail:%s.", _cc_last_error(_cc_last_errno()));
        return false;
    }

    /* if we can't terminate nicely, at least allow the socket to be reused*/
    _cc_set_socket_reuseaddr(fd);

    e = _cc_alloc_event(async, _CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_);
    if (e == NULL) {
        return false;
    }

    e->fd = fd;
    e->callback = _http_request_callback;
    e->timeout = 1000;
    e->data = (uintptr_t)request;

    _cc_reset_http_request(request);

    if (request->url.scheme.ident == _CC_SCHEME_HTTPS_) {
        _cc_http_request_ssl(openSSL, request, e);

        // 设置ALPN协议为h2
        unsigned char alpn_protos[] = {2, 'h', '2'};
        _SSL_set_alpn_protos(request->io->ssl, alpn_protos, sizeof(alpn_protos));
    }

    _cc_inet_ipv4_addr(&sa, request->url.host, request->url.port);

    /* required to get parallel v4 + v6 working */
    if (sa.sin_family == AF_INET6) {
        e->flags |= _CC_EVENT_SOCKET_IPV6_;
#if defined(IPV6_V6ONLY)
        _cc_socket_ipv6only(e->fd);
#endif
    }

    if (async->connect(async, e, (_cc_sockaddr_t*)&sa, sizeof(struct sockaddr_in))) {
        return true;
    }

    _cc_free_event(async, e);
    return false;
}

int main(int argc, char *const argv[]) {
    openSSL = _SSL_init(_CC_SSL_DEFAULT_PROTOCOLS_);

    _cc_alloc_async_event(0, NULL);

    url_request("https://www.iconfont.cn/", NULL);

    while (getchar() != 'q') {
        _cc_sleep(100);
    }
    _cc_free_async_event();
    _SSL_quit(openSSL);
    return 0;
}
