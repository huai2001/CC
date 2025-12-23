#include <libcc/http_request.h>
#include <libcc/http2.h>

static _cc_OpenSSL_t *openSSL = nullptr;
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

} _cc_http2_client_t;

_cc_http2_client_t http_clients[128];

/**
 * 解析 HPACK 可变长度整数
 * @param data 输入数据指针（解析后指针会更新）
 * @param end 输入数据结束指针
 * @param prefix_bits 前缀位数（如 5、6、7 或 8）
 * @param value 输出参数，存储解析后的整数值
 * @return 成功返回 0，失败返回 -1
 */
int32_t hpack_decode_integer(
    const byte_t **data,
    const byte_t *end,
    uint8_t prefix_bits,
    uint32_t *value
) {
    const byte_t *ptr = *data;
    uint32_t shift = 0;
    // 提取前缀掩码（如 prefix_bits=5 时，掩码为 0x1F）
    uint8_t mask = (1 << prefix_bits) - 1;
    uint32_t result = (*ptr++) & mask;

    // 检查前缀位数是否合法（1-8）
    if (prefix_bits < 1 || prefix_bits > 8) {
        _cc_logger_error(_T("Invalid prefix bits: %u"), prefix_bits);
        return -1;
    }

    // 如果前缀位已包含完整整数，直接返回
    if (result < mask) {
        *value = result;
        *data = ptr;
        return 1;
    }

    // 解析多字节整数
    while (ptr < end) {
        byte_t byte = *ptr++;
        if (shift > 28) { // 防止溢出（32 位整数最多左移 28 位）
            _cc_logger_error(_T("Integer overflow"));
            return -1;
        }

        result += (byte & 0x7F) << shift;
        shift += 7;

        if ((byte & 0x80) == 0) {
            *value = result;
            *data = ptr;
            return 0;
        }
    }

    _cc_logger_error(_T("Incomplete integer encoding"));
    return -1;
}

/**
 * 解析单个 HPACK 字段
 * @param decoder HPACK 解码器上下文
 * @param data 输入数据指针（解析后指针会更新）
 * @param end 输入数据结束指针
 * @param header 输出参数，存储解析后的字段
 * @return 成功返回 0，失败返回 -1
 */
