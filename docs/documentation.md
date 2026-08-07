# 用户指南
***
## 介绍

libcc 是一个轻量级、跨平台的 C语言库，专注于为开发者提供简洁高效的基础设施接口和丰富的协议支持。其核心设计理念是：

-   **高性能**：基于事件驱动的非阻塞 I/O 模型，支持高并发网络编程
-   **跨平台**：统一封装 Windows IOCP、Linux epoll 和 macOS kqueue 等系统特性
-   **模块化**：按需编译机制，最小化二进制体积
-   **易用性**：简洁直观的 API 设计，降低学习曲线

**核心功能**

libcc 提供以下核心功能模块：

-   **事件驱动框架**：支持 TCP/UDP 套接字、定时器和文件 I/O 事件
-   **协议栈**：内置 HTTP/1.1、WebSocket 等应用层协议实现
-   **线程模型**：提供线程池、原子操作等并发编程工具
-   **扩展模块**：可选集成 MySQL、SQLite、OpenSSL 等第三方库

**适用场景**

libcc 特别适合以下场景：

-   高性能网络服务器开发（如游戏服务器、即时通讯服务）
-   需要跨平台部署的网络应用
-   对执行效率和资源占用有严格要求的场景
**版本特性：**当前稳定版 v2.0.0 已通过 1000+ 测试用例验证，支持 Linux3.2+、Windows 10+ 和 macOS 10.12+ 系统。

