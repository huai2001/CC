#include <stdio.h>
#include <libcc.h>
#include <locale.h>

#define MAX_MSG_LENGTH 1024

static char *channel = "#dyadbots";
//定义一个事件对象
static _cc_thread_t* network_thread;
static _cc_event_cycle_t network_event;
static _cc_event_t *curr_event;
//线程生命周期变量
static bool_t keep_active = true;
static int is_registered = 0;

int32_t thread_running(_cc_thread_t *t, pvoid_t args) {
    while(keep_active)
        _cc_event_wait((_cc_event_cycle_t*)args, 100);
    
    return 0;
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
        /*if (buf[len - 1] == '\n') {
            buf[len - 1] = 0;
        }*/
        return _cc_event_send(e, (byte_t*)buf, (len * sizeof(char)));
    }
    return true;
}

static bool_t close_event(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    if (curr_event == e) {
        curr_event = nullptr;
        _putts(_T("Press 'enter' key to exit\n"));
    }
    
    return false;
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

    /* Handle RPL_WELCOME */
    if (!is_registered && strstr(data, "001")) {
        /* Join channel */
        len = _sntprintf(buf, _cc_countof(buf), _T("JOIN %s\r\n"), channel);
        _cc_event_send(e, (byte_t*)buf, len);
        is_registered = 1;
    }
}

static bool_t network_event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t which) {
    /*成功连接服务器*/
    if (which & _CC_EVENT_CONNECTED_) {
        tchar_t data[1024];
        tchar_t nick[32];
        uint16_t len;
        
        _sntprintf(nick, _cc_countof(nick), _T("CCBot %04X"), ((uint32_t)time(nullptr)) % 0xffff);
        len = _sntprintf(data, _cc_countof(data), _T("NICK %s\r\nUSER %s %s bla : %s\r\n"), 
            nick, nick, nick, nick);

        curr_event = e;
        printf("send: %s, %s\n", nick, data);
        return _cc_event_send(e, (byte_t*)data, len);
    }
    
    /*无法连接*/
    if (which & _CC_EVENT_DISCONNECT_) {
        _tprintf(_T("Disconnect to server: %s.\n"), _cc_last_error(_cc_last_errno()));
        return close_event(cycle, e);
    }
    
    /*有数据可以读*/
    if (which & _CC_EVENT_READABLE_) {
        if (!_cc_event_recv(e)) {
            _tprintf(_T("TCP close %d\n"), e->fd);
            return close_event(cycle, e);
        }

        if (e->buffer && e->buffer->r.length > 0) {
            _cc_event_rbuf_t *r = &e->buffer->r;
            int i;
            int start;
            
            start = 0;
            r->bytes[r->length - 1] = 0;

            for (i = 0; i < r->length; i++) {
                if (r->bytes[i] == '\n') {
                    r->bytes[i] = 0;
                    onLine(e, (char_t*)&r->bytes[start], i - start);
                    start = i + 1;
                }
            }

            i = r->length - start;
            if (i > 0) {
                memmove(r->bytes, &r->bytes[start], i);
            }
            r->length = i;
        }
        
        return true;
    }
    
    /*可写数据*/
    if (which & _CC_EVENT_WRITABLE_) {
        _cc_event_sendbuf(e);
        return true;
    }
    
    /*连接超时*/
    if (which & _CC_EVENT_TIMEOUT_) {
        _tprintf(_T("TCP timeout %d\n"), e->fd);
        return close_event(cycle, e);
    }
    
    return true;
}

int main (int argc, char * const argv[]) {
    struct sockaddr_in adress;
    /*初始化系统网络*/
    _cc_install_socket();

    keep_active = true;

    /*初始化事件 select iocp, epoll, kqueue */
    if (_cc_init_event_poller(&network_event) == false) {
        _cc_uninstall_socket();
        return 0;
    }
    /*启动线程*/
    network_thread = _cc_create_thread(thread_running, "irc client", &network_event);
    printf("starting \n");
/*
    {

        int sockfd;
        _cc_inet_ipv4_addr(&adress, "202.103.224.72", 80);

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd) {
            int flags = ioctl(sockfd, F_GETFL, 0);
            ioctl(sockfd, F_SETFL, flags | O_NONBLOCK);

            connect(sockfd, (struct sockaddr*)&adress, sizeof(struct sockaddr_in));
        }
        printf("OK1\n");
    }
*/
    _cc_inet_ipv4_addr(&adress, "202.103.224.72", 80);
    if (_cc_tcp_connect(&network_event, _CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_|_CC_EVENT_BUFFER_, (_cc_sockadr_t*)&adress, 60000, network_event_callback, nullptr)) {
        printf("OK\n");
        fflush(stdout);
        fflush(stdin);
        while (curr_event) {
            if (!send_message(curr_event)) {
                break;
            }
        }
    }

    printf("disconnected\n");

    keep_active = false;
    _cc_sleep(1000);

    /*释放资源*/
    keep_active = false;
    /*等待线程退出*/
    _cc_wait_thread(network_thread, nullptr);
    /*释放事件资源*/
    network_event.delegator.quit(&network_event);
    /*卸载系统网络*/
    _cc_uninstall_socket();
    
    return 0;
}
