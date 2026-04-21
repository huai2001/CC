# HTTP 1.x Server 精简版教程

***

## 概述

本教程介绍如何使用 libcc 内置的 HTTP 解析功能，示例代码基于项目中的 [`tests/test_http.c`](https://github.com/libcc/libcc/blob/2.0/tests/test_http.c)，此处为精简版实现。

## HTTP 协议版本特性

### HTTP/1.0 核心特性
- **请求头/响应头**：首次引入头信息概念，采用 `Key-Value` 形式组织。
- **无状态连接**：默认每次请求后断开连接，需重新建立。

### HTTP/1.1 主要改进
- **持久连接**：默认保持连接复用，显著降低延迟。
- **Host 字段**：支持单 IP 多域名（虚拟主机）。
- **缓存优化**：引入更精细的缓存控制机制。
- **流水线支持**：允许连续发送多个请求无需等待响应。

![HTTP 头部结构示意图](https://libcc.cn/assets/images/http-header.png)

## 简易 HTTP 服务实现, test_http.c 支持SSL

## 核心数据结构

```c
#include <libcc/dirent.h>
#include <libcc/event.h>

/* HTTP 连接上下文结构体 */
typedef struct _http {
    uint8_t state;                          // 当前HTTP解析状态
    size_t payload;                         // POST请求体大小
    _cc_io_buffer_t *io;                    // 网络读写缓冲区
    _cc_http_request_header_t *request;     // 解析后的HTTP头部
    _cc_buf_t buffer;                       // 存储POST请求体数据
    _cc_file_t *file;                       // 静态资源文件句柄
} _http_t;
```

## 全局配置与回调声明

```c
/* 静态资源根目录 */
static const tchar_t *root = _T("/opt/homebrew/var/www/");

/* 事件回调函数声明 */
// 连接建立
static bool_t _handle_accept(_cc_async_event_t *async, _cc_event_t *e);
// 连接关闭
static bool_t _handle_close(_cc_async_event_t *async, _cc_event_t *e);
// 数据读取
static bool_t _handle_read(_cc_async_event_t *async, _cc_event_t *e);
// 数据发送  
static bool_t _handle_write(_cc_async_event_t *async, _cc_event_t *e);
// 超时处理
static bool_t _handle_timeout(_cc_async_event_t *async, _cc_event_t *e);
```
## 网络事件分发器

```c
/**
 * 事件分发函数
 * @param async 异步事件管理器
 * @param e 当前事件对象
 * @param which 触发的事件类型掩码
 * @return 是否继续处理事件
 */
static bool_t _handle_event(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    /* 新连接建立 */
    if (which & _CC_EVENT_ACCEPT_) {
        return _handle_accept(async, e);
    } 
    /* 连接关闭 */
    else if (which & _CC_EVENT_CLOSED_) {
        return _handle_close(async, e);
    }
    /* 可读事件 */
    if (which & _CC_EVENT_READABLE_) {
        if (!_handle_read(async, e)) {
            return false;
        }
    }
    /* 可写事件 */
    if (which & _CC_EVENT_WRITABLE_) {
        if (!_handle_write(async, e)) {
            return false;
        }
    }
    /* 超时事件 */
    if (which & _CC_EVENT_TIMEOUT_) {
        if (!_handle_timeout(async, e)) {
            return false;
        }
    }
    return true;
}
```
## 连接建立处理

```c
/**
 * 新连接建立回调
 * @param async 异步事件管理器
 * @param e 监听事件对象
 * @return 是否成功处理
 */
static bool_t _handle_accept(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_socket_t fd;
    _cc_event_t *event;
    _http_t *http;
    struct sockaddr_in remote_addr = {0};
    _cc_socklen_t remote_addr_len = sizeof(struct sockaddr_in);
    _cc_async_event_t *async2 = _cc_get_async_event();

    // 接受新连接
    fd = async->accept(async, e, (_cc_sockaddr_t *)&remote_addr, &remote_addr_len);
    if (fd == _CC_INVALID_SOCKET_) {
        _cc_logger_debug(_T("thread %d accept fail %s."), 
                        _cc_get_thread_id(NULL),
                        _cc_last_error(_cc_last_errno()));
        return false;
    }

    // 创建事件对象
    event = _cc_event_alloc(async2, _CC_EVENT_TIMEOUT_ | _CC_EVENT_READABLE_);
    if (event == NULL) {
        _cc_close_socket(fd);
        return false;
    }

    // 设置为非阻塞模式
    _cc_set_socket_nonblock(fd, 1);

    // 初始化HTTP上下文
    http = (_http_t*)_cc_malloc(sizeof(_http_t));
    http->state = _CC_HTTP_STATUS_HEADER_;
    http->request = NULL;
    http->payload = 0;
    http->file = NULL;
    http->io = _cc_alloc_io_buffer(_CC_IO_BUFFER_SIZE_);
    _cc_alloc_buf(&http->buffer, _CC_IO_BUFFER_SIZE_);

    // 配置事件参数
    event->fd = fd;
    event->callback = e->callback;
    event->timeout = e->timeout;
    event->data = (uintptr_t)http;

    // 注册到事件循环
    if (async2->attach(async2, event) == false) {
        _cc_logger_debug(_T("thread %d add socket (%d) event fial."), 
                        _cc_get_thread_id(NULL), fd);
        _cc_free_event(async2, event);
        return false;
    }
    return true;
}
```
## 连接关闭处理

```c
/**
 * 连接关闭回调
 * @param async 异步事件管理器
 * @param e 关闭事件对象
 * @return 是否成功释放资源
 */
static bool_t _handle_close(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_logger_debug(_T("%d _handle_close."), e->ident);
    
    if (e->data) {
        _http_t *http = (_http_t*)e->data;
        
        // 释放IO缓冲区
        if (http->io) {
            _cc_free_io_buffer(http->io);
        }
        
        // 释放HTTP请求头
        if (http->request) {
            _cc_http_free_request_header(&http->request);
        }
        
        // 释放数据缓冲区
        if (http->buffer.bytes) {
            _cc_free_buf(&http->buffer);
        }
        
        // 关闭文件句柄
        if (http->file) {
            _cc_fclose(http->file);
        }
        
        // 清理上下文
        e->data = 0;
        _cc_free(http);
        return true;
    }
    return false;
}
```
## HTTP 响应处理函数

```c
/**
 * 400 错误请求响应
 * @param e 事件对象
 * @param io IO缓冲区
 */
_CC_API_PRIVATE(void) response_bad_request(_cc_event_t *e, _cc_io_buffer_t *io) {
    struct {const tchar_t *ptr; size_t length;} body = {_CC_STRING("<HTML><HEAD><TITLE>BAD REQUEST</TITLE></HEAD><BODY><P>Your browser sent a bad request, such as a POST without a Content-Length.</P></BODY></HTML>")};

    io->w.off += snprintf((char*)io->w.bytes + io->w.off, io->w.limit - io->w.off,"HTTP/1.1 400 BAD REQUEST\r\nConnection: close;\r\nContent-type: text/html\r\nContent-Length: %d\r\n\r\n", (int32_t)body.length);
    memcpy(io->w.bytes + io->w.off, body.ptr, body.length * sizeof(char_t));
    io->w.off += (int32_t)body.length * sizeof(char_t);
    _cc_io_buffer_flush(e, io);
}

/**
 * 404 资源未找到响应
 * @param e 事件对象
 * @param io IO缓冲区
 */
_CC_API_PRIVATE(void) not_found(_cc_event_t *e, _cc_io_buffer_t *io) {
    struct {const tchar_t *ptr; size_t length;} body = {_CC_STRING("<HTML><HEAD><TITLE>Not Found</TITLE></HEAD><BODY><p>The server could not find the requested URL.</p></BODY></HTML>")};

    io->w.off += snprintf((char*)io->w.bytes + io->w.off, io->w.limit - io->w.off,"HTTP/1.1 404 NOT FOUND\r\nConnection: close;\r\nContent-type: text/html\r\nContent-Length: %d\r\n\r\n", (int32_t)body.length);
    memcpy(io->w.bytes + io->w.off, body.ptr, body.length * sizeof(char_t));
    io->w.off += (int32_t)body.length * sizeof(char_t);
    _cc_io_buffer_flush(e, io);
}

/**
 * 200 成功响应
 * @param e 事件对象
 * @param io IO缓冲区
 */
_CC_API_PRIVATE(void) response_ok(_cc_event_t *e, _cc_io_buffer_t *io) {
    struct {const tchar_t *ptr; size_t length;} body = {_CC_STRING("<HTML><HEAD><TITLE>Welcome to HTTP</TITLE></HEAD><BODY><p>If you see this page, the web server is successfully</p></BODY></HTML>")};
    
    io->w.off += snprintf((char*)io->w.bytes + io->w.off, io->w.limit - io->w.off,"HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-type: text/html\r\nContent-Length: %d\r\n\r\n", (int32_t)body.length);
    memcpy(io->w.bytes + io->w.off, body.ptr, body.length * sizeof(char_t));
    io->w.off += (int32_t)body.length * sizeof(char_t);
    _cc_io_buffer_flush(e, io);
}

/**
 * 文件响应处理
 * @param e 事件对象
 * @param io IO缓冲区
 * @param www_file 请求的文件路径
 */
_CC_API_PRIVATE(void) request_file(_cc_event_t *e, _cc_io_buffer_t *io, tchar_t *www_file) {
    _http_t *http = (_http_t*)e->data;
    http->file = _cc_fopen(www_file, "rb");
    if (http->file) {
        uint64_t size = _cc_file_size(http->file);
        io->w.off += snprintf((char*)io->w.bytes + io->w.off, io->w.limit - io->w.off,
                            "HTTP/1.1 200 OK\r\n"
                            "Connection: Keep-Alive\r\n"
                            "Content-type: text/html\r\n"
                            "Content-Length: %lld\r\n\r\n", 
                            size);
        _CC_SET_BIT(_CC_EVENT_WRITABLE_, e->flags);
    } else {
        not_found(e, io);
    }
}

_CC_API_PRIVATE(void) response_options(_cc_event_t *e, _cc_io_buffer_t *io) {
    struct {const tchar_t *ptr; size_t length;} body = {_CC_STRING(
        "HTTP/1.1 200 OK\r\n"\
        "Access-Control-Allow-Origin: *\r\n"\
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"\
        "Access-Control-Allow-Headers: *\r\n"\
        "Access-Control-Allow-Credentials: true\r\n"\
        "Connection: Keep-Alive;\r\n"\
        "Content-type: application/json\r\n"\
        "Content-Length: 0\r\n\r\n")};

    if ((io->w.off + body.length) > io->w.limit) {
        _cc_realloc_write_buffer(io, io->w.off + (int32_t)body.length);
    }
    memcpy(io->w.bytes + io->w.off, body.ptr, body.length * sizeof(char_t));
    io->w.off += (int32_t)body.length * sizeof(char_t);

    _cc_io_buffer_flush(e, io);
}
```

## 请求处理逻辑

```c
/**
 * 从HTTP头中获取Content-Length值
 * @param headers HTTP头红黑树
 * @return 内容长度(字节)
 */
_CC_API_PRIVATE(int64_t) _get_content_length(_cc_rbtree_t *headers) {
    const _cc_http_header_t *data = _cc_http_header_find(headers, _T("Content-Length"));
    return (data ? _ttoi(data->value) : 0);
}

/**
 * 数据读取回调
 * @param async 异步事件管理器
 * @param e 事件对象
 * @return 是否继续处理
 */
static bool_t _handle_read(_cc_async_event_t *async, _cc_event_t *e) {
    _http_t *http = (_http_t*)e->data;
    _cc_io_buffer_t *io = http->io;
    
    // 读取网络数据
    int32_t off = _cc_io_buffer_read(e, io);
    if (off < 0) {
        return false; // 读取失败
    } else if (off == 0) {
        return true;  // 无数据
    }

    // 已建立连接但未完成请求
    if (http->state == _CC_HTTP_STATUS_ESTABLISHED_) {
        return false;
    } 
    // 解析HTTP头部
    else if (http->state == _CC_HTTP_STATUS_HEADER_) {
        http->state = _cc_http_header_parser(
            (_cc_http_header_fn_t)_cc_http_alloc_request_header, 
            (pvoid_t *)&http->request, 
            io->r.bytes, 
            &io->r.off
        );

        // 头部解析未完成
        if (http->state == _CC_HTTP_STATUS_HEADER_) {
            return true;
        } 
        // 头部解析错误
        else if (http->state != _CC_HTTP_STATE_PAYLOAD_ || http->request == NULL) {
            response_bad_request(e, io);
            return false;
        }

        // 获取POST内容长度
        http->payload = _get_content_length(&http->request->headers);
        if (http->payload == 0) {
            http->state = _CC_HTTP_STATUS_ESTABLISHED_;
        }

        // 分配POST缓冲区
        if (http->buffer.bytes == NULL && http->payload > 0) {
            _cc_alloc_buf(&http->buffer, (size_t)http->payload);
        }
    } 

    // 处理POST负载数据
    if (http->state == _CC_HTTP_STATUS_PAYLOAD_) {
        _cc_buf_append(&http->buffer, io->r.bytes, io->r.off);
        if (http->buffer.length >= http->payload) {
            http->state = _CC_HTTP_STATUS_ESTABLISHED_;
        }
        io->r.off = 0;
    }

    // 请求处理完成
    if (http->state == _CC_HTTP_STATUS_ESTABLISHED_) {
        // 根路径请求
        if (http->request->script[0] == '/' && http->request->script[1] == 0) {
            request_ok(e, io);
        } 
        // 文件请求
        else {
            tchar_t www_file[_CC_MAX_PATH_] = {0};
            _stprintf(www_file, _T("%s%s"), root, http->request->script);
            
            if (_taccess(www_file, _CC_ACCESS_F_) == 0) {
                request_file(e, io, www_file);
            } else {
                response_not_found(e, io);
            }
        }

        _cc_logger_info("http:%s %s %s", 
                      http->request->method,
                      http->request->script,
                      http->request->protocol);

        // 记录POST原始数据
        if (_tcsicmp(http->request->method, _T("POST")) == 0) {
            _cc_logger_info("RAW:%.*s", http->buffer.length, http->buffer.bytes);
        }
        
        // 重置状态并记录日志
        http->buffer.length = 0;
        http->state = _CC_HTTP_STATUS_HEADER_;
        // 释放请求头
        _cc_http_free_request_header(&http->request);
    }

    return true;
}
```

## 数据发送处理

```c
/**
 * 数据发送回调
 * @param async 异步事件管理器
 * @param e 事件对象
 * @return 是否继续发送
 */
static bool_t _handle_write(_cc_async_event_t *async, _cc_event_t *e) {
    _http_t *http = (_http_t*)e->data;
    _cc_io_buffer_t *io = http->io;
    _cc_logger_debug(_T("%d onWrite."), e->ident);
    
    // 刷新缓冲区数据
    if (io->w.off) {
        if (_cc_io_buffer_flush(e, http->io) < 0) {
            return false; // 发送失败
        }
    }

    // 发送文件内容
    if (http->file) {
        int32_t off = (int32_t)_cc_fread(
            http->file, 
            io->w.bytes + io->w.off, 
            1, 
            io->w.limit - io->w.off
        );
        
        if (off <= 0) {
            _cc_fclose(http->file);
            http->file = NULL;
        } else {
            io->w.off += off;
            _CC_SET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        }
    }
    return true;
}
```

## 连接超时处理

```c
/**
 * 连接超时回调
 * @param async 异步事件管理器
 * @param e 事件对象
 * @return 是否保持连接
 */
static bool_t _handle_timeout(_cc_async_event_t *async, _cc_event_t *e) {
    _cc_logger_debug(_T("%d onTimeout."), e->ident);
    return false; // 返回false表示关闭连接
}
```

## HTTP 服务监听

```c
/**
 * 启动HTTP监听
 * @param host 监听地址(nullptr表示任意地址)
 * @param port 监听端口
 * @return 是否成功
 */
bool_t addListener(const tchar_t *host, uint16_t port) {
    struct sockaddr_in sa;
    _cc_async_event_t *async = _cc_get_async_event();
    _cc_event_t *event = _cc_event_alloc(async, _CC_EVENT_ACCEPT_);
    
    _cc_assert(async != NULL);
    _cc_assert(event != NULL);
    
    if (event == NULL) {
        return false;
    }

    // 配置事件参数
    event->timeout = 60000; // 60秒超时
    event->callback = doEvent;

    // 绑定地址和端口
    _cc_inet_ipv4_addr(&sa, host, port);
    if (!_cc_tcp_listen(async, event, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
        _cc_free_event(async, event);
        _cc_assert(FALSE);
        return false;
    }
    return true;
}
```

## 主函数

```c
/**
 * 程序入口
 * @return 退出码
 */
int main() {
    int c;
    
    // 初始化异步事件系统
    _cc_alloc_async_event(0, NULL);

    // 启动HTTP监听(8080端口)
    addListener(NULL, 8080);

    // 主循环(按q键退出)
    while((c = getchar()) != 'q') {
        _cc_sleep(100); // 100ms延迟
    }
    
    // 释放资源
    _cc_free_async_event();
    return 0;
}
```