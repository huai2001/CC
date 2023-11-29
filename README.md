****
# libcc

![platform](https://img.shields.io/badge/platform-linux%20%7C%20windows%20%7C%20macos-blue)
<br>

`libcc`是跨平台，多线程，轻量级的C语言库，提供了更简单的接口和更丰富的协议。提供的函数功能包括：字符串、日志、双向/单向链表、哈希表、网络通信、JSON、XML、INI配置文件读取、AES、DES、MD2、MD4、MD5、base16/base64编码/解码、url编码/解码、时间轮计时器等等。详细信息请参阅C文件
```C
//跨平台，超轻量，易扩展，框架代码如下：
#define CALL(fn, ...) fn(__VA_ARGS__)

//使用示例：输出hello World!
CALL(printf, "Hello World!\n");
/*
 * 开一个玩笑:)！
 * 
 * 只是出于对C的热爱，累积一份属于自己的C代码
 */
```

## ✨ 特征

- 跨平台（Linux, Windows, MacOS, FreeBSD, Android, iOS）
- 高性能事件循环（网络IO事件、定时器事件）
- TCP/UDP服务端/客户端/代理
- TCP支持心跳、转发、拆包、多线程安全write和close等特性

## ⌛️ 安装教程

见[REANDME.Makefile](REANDME.Makefile)

libcc提供了以下构建方式:

```shell
#1、通过Makefile:编译 (Linux,freeBSD,macOS)
cd yourdirname/CC/build
make path
./build.sh

#2、通过VS2010编译 (Windows)
proj.Win/libcc_vs2010.vcxproj

#3、通过Android JNI编译
#打开 .bash_profile 配置 $NDK_ROOT = (Android NDK目录)
cd proj.Android/JNI
./build_NDK.sh

#4、通过Xcode 编译(macOS,iOS)
proj.OSX/cc.xcodeproj
proj.IOS/cc.xcodeproj

#5、ubuntu 编译
sudo apt-get install libmysqlclient-dev
sudo apt-get install sqlite3
sudo apt-get install libsqlite3-dev 
sudo apt-get install libjpeg-dev
sudo apt-get install libpng-dev

```

## ⚡️ 入门与体验

```shell
# 下载编译
git clone https://github.com/huai2001/CC.git
cd CC/build
make path
./build.sh
```
#### Support
Email: [huai2011@163.com](mailto:huai2011@163.com)

## ✨ TCP Client
```C
#include <libcc.h>

#define MAX_MSG_LENGTH 1024

static bool_t keep_active = true;
static _cc_event_t *curr_event;

static void onLine(_cc_event_t *e, const char_t* data, uint16_t length) {
    tchar_t buf[1024];
    int32_t len;

    printf("%s\n", data);

    /* Handle PING */
    if (!memcmp(data, "PING", 4)) {
        len = _sntprintf(buf, _cc_countof(buf), _T("PONG%s\r\n"), data + 4);
        _cc_event_send(e, (byte_t*)buf, len);
    }
}

static bool_t send_message(_cc_event_t *e) {
    int32_t len = 0;
    char buf[MAX_MSG_LENGTH];
    
    while (fgets(buf, MAX_MSG_LENGTH, stdin) == buf) {
        if (strncmp(buf, "exit", 4) == 0) return false;
        
        if (e && e->fd == _CC_INVALID_SOCKET_) {
            keep_active = false;
            return false;
        }
        
        len = (int32_t)strlen(buf);
        return _cc_event_send(e, (byte_t*)buf, (len * sizeof(char)));
    }
    return true;
}

static bool_t _callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t events) {
    if (events & _CC_EVENT_CONNECT_) {
        _tprintf(_T(" connect to server!\n"));
        curr_event = e;
    }
    
    if (events & _CC_EVENT_DISCONNECT_) {
        _tprintf(_T("Disconnect to server: %d.\n"), e->fd);
        keep_active = false;
        return false;
    }

    if (events & _CC_EVENT_READABLE_) {
        if (!_cc_event_recv(e)) {
            _tprintf(_T("TCP close %d\n"), e->fd);
            keep_active = false;
            return false;
        }

        if (e->buffer && e->buffer->r.length > 0) {
            _cc_event_rbuf_t *r = &e->buffer->r;
            int i;
            int start;
            start = 0;
            r->buf[r->length] = 0;

            for (i = 0; i < r->length; i++) {
                if (r->buf[i] == '\n') {
                    r->buf[i] = 0;
                    onLine(e, (char_t*)&r->buf[start], i - start);
                    start = i + 1;
                }
            }

            i = r->length - start;
            if (i > 0) {
                memmove(r->buf, &r->buf[start], i);
            }
            r->length = i;
        }
    }

    if (events & _CC_EVENT_WRITABLE_) {
        if (_cc_event_sendbuf(e) < 0) {
            return false;
        }
    }

    if (events & _CC_EVENT_TIMEOUT_) {
        _tprintf(_T("TCP Timeout - %d\n"), e->fd);
        return false;
    }

    return true;
}

int _tmain (int argc, tchar_t * const argv[]) {
    _cc_event_cycle_t cycle;
    struct sockaddr_in sa;
    curr_event = NULL;

    _cc_install_socket();

    if (!_cc_init_event_poller(&cycle)) {
        return 0;
    }
    _cc_inet_ipv4_addr(&sa, "127.0.0.1", 8088);
    curr_event = _cc_alloc_event(cycle,  _CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_);
    if (curr_event == NULL) {
        return 0;
    }
    curr_event->callback = _callback;
    curr_event->timeout = 6000;

    if (!_cc_tcp_connect(&cycle, curr_event, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
        _cc_free_event(&cycle, curr_event);
        return;
    }

    while(keep_active) {
        _cc_event_wait(&cycle, 100);
        if (curr_event) {
            send_message(curr_event);
        }
    }

    cycle.driver.quit(&cycle);
    _cc_uninstall_socket();
    return 1;
}
```
## ✨ TCP Server
```C

#include <libcc.h>

static bool_t keep_active = true;

static bool_t ping(_cc_event_t *e) {
    tchar_t buf[1024];
    int32_t len;

    /* Handle PING */
    len = _sntprintf(buf, _cc_countof(buf), _T("PING%d\r\n"), (rand() % 1000) + 100);
    return _cc_event_send(e, (byte_t*)buf, len) >= 0;
}


static void onLine(_cc_event_t *e, const char_t* data, uint16_t length) {
    tchar_t buf[1024];
    int32_t len;

    printf("%s\n", data);

    /* Handle PING */
    if (!memcmp(data, "PING", 4)) {
        len = _sntprintf(buf, _cc_countof(buf), _T("PONG%s\r\n"), data + 4);
        _cc_event_send(e, (byte_t*)buf, len);
    }
}

static bool_t _callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t events) {
    if (events & _CC_EVENT_ACCEPT_) {
        _cc_socket_t fd;
        _cc_sockaddr_t remote_addr = {0};
        _cc_socklen_t remote_addr_len = sizeof(_cc_sockaddr_t);
        _cc_event_cycle_t *cycle = _cc_get_event_cycle();
        fd = _cc_event_accept(cycle, e, &remote_addr, &remote_addr_len);
        if (fd == _CC_INVALID_SOCKET_) {
            _cc_logger_debug(_T("accept fail %s."), _cc_last_error(_cc_last_errno()));
            return true;
        }
        
        _cc_set_socket_nonblock(fd, 1);

        if (cycle->driver.add(cycle, _CC_EVENT_TIMEOUT_|_CC_EVENT_READABLE_|_CC_EVENT_BUFFER_, fd, e->timeout, _callback, NULL) == NULL) {
            _cc_logger_debug(_T("thread %d add socket (%d) event fial."), _cc_get_thread_id(NULL), fd);
            return true;
        }

        {
            struct sockaddr_in* remote_ip = (struct sockaddr_in*)&remote_addr;
            byte_t *ip_addr = (byte_t *)&remote_ip->sin_addr.s_addr;
            _cc_logger_debug(_T("TCP accept [%d,%d,%d,%d] fd:%d"), ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], fd);
        }
        return true;
    }

    if (events & _CC_EVENT_DISCONNECT_) {
        _tprintf(_T("Disconnect to server: %d.\n"), e->fd);
        return false;
    }

    if (events & _CC_EVENT_READABLE_) {
        if (!_cc_event_recv(e)) {
            _tprintf(_T("TCP close %d\n"), e->fd);
            return false;
        }

        if (e->buffer->r.length > 0) {
            _cc_event_rbuf_t *r = &e->buffer->r;
            int i;
            int start;
            start = 0;
            r->buf[r->length] = 0;

            for (i = 0; i < r->length; i++) {
                if (r->buf[i] == '\n') {
                    r->buf[i] = 0;
                    onLine(e, (char_t*)&r->buf[start], i - start);
                    start = i + 1;
                }
            }

            i = r->length - start;
            if (i > 0) {
                memmove(r->buf, &r->buf[start], i);
            }
            r->length = i;
        }
    }

    if (events & _CC_EVENT_WRITABLE_) {
        if (_cc_event_sendbuf(e) < 0) {
            return false;
        }
    }

    if (events & _CC_EVENT_TIMEOUT_) {
        _tprintf(_T("TCP Timeout - %d\n"), e->fd);
        return ping(e);
    }

    return true;
}

int _tmain (int argc, tchar_t * const argv[]) {
    _cc_event_cycle_t cycle;
    _cc_event_t *e;
    struct sockaddr_in sa;

    _cc_install_socket();

    if (!_cc_init_event_poller(&cycle)) {
        return 0;
    }

    e = _cc_alloc_event(&cycle, _CC_EVENT_ACCEPT_);
    if (e == NULL) {
        return - 1;
    }
    e->timeout = 60000;
    e->callback = network_event_callback;
    
    _cc_inet_ipv4_addr(&sa, NULL, 8088);
    if (!_cc_tcp_listen(&cycle, e, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in))) {
        _cc_free_event(&cycle, e);
        return -1;
    }

    _cc_logger_debug(_T("listen port: %d"), 8088);

    while(keep_active) {
        _cc_event_wait(&cycle, 100);
    }

    cycle.driver.quit(&cycle);
    _cc_uninstall_socket();
    return 1;
}
```
功能函数
------
