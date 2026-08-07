# WebSocket Server 教程

***

## 场景介绍

WebSocket 是一种网络通信协议，支持客户端与服务器之间建立持久连接，并实现全双工通信。与传统的 HTTP 协议不同，WebSocket 允许双方同时主动发送数据，避免了 HTTP 的单向请求-响应模式。

### 核心优势
- **持久连接**：无需频繁建立和断开连接，减少延迟。
- **双向通信**：客户端和服务器均可主动发送数据。
- **低开销**：适用于实时性要求高的场景，如在线聊天、实时游戏等。

### 适用场景
- **实时通信**：如聊天应用、在线协作工具。
- **高频数据推送**：如股票行情、实时监控。
- **游戏开发**：支持低延迟的多人互动。

## WebSocket 帧结构

WebSocket 帧是客户端与服务器通信的基本数据单元，其结构按字节划分，各部分功能如下：

### 1. 字节 1：控制帧的基础标识
- **FIN（第 1 位）**：标识当前帧是否为完整帧（`1`：完整帧；`0`：需后续帧拼接）。
- **Rsv1/Rsv2/Rsv3（第 2-4 位）**：保留位，必须设为 `0`。
- **Opcode（第 5-8 位）**：定义帧类型，如：
  - `0`：文本帧
  - `1`：二进制帧
  - `8`：关闭帧

### 2. 字节 2：数据长度的标识
- **MASK（第 1 位）**：标识是否启用掩码（`1`：启用；`0`：禁用）。
- **Payload length（第 2-8 位）**：初始负载长度标识，范围 0-125。若长度超过 125，需扩展后续字节。

### 3. 字节 3-10：扩展长度存储
- **当 `Payload length=126`**：字节 3-4 存储实际负载长度（范围 0-65535）。
- **当 `Payload length=127`**：字节 3-10 存储实际负载长度（范围 0-2^63-1）。

### 4. 字节 11-14：掩码密钥
- **当 `MASK=1`**：字节 11-14 存储 4 字节掩码密钥，用于数据加密/解密。

### 5. 字节 15 及后续：数据负载
- **`MASK=0`**：直接存储原始数据。
- **`MASK=1`**：存储加密后的数据，需用掩码密钥解码。