## 快速集成
**Step 1.** 按照[Build Tools](https://libcc.cn/docs/libcc/build-tools.html)设置开发环境，确保系统已安装必要的编译工具链（如GCC、Clang 或 MSVC）。
**Step 2.** 包含[libcc](https://libcc.cn/)头文件到你的源代码项目中，确保路径配置正确。
**Step 3.** 将以下代码段添加到`main.c`文件中，这是一个简单的多线程示例，展示 libcc 的基本用法：
```c
#include <libcc.h>
static _cc_thread_local_t int thread_local_value = 0;
static int global_value = 0;

static int32_t fn(void *args) {
    global_value++;
    thread_local_value++;
    printf("Global value:%d!\nThread local value:%d!\n", global_value,thread_local_value);
    return 0;
}

int main(int argc, char *const argv[]) {
    _cc_thread_t *threads[10];
    int i;
    // 创建 10 个线程
    for (i = 0; i < _cc_countof(thread); i++) {
        threads[i] = _cc_thread(fn, "work thread", NULL);
    }

    for (i = 0; i < _cc_countof(threads); i++) {
        int32_t state = 0;
        _cc_wait_thread(threads[i], &state); // 等待线程退出
    }

    return 0;
}
```
**Step 4.** 编译并运行。

## 构建选项

libcc 支持多种构建方式，以适应不同的开发环境和目标平台。以下是主要的构建选项：

**1. 通过 Makefile 编译 (Linux, FreeBSD, macOS)**

``` bash
# 下载源码
git clone https://github.com/libcc/libcc.git
cd libcc

# 编译静态库（适用于需要静态链接的场景）
make .a platform=linux debug=1

# 编译动态库（适用于需要动态加载的场景）
make .so platform=linux debug=1

# 或者使用构建脚本
cd ./build
./build.sh debug
```

**适用场景：** Linux/Unix 环境下的开发，需要自定义编译选项的场景。

**2. Windows 下 MSYS2 环境执行**

``` bash
./build.cmd debug
```

**适用场景：** Windows 平台下的开发，需要兼容 Unix 工具链的场景。

**3. 通过 Visual Studio 编译 (Windows)**

``` bash
proj.Win/libcc.vcxproj
```

**适用场景：** Windows 平台下的原生开发，需要 Visual Studio 集成的场景。

**4. 通过 Android JNI 编译**

``` bash
# 配置 NDK 环境
cd proj.Android/JNI
$NDK/ndk-build NDK_DEBUG=1

# 或者使用构建脚本
./build_NDK.sh
```

**适用场景：** Android 平台下的原生开发，需要通过 JNI 集成的场景。

**5. 通过 Xcode 编译 (macOS, iOS)**

``` bash
proj.OSX/cc.xcodeproj
proj.IOS/cc.xcodeproj
```

**适用场景：** macOS 和 iOS 平台下的开发，需要 Xcode 集成的场景。

**构建问题排查：**

-   确保已安装必要的编译工具链（如 GCC、Clang 或 MSVC）
-   检查环境变量配置是否正确（如 NDK_ROOT）
-   查看构建日志中的错误信息
-   参考 [构建问题排查指南](https://libcc.cn/docs/libcc/documentation.html)
 
**注意：**libcc 项目里通过修改./Makefile文件中的以下选项来启用或禁用特定模块：

-   `USE_LIB_URL_REQUEST=1` - 启用 URL 请求模块
-   `USE_LIB_MYSQL=1` - 启用 MySQL 数据库支持
-   `USE_LIB_SQLSEVER=1` - 启用 SQLSERVER 数据库支持
-   `USE_LIB_SQLITE3=1` - 启用 SQLite 数据库支持
-   `USE_LIB_SMTP=1` - 启用 SMTP 邮件发送支持
-   `USE_LIB_OPENSSL=1` - 启用 OpenSSL 加密支持

根据项目需求灵活配置这些选项，可以显著减少库的体积和依赖项。

## 教程索引

本页面既是 API
参考，也是教程入口。下面列出一组建议的入门教程（均可作为后续页面或示例拓展）：

-   [入门：构建并运行第一个示例](https://libcc.cn/docs/libcc/tutorials/getting-started.html) --- 从零开始，5 分钟内完成 libcc 的安装、编译和运行，体验其简洁高效的开发流程。
-   [事件循环与定时器](https://libcc.cn/docs/libcc/tutorials/event-loop.html) --- 掌握 libcc 的核心事件驱动模型，实现高性能的时间轮计时器和异步任务调度。
-   [网络编程：TCP 服务端/客户端](https://libcc.cn/docs/libcc/tutorials/event-tcp.html) --- 通过 `_cc_tcp_listen` 和 `_cc_tcp_connect`快速构建高并发、低延迟的网络应用。
-   [HTTP 服务端](https://libcc.cn/docs/libcc/tutorials/event-http.html) --- 利用 libcc 内置的 HTTP1.x 支持，轻松开发。
-   [WebSocket 服务端](https://libcc.cn/docs/libcc/tutorials/event-ws.html) --- 利用 libcc 内置的 WebSocket 支持，轻松开发实时通信。
-   [多线程与同步](#tutorial-threading) --- 学习 libcc 的多线程编程模型，通过 `_cc_thread` 和原子操作实现高效的并发控制。
-   [数据库：SQLite/MySQL 示例](https://libcc.cn/docs/libcc/tutorials/database.html) --- 使用 `_cc_register_sqlite` 和 `_cc_register_mysql` 快速集成数据库，实现数据持久化和高效查询。
-   [调试与日志](#tutorial-debugging) --- 掌握 `_cc_logger_*` 工具链，实现多级别日志记录和快速故障排查，提升开发效率。
-   [贡献指南](#tutorial-contribute) --- 加入 libcc 开源社区，了解如何提交代码、运行测试和遵循开发规范，共同推动项目发展。

每个教程都可配套一个小示例（放在 `tests` 目录）。

# API 参考

## Event

### Event flags
libcc 使用事件标志来标识不同类型的事件和描述符属性。这些标志可以组合使用，以满足不同的应用场景需求。

```c
#define _CC_EVENT_UNKNOWN_                      0x0000 /**< 未设置的事件标识 */

#define _CC_EVENT_ACCEPT_                       0x0001 /**< 可接受新连接的事件标识 */
#define _CC_EVENT_WRITABLE_                     0x0002 /**< 可写入数据的事件标识 */
#define _CC_EVENT_READABLE_                     0x0004 /**< 可读取数据的事件标识 */
#define _CC_EVENT_CONNECT_                      0x0008 /**< 连接成功或失败的事件标识 */

#define _CC_EVENT_CLOSED_                       0x0080 /**< 连接或文件描述符关闭的事件标识 */

#define _CC_EVENT_PENDING_                      0x0100 /**< 事件待处理标识 */
#define _CC_EVENT_SOCKET_UDP_                   0x0200 /**< 描述符为 UDP 协议的事件标识 */
#define _CC_EVENT_SOCKET_IPV6_                  0x0400 /**< Socket 连接为 IPv6 的事件标识 */

#define _CC_EVENT_NONBLOCKING_                  0x1000 /**< 非阻塞模式的事件标识 */
#define _CC_EVENT_CLOEXEC_                      0x2000 /**< 子进程不继承描述符的事件标识 */

/** Used in _cc_event_t to determine what the fd is */
#define _CC_EVENT_SOCKET_                       0x010000 /**< 描述符为 Socket 的事件标识 */
#define _CC_EVENT_FILE_                         0x020000 /**< 描述符为文件的事件标识 */
#define _CC_EVENT_TIMEOUT_                      0x040000 /**< 超时事件标识 */
```

**常见标志组合**

-   `_CC_EVENT_READABLE_ | _CC_EVENT_WRITABLE_` - 同时监听读写事件
-   `_CC_EVENT_ACCEPT_ | _CC_EVENT_NONBLOCKING_` - 非阻塞模式下的接受连接事件
-   `_CC_EVENT_SOCKET_ | _CC_EVENT_SOCKET_UDP_` - UDP 套接字事件

**使用建议**

-   对于 TCP 服务器，通常需要监听 `_CC_EVENT_ACCEPT_` 事件
-   对于客户端连接，通常需要监听 `_CC_EVENT_READABLE_` 和 `_CC_EVENT_WRITABLE_` 事件
-   使用 `_CC_EVENT_NONBLOCKING_` 可以提高 I/O 性能
-   使用 `_CC_EVENT_CLOEXEC_` 可以防止子进程继承不需要的文件描述符

### \_cc_event_t

libcc 事件系统的核心数据结构，用于表示和管理各种 I/O 事件、定时器事件等。

``` c
typedef struct _cc_event _cc_event_t;
struct _cc_event {
    /* 当前事件标识，如：读、写、连接、监听、定时器 等事件 _CC_EVENT_* flags */
    uint32_t flags;
    /* 标识哪些事件已经提交给系统了。如：读事件提交给epoll，kqueue，iocp等 
     * 主要通过flags判断是否发生变化，防止事件重复多次提交
    */
    uint32_t filter;

    /* Socket、文件描述符 */
    _cc_socket_t fd;
#ifdef _CC_EVENT_USE_IOCP_
    /*iocp accept 的Socket描述符*/
    _cc_socket_t accept_fd;
#endif

    /* 事件时间轮的超时时间 */
    uint32_t timeout;
    /* 当前事件在时间轮中的槽位 */
    uint32_t expire;

    /*
     * 64-bit local handle.
     * High 32 bits: generation/version.
     * Low 32 bits : 0xFFF(async index)FFFFF(self index).
     */
    uint64_t ident;

    /* 事件回调函数 可读、可写、超时等事件通知. */
    _cc_event_callback_t callback;

    /* 用户绑定的参数. */
    uintptr_t data;

    /* 入侵式链表双向指针，_cc_async_event_t结构体中管理，时间轮链表，no_timer链表 */
    _cc_list_t lnk;
};
```

**字段说明**

-   `flags` - 当前事件标识，使用 `_CC_EVENT_*` 宏定义组合
-   `filter` - 已提交给系统的事件标识，用于防止重复提交
-   `ident` - 事件唯一 ID，用于快速查找
-   `fd` - 关联的文件描述符或套接字
-   `lnk` - 链表指针，用于事件管理器内部维护,时间轮
-   `callback` - 事件回调函数，原型为 `typedef void (*_cc_event_callback_t)(_cc_event_t* e, uint32_t which);`
-   `data` - 用户自定义数据指针

**示例**

```c
// 创建事件
_cc_event_t *event = _cc_event_alloc(async, _CC_EVENT_READABLE_);
event->fd = socket_fd;
event->callback = event_handler;
event->data = user_data;

// 注册事件
...

// 事件回调函数示例
static void event_handler(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    if (which & _CC_EVENT_READABLE_) {
        // 处理可读事件
    }
    if (which & _CC_EVENT_WRITABLE_) {
        // 处理可写事件
    }
}
```

**注意事项**

-   不要直接修改结构体字段，应使用提供的 API 函数
-   在多线程环境中使用时需要确保线程安全

事件结构体，作用与套接字、定时器等事件。

### \_cc_async_event_t

``` c
typedef struct _cc_async_event _cc_async_event_t;
struct _cc_async_event {
    /*是否运行中*/
    byte_t running;
    /*唯一ID*/
    uint16_t ident;
    /*事件回调计数*/
    int32_t processed;
    int32_t actives;

    /*时间轮相关*/
    uint32_t timer;
    uint32_t diff;
    uint64_t tick;
    /* 时间轮的时间槽（双向链表） */
    _cc_list_iterator_t nears[_CC_TIMEOUT_NEAR_];
    _cc_list_iterator_t level[_CC_TIMEOUT_MAX_LEVEL_][_CC_TIMEOUT_LEVEL_];
    /*待处理事件*/
    _cc_list_iterator_t pending;
    /*没有定时的事件 双向链表*/
    _cc_list_iterator_t no_timer;

    /*多线程安全锁*/
#ifdef _CC_EVENT_USE_MUTEX_
    _cc_mutex_t *lock;
#else
    /*使用自旋锁，只对时间轮双向链表push和pop操作锁*/
    _cc_atomic_lock_t lock;
#endif
    /*发生变化的事件，需要重新调整的缓存数组*/
    _cc_array_t changes;

    /*私有的数据内部使用在各平台的参数，如kqueue，epoll，iocp */
    _cc_async_event_priv_t *priv;
    /* 携带用户的参数*/
    pvoid_t args;

    /*事件接口 select，epoll，kqueue，iocp 统一接口*/
    /*对事件重置，放入changs数组等待统一处理*/
    bool_t (*reset)(_cc_async_event_t *async, _cc_event_t *e); 
    /*新事件，放入changs数组等待统一处理*/
    bool_t (*attach)(_cc_async_event_t *async, _cc_event_t *e);
    /**/
    bool_t (*connect)(_cc_async_event_t *async, _cc_event_t *e, const _cc_sockaddr_t *sa, const _cc_socklen_t sa_len);
    /**/
    _cc_socket_t (*accept)(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sa, _cc_socklen_t *sa_len);
    /**/
    bool_t (*disconnect)(_cc_async_event_t *async, _cc_event_t *e);
    /**/
    bool_t (*wait)(_cc_async_event_t *async, uint32_t timeout);
    /**/
    bool_t (*free)(_cc_async_event_t *async);
};
```

async 事件管理结构，包含活动事件列表以及一些管理信息和跨平台接口。epoll、kqueue、iocp、时间轮等

### \_cc_event_alloc

``` c
_CC_API_PUBLIC(_cc_event_t*) _cc_event_alloc(_cc_async_event_t *async, const uint32_t flags);
```

分配并初始化一个新的事件结构体，用于管理套接字、文件描述符或定时器事件。此函数是 libcc 事件驱动模型的核心接口之一，适用于高并发网络编程场景。

**参数：**

-   `async` - 指向事件管理器的指针，用于关联事件与事件循环。必须为非 NULL 值。
-   `flags` - 事件标识，用于指定事件的类型和行为。常用值包括：
    -   `_CC_EVENT_READABLE_` - 可读事件
    -   `_CC_EVENT_WRITABLE_` - 可写事件
    -   `_CC_EVENT_TIMEOUT_` - 超时事件
    -   `_CC_EVENT_PENDING_` - 待处理事件
    可以通过位或操作组合多个标识。

**返回值：**

-   成功时返回指向新分配的事件结构体的指针。
-   失败时返回 NULL，通常是由于内存不足或参数无效。

**示例：**

``` c
_cc_async_event_t *async = _cc_async_event_create();
_cc_event_t *event = _cc_event_alloc(async, _CC_EVENT_READABLE | _CC_EVENT_WRITABLE);
if (event) {
    // 事件分配成功，可以进一步配置和使用
}
```

**注意事项：**

-   调用者需负责在事件不再使用时通过 \_cc_event_free 释放资源。
-   事件标识的组合需符合实际需求，避免不必要的性能开销。
-   在多线程环境中使用时，需确保对事件管理器的操作是线程安全的。

### \_cc_free_event

``` c
_CC_API_PUBLIC(void) _cc_free_event(_cc_async_event_t *async, _cc_event_t *e);
```

释放事件资源并关闭相关连接。此函数是 libcc 事件生命周期管理的关键接口，用于安全地释放事件占用的所有资源，包括：

-   关闭关联的文件描述符或套接字
-   从事件管理器中注销事件
-   释放事件结构体内存

**参数：**

-   `async` - 指向事件管理器的指针，必须是通过 \_cc_async_event_create 创建的合法实例
-   `e` - 要释放的事件指针，必须是通过 \_cc_event_alloc 分配的合法事件

**返回值：**

无返回值（void）

**示例：**

``` c
_cc_async_event_t async;
_cc_register_select(&async);
_cc_event_t *event = _cc_event_alloc(&async, _CC_EVENT_READABLE);
// ... 使用事件 ...
if (!async.attach(&async, event)) {
    //附加失败，安全释放事件
    _cc_free_event(&async, event);
}
...
//无需手动释放
```

**注意事项：**

-   调用此函数后，事件指针 `e` 将不再可用
-   确保在调用此函数前，所有待处理的 I/O 操作已完成
-   如果已经成功附加到了事件管理器中，使用者无需手动调用释放
-   不要重复释放同一个事件

### \_cc_get_async_event()

``` c
_CC_API_PUBLIC(_cc_async_event_t *) _cc_get_async_event(void);
```

获取一个已启用的事件管理器

### \_cc_get_event_by_id()

``` c
_CC_API_PUBLIC(_cc_event_t *) _cc_get_event_by_id(uint32_t ident);
```

通过 **event** 中的 **ident** 获取 **\_cc_event_t** 结构体指针

### \_cc_get_async_event_by_id()

``` c
_CC_API_PUBLIC(_cc_async_event_t *) _cc_get_async_event_by_id(uint32_t ident);
```

通过 **event** 中的 **ident** 获取 **\_cc_async_event_t** 结构体指针

### \_cc_register_select()

``` c
_CC_API_PUBLIC(bool_t) _cc_register_select(_cc_async_event_t*);
```

初始化一个`\_cc_async_event_t`结构体为select

### \_cc_register_iocp()

``` c
_CC_API_PUBLIC(bool_t) _cc_register_iocp(_cc_async_event_t*);
```

初始化一个'\_cc_async_event_t`结构体为 Windows IOCP

### \_cc_register_epoll()

``` c
_CC_API_PUBLIC(bool_t) _cc_register_epoll(_cc_async_event_t*);
```

初始化一个`\_cc_async_event_t`结构体为 Linux epoll

### \_cc_register_kqueue()

``` c
_CC_API_PUBLIC(bool_t) _cc_register_kqueue(_cc_async_event_t*);
```

初始化一个`\_cc_async_event_t`结构体为 Unix (mac、BSD) 的 kqueue 

使用的例子:
```c
_cc_async_event_t async;
_cc_event_t *event;
_cc_register_select(&async);

//创建一个定时器
event = _cc_event_alloc(&async, _CC_EVENT_TIMEOUT_);
event->callback = event_handler;
event->timeout = 1000;

// ... 使用事件 ...
if (!async.attach(&async, event)) {
    //附加失败，安全释放事件
    _cc_free_event(&async, event);
}

// Event loop
while(async.running) {
    async.wait(&async, 1000/* 1 sec */);
}
//关闭所有连接，并释放所有资源。
async.free(&async);
```

### \_cc_tcp_listen()

``` c
_CC_API_PUBLIC(bool_t) _cc_tcp_listen(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sockaddr, _cc_socklen_t socklen);
```

创建一个 TCP 监听事件，并将此监听事件附加到异步事件管理器的变更队列中。该函数通常用于服务器端，监听指定地址和端口的 TCP 连接请求。

**参数说明**

-   `async` - 异步事件管理器指针，用于管理事件循环和 I/O 事件
-   `e` - 事件结构体指针，用于存储监听事件的相关信息
-   `sockaddr` - 指向 `struct sockaddr` 结构体的指针，指定监听的地址和端口
-   `socklen` - `struct sockaddr` 结构体的实际大小

**返回值**

-   `true` - 监听事件创建成功
-   `false` - 监听事件创建失败，通常是由于参数无效或系统资源不足

**示例代码**

``` c
_cc_async_event_t *async = _cc_get_async_event();
_cc_event_t *e = _cc_event_alloc(async, _CC_EVENT_ACCEPT);
struct sockaddr_in sa;
_cc_inet_ipv4_addr(&sa, "0.0.0.0", 8080);

if (_cc_tcp_listen(async, e, &sa, sizeof(struct sockaddr_in))) {
    printf("TCP 监听已启动，端口: 8080\n");
} else {
    printf("TCP 监听启动失败\n");
}
```

**注意事项**

-   调用此函数前，需确保 `async` 和 `e` 已正确初始化
-   监听地址和端口需确保未被占用
-   失败时需检查系统错误日志以确定具体原因

### \_cc_tcp_connect()

``` c
_CC_API_PUBLIC(bool_t) _cc_tcp_connect(_cc_async_event_t *async, _cc_event_t *e, _cc_sockaddr_t *sockaddr, _cc_socklen_t socklen);
```

创建一个 TCP 连接事件，并将此连接事件附加到异步事件管理器的变更队列中。该函数通常用于客户端，发起与指定服务器的 TCP 连接。

**参数说明**

-   `async` - 异步事件管理器指针，用于管理事件循环和 I/O 事件
-   `e` - 事件结构体指针，用于存储连接事件的相关信息
-   `sockaddr` - 指向 `struct sockaddr` 结构体的指针，指定目标服务器的地址和端口
-   `socklen` - `struct sockaddr` 结构体的实际大小

**返回值**

-   `true` - 连接事件创建成功
-   `false` - 连接事件创建失败，通常是由于参数无效或系统资源不足

**示例代码**

``` c
_cc_async_event_t *async = _cc_get_async_event();
_cc_event_t *e = _cc_event_alloc(async, _CC_EVENT_CONNECT);
struct sockaddr_in sa;
_cc_inet_ipv4_addr(&sa, "127.0.0.1", 8080);

if (_cc_tcp_connect(async, e, &sa, sizeof(sa))) {
    printf("TCP 连接已发起，目标: 127.0.0.1:8080\n");
} else {
    printf("TCP 连接发起失败\n");
}
```

**注意事项**

-   调用此函数前，需确保 `async` 和 `e` 已正确初始化
-   目标服务器需确保可访问且端口开放
-   失败时需检查系统错误日志以确定具体原因

### \_cc_alloc_async_event()

``` c
_CC_API_PUBLIC(bool_t) _cc_alloc_async_event(int32_t cores, void (*cb)(_cc_async_event_t*,bool_t));
```

多线程异步事件

参数：

-   `cores` - 创建线程数，0为CPU核数
-   `cb` - 线程创建/结束时回调。

返回值: true/false

### \_cc_free_async_event()

``` c
_CC_API_PUBLIC(bool_t) _cc_free_async_event(void);
```

关闭所有异步事件，并释放所有资源

### \_cc_async_event_abort()

``` c
_CC_API_PUBLIC(void) _cc_async_event_abort(void);
```

中断异步事件所有线程，负责安全退出

### \_cc_async_event_is_running()

``` c
_CC_API_PUBLIC(bool_t) _cc_async_event_is_running(void);
```

判断是否运行中

**示例代码**

``` c
static bool_t timeout_handler(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    if (which & _CC_EVENT_TIMEOUT_) {
        _cc_logger_debug(_T("%d timer timeout. arg:%ld"), e->ident, e->data);
        _cc_async_event_abort();
        return false;
    } else if (which & _CC_EVENT_CLOSED_) {
        _cc_logger_debug(_T("%d destroy timeout. arg:%ld"), e->ident, e->data);
    }
    //返回false 停止计时器，true 继续定时回调
    return true;
}

int main(int argc, char *const argv[]) {
    _cc_alloc_async_event(0, NULL);
    //10秒后安全退出
    _cc_add_event_timeout(&async, 10000, timeout_handler, 1);
    // 主线程 loop
    while(_cc_async_event_is_running()) {
        _cc_sleep(10);
    }

    //关闭所有连接，并释放所有资源。
    _cc_free_async_event();
    return 0;
}
```

### \_cc_send()

``` c
_CC_API_PUBLIC(int32_t) _cc_send(_cc_socket_t fd, const byte_t* buf, int32_t length);
```

通过指定的 Socket 描述符发送数据。该函数是线程安全的，适用于高并发场景。

**参数说明**

-   `fd` - 已建立连接的 Socket 描述符，需确保处于可写状态
-   `buf` - 指向待发送数据的缓冲区指针，缓冲区内容在发送过程中不会被修改
-   `length` - 待发送数据的长度（字节数），需小于等于缓冲区实际大小

**返回值**

-   **成功**：返回实际发送的字节数，可能小于请求的 `length`
-   **失败**：返回 -1，可通过 `_cc_last_error()` 获取详细错误信息

**示例代码**

``` c
byte_t data[] = "Hello, World!";
int32_t sent = _cc_send(sock_fd, data, strlen(data));
if (sent == -1) {
    printf("发送失败，错误码: %d\n", _cc_last_error());
} else {
    printf("成功发送 %d 字节数据\n", sent);
}
```

**注意事项**

-   调用前需确保 Socket 已成功建立连接
-   对于非阻塞 Socket，返回值可能小于请求长度，需配合事件驱动模型处理
-   网络异常时建议重试或关闭连接

### \_cc_recv()

``` c
_CC_API_PUBLIC(int32_t) _cc_recv(_cc_socket_t fd, byte_t* buf, int32_t length);
```

通过指定的 Socket 描述符接收数据。该函数支持阻塞和非阻塞模式，适用于各种网络场景。

**参数说明**

-   `fd` - 已建立连接的 Socket 描述符，需确保处于可读状态
-   `buf` - 指向接收数据的缓冲区指针，需确保有足够的空间存储数据
-   `length` - 缓冲区的最大容量（字节数），建议与实际需求匹配

**返回值**

-   **成功**：返回实际接收的字节数
-   **对端关闭**：返回 0，表示连接已正常终止
-   **失败**：返回 -1，可通过 `_cc_last_error()` 获取详细错误信息

**示例代码**

``` c
byte_t buffer[1024];
int32_t received = _cc_recv(sock_fd, buffer, sizeof(buffer));
if (received == -1) {
    printf("接收失败，错误码: %d\n", _cc_last_error());
} else if (received == 0) {
    printf("对端已关闭连接\n");
} else {
    printf("成功接收 %d 字节数据: %.*s\n", received, received, buffer);
}
```

**注意事项**

-   调用前需确保 Socket 已成功建立连接
-   对于非阻塞 Socket，需配合事件驱动模型处理部分接收的情况
-   返回 0 时应主动关闭连接释放资源
## Timer
### \_cc_add_event_timeout()

``` c
_CC_API_PUBLIC(_cc_event_t*) _cc_add_event_timeout(_cc_async_event_t *async, uint32_t ms, _cc_event_callback_t callback, uintptr_t data);
```

向事件管理器添加一个软计时器。该计时器基于时间轮算法实现，适用于需要周期性触发或延迟触发的场景。

**参数说明**

-   `async` - 异步事件管理器指针，用于管理事件循环和计时器
-   `milliseconds` - 计时器触发间隔（毫秒），最小精度为 10ms
-   `callback` - 计时器触发时调用的回调函数，需返回 `true` 以继续触发或 `false` 以停止
-   `data` - 用户自定义数据，会传递给回调函数

**返回值**

-   **成功**：返回指向新创建计时器的指针
-   **失败**：返回 `NULL`，通常是由于参数无效或系统资源不足

**示例代码**

``` c
static bool_t timeout_handler(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    if (which & _CC_EVENT_TIMEOUT_) {
        _cc_logger_debug(_T("%d timer timeout. arg:%ld"), e->ident, e->data);
    } else if (which & _CC_EVENT_CLOSED_) {
        _cc_logger_debug(_T("%d destroy timeout. arg:%ld"), e->ident, e->data);
        //退出Event Loog
        async->running = false;
    }
    //返回false 停止计时器，并释放，true 继续定时触发
    return true;
}

_cc_event_t *timer = _cc_add_event_timeout(_cc_get_async_event(), 1000, timeout_handler, 10);
if (!timer) {
    printf("Failed to create timer\n");
}
```

**注意事项**

-   回调函数中避免执行耗时操作，以免影响事件循环性能
-   计时器精度受系统调度影响，不适用于需要高精度计时的场景
-   需手动调用 `_cc_kill_event_timeout` 释放资源

### \_cc_kill_event_timeout()

``` c
_CC_API_PUBLIC(bool_t) _cc_kill_event_timeout(_cc_async_event_t *async, _cc_event_t *e);
```

从事件管理器中移除指定的计时器并释放相关资源。该操作是线程安全的。

**参数说明**

-   `async` - 异步事件管理器指针，需与创建计时器时使用的指针一致
-   `e` - 要移除的计时器指针，需通过 `_cc_add_event_timeout` 获取

**返回值**

-   **成功**：返回 `true`
-   **失败**：返回 `false`，通常是由于计时器已被移除或参数无效

**示例代码**

``` c
_cc_async_event_t *async = _cc_get_async_event();
_cc_event_t *timer = _cc_add_event_timeout(async, 1000, timeout_handler, 0);
// ...
if (!_cc_kill_event_timeout(async, timer)) {
    printf("Failed to remove timer\n");
}
```

**注意事项**

-   移除计时器后，对应的回调函数将不再被触发
-   重复移除同一计时器会导致失败

**示例代码**

``` c
static bool_t timeout_handler(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    if (which & _CC_EVENT_TIMEOUT_) {
        _cc_logger_debug(_T("%d timer timeout. arg:%ld"), e->ident, e->data);
    } else if (which & _CC_EVENT_CLOSED_) {
        _cc_logger_debug(_T("%d destroy timeout. arg:%ld"), e->ident, e->data);
        //退出Event Loog
        async->running = false;
    }
    // 返回 true 继续定时触发，false 停止计时器并释放
    return true;
}

int main(int argc, char *const argv[]) {
    _cc_async_event_t async;
    _cc_event_t *event;
    _cc_register_timeout(&async, NULL);

    //创建一个定时器
    _cc_add_event_timeout(&async, 10000, timeout_handler, 1);
    //
    event = _cc_add_event_timeout(&async, 20000, timeout_handler, 2);
    //删除定时器
    _cc_kill_event_timeout(&async, event);
    // Event loop
    while(async.running) {
        async.wait(&async, 100/* 0.1 sec */);
    }

    //关闭所有连接，并释放所有资源。
    async.free(&async);
    return 0;
}
```
## IO Buffer
### \_cc_io_buffer_t

IO buffer 是一种高效的数据结构，用于管理网络 I/O
操作中的数据缓冲。它支持动态增长和收缩，并提供了线程安全的读写操作。

**结构定义**

``` c
typedef struct _cc_io_buffer _cc_io_buffer_t;
typedef struct _cc_io_data {
    int32_t limit;  // 缓冲区总容量
    int32_t off;    // 当前数据偏移量
    byte_t *bytes;  // 数据存储指针
} _cc_io_data_t;

struct _cc_io_buffer {
    _cc_io_data_t r;                // 读缓冲区
    _cc_io_data_t w;                // 写缓冲区
    _cc_atomic_lock_t lock_of_writable; // 写缓冲区锁
    _cc_SSL_t *ssl;                 // SSL/TLS 上下文
};
```

**成员说明**

-   `r` - 读缓冲区，用于存储接收到的数据
-   `w` - 写缓冲区，用于存储待发送的数据
-   `lock_of_writable` - 原子锁，确保多线程环境下写缓冲区的安全访问
-   `ssl` - 支持 TLS/SSL 加密通信的上下文指针

**典型使用场景**

-   网络协议解析
-   高性能服务器开发
-   SSL/TLS 加密通信

**注意事项**

-   读写操作需遵循缓冲区边界检查
-   多线程环境下需正确使用锁机制
-   SSL/TLS 操作需先初始化上下文

![\_cc_io_buffer_t 结构示意图](https://libcc.cn/assets/images/libcc/io_buffer.svg)

### \_cc_alloc_io_buffer()

``` c
_CC_API_PUBLIC(_cc_io_buffer_t *) _cc_alloc_io_buffer(int32_t limit,_cc_SSL_t *ssl);
```

动态分配一个 IO Buffer 结构体，并初始化其读写缓冲区。

**参数说明**

-   `limit` - 读写缓冲区的初始大小（字节数），建议根据实际需求设置合理值
-   `ssl` - SSL 句柄 负责 SSL 协议
**返回值**

-   **成功**：返回指向新分配的 IO Buffer 的指针
-   **失败**：返回 `NULL`，通常是由于内存不足或参数无效

**示例代码**

``` c
_cc_io_buffer_t *buffer = _cc_alloc_io_buffer(4096, NULL);
if (!buffer) {
    printf("Failed to allocate IO buffer\n");
    return;
}
// 使用 buffer...
```

**注意事项**

-   分配后需手动调用 `_cc_free_io_buffer` 释放资源
-   初始大小应根据应用场景合理设置，避免频繁扩容

### \_cc_free_io_buffer()

``` c
_CC_API_PUBLIC(void) _cc_free_io_buffer(_cc_io_buffer_t *io);
```

释放 IO Buffer 结构体及其关联的缓冲区资源。注意：如何 \_cc_alloc_io_buffer 中绑定了SSL 那么也会同时释放 SSL 资源。

**参数说明**

-   `io` - 指向通过 `_cc_alloc_io_buffer` 分配的 IO Buffer 指针

**示例代码**

``` c
_cc_SSL_t *ssl = _SSL_accept(openSSL, fd);
_cc_io_buffer_t *buffer = _cc_alloc_io_buffer(4096, ssl);
// 使用 buffer...
// 释放 buffer 和 SSL资源。
_cc_free_io_buffer(buffer);
```

**注意事项**
-   绑定了SSL 那么也会同时释放 SSL 资源。
-   释放后指针将不可用，避免重复释放
-   建议在程序退出前释放所有分配的 IO Buffer

### \_cc_realloc_read_buffer()

``` c
_CC_API_PUBLIC(void) _cc_realloc_read_buffer(_cc_io_buffer_t *io,int32_t limit);
```

动态调整读缓冲区的大小，支持扩容或缩容。

**参数说明**

-   `io` - 指向已分配的 IO Buffer 指针
-   `limit` - 新的读缓冲区大小（字节数），必须大于 0

**示例代码**

``` c
_cc_io_buffer_t *buffer = _cc_alloc_io_buffer(1024);
_cc_realloc_read_buffer(buffer, 2048); // 扩容读缓冲区
```

**注意事项**

-   调整大小时会保留现有数据
-   频繁调整大小可能影响性能，建议预分配足够空间

### \_cc_realloc_write_buffer()

``` c
_CC_API_PUBLIC(void) _cc_realloc_write_buffer(_cc_io_buffer_t *io,int32_t limit);
```

动态调整写缓冲区的大小，支持扩容或缩容。

**参数说明**

-   `io` - 指向已分配的 IO Buffer 指针
-   `limit` - 新的写缓冲区大小（字节数），必须大于 0

**示例代码**

``` c
_cc_io_buffer_t *buffer = _cc_alloc_io_buffer(1024);
_cc_realloc_write_buffer(buffer, 2048); // 扩容写缓冲区
```

**注意事项**

-   调整大小时会保留现有数据
-   频繁调整大小可能影响性能，建议预分配足够空间

### \_cc_io_buffer_send()

``` c
_CC_API_PUBLIC(int32_t) _cc_io_buffer_send(_cc_event_t *e, _cc_io_buffer_t *io, const byte_t *bytes, int32_t length);
```

将数据发送到 Socket，若 Socket 缓冲区已满，则写入 IO Buffer 的写缓冲区等待后续发送。

**参数说明**

-   `e` - 事件管理器指针，用于管理异步 I/O 操作
-   `io` - IO Buffer 指针，用于缓存未发送的数据
-   `bytes` - 待发送数据的指针
-   `length` - 待发送数据的长度（字节数）

**返回值**

-   **成功**：返回实际发送的字节数
-   **失败**：返回 -1，可通过 `_cc_last_error()` 获取错误信息

**示例代码**

``` c
byte_t data[] = "Hello, World!";
int32_t sent = _cc_io_buffer_send(event, buffer, data, strlen(data));
if (sent == -1) {
    printf("发送失败，错误码: %d\n", _cc_last_error());
}
```

**注意事项**

-   数据可能被部分发送，需检查返回值
-   未发送的数据会缓存在写缓冲区，需定期调用 `_cc_io_buffer_flush`

### \_cc_io_buffer_flush()

``` c
_CC_API_PUBLIC(int32_t) _cc_io_buffer_flush(_cc_event_t *e, _cc_io_buffer_t *io);
```

将 IO Buffer 中的待发送数据推送到 Socket 缓冲区。适用于异步 I/O 场景，确保数据高效传输。

**参数说明**

-   `e` - 事件管理器指针，用于管理异步 I/O 操作
-   `io` - IO Buffer 指针，包含待发送的数据

**返回值**

-   **成功**：返回实际推送的字节数
-   **失败**：返回 -1，可通过 `_cc_last_error()` 获取错误信息

**示例代码**

``` c
int32_t flushed = _cc_io_buffer_flush(event_mgr, buffer);
if (flushed == -1) {
    printf("推送失败，错误码: %d\n", _cc_last_error());
} else {
    printf("成功推送 %d 字节数据\n", flushed);
}
```

**注意事项**

-   调用前需确保 Socket 处于可写状态
-   部分数据可能因缓冲区满而未被推送，需多次调用
-   建议配合事件驱动模型使用，避免阻塞
## Time
### \_cc_timestamp()

``` c
_CC_API_PUBLIC(uint64_t) _cc_timestamp(void);
```

获取当前时间戳

### \_cc_civil_to_days()

``` c
_CC_API_PUBLIC(int64_t) _cc_civil_to_days(int year, int month, int day, int *day_of_week, int *day_of_year);
```

给定一个日期，返回自1970年1月1日以来的天数，一周中的一天\[0-6,0是星期日\]，一年中的一天\[0-365\]。

参数：

-   `year` - 年
-   `month` - 月
-   `day` - 日
-   `day_of_week` - 星期几
-   `day_of_year` - 一年中的第几天

### \_cc_days_in_month()

``` c
_CC_API_PUBLIC(int) _cc_days_in_month(int year, int month);
```

给定一个年月，返回月中的天数\[28、30、31\]？

参数：

-   `year` - 年
-   `month` - 月

返回值: 月中的天数

### \_cc_day_of_year()

``` c
_CC_API_PUBLIC(int) _cc_day_of_year(int year, int month, int day);
```

给定一个日期，返回自1970年1月1日以来的天数，一年中的一天\[0-365\]？

参数：

-   `year` - 年
-   `month` - 月
-   `day` - 月

返回值: 一年中的第几天

### \_cc_day_of_week()

``` c
_CC_API_PUBLIC(int) _cc_day_of_week(int year, int month, int day);
```

给定一个日期，返回自1970年1月1日以来的天数，一周中的一天\[0-6,0是星期日\]？

参数：

-   `year` - 年
-   `month` - 月
-   `day` - 月

返回值: 星期几

### \_cc_sleep()

``` c
_CC_API_PUBLIC(void) _cc_sleep(uint32_t ms);
```

睡眠

参数：

-   `ms` - 毫秒

### \_cc_nsleep()

``` c
_CC_API_PUBLIC(void) _cc_nsleep(uint64_t ns);
```

睡眠

参数：

-   `ns` - 纳秒

### \_cc_get_ticks()

``` c
_CC_API_PUBLIC(uint64_t) _cc_get_ticks(void);
```

用于获取系统启动后经过的毫秒数

### \_cc_get_ticks_ns()

``` c
_CC_API_PUBLIC(uint64_t) _cc_get_ticks_ns(void);
```

用于获取系统启动后经过的纳秒数

## URL

``` c
/* http://user_name:user_password@localhost/index.html?id=1&tid=2#top */
typedef struct _cc_url {
    /* (eg: http,ftp,maito) */
    struct {
        uint32_t ident;
        _cc_sds_t value;
    } scheme;

    /* is IPv6*/
    bool_t ipv6;
    /* (eg: port) */
    uint32_t port;

    /* (eg: localhost) */
    _cc_sds_t host;
    /* (eg: /v1/index.html) */
    _cc_sds_t path;
    /* (eg: /v1/index.html?id=1&tid=2#top) */
    _cc_sds_t request;
    /* (eg: id=1&tid=2) */
    _cc_sds_t query;
    /* (eg: top) */
    _cc_sds_t fragment;
    /* (eg: user_name) */
    _cc_sds_t username;
    /* (eg: user_password) */
    _cc_sds_t password;
} _cc_url_t;
```

### \_cc_alloc_url()

``` c
_CC_API_PUBLIC(bool_t) _cc_alloc_url(_cc_url_t *u, const tchar_t *url);
```

解析一个URL

参数：

-   `u` - \_cc_url_t 结构体指针
-   `url` - 网址

### \_cc_free_url()

``` c
_CC_API_PUBLIC(bool_t) _cc_free_url(_cc_url_t *u);
```

释放URL结构体的资源

参数：

-   `u` - \_cc_url_t 结构体指针

使用的例子:

``` c
char_t *url = "http://libcc.cn/index.html?id=1&tid=2#top";
_cc_url_t u;
_cc_alloc_url(&u, url);
printf("%s :// %s : %d / %s #%s\n",u.scheme.value, u.host, u.port, u.request, u.fragment);
_cc_free_url(&u);
```

### \_cc_url_encode()

``` c
_CC_API_PUBLIC(int32_t) _cc_url_encode(const tchar_t *src, int32_t src_len, tchar_t *dst, int32_t dst_len);
```

URL 编码：将字符串编码为url编码，并将编码后的字符串写入dst

参数：

-   `src` - 需要编码的字符串
-   `src_len` - 字符串长度
-   `dst` - 编码后的字符串缓冲区
-   `dst_len` - 字符串缓冲区大小

返回值: 编码后字符串长度

### \_cc_url_decode()

``` c
_CC_API_PUBLIC(int32_t) _cc_url_decode(const tchar_t *src, int32_t src_len, tchar_t *dst, int32_t dst_len);
```

URL 编码：将字符串编码为url编码，并将编码后的字符串写入dst

参数：

-   `src` - 需要解码的字符串
-   `src_len` - 字符串长度
-   `dst` - 解码后的字符串缓冲区
-   `dst_len` - 字符串缓冲区大小

返回值: 解码后字符串长度

### \_cc_raw_url_encode()

``` c
_CC_API_PUBLIC(int32_t) _cc_raw_url_encode(const tchar_t *src, int32_t src_len, tchar_t *dst, int32_t dst_len);
```

URL 编码：将字符串编码为url编码，并将编码后的字符串写入dst

参数：

-   `src` - 需要编码的字符串
-   `src_len` - 字符串长度
-   `dst` - 编码后的字符串缓冲区
-   `dst_len` - 字符串缓冲区大小

返回值: 编码后字符串长度

### \_cc_raw_url_decode()

``` c
_CC_API_PUBLIC(int32_t) _cc_raw_url_decode(const tchar_t *src, int32_t src_len, tchar_t *dst, int32_t dst_len);
```

URL 编码：将字符串编码为url编码，并将编码后的字符串写入dst

参数：

-   `src` - 需要解码的字符串
-   `src_len` - 字符串长度
-   `dst` - 解码后的字符串缓冲区
-   `dst_len` - 字符串缓冲区大小

返回值: 解码后字符串长度

**注意：** \_cc_url_decode() 和 \_cc_raw_url_decode() 的主要区别在于对**加号**\'+\'的处理方式 \_cc_url_decode() 在解码时，会将字符串中的加号 \'+\' 转换为空格 \' \' 而 \_cc_raw_url_decode() 则会将加号 \'+\' 保持原样，仅对百分号 \'%\' 后跟两位十六进制数的格式（如 %20）进行解码

## SQL
``` c
typedef struct _cc_sql_delegate _cc_sql_delegate_t;
typedef struct _cc_sql _cc_sql_t;
typedef struct _cc_sql_result _cc_sql_result_t;
struct _cc_sql_delegate {
    /**/
    _cc_sql_t *(*connect)(const tchar_t *);
    /**/
    bool_t (*disconnect)(_cc_sql_t *);
    /**/
    bool_t (*reset)(_cc_sql_t *, _cc_sql_result_t *);
    /**/
    bool_t (*step)(_cc_sql_t *, _cc_sql_result_t *);
    /**/
    bool_t (*execute)(_cc_sql_t *, const _cc_string_t *, _cc_sql_result_t **);
    /**/
    bool_t (*auto_commit)(_cc_sql_t *, bool_t);
    /**/
    bool_t (*begin_transaction)(_cc_sql_t *);
    /**/
    bool_t (*commit)(_cc_sql_t *);
    /**/
    bool_t (*rollback)(_cc_sql_t *);
    /**/
    bool_t (*next_result)(_cc_sql_t *, _cc_sql_result_t *);
    /**/
    bool_t (*free_result)(_cc_sql_t *, _cc_sql_result_t *);
    /**/
    int32_t (*get_num_fields)(_cc_sql_result_t *);
    /**/
    uint64_t (*get_num_rows)(_cc_sql_result_t *);
    /**/
    bool_t (*fetch)(_cc_sql_result_t *);
    /**/
    pvoid_t (*get_stmt)(_cc_sql_result_t *);
    /**/
    uint64_t (*get_last_id)(_cc_sql_t *, _cc_sql_result_t *);
    /**/
    bool_t (*bind)(_cc_sql_result_t *, int32_t, const void *, size_t, uint8_t);
    /**/
    int32_t (*get_int)(_cc_sql_result_t *, int32_t);
    /**/
    int64_t (*get_int64)(_cc_sql_result_t *, int32_t);
    /**/
    float64_t (*get_float)(_cc_sql_result_t *, int32_t);
    /**/
    size_t (*get_string)(_cc_sql_result_t *, int32_t, tchar_t*, size_t);
    /**/
    size_t (*get_blob)(_cc_sql_result_t *, int32_t, byte_t **);
    /**/
    bool_t (*get_datetime)(_cc_sql_result_t *, int32_t, struct tm*);
};
```

### \_cc_register_mysql()

``` c
_CC_API_PUBLIC(bool_t) _cc_register_mysql(_cc_sql_delegate_t *delegator);
```

初始化mysql

[使用示例参见mysql](https://github.com/libcc/libcc/blob/2.0/tests/test_mysql.c)

### \_cc_register_sqlsvr()

``` c
_CC_API_PUBLIC(bool_t) _cc_register_sqlsvr(_cc_sql_delegate_t *delegator);
```

初始化sql server

[使用示例参见sql server](https://github.com/libcc/libcc/blob/2.0/tests/test_sqlserver.c)

### \_cc_register_sqlite()

``` c
_CC_API_PUBLIC(bool_t) _cc_register_sqlite(_cc_sql_delegate_t *delegator);
```

初始化sqlite3

[使用示例参见sqlit3](https://github.com/libcc/libcc/blob/2.0/tests/test_sqlite.c)

### \_cc_register_oci8()

``` c
_CC_API_PUBLIC(bool_t) _cc_register_oci8(_cc_sql_delegate_t *delegator);
```

初始化 OCI8

使用的例子:

``` c
_cc_sql_delegate_t mysql;
_cc_register_sqlite(&mysql);
_cc_sql_t *conn = mysql.connect("mysql://root:123654asd@127.0.0.1:3306/test");
if (conn) {
    printf("connection succed\n");
    mysql.disconnect(conn);
} else {
    printf("connection failed\n");
}
```
## JSON
### 基础数据访问

以下函数组提供了从 JSON 对象/数组中提取各种类型数据的标准方法：

**对象字段访问**

``` c
// 获取JSON对象中的整型 (返回0如果字段不存在或类型不匹配)
int64_t _cc_json_object_find_integer(const _cc_json_t *ctx, const tchar_t *keyword);

// 获取JSON对象中的浮点数 (返回0.0如果字段不存在或类型不匹配)
float64_t _cc_json_object_find_float(const _cc_json_t *ctx, const tchar_t *keyword);

// 获取JSON对象中的字符串 (返回NULL如果字段不存在或类型不匹配)
const _cc_sds_t _cc_json_object_find_string(const _cc_json_t *ctx, const tchar_t *keyword);

// 获取JSON对象中的数组 (返回NULL如果字段不存在或类型不匹配)
_cc_array_t _cc_json_object_find_array(const _cc_json_t *ctx, const tchar_t *keyword);

// 获取JSON对象中的子对象 (返回NULL如果字段不存在或类型不匹配)
const _cc_rbtree_t *_cc_json_object_find_object(const _cc_json_t *ctx, const tchar_t *keyword);

// 获取JSON对象中的布尔值 (返回false如果字段不存在或类型不匹配)
bool_t _cc_json_object_find_boolean(const _cc_json_t *ctx, const tchar_t *keyword);
```

**数组元素访问**

``` c
// 获取JSON数组中的整型 (索引越界返回0)
int64_t _cc_json_array_find_integer(const _cc_json_t *ctx, const uint32_t index);

// 获取JSON数组中的浮点数 (索引越界返回0.0)
float64_t _cc_json_array_find_float(const _cc_json_t *ctx, const uint32_t index);

// 获取JSON数组中的字符串 (索引越界返回NULL)
const _cc_sds_t _cc_json_array_find_string(const _cc_json_t *ctx, const uint32_t index);

// 获取JSON数组中的子对象 (索引越界返回NULL)
const _cc_rbtree_t* _cc_json_array_find_object(const _cc_json_t *ctx, const uint32_t index);

// 获取JSON数组中的子数组 (索引越界返回NULL)
_cc_array_t _cc_json_array_find_array(const _cc_json_t *ctx, const uint32_t index);

// 获取JSON数组中的布尔值 (索引越界返回false)
bool_t _cc_json_array_find_boolean(const _cc_json_t *ctx, const uint32_t index);
```

**使用示例**

``` c
// 解析JSON对象
_cc_json_t* json = _cc_json_parse("{\"name\":\"John\", \"age\":30}", 18);

// 获取字段值
const _cc_sds_t name = _cc_json_object_find_string(json, "name");
int64_t age = _cc_json_object_find_number(json, "age");

// 处理数组
_cc_array_t arr = _cc_json_object_find_array(json, "items");
if (arr) {
    for (int32_t i = 0; i < _cc_array_length(arr); i++) {
        _cc_json_t *it = (_cc_json_t*)_cc_array_value(arr,i);
        //....
    }
}

_cc_free_json(json);
```

**注意事项**

-   所有查找函数都是线程安全的
-   返回的字符串和容器指针在 JSON 对象释放前有效
-   数字类型会自动尝试转换（如字符串\"123\"可转为数字123）
-   建议总是检查返回值有效性

### \_cc_json_from_file()

``` c
_CC_API_PUBLIC(_cc_json_t *) _cc_json_from_file(const tchar_t *file);
```

从文件系统加载并解析 JSON 数据，支持 UTF-8 编码。

**参数说明**

-   `file` - JSON 文件路径，支持相对路径和绝对路径

**返回值**

-   **成功**：返回解析后的 JSON 对象树
-   **失败**：返回 NULL，可能原因包括：
    -   文件不存在或不可读
    -   内存不足
    -   JSON 格式错误

**示例代码**

``` c
_cc_json_t* config = _cc_json_from_file("config.json");
if (!config) {
    printf("无法加载配置文件\n");
    return;
}
// 使用 config...
_cc_free_json(config);
```

**注意事项**

-   文件大小限制为 10MB（可通过宏修改）
-   文件内容应为标准 JSON 格式
-   返回的对象必须通过 `_cc_free_json` 释放
-   Windows 路径建议使用正斜杠或双反斜杠
### \_cc_json_parse()

``` c
_CC_API_PUBLIC(_cc_json_t *) _cc_json_parse(const tchar_t *src, size_t length);
```

解析内存中的 JSON 字符串，构建对象树。

**参数说明**

-   `src` - JSON 字符串指针，需以 null 结尾或通过 length 指定长度
-   `length` - 字符串长度（字节数），为 -1 时自动计算长度

**返回值**

-   **成功**：返回解析后的 JSON 对象树
-   **失败**：返回 NULL，可能原因包括：
    -   字符串为 NULL
    -   内存不足
    -   JSON 格式错误

**示例代码**

``` c
const tchar_t* json_str = "{\"key\":\"value\"}";
_cc_json_t* obj = _cc_json_parse(json_str, 0);
if (!obj) {
    printf("JSON 解析失败\n");
    return;
}
// 使用 obj...
_cc_free_json(obj);
```

**注意事项**

-   字符串内容在解析期间不会被修改
-   大字符串解析可能耗时，建议在非主线程执行
-   返回的对象必须通过 `_cc_free_json` 释放
-   性能关键场景可复用已分配的 JSON 对象
### \_cc_json_dump()

``` c
_CC_API_PUBLIC(void) _cc_json_dump(_cc_json_t *ctx, _cc_buf_t* buf);
```

将 JSON 对象树序列化为字符串，支持格式化输出。

**参数说明**

-   `ctx` - 要序列化的 JSON 对象，不能为 NULL
-   `buf` - 输出缓冲区，需预先初始化

**示例代码**

``` c
_cc_buf_t buf;

_cc_json_dump(json_obj, &buf);
printf("JSON: %.*s\n", (int)buf.len, buf.data);

_cc_free_buf(&buf);
```

### \_cc_json_alloc_object()

``` c
_CC_API_PUBLIC(_cc_json_t *) _cc_json_alloc_object(byte_t type, const tchar_t *keyword);
```

创建一个JSON对象结构体

参数：

-   `type` - JSON类型
-   `keyword` - JSON对象关键字

JSON Type：

``` c
/*
 * JSON Types:
 */
enum _CC_JSON_TYPES_ {
    _CC_JSON_NULL_ = 0, //空类型
    _CC_JSON_BOOLEAN_,  //布尔型
    _CC_JSON_FLOAT_,    //浮点型
    _CC_JSON_INT_,      //整数型
    _CC_JSON_OBJECT_,   //对象
    _CC_JSON_ARRAY_,    //数组
    _CC_JSON_STRING_    //字符串
};
```

### \_cc_free_json()

``` c
_CC_API_PUBLIC(void) _cc_free_json(_cc_json_t *ctx);
```

释放JSON所有资源

参数：

-   `ctx` - JSON结构体
## XML
### XML 节点结构

XML 文档在内存中以树形结构表示，每个节点包含以下字段：

``` c
typedef struct _cc_xml {
    /* 节点类型，见 _CC_XML_TYPES_ 枚举 */
    byte_t type;

    /* 节点名称（标签名） */
    _cc_sds_t name;

    /* 节点内容联合体 */
    union {
        _cc_sds_t uni_comment;    // 注释内容
        _cc_sds_t uni_doctype;    // DOCTYPE 声明
        _cc_xml_context_t uni_context; // 文本内容
        _cc_list_iterator_t uni_child; // 子节点列表
    } element;

    /* 兄弟节点链表指针 */
    _cc_list_iterator_t lnk;
    
    /* 属性表（键值对） */
    _cc_rbtree_t attr;
} _cc_xml_t;
```

**节点类型**

``` c
/**
 * XML 节点类型枚举
 */
enum _CC_XML_TYPES_ {
    _CC_XML_NULL_ = 0,  // 空节点
    _CC_XML_COMMENT_,   // 注释节点 
    _CC_XML_CONTEXT_,   // 文本内容节点
    _CC_XML_DOCTYPE_,   // 文档类型声明 
    _CC_XML_CHILD_      // 普通元素节点 
};
```

**内存管理**

-   节点通过 `_cc_alloc_xml_element` 创建
-   必须通过 `_cc_free_xml` 释放
-   子节点和属性会随父节点一起释放

### \_cc_xml_from_file()

``` c
_CC_API_PUBLIC(_cc_xml_t*) _cc_xml_from_file(const tchar_t* file);
```

加载并解析 XML 文件，支持 UTF-8 编码。

**参数说明**

-   `file` - XML 文件路径（支持相对/绝对路径）

**返回值**

-   **成功**：返回文档根节点
-   **失败**：返回 NULL，可能原因包括：
    -   文件不存在或不可读
    -   内存不足
    -   XML 格式错误

**示例代码**

``` c
_cc_xml_t* doc = _cc_xml_from_file("config.xml");
if (!doc) {
    printf("XML 解析失败\n");
    return;
}
// 使用 doc...
_cc_free_xml(doc);
```

**注意事项**

-   文件大小限制为 5MB（可通过宏修改）
-   支持 XML 1.0 标准
-   Windows 路径建议使用正斜杠或双反斜杠
-   返回的文档必须通过 `_cc_free_xml` 释放

### \_cc_xml_parse()

``` c
_CC_API_PUBLIC(_cc_xml_t *) _cc_xml_parse(const tchar_t *src, size_t length);
```

解析内存中的 XML 字符串，构建对象树。

**参数说明**

-   `src` - XML 字符串指针（需以 null 结尾或通过 length 指定长度）
-   `length` - 字符串长度（字节数），为 0 时自动计算

**返回值**

-   **成功**：返回文档根节点
-   **失败**：返回 NULL，可能原因包括：
    -   字符串为 NULL
    -   内存不足
    -   XML 格式错误

**示例代码**

``` c
const tchar_t* xml_str = "test";
_cc_xml_t* doc = _cc_xml_parse(xml_str, 0);
if (!doc) {
    printf("XML 解析失败\n");
    return;
}
// 使用 doc...
_cc_free_xml(doc);
```

**注意事项**

-   字符串内容不会被修改
-   大字符串解析可能耗时，建议在非主线程执行
-   返回的对象必须通过 `_cc_free_xml` 释放

### \_cc_xml_dump()

``` c
_CC_API_PUBLIC(void) _cc_xml_dump(_cc_xml_t *ctx, _cc_buf_t* buf);
```

XML序列化字符串

参数：

-   `ctx` - XML结构体
-   `length` - XML 序列化到_cc_buf_t结构体中

### \_cc_alloc_xml_element()

``` c
_CC_API_PUBLIC(_cc_xml_t*) _cc_alloc_xml_element(byte_t type);
```

创建一个XML对象结构体

参数：

-   `type` - XML类型

JSON Type：

``` c
/**
 * @brief XML Types:
 */
enum _CC_XML_TYPES_ {
    _CC_XML_NULL_ = 0,  // 空
    _CC_XML_COMMENT_,   // 注解
    _CC_XML_CONTEXT_,   // 内容
    _CC_XML_DOCTYPE_,   // doctype
    _CC_XML_CHILD_      // 子节点
};
```

### \_cc_free_xml()

``` c
_CC_API_PUBLIC(void) _cc_free_xml(_cc_xml_t* ctx);
```

释放XML所有资源

参数：

-   `ctx` - XML结构体

### \_cc_xml_element_append()

``` c
_CC_API_PUBLIC(bool_t) _cc_xml_element_append(_cc_xml_t* ctx, _cc_xml_t* child);
```

XML附加子节点

参数：

-   `ctx` - XML结构体
-   `child` - XML 子节点

### \_cc_xml_element_find()

``` c
_CC_API_PUBLIC(_cc_xml_t*) _cc_xml_element_find(_cc_xml_t* ctx, tchar_t* name);
```

XML查找，通过nodeName 查找

参数：

-   `ctx` - XML结构体
-   `name` - XML 子节点名称

### \_cc_xml_element_first_child()

``` c
_CC_API_PUBLIC(_cc_xml_t*) _cc_xml_element_first_child(_cc_xml_t* ctx);
```

获取第一个子节点

参数：

-   `ctx` - XML结构体

### \_cc_xml_element_next_child()

``` c
_CC_API_PUBLIC(_cc_xml_t*) _cc_xml_element_next_child(_cc_xml_t* ctx);
```

获取下一个子节点

参数：

-   `ctx` - XML结构体

### \_cc_xml_element_text()

``` c
_CC_API_PUBLIC(const _cc_sds_t) _cc_xml_element_text(_cc_xml_t* ctx);
```

获取节点文本内容

参数：

-   `ctx` - XML结构体

### \_cc_xml_element_attr()

``` c
_CC_API_PUBLIC(const _cc_sds_t) _cc_xml_element_attr(_cc_xml_t* ctx, const tchar_t* keyword);
```

获取节点上的属性值

参数：

-   `ctx` - XML结构体
-   `keyword` - 属性名称

返回值: 字符串

### \_cc_xml_element_set_attr()

``` c
_CC_API_PUBLIC(bool_t) _cc_xml_element_set_attr(_cc_xml_t* ctx, const tchar_t* keyword, const tchar_t* fmt, ...);
```

设置节点属性

参数：

-   `ctx` - XML结构体
-   `keyword` - 属性名称
-   `fmt` - 格式化字符串
## INI
### INI 结构定义

INI 配置在内存中以键值对和分节（Section）的形式组织，结构如下：

``` c
/*
 * INI Types:
 */
enum _CC_INI_TYPES_ {
    _CC_INI_NULL_ = 0,  // 空类型    
    _CC_INI_SECTION_,   // 分节类型
    _CC_INI_BOOLEAN_,   // 布尔类型
    _CC_INI_FLOAT_,     // 浮点类型
    _CC_INI_INT_,       // 整数类型
    _CC_INI_STRING_     // 字符串类型
};

typedef struct _cc_ini {
    /* 节点类型 */
    byte_t type;    
    
    /* 节点名称（Section 名或键名） */
    _cc_sds_t name;

    /* 节点值联合体 */
    union {
        bool_t uni_boolean;         // 布尔值
        int64_t uni_int;            // 整数值
        float64_t uni_float;        // 浮点值
        _cc_sds_t uni_string;       // 字符串值
        _cc_rbtree_t uni_object;    // 子 Section
    } element;
    
    /* 同级节点链表指针 */
    _cc_rbtree_iterator_t lnk;
} _cc_ini_t;
```

**内存管理**

-   节点通过解析函数（如 `_cc_ini_from_file`）创建
-   必须通过 `_cc_free_ini` 释放
-   子 Section 会随父节点一起释放

### \_cc_ini_from_file()

``` c
_CC_API_PUBLIC(_cc_ini_t*) _cc_ini_from_file(const tchar_t* file);
```

加载并解析 INI 配置文件，支持标准 INI 格式。

**参数说明**

-   `file` - INI 文件路径（支持相对/绝对路径）

**返回值**

-   **成功**：返回解析后的 INI 根节点
-   **失败**：返回 NULL，可能原因包括：
    -   文件不存在或不可读
    -   内存不足
    -   INI 格式错误

**示例代码**

``` c
_cc_ini_t* config = _cc_ini_from_file("config.ini");
if (!config) {
    printf("INI 解析失败\n");
    return;
}
// 使用 config...
_cc_free_ini(config);
```

**注意事项**

-   文件大小限制为 1MB（可通过宏修改）
-   支持 UTF-8 编码
-   返回的对象必须通过 `_cc_free_ini` 释放

### \_cc_ini_parse()

``` c
_CC_API_PUBLIC(_cc_ini_t *) _cc_ini_parse(const tchar_t *src, size_t length);
```

解析内存中的 INI 格式字符串，构建配置树。

**参数说明**

-   `src` - INI 字符串指针（需以 null 结尾或通过 length 指定长度）
-   `length` - 字符串长度（字节数），为 0 时自动计算

**返回值**

-   **成功**：返回解析后的 INI 根节点
-   **失败**：返回 NULL，可能原因包括：
    -   字符串为 NULL
    -   内存不足
    -   INI 格式错误

**示例代码**

``` c
const tchar_t* ini_str = "[Section]\nkey=value";
_cc_ini_t* config = _cc_ini_parse(ini_str, 0);
if (!config) {
    printf("INI 解析失败\n");
    return;
}
// 使用 config...
_cc_free_ini(config);
```

**注意事项**

-   字符串内容不会被修改
-   返回的对象必须通过 `_cc_free_ini` 释放
-   性能关键场景可复用已分配的 INI 对象

### \_cc_free_ini()

``` c
_CC_API_PUBLIC(void) _cc_free_ini(_cc_ini_t* ctx);
```

释放 INI 配置对象及其所有关联资源（包括子 Section 和键值对）。

**参数说明**

-   `ctx` - 要释放的 INI 配置对象（可为 NULL）

**示例代码**

``` c
_cc_ini_t* config = _cc_ini_from_file("config.ini");
// 使用 config...
_cc_free_ini(config);  // 安全释放
```

**注意事项**

-   重复释放已释放的对象会导致未定义行为
-   线程不安全，需确保无其他线程正在访问该对象
-   释放后应将指针置为 NULL

### \_cc_ini_dump()

``` c
_CC_API_PUBLIC(void) _cc_ini_dump(_cc_ini_t *ctx, _cc_buf_t* buf);
```

将 INI 配置对象序列化为字符串并写入缓冲区。

**参数说明**

-   `ctx` - 要序列化的 INI 配置对象
-   `buf` - 目标缓冲区（需预先初始化）

**示例代码**

``` c
_cc_buf_t buf;
_cc_ini_dump(config, &buf);
printf("INI content: %s\n", buf.data);
_cc_buf_free(&buf);
```

**注意事项**

-   缓冲区空间不足时会自动扩展
-   序列化结果包含换行符和缩进，适合人类阅读
-   线程不安全，需确保无其他线程正在修改配置

### \_cc_ini_find()

``` c
_CC_API_PUBLIC(_cc_ini_t*) _cc_ini_find(_cc_ini_t* ctx, const tchar_t* name);
```

查找 INI 配置中的 Section 或键值。

**参数说明**

-   `ctx` - 起始查找的 INI 节点（通常为根节点）
-   `name` - 目标名称（格式为 \"section\"）

**返回值**

-   **找到**：返回对应的 Section 或键值对节点
-   **未找到**：返回 NULL

**示例代码**

``` c
// 查找 "database"
_cc_ini_t* database = _cc_ini_find(config, "database");
if (database) {
    // 查找 "port"
    _cc_ini_t* port_node = _cc_ini_find(database, "port");
    if (port_node) {
        printf("Found port: %ld\n", port_node->element.uni_int);
    }
}
```

**注意事项**

-   名称区分大小写
-   频繁查找建议缓存结果
-   线程不安全，需外部同步

### \_cc_ini_find_string()

``` c
_CC_API_PUBLIC(_cc_sds_t) _cc_ini_find_string(_cc_ini_t* ctx, const tchar_t* name);
```

INI 查找属性字符串值

参数：

-   `ctx` - INI 结构体
-   `name` - INI 名称
## Logger
### 日志等级

``` c
    // 日志严重级别枚举
    enum {
        _CC_LOG_LEVEL_EMERG_ = 0, // 系统不可用（最高级别）
        _CC_LOG_LEVEL_ALERT_,     // 需要立即采取行动
        _CC_LOG_LEVEL_CRIT_,      // 关键错误
        _CC_LOG_LEVEL_ERROR_,     // 一般错误
        _CC_LOG_LEVEL_WARNING_,   // 警告信息
        _CC_LOG_LEVEL_NOTICE_,    // 正常但重要的事件
        _CC_LOG_LEVEL_INFO_,      // 信息性消息
        _CC_LOG_LEVEL_DEBUG_      // 调试信息（最低级别）
    };
```
**等级说明**

-   `EMERG` - 系统崩溃或完全不可用
-   `ALERT` - 需要立即人工干预的严重问题
-   `CRIT` - 关键业务逻辑错误
-   `ERROR` - 运行时错误但不影响系统继续运行
-   `WARNING` - 潜在问题警告
-   `NOTICE` - 重要但正常的系统事件
-   `INFO` - 一般信息记录
-   `DEBUG` - 开发调试信息

**使用建议**

-   生产环境建议记录 WARNING 及以上级别
-   开发环境可开启 DEBUG 级别日志
-   不同级别日志建议输出到不同文件

### \_cc_loggerW()

``` c
_CC_API_PUBLIC(void) _cc_loggerW(const tchar_t *file, int line, uint8_t level, const wchar_t* fmt, ...);
```

宽字符版本日志输出函数，支持格式化字符串和可变参数。

**参数说明**

-   `file` - 源代码文件名，通常使用 `__FILE__` 宏
-   `line` - 源代码行号，通常使用 `__LINE__` 宏
-   `level` - 日志级别，取值范围为日志等级枚举值
-   `fmt` - 格式化字符串，支持标准 printf 格式
-   `...` - 可变参数，匹配格式化字符串

**示例代码**

``` c
_cc_loggerW(__FILE__, __LINE__, _CC_LOG_LEVEL_ERROR_, 
                  L"Failed to open file: %s", L"config.txt");
```

**注意事项**

-   仅当定义了 `_CC_UNICODE_` 宏时可用
-   格式化字符串必须使用宽字符 (L\"...\")
-   性能敏感场景建议使用静态日志宏

### \_cc_loggerA()

``` c
_CC_API_PUBLIC(void) _cc_loggerA(const tchar_t *file, int line, uint8_t level, const char_t* fmt, ...);
```

多字节字符版本日志输出函数，支持格式化字符串和可变参数。

**参数说明**

-   `file` - 源代码文件名，通常使用 `__FILE__` 宏
-   `line` - 源代码行号，通常使用 `__LINE__` 宏
-   `level` - 日志级别，取值范围为日志等级枚举值
-   `fmt` - 格式化字符串，支持标准 printf 格式
-   `...` - 可变参数，匹配格式化字符串

**示例代码**

``` c
_cc_loggerA(__FILE__, __LINE__, _CC_LOG_LEVEL_ERROR_, 
                  "Failed to open file: %s", "config.txt");
```

**注意事项**

-   未定义 `_CC_UNICODE_` 宏时使用此版本
-   格式化字符串使用多字节字符
-   性能敏感场景建议使用静态日志宏

### 日志快捷宏

``` c
/**/
#ifdef _CC_UNICODE_
    #ifdef __CC_MSVC__
        #define _cc_logger(LEVEL, FMT, ...) _cc_loggerW(_CL(_CC_FILE_), _CC_LINE_, LEVEL, _CL(FMT), ##__VA_ARGS__)
    #else
        #define _cc_logger(LEVEL, FMT, ARGS...) _cc_loggerW(_CL(_CC_FILE_), _CC_LINE_, LEVEL, _CL(FMT), ##ARGS)
    #endif
    
    #define _cc_logger_warin _cc_loggerW_warin
    #define _cc_logger_debug _cc_loggerW_debug
    #define _cc_logger_info _cc_loggerW_info
    #define _cc_logger_error _cc_loggerW_error
    #define _cc_logger_alert _cc_loggerW_alert
#else
    #ifdef __CC_MSVC__
        #define _cc_logger(LEVEL, FMT, ...) _cc_loggerA(_CC_FILE_, _CC_LINE_, LEVEL, FMT, ##__VA_ARGS__)
    #else
        #define _cc_logger(LEVEL, FMT, ARGS...) _cc_loggerA(_CC_FILE_, _CC_LINE_, LEVEL, FMT, ##ARGS)
    #endif

    #define _cc_logger_warin _cc_loggerA_warin
    #define _cc_logger_debug _cc_loggerA_debug
    #define _cc_logger_info _cc_loggerA_info
    #define _cc_logger_error _cc_loggerA_error
    #define _cc_logger_alert _cc_loggerA_alert
#endif
```

**宏说明**

-   `_cc_logger` - 自动选择宽字符或多字节版本
-   `_cc_logger_*` - 各日志级别的快捷宏

**示例代码**

``` c
// 动态字符串示例
_cc_logger_error("Error code: %d", errno);

// 快捷宏示例
_cc_logger_debug("Debug value: %f", debug_value);
_cc_logger_alert("Critical alert!");
```

**性能建议**

## Syslog
Syslog（系统日志）是一种用于在互联网协议（TCP/IP）网络中传递记录消息的标准协议，它采用主从式架构，允许设备和应用程序将事件消息发送到中央日志服务器进行集中管理和分析

支持 [RFC 3164](https://www.rfc-editor.org/rfc/rfc3164.txt) 和 [RFC 5424](https://www.rfc-editor.org/rfc/rfc5424.txt)协议
``` c
    // Facility
    enum {
        _CC_LOG_FACILITY_KERN_ = 0, // Kernel messages
        _CC_LOG_FACILITY_USER_,     // User-level messages
        _CC_LOG_FACILITY_MAIL_,     // Mail system
        _CC_LOG_FACILITY_DAEMON_,   // System daemons
        _CC_LOG_FACILITY_AUTH_,     // Security/authentication messages
        _CC_LOG_FACILITY_SYSLOG_,   // Messages generated internally by syslogd
        _CC_LOG_FACILITY_LPR_,      // Line printer subsystem
        _CC_LOG_FACILITY_NEWS_,     // Network news subsystem
        _CC_LOG_FACILITY_UUCP_,     // UUCP subsystem
        _CC_LOG_FACILITY_CRON_,     // Clock daemon
        _CC_LOG_FACILITY_AUTHPRIV_, // Security/authentication messages
        _CC_LOG_FACILITY_FTP_,      // FTP daemon
        _CC_LOG_FACILITY_NTP_,      // NTP subsystem

        _CC_LOG_FACILITY_SECURITY_, // Log audit
        _CC_LOG_FACILITY_CONSOLE_,  // Log console

        // //Locally-used facilities
        _CC_LOG_FACILITY_LOCAL0_ = 16,
        _CC_LOG_FACILITY_LOCAL1_,
        _CC_LOG_FACILITY_LOCAL2_,
        _CC_LOG_FACILITY_LOCAL3_,
        _CC_LOG_FACILITY_LOCAL4_,
        _CC_LOG_FACILITY_LOCAL5_,
        _CC_LOG_FACILITY_LOCAL6_,
        _CC_LOG_FACILITY_LOCAL7_ = 23 // Memory tracking
    };

    // Severity Level
    enum {
        _CC_LOG_LEVEL_EMERG_ = 0, // System is unusable
        _CC_LOG_LEVEL_ALERT_,     // Action must be taken immediately
        _CC_LOG_LEVEL_CRIT_,      // Critical
        _CC_LOG_LEVEL_ERROR_,     // Error
        _CC_LOG_LEVEL_WARNING_,   // Warning
        _CC_LOG_LEVEL_NOTICE_,    // Normal but significant condition
        _CC_LOG_LEVEL_INFO_,      // Informational messages
        _CC_LOG_LEVEL_DEBUG_      // Debug-level messages
    };
```

### \_cc_syslog_header()

``` c
int syslog_pid = _cc_getpid();
int syslog_fd = 0;
char_t *syslog_host = "libcc";
char_t *syslog_app = "app";

_CC_API_PUBLIC(size_t) _cc_syslog_header(uint8_t pri, tchar_t *buffer, size_t buffer_length) {
#ifndef _CC_SYSLOG_RFC5424_
    tchar_t syslog_timestamp[64];
#endif
    struct tm tm_now;
    time_t now = time(NULL);

    _cc_localtime(&now, &tm_now);

#ifdef _CC_SYSLOG_RFC5424_
    // RFC 5424
    return _sntprintf(buffer, buffer_length, _T("<%d>%d %04d-%02d-%02dT%02d:%02d:%02dZ %s %s %d ID:%lld "),
                                (int)pri, _CC_SYSLOG_VERSIOV_, 
                                tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                                tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, 
                                syslog_host, syslog_app, syslog_pid, syslog_fd);
#else
    // RFC 3164
    _tcsftime(syslog_timestamp, _cc_countof(syslog_timestamp), _T("%b %d %H:%M:%S"), &tm_now);
    return _sntprintf(buffer, buffer_length, _T("<%d>%s %s %s[%d]: "),
                                (int)pri, syslog_timestamp, syslog_host, syslog_app, syslog_pid);
#endif
}
```
生成符合 RFC 标准的 Syslog 消息头。

**参数说明**

-   `pri` - 消息优先级，计算公式：`Facility × 8 + Severity`
-   `buffer` - 存储消息头的缓冲区，需预先分配
-   `buffer_length` - 缓冲区大小，建议至少 1024 字节

**返回值**

-   **成功**：返回消息头长度
-   **失败**：返回 0，表示缓冲区不足或参数无效

**示例代码**

``` c
tchar_t header[1024];
size_t len = _cc_syslog_header(_CC_SYSLOG_PRI(_CC_LOG_FACILITY_USER_,_CC_LOG_LEVEL_INFO), header, 1024);
if (len == 0) {
    printf("Failed to format syslog header\n");
}
```
## Utility