/*
int hpack_decode_field(
    hpack_decoder_t *decoder,
    const byte_t **data,
    const byte_t *end,
    _cc_http_header_t *header
) {
    const byte_t *name, *value;
    const byte_t *ptr = *data;
    // 解析字段前缀（前 2-4 位）
    byte_t prefix = (*ptr) & 0xE0; // 取前 3 位
    uint32_t index;
    size_t name_length, value_length;

    if (ptr >= end) {
        _cc_logger_error(_T("No data to decode"));
        return -1;
    }

    // 根据前缀判断字段类型
    if ((*ptr & 0x80) != 0) {
        // 索引字段（Indexed Header Field）
        if (hpack_decode_integer(&data, end, 7, &index) < 0) {
            _cc_logger_error(_T("Failed to decode indexed field index"));
            return -1;
        }
        if (!hpack_get_indexed_field(decoder, index, &name, &name_length, &value, &value_length)) {
            _cc_logger_error(_T("Invalid indexed field index: %u"), index);
            return -1;
        }
    } else if ((*data & 0xC0) == 0x40) {
        // 字面量字段（Literal Header Field with Indexed Name）
        if (hpack_decode_integer(&data, end, 6, &index) < 0) {
            _cc_logger_error(_T("Failed to decode literal field index"));
            return -1;
        }
        if (!hpack_get_indexed_name(decoder, index, &name, &name_length)) {
            _cc_logger_error(_T("Invalid literal field name index: %u"), index);
            return -1;
        }
        if (hpack_decode_string(&data, end, &value, &value_length) < 0) {
            _cc_logger_error(_T("Failed to decode literal field value"));
            return -1;
        }
        // 可选：将字段添加到动态表
        if ((*data & 0x20) != 0) {
            hpack_add_dynamic_entry(decoder, name, name_length, value, value_length);
        }
    } else if ((*data & 0xF0) == 0x00) {
        // 字面量字段（Literal Header Field without Indexed Name）
        if (hpack_decode_string(&data, end, &name, &name_length) < 0) {
            _cc_logger_error(_T("Failed to decode literal field name"));
            return -1;
        }
        if (hpack_decode_string(&data, end, &value, &value_length) < 0) {
            _cc_logger_error(_T("Failed to decode literal field value"));
            return -1;
        }
        // 可选：将字段添加到动态表
        if ((*data & 0x20) != 0) {
            hpack_add_dynamic_entry(decoder, name, name_len, value, value_len);
        }
    } else if ((*data & 0xE0) == 0x20) {
        // 动态表大小更新（Dynamic Table Size Update）
        uint32_t max_size;
        if (hpack_decode_integer(&data, end, 5, &max_size) < 0) {
            _cc_logger_error(_T("Failed to decode dynamic table size update"));
            return -1;
        }
        hpack_set_dynamic_table_size(decoder, max_size);
        *ptr = data;
        return 0; // 无字段输出
    } else {
        _cc_logger_error(_T("Invalid HPACK field prefix: 0x%02X"), *data);
        return -1;
    }

    // 存储解析后的字段
    header->name = (byte_t *)malloc(name_len + 1);
    header->value = (byte_t *)malloc(value_len + 1);
    if (header->name == NULL || header->value == NULL) {
        _cc_logger_error(_T("Failed to allocate memory for header field"));
        free(header->name);
        free(header->value);
        return -1;
    }
    memcpy(header->name, name, name_len);
    memcpy(header->value, value, value_len);
    header->name[name_len] = '\0';
    header->value[value_len] = '\0';

    *ptr = data;
    return 0;
}
*/
static int32_t _cc_http2_send_ping(uint32_t stream_id, byte_t *buffer, size_t size, uint64_t opaque_data) {
    // PING frame size (9 bytes header + 8 bytes payload)
    if (size < 17) {
        return 0;
    }
    // Construct PING frame header
    buffer[0] = 0x00; // Length (24 bits, high byte)
    buffer[1] = 0x00; // Length (middle byte)
    buffer[2] = 0x08; // Length (low byte, 8 bytes payload)
    buffer[3] = _CC_HTTP2_FRAME_TYPE_PING_; // Frame type (0x06)
    buffer[4] = 0x01; // Flags (ACK bit set)
    buffer[5] = (stream_id >> 24) & 0xFF; // Stream ID (high byte)
    buffer[6] = (stream_id >> 16) & 0xFF; // Stream ID (middle byte)
    buffer[7] = (stream_id >> 8) & 0xFF; // Stream ID (low byte)
    buffer[8] = stream_id & 0xFF; // Stream ID (lowest byte)
    
    // Fill payload (8 bytes, can be any opaque data)
    memcpy(buffer + 9, &opaque_data, sizeof(uint64_t));

    // Total bytes sent (9 bytes header + 8 bytes payload)
    return 17;
}

static void _cc_http2_close_connection(_cc_http2_client_t *client, uint32_t stream_id) {

}


