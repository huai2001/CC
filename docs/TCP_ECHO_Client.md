# TCP 客户端

> 多线程 TCP echo 客户端例子

## TCP Event 定义
```C
//定义一个事件对象
static _cc_event_cycle_t network_event;
//定义一个线程
static _cc_thread_t *network_thread = NULL;
//线程生命周期变量
static bool_t keep_active = true;
```

多线程回调函数
------
```c
int32_t thread_running(_cc_thread_t *t, pvoid_t args)
{
    while(keep_active)
        _cc_event_wait((_cc_event_cycle_t*)args, 100);

    return 0;
}
```

TCP 发送函数
------
```c
static bool_t send_data(_cc_event_cycle_t *cycle, _cc_event_t *e)
{
    tchar_t *send_str = _T("testes");
    if (_cc_event_send(e, send_str, sizeof(tchar_t) * strlen(send_str)) > 0){
        return true;
    }
    return false;
}
```

TCP事件回调函数
--------
```c
static bool_t network_event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t events)
{
    /*成功连接服务器*/
    if (events & CC_EVENT_CONNECTED) {
        _tprintf(_T("%d connect to server.\n"), e->fd);
        
        return send_data(cycle, e);
    }
    /*无法连接*/
    if (events & _CC_EVENT_DISCONNECT_) {
        _tprintf(_T("%d disconnect to server.\n"), e->fd);
        
        return false;
    }
    
    /*有数据可以读*/
    if (events & CC_EVENT_READ) {
        int32_t len = 0;
        tchar_t buf[256];
        len = _cc_recv(e->fd, buf, sizeof(buf));
        if (len <= 0) {
            _tprintf(_T("TCP close %d\n"), e->fd);
            return false;
        }
        buf[len] = 0;
        printf("%s\n", buf);
        return send_data(cycle, e);
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

TCP连接函数
--------
```c
static _cc_event_t *connect_server(uint32_t addr, uint16_t port)
{
    struct sockaddr_in sin;
    _cc_event_t* e = NULL;
    _cc_socket_t fd = CC_INVALID_SOCKET;
    
    /*TCP*/
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    bzero(&sin, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = addr;
    
    e = network_event.driver.connect(&network_event, CC_EVENT_CONNECT|_CC_EVENT_TIMEOUT_, fd, 
                                        120000, network_event_callback, NULL, 
                                        (_cc_sockaddr_t*)&sin, sizeof(struct sockaddr_in));
    if (!e) {
        _cc_close_socket(fd);
    }
    
    return e;
}
```

主函数
--------
```c
int main (int argc, char * const argv[])
{
    char c = 0;
    /*初始化系统网络*/
    _cc_install_socket();
    
    keep_active = true;
    /*初始化事件select iocp, epoll, kqueue*/
    if(_cc_init_event_poller(&network_event) == false) {
        _cc_uninstall_socket();
        return 0;
    }
    /*启动线程*/
    network_thread = _cc_create_thread(thread_running, "network", &network_event);
    
    /*连接到服务端口为8088*/
    if(connect_server(_cc_inet_addr(_T("127.0.0.1")), 8088) == NULL) {
        _tprintf(_T("Unable to connect to the network port 8088\n"));
    }
    
    /*等待用户输入q退出*/
    while((c = getchar()) != 'q') {
        _cc_sleep(100);
    }
    
    /*释放资源*/
    keep_active = false;
    /*等待线程退出*/
    _cc_wait_thread(network_thread, NULL);
    /*释放事件资源*/
    network_event.driver.quit(&network_event);
    /*卸载系统网络*/
    _cc_uninstall_socket();
    
    return 0;
}
```