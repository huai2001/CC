# TCP 服务端与客户端
***
介绍如何使用 libcc 的 TCP API：创建监听事件、accept 新连接、处理读写、以及发起主动连接。

## TCP 服务端
服务端示例基于 [`tests/test_event.c`](https://github.com/libcc/libcc/blob/2.0/tests/test_event.c)，实现一个简单的回显服务器。

### 核心概念
-   **监听事件**：使用 `_CC_EVENT_ACCEPT_` 标志创建监听 socket。
-   **连接事件**：新连接通过 `async->accept()` 获取，创建新事件处理连接。
-   **读写事件**：使用 `_CC_EVENT_READABLE_` 和 `_CC_EVENT_WRITABLE_` 处理数据传输。
-   **非阻塞 I/O**：所有 socket 设为非阻塞模式，避免阻塞事件循环。

### 示例代码（服务端）

```c
#include <libcc.h>
#include <stdio.h>

static bool_t echo_handler(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    if (which & _CC_EVENT_ACCEPT_) {
        /* 新连接建立，accept 并创建新事件 */
        _cc_socket_t fd;
        _cc_event_t *new_event;
        struct sockaddr_in remote_addr = {0};
        _cc_socklen_t addr_len = sizeof(struct sockaddr_in);

        fd = async->accept(async, e, (_cc_sockaddr_t*)&remote_addr, &addr_len);
        if (fd == _CC_INVALID_SOCKET_) {
            _cc_logger_error(_T("accept failed: %s"), _cc_last_error(_cc_last_errno()));
            return true;
        }

        /* 设置非阻塞模式 */
        _cc_set_socket_nonblock(fd, true);

        /* 创建新事件处理连接 */
        new_event = _cc_event_alloc(async, _CC_EVENT_TIMEOUT_ | _CC_EVENT_READABLE_);
        if (!new_event) {
            _cc_close_socket(fd);
            return true;
        }

        new_event->fd = fd;
        new_event->callback = echo_handler;
        new_event->timeout = 60000; /* 60秒超时 */

        if (!async->attach(async, new_event)) {
            _cc_logger_error(_T("attach event failed"));
            _cc_free_event(async, new_event);
            return true;
        }

        _cc_logger_info(_T("New connection from %s:%d"), 
                       inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));
        return true;
    }

    if (which & _CC_EVENT_READABLE_) {
        /* 读取数据并回显 */
        char buf[1024];
        int32_t r = _cc_recv(e->fd, (byte_t*)buf, sizeof(buf));
        if (r > 0) {
            /* 发送回显数据 */
            _cc_send(e->fd, (byte_t*)buf, r);
            return true;
        } else if (r == 0) {
            /* 对端关闭连接 */
            _cc_logger_info(_T("Connection closed by peer"));
            return false;
        } else {
            /* 读取错误 */
            _cc_logger_error(_T("recv failed: %s"), _cc_last_error(_cc_last_errno()));
            return false;
        }
    }

    if (which & _CC_EVENT_CLOSED_) {
        /* 连接关闭 */
        _cc_logger_info(_T("Connection closed"));
        return false;
    }

    return true;
}

int main(void) {
    struct sockaddr_in sa;
    _cc_async_event_t async;
    _cc_event_t *e;

    /* 初始化 socket 库 */
    _cc_install_socket();

    /* 注册事件轮询器 */
    if (!_cc_register_poller(&async)) {
        fprintf(stderr, "register poller failed\n");
        return EXIT_FAILURE;
    }

    /* 创建监听事件 */
    e = _cc_event_alloc(&async, _CC_EVENT_ACCEPT_);
    if (!e) {
        fprintf(stderr, "alloc event failed\n");
        async.free(&async);
        return EXIT_FAILURE;
    }

    e->callback = echo_handler;
    e->timeout = 60000;

    /* 绑定地址和端口 */
    _cc_inet_ipv4_addr(&sa, NULL, 8081);
    if (!_cc_tcp_listen(&async, e, (_cc_sockaddr_t*)&sa, sizeof(struct sockaddr_in))) {
        fprintf(stderr, "listen failed\n");
        _cc_free_event(&async, e);
        async.free(&async);
        return EXIT_FAILURE;
    }

    _cc_logger_info(_T("Server listening on port 8081"));

    /* 事件循环 */
    while (async.running) {
        async.wait(&async, 100);
    }

    async.free(&async);
    return EXIT_SUCCESS;
}
```

## TCP 客户端
客户端示例基于 [`tests/test_tcp_client.c`](https://github.com/libcc/libcc/blob/2.0/tests/test_tcp_client.c)，实现一个简单的客户端连接并发送数据。

### 示例代码（客户端）

```c
#include <libcc.h>
#include <stdio.h>

static bool_t client_handler(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    if (which & _CC_EVENT_CONNECT_) {
        /* 连接成功 */
        _cc_logger_info(_T("Connected to server"));
        /* 发送数据 */
        const char *msg = "Hello, Server!";
        _cc_send(e->fd, (byte_t*)msg, strlen(msg));
        return true;
    }

    if (which & _CC_EVENT_READABLE_) {
        /* 读取服务器响应 */
        char buf[1024];
        int32_t r = _cc_recv(e->fd, (byte_t*)buf, sizeof(buf));
        if (r > 0) {
            buf[r] = '\0';
            _cc_logger_info(_T("Received: %s"), buf);
            /* 处理完响应后关闭连接 */
            return false;
        } else if (r == 0) {
            _cc_logger_info(_T("Server closed connection"));
            return false;
        } else {
            _cc_logger_error(_T("recv failed: %s"), _cc_last_error(_cc_last_errno()));
            return false;
        }
    }

    if (which & _CC_EVENT_CLOSED_) {
        /* 连接关闭 */
        _cc_logger_info(_T("Connection closed"));
        return false;
    }

    return true;
}

int main(void) {
    struct sockaddr_in sa;
    _cc_async_event_t async;
    _cc_event_t *e;

    /* 初始化 socket 库 */
    _cc_install_socket();

    /* 注册事件轮询器 */
    if (!_cc_register_poller(&async)) {
        fprintf(stderr, "register poller failed\n");
        return EXIT_FAILURE;
    }

    /* 创建连接事件 */
    e = _cc_event_alloc(&async, _CC_EVENT_CONNECT_ | _CC_EVENT_TIMEOUT_ | _CC_EVENT_READABLE_);
    if (!e) {
        fprintf(stderr, "alloc event failed\n");
        async.free(&async);
        return EXIT_FAILURE;
    }

    e->callback = client_handler;
    e->timeout = 60000; /* 60秒超时 */

    /* 设置目标服务器地址 */
    _cc_inet_ipv4_addr(&sa, "127.0.0.1", 8081);
    if (!_cc_tcp_connect(&async, e, (_cc_sockaddr_t*)&sa, sizeof(struct sockaddr_in))) {
        fprintf(stderr, "connect failed\n");
        _cc_free_event(&async, e);
        async.free(&async);
        return EXIT_FAILURE;
    }

    /* 事件循环 */
    while (async.running) {
        async.wait(&async, 100);
    }

    async.free(&async);
    return EXIT_SUCCESS;
}
```

## 详细说明
-   **初始化**：调用 `_cc_install_socket()` 初始化 socket 库。
-   **监听**：`_cc_tcp_listen()` 创建监听 socket 并绑定到指定端口。
-   **连接**：`_cc_tcp_connect()` 发起异步连接，连接成功后触发 `_CC_EVENT_CONNECT_`。
-   **数据传输**：使用 `_cc_send()` 和 `_cc_recv()` 进行数据收发。
-   **非阻塞**：所有 socket 必须设为非阻塞模式。
-   **错误处理**：检查所有 API 返回值，使用 `_cc_last_error()` 获取错误信息。

## 注意事项
-   **资源管理**：连接关闭时事件会自动释放，无需手动调用 `_cc_free_event()`。
-   **缓冲区管理**：`_cc_recv()` 可能返回部分数据，需处理不完整消息。
-   **并发连接**：服务端可同时处理多个客户端连接。
-   **超时处理**：设置合理的超时时间，避免连接挂起。
-   **平台差异**：确保编译时包含网络相关模块。
-   **调试**：使用 `_cc_logger_*` 记录连接和数据传输事件。
-   **性能优化**：对于高并发场景，考虑使用线程池或异步 I/O。
-   **安全**：验证输入数据，避免缓冲区溢出攻击。
