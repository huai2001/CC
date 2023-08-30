# TCP 服务端

> 多线程 TCP echo 服务例子

## TCP Event 定义
```C
//定义一个结构体
typedef struct _network_event {
    _cc_event_cycle_t cycle;
    _cc_thread_t *t;
    bool_t keep_active;
}network_event_t;

//线程生命周期变量
static bool_t keep_active = true;
//线程最大数量
static int32_t max_thread_count = 0;
static _cc_atomic32_t max_network_event_refcount = 0;

static network_event_t *network_event = NULL;
```

多线程回调函数
------
```c
static int32_t thread_running(_cc_thread_t *t, pvoid_t args)
{
    /*创建线程锁*/
    network_event_t *n = (network_event_t*)args;

    n->keep_active = keep_active;

    while(keep_active)
        _cc_event_wait((_cc_event_cycle_t*)&n->cycle, 100);

    n->keep_active = false;

    /*释放事件资源*/
    n->cycle.driver.quit(&n->cycle);
    return 0;
}
```

Event 功能函数
--------
```c
static _cc_event_cycle_t *get_event_cycle()
{
    network_event_t *n = NULL;
    do {
        n = (network_event + ((++max_network_event_refcount) % max_thread_count));
    }while (n->keep_active == false);
    
    return &n->cycle;
}

static bool_t send_data(_cc_event_cycle_t *cycle, _cc_event_t *e, byte_t *buf, int32_t len)
{
    if (_cc_event_send(e, buf, len) > 0){
        return true;
    }

    return false;
}
```

TCP监听端口
--------
```c
/**/
_cc_event_t* _tcp_listen(const tchar_t *host, uint16_t port, uint32_t timeout, _cc_event_callback_t callback, pvoid_t)
{
    _cc_event_t *e = NULL;
    _cc_event_cycle_t *cycle = get_event_cycle();
    _cc_socket_t fd = CC_INVALID_SOCKET;
    struct sockaddr_in sin;
    bzero(&sin, sizeof(struct sockaddr_in));

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;

    if(bind(fd, (struct sockaddr*)&sin, sizeof(sin)) != CC_SOCKET_ERROR) {
        listen(fd, SOMAXCONN);
        e = cycle->driver.add(cycle, CC_EVENT_ACCEPT, fd, timeout, callback, args);
    } else {
        int32_t err = _cc_last_errno();
        CC_ERROR_LOG(_T("socket bind port(%d) error(%d) %s"), port, err, _cc_last_error(err));
    }
    return e;
}
```

TCP事件回调函数
--------
```c
static bool_t network_event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t events)
{
    /*新连接事件*/
    if (events & CC_EVENT_ACCEPT) {
        _cc_socket_t fd = CC_INVALID_SOCKET;
        _cc_sockaddr_t remote_addr = {0};
        _cc_socklen_t remote_addr_len = sizeof(_cc_sockaddr_t);
        _cc_event_cycle_t *cycle = get_event_cycle();

        fd = _cc_event_accept(cycle, e, &remote_addr, &remote_addr_len);
        if (fd == CC_INVALID_SOCKET) {
            _tprintf(_T("thread %d accept fail.\n"), _cc_get_thread_id(NULL));
            return true;
        }

        _cc_set_socket_nonblock(fd, 1);
        if (!cycle->driver.add(cycle, CC_EVENT_TIMEOUT|_CC_EVENT_READABLE_|_CC_EVENT_BUFFER_, fd, 30000, network_event_callback, NULL)) {
            _tprintf(_T("thread %d add socket (%d) event fial.\n"), _cc_get_thread_id(NULL), fd);
            _cc_close_socket(fd);
            return true;
        }

        {
            struct sockaddr_in* remote_ip = (struct sockaddr_in*)&remote_addr;
            byte_t *ip_addr = (byte_t *)&remote_ip->sin_addr.s_addr;
            _tprintf(_T("TCP accept [%d,%d,%d,%d] fd:%d\n"), ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], fd);
        }
        return true;
    }

    /*无法连接*/
    if (events & _CC_EVENT_DISCONNECT_) {
        _tprintf(_T("%d disconnect to client.\n"), e->fd);
        
        return false;
    }

    /*有数据可以读*/
    if (events & CC_EVENT_READ) {
        if (_cc_event_recv(e) <= 0) {
            _tprintf(_T("TCP close %d\n"), e->fd);
            _cc_event_delete(cycle, e);
            return false;
        }
        
        send_data(cycle, e, e->buffer->r.buf, e->buffer->r.length);
        e->buffer->r.length = 0;
        
        return true;
    }

    /*可写数据*/
    if (events & CC_EVENT_WRITE) {

    }

    /*连接超时*/
    if (events & CC_EVENT_TIMEOUT) {
        _tprintf(_T("TCP timeout %d\n"), e->fd);
        return false;
    }

    return true;
}
```

主函数
--------
```c
int main (int argc, char * const argv[])
{
    char c = 0;
    int32_t i = 0;
    /*初始化系统网络*/
    _cc_install_socket();

    /*获取CPU线程数量*/
    max_thread_count = _cc_cpu_count() * 2;
    /*分配内存*/
    network_event = _cc_malloc(sizeof(network_event_t) * max_thread_count);

    keep_active = true;

    /*启动线程*/
    for (i = 0; i < max_thread_count; ++i) {
        network_event_t *n = (network_event_t*)(network_event + i);
        /*初始化事件select iocp, epoll, kqueue*/
        if (_cc_init_event_poller(&n->cycle) == false) {
            continue;
        }
        n->t = _cc_create_thread(thread_running, "network", n);
    }

    _tcp_listen(NULL, 8088, 180000,network_event_callback, NULL);
    _tprintf(_T("ListenPort:%d\n"), 8088);

    /*等待用户输入q退出*/
    while((c = getchar()) != 'q') {
        _cc_sleep(100);
    }

    /*释放资源*/
    keep_active = false;

    /*等待线程退出*/
    for (i = 0; i < max_thread_count; ++i) {
        network_event_t *n = (network_event_t*)(network_event + i);
        if (n->t) {
            _cc_wait_thread(n->t, NULL);
        }
    }

    /*卸载系统网络*/
    _cc_uninstall_socket();
    
    return 0;
}
```