示例示意图：
![WebSocket 帧结构示意图](https://libcc.cn/assets/images/ws-package.png)

## WebSocket 服务实现

WebSocket 服务端的核心功能包括连接管理、心跳包处理、HTTP 握手升级以及网络事件处理。以下是关键部分的实现细节。

### 1. WebSocket 结构体

```c
typedef struct _ws {
    uint8_t state;                          // HTTP 握手状态
    uint64_t length;                        // WebSocket 大数据，包大于接收缓存区时使用
    _cc_io_buffer_t *io;                    // 网络读写缓存区
    _cc_ws_header_t header;                 // WebSocket 头部信息
    _cc_http_request_header_t *request;     // HTTP 请求信息
} _cc_ws_t;
```

### 2. 连接管理

- **分配连接资源**：初始化 WebSocket 结构体，分配网络 IO 缓存区。
- **释放连接资源**：清理 HTTP 请求头和网络缓存区。

```c
/* 分配一个 WebSocket 结构体 */
_CC_API_PRIVATE(_cc_ws_t*) _ws_alloc(_cc_socket_t fd) {
    _cc_SSL_t *ssl = NULL;
    _cc_ws_t *ws = (_cc_ws_t*)_cc_malloc(sizeof(_cc_ws_t));
    ws->state = _CC_HTTP_STATUS_HEADER_;
    ws->request = NULL;
    ws->header.state = WS_DATA_OK;

    // 如果支持 SSL 的 WSS 协议，创建 SSL_accept
    // ssl = _SSL_accept(openSSL, fd);
    // 分配网络 IO 读写缓存区
    ws->io = _cc_alloc_io_buffer(_CC_IO_BUFFER_SIZE_, ssl);
    
    return ws;
}

/* 释放 WebSocket 资源 */
_CC_API_PRIVATE(void) _ws_free(_cc_ws_t *ws) {
    if (ws->request) {
        // 释放 HTTP 头部数据信息
        _cc_http_free_request_header(&ws->request);
    }

    if (ws->io) {
        // 释放读写缓存区
        _cc_free_io_buffer(ws->io);
    }

    _cc_free(ws);
}
```

### 3. 心跳包处理

心跳包用于维持连接活跃性，防止超时断开。

```c
/* WebSocket 心跳包 */
_CC_API_PRIVATE(bool_t) _ws_heartbeat(_cc_event_t *e, byte_t oc) {
    _cc_ws_t *ws = (_cc_ws_t*)e->data;
    _cc_io_buffer_t *io = (_cc_io_buffer_t*)ws->io;
    byte_t buf[2];
    buf[0] = 0x80 | oc;
    buf[1] = 0;
    return (_cc_io_buffer_send(e, io, buf, 2) > 0);
}
```

### 4. HTTP 握手升级

将 HTTP 连接升级为 WebSocket 连接，包括握手响应和协议验证。

```c
/* WebSocket 响应握手 */
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
    // _cc_logger_debug("Sec-WebSocket-Key: %s", ws_key->value);

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
    return _cc_io_buffer_flush(e, io);
}

/* HTTP 升级为 WebSocket 握手 */
_CC_API_PRIVATE(bool_t) _ws_http_handler(_cc_event_t *e, _cc_ws_t *ws) {
    _cc_io_buffer_t *io = (_cc_io_buffer_t*)ws->io;
    if (ws->state == _CC_HTTP_STATUS_HEADER_) {
        const _cc_http_header_t *connection, *upgrade;
        // 解析 HTTP 头部信息
        ws->state = _cc_http_header_parser((_cc_http_header_fn_t)_cc_http_alloc_request_header, (pvoid_t *)&ws->request, io->r.bytes, &io->r.off);
        if (ws->state != _CC_HTTP_STATUS_PAYLOAD_) {
            return ws->state == _CC_HTTP_STATUS_HEADER_;
        } else if (ws->request == NULL) {
            return false;
        }
        
        connection = _cc_http_header_find(&ws->request->headers, _T("Connection"));
        upgrade = _cc_http_header_find(&ws->request->headers, _T("Upgrade"));
        if (connection == NULL || upgrade == NULL) {
            // 协议非 WebSocket 升级协议，直接断开连接
            return false;
        } else if (_tcsicmp("Upgrade", connection->value) != 0 || _tcsicmp("websocket", upgrade->value) != 0) {
            // 协议非 WebSocket 升级协议不支持，直接断开连接
            return false;
        }
        /* WebSocket 握手过程中没有类似 HTTP POST 的请求头与请求体结构，直接接收完成 */
        ws->state = _CC_HTTP_STATUS_ESTABLISHED_;
    } 
    // 直接丢弃多余数据
    if (io->r.off != 0) {
        io->r.off = 0;
    }
    if (ws->state == _CC_HTTP_STATUS_ESTABLISHED_) {
        return _ws_response_header(e, ws);
    }
    return true;
}
```

### 5. 网络事件处理

处理网络事件（如连接建立、超时、读写事件等）。

```c
_CC_API_PRIVATE(bool_t) _ws_handler(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
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
        // 设置非阻塞
        _cc_set_socket_nonblock(fd, 1);

        event = _cc_alloc_event(async, _CC_EVENT_TIMEOUT_|_CC_EVENT_READABLE_);
        if (event == NULL) {
            _cc_close_socket(fd);
            _ws_free(ws);
            return true;
        }

        event->fd = fd;
        event->callback = e->callback;
        event->timeout = e->timeout;
        event->data = (uintptr_t)_ws_alloc(fd);

        if (async->attach(async, event) == false) {
            _cc_logger_debug(_T("thread %d add socket (%d) event fial."), _cc_get_thread_id(NULL), fd);
            _cc_free_event(async, event);
            _ws_free(ws);
            return true;
        }
        return true;
    } else if (which & _CC_EVENT_TIMEOUT_) {
        _cc_logger_debug(_T("TCP timeout %d"), e->fd);
        // 发送心跳包
        if (_ws_heartbeat(e, WS_OP_PONG)) {
            return true;
        }
        return false;
    } else if (which & _CC_EVENT_CLOSED_) {
        _cc_logger_debug(_T("%d disconnect to client."), e->fd);
        // 释放资源
        if (e->data) {
            _ws_free((_cc_ws_t*)e->data);
            e->data = 0;
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
            } else if (!_ws_http_handler(e, ws)) {
                return false;
            }
        } while(true);
    }
    /* 写缓存中还有数据继续推送 */
    if (which & _CC_EVENT_WRITABLE_) {
        if (ws->io) {
            return _cc_io_buffer_flush(e, ws->io);
        } else {
            _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        }
    }

    return true;
}
```
### 6.主函数
```c
int main(int argc, char *const argv[]) {
    struct sockaddr_in sa;
    _cc_async_event_t async;
    _cc_event_t *e;

    // 监听端口
    uint16_t port = 5500;

    // 初始化网络库
    _cc_install_socket();

    // 注册事件轮询器
    if (_cc_register_poller(&async) == false) {
        return 1;
    }

    // 创建事件对象
    e = _cc_alloc_event(&async, _CC_EVENT_ACCEPT_);
    if (e == NULL) {
        async.free(&async);
        return -1;
    }

    // 设置事件回调函数和超时时间
    e->callback = _ws_handler;
    e->timeout = 60000;

    // 绑定监听地址和端口
    _cc_inet_ipv4_addr(&sa, NULL, port);
    _cc_tcp_listen(&async, e, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in));

    _cc_logger_debug(_T("listen port: %d"), port);

    // 事件循环 - 等待网络事件
    while (1) {
        async.wait(&async, 100);
    }

    // 释放资源
    async.free(&async);
    return 0;
}
```