static bool_t url_request_header(_cc_http_request_t *request, _cc_event_t *e) {
    _cc_url_t *u = &request->url;
    _cc_buf_t *buf = &request->buffer;
    _cc_io_buffer_t *io = request->io;

    _cc_buf_cleanup(buf);

    /* send client connection preface */
    const char preface[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
    char headers[256];

    size_t length = snprintf(headers, sizeof(headers),
             ":method: GET\r\n"
             ":path: %s\r\n"
             ":scheme: https\r\n"
             ":authority: libcc.com\r\n"
             "user-agent: c-http2-client\r\n",  u->request);

    io->w.off = sizeof(preface) - 1;
    memcpy(io->w.bytes, preface, io->w.off);

    io->w.off += _cc_http2_frame_header(io->w.bytes + io->w.off, _CC_HTTP2_FRAME_TYPE_HEADERS_, _CC_HTTP2_FRAME_FLAG_END_HEADERS_, 1, length);

    memcpy(io->w.bytes + io->w.off, headers, length);
    io->w.off += length;

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
    //wait SSL handshake complete
    return true;
}

static bool_t _http_request_callback(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    _cc_http_request_t *request = (_cc_http_request_t *)e->data;

    if (_CC_ISSET_BIT(_CC_EVENT_CLOSED_, which)) {
        //printf("disconnect\n");
        _cc_logger_warin("_cc_http_request_ _CC_EVENT_CLOSED_ %d",e->ident);
        _cc_free_http_request(request);
        return false;
    } else if (_CC_ISSET_BIT(_CC_EVENT_TIMEOUT_, which)) {
        if (request->url.scheme.ident == _CC_SCHEME_HTTPS_) {
            if (request->io && request->io->ssl && !request->io->ssl->is_handshaked) {
                return _handshaking(e, request);
            }
        }
        if (request->response && request->response->keep_alive) {
            return url_request_header(request, e);
        }
        return false;
    } else if (_CC_ISSET_BIT(_CC_EVENT_CONNECT_, which)) {
        _cc_logger_info(_T("url_request connected,%s"), request->url.host);
        if (request->url.scheme.ident == _CC_SCHEME_HTTPS_) {
            return _handshaking(e, request);
        }
        _CC_SET_BIT(_CC_EVENT_READABLE_, e->flags);
        return url_request_header(request, e);
    }

    if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_, which)) {
        //printf("send buffer\n");
        return _cc_io_buffer_flush(e,request->io) >= 0;
    }

    if (_CC_ISSET_BIT(_CC_EVENT_READABLE_, which)) {
        int32_t off = _cc_io_buffer_read(e, request->io);
        if (off < 0) {
            return false;
        } else if (off == 0) {
            return true;
        }

        off = 0;
        // 解析帧头
        while (request->io->r.off >= _CC_HTTP2_FRAME_HEADER_SIZE_) {
            int32_t r;
            byte_t *buffer = request->io->r.bytes + off;
            _cc_http2_frame_header_t header;
            _cc_http2_client_t *client = (_cc_http2_client_t *)&http_clients[0];

            header.length = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
            header.type = buffer[3];
            header.flags = buffer[4];
            header.stream_id = (buffer[5] << 24) | (buffer[6] << 16) | (buffer[7] << 8) | buffer[8];
            
            printf("Received frame: type=0x%02x, length=%u, stream_id=%u\n",
                   header.type, header.length, header.stream_id);

            r = (_CC_HTTP2_FRAME_HEADER_SIZE_ + header.length);
            buffer += _CC_HTTP2_FRAME_HEADER_SIZE_;
            
            switch (header.type) {
            case _CC_HTTP2_FRAME_TYPE_DATA_:
                if (header.length > 0) {
                    printf("Response data: %.*s\n", header.length, buffer);
                }
                break;
            case _CC_HTTP2_FRAME_TYPE_HEADERS_:
                if (header.length > 0) {
                    // _cc_http2_headers_t headers;
                    // if (!_cc_http2_parse_headers(buffer, header.length, &headers)) {
                    //     _cc_logger_error(_T("Failed to parse HEADERS frame"));
                    //     return false;
                    // }
                    //printf("Received HEADERS frame: stream_id=%u, field_count=%u\n", header.stream_id, headers.count);
                    //_cc_http2_free_headers(&headers);
                }
                break;
            case _CC_HTTP2_FRAME_TYPE_PRIORITY_:
                if (header.length == 5) {
                    uint32_t dependent_stream_id = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
                    uint8_t weight = buffer[4];
                    printf("Received PRIORITY frame: stream_id=%u, dependent_stream_id=%u, weight=%u\n",
                           header.stream_id, dependent_stream_id, weight);
                    //_cc_http2_update_stream_priority(header.stream_id, dependent_stream_id, weight);
                } else {
                    _cc_logger_error(_T("Invalid PRIORITY frame length: %u"), header.length);
                    return false;
                }
                break;
            case _CC_HTTP2_FRAME_TYPE_RST_STREAM_:
                if (header.length == 4) {
                    uint32_t error_code = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
                    printf("Received RST_STREAM frame: stream_id=%u, error_code=%u\n", header.stream_id, error_code);
                    //_cc_http2_close_stream(header.stream_id, error_code);
                } else {
                    _cc_logger_error(_T("Invalid RST_STREAM frame length: %u"), header.length);
                    return false;
                }
                break;
            case _CC_HTTP2_FRAME_TYPE_SETTINGS_: {
                int32_t i;
                if (header.length % 6 != 0) {
                    _cc_logger_error(_T("Invalid SETTINGS frame length: %u"), header.length);
                    return false;
                }
                for (i = 0; i < header.length; i += 6) {
                    uint16_t settings_id = (uint16_t)(buffer[i] << 8) | (uint16_t)(buffer[i + 1]);
                    uint32_t value = (uint32_t)(buffer[i + 2] << 24) | 
                                     (uint32_t)(buffer[i + 3] << 16) | 
                                     (uint32_t)(buffer[i + 4] << 8) | 
                                     (uint32_t)(buffer[i + 5]);
                    printf("Received SETTINGS frame: id=%u, value=%u\n", settings_id, value);
                    //_cc_http2_update_settings(settings_id, value);
                    switch (settings_id) {
                    case _CC_HTTP2_SETTINGS_HEADER_TABLE_SIZE_:
                        client->header_table_size = value;
                        break;
                    case _CC_HTTP2_SETTINGS_ENABLE_PUSH_:
                        client->enable_push = value;
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
                    default:
                        printf("  Unknown setting id: %u\n", settings_id);
                        break;
                    }
                }
            }
                break;
            case _CC_HTTP2_FRAME_TYPE_PUSH_PROMISE_:
                printf("Received PUSH_PROMISE frame: stream_id=%u\n", header.stream_id);
                //_cc_http2_handle_push_promise(header.stream_id, buffer, header.length);
                break;
            case _CC_HTTP2_FRAME_TYPE_PING_: {
                int32_t ping = _cc_http2_send_ping(header.stream_id, request->io->w.bytes, request->io->w.limit - request->io->w.off, 0x1122334455667788);
                if (ping > 0) {
                    request->io->w.off += ping;
                    _cc_io_buffer_flush(e, request->io);
                }
                printf("Received PING frame: stream_id=%u\n", header.stream_id);
            }
                break;
            case _CC_HTTP2_FRAME_TYPE_GOAWAY_: {
                uint32_t last_stream_id = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
                uint32_t error_code = (buffer[4] << 24) | (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];
                printf("Received GOAWAY frame: last_stream_id=%u, error_code=%u\n", last_stream_id, error_code);
                _cc_http2_close_connection(client, last_stream_id);
            }
                break;
            case _CC_HTTP2_FRAME_TYPE_WINDOW_UPDATE_: {
                uint32_t window_size = (uint32_t)((buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3]);
                printf("Received WINDOW_UPDATE frame: stream_id=%u, increment=%u\n", header.stream_id, (uint32_t)window_size);
                client->connection_window_size += window_size;
            }
                break;
            case _CC_HTTP2_FRAME_TYPE_CONTINUATION_:
                printf("Received CONTINUATION frame: stream_id=%u\n", header.stream_id);
                //_cc_http2_merge_headers(header.stream_id, buffer, header.length);
                break;
            default:
                printf("Unknown frame type: 0x%02x\n", header.type);
                return false;
            }
            request->io->r.off -= r;
            off += r;
        }

        if (request->io->r.off > 0 && off > 0) {
            memmove(request->io->r.bytes, request->io->r.bytes + off, request->io->r.off);
        }
    }
    return true;
}

static bool_t url_request(const tchar_t *url, pvoid_t args) {
    _cc_http_request_t *request = _cc_url_request(url, args);

    if (!url_request_connect(request)) {
        _cc_free_http_request(request);
        return false;
    }
    memset(&http_clients, 0, sizeof(http_clients));
    return true;
}

static bool_t url_request_connect(_cc_http_request_t *request) {
    struct sockaddr_in sa;
    _cc_socket_t fd;
    _cc_event_t *e;
    _cc_async_event_t *async = _cc_get_async_event();
    if (request == nullptr) {
        return false;
    }

    /*Open then socket*/
    fd = _cc_socket(AF_INET, _CC_SOCK_NONBLOCK_ | _CC_SOCK_CLOEXEC_ | SOCK_STREAM, 0);
    if (fd == -1) {
        _cc_logger_error(_T("socket fail:%s."), _cc_last_error(_cc_last_errno()));
        return false;
    }

    /* if we can't terminate nicely, at least allow the socket to be reused*/
    _cc_set_socket_reuseaddr(fd);

    e = _cc_alloc_event(async, _CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_);
    if (e == nullptr) {
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

    _cc_alloc_async_event(0, nullptr);

    url_request("https://ws.libcc.cn", nullptr);

    while (getchar() != 'q') {
        _cc_sleep(100);
    }
    _cc_free_async_event();
    _SSL_quit(openSSL);
    return 0;
}
