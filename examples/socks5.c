#include <stdio.h>
#include <libcc.h>
#include <locale.h>

//定义一个事件对象
static _cc_event_cycle_t network_event;
//定义一个线程
static _cc_thread_t *network_thread = nullptr;
//线程生命周期变量
static bool_t keep_active = true;

static int socks5_status = 0;

int32_t thread_running(_cc_thread_t *t, pvoid_t args) {
    while(keep_active)
        _cc_event_wait((_cc_event_cycle_t*)args, 100);
    
    return 0;
}

/*
https://www.ietf.org/rfc/rfc1928.txt
+----+----------+----------+
|VER | NMETHODS | METHODS  |
+----+----------+----------+
| 1  |    1     | 1 to 255 |
+----+----------+----------+
METHOD:
 0x00 不需要认证（常用）
 0x01 GSSAPI认证
 0x02 账号密码认证（常用）
 0x03 - 0x7F IANA分配
 0x80 - 0xFE 私有方法保留
 0xFF 无支持的认证方法
*/

typedef struct _cc_socks5_methods {
    byte_t version;
    byte_t n;
    byte_t auth;
} _cc_socks5_methods_t;

bool_t send_socks5(_cc_event_cycle_t *cycle, _cc_event_t *e, byte_t *data, int32_t len) {
    int32_t sent = _cc_event_send(e, data, len);
    if (sent > 0){
        return true;
    }
    return false;
}

static bool_t send_socks5_methods(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_socks5_methods_t method;
    method.version = 0x05;
    method.n = 0x01;
    method.auth = 0x02;
    
    buf[i++] = 0x05;
    buf[i++] = 0x01;
    buf[i++] = 0x02;
    
    return send_socks5(cycle, e, (byte_t*)&method, sizeof(_cc_socks5_methods_t));
}

/*
+----+-----------------+-------------+-----------------+------------+
|VER | USERNAME_LENGTH |  USERNAME   | PASSWORD_LENGTH |  PASSWORD  |
+----+-----------------+-------------+-----------------+------------+
| 1  |        1        |  1 to 255   |        1        |  1 to 255  |
+----+-----------------+-------------+-----------------+------------+
*/
static bool_t send_socks5_auth(_cc_event_cycle_t *cycle, _cc_event_t *e, const tchar_t *user_name, const tchar_t *password) {
    byte_t buf[1024];
    byte_t *p;
    byte_t c = 0,len = 0;
    int32_t i = 0;
    
    buf[i++] = 0x01;
    p = buf[i++];
    while(user_name && *user_name != 0) {
        buf[i++] = *user_name++;
        len++;
    }
    *p = len;

    len = 0;
    p = buf[i++];
    while(password && *password != 0) {
        buf[i++] = *password++;
        len++;
    }
    *p = len;
    
    return send_socks5(cycle, e, buf, i);
}

/*
socks5协议部分（5.客户端认证成功后开始进行请求）
+----+-----+-------+------+----------+----------+
|VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
+----+-----+-------+------+----------+----------+
| 1  |  1  | X'00' |  1   | Variable |    2     |
+----+-----+-------+------+----------+----------+
VER.是SOCKS协议版本，这里应该是0x05.
CMD.是SOCKS的命令码:
    0x01:表示CONNECT请求
    0x02:表示BIND请求
    0x03:表示UDP转发
RSV.0x00:保留，无实际作用
ATYP.DST.ADDR类型:
    0x01:表示IPV4地址
    0x03:表示域名格式
    0x04:表示IPV6地址
DST.ADDR.目的地址
    当ATYP=0x01 DST.ADDR部分为四字节长度，内容为IP本身
    当ATYP=0x03 第一个部分为一个1字节表示域名长度，第二部分就是剩余内容为具体域名。Active表示长度不定。没有\0作为结尾
DST.PORT 网络字节序表示的目的端口
*/
static bool_t send_socks5_connect(_cc_event_cycle_t *cycle, _cc_event_t *e, const tchar_t *domain, int16_t port){
    byte_t buf[128];
    byte_t c = 0,len = 0;
    byte_t *p;
    int32_t i = 0;
    
    buf[i++] = 0x05;
    buf[i++] = 0x01;
    buf[i++] = 0x00;
    buf[i++] = 0x03;
    
    p = buf[i++];
    while(domain && *domain != 0) {
        buf[i++] = *domain++;
        len++;
    }
    *p = len;
    
    buf[i++] = (byte_t)(port/256);
    buf[i++] = (byte_t)(port%256);
    
    return send_socks5(cycle, e, buf, i);
}

/*
+----+-----+-------+------+----------+----------+
|VER | REP |  RSV  | ATYP | BND.ADDR | BND.PORT |
+----+-----+-------+------+----------+----------+
| 1  |  1  | X'00' |  1   | Variable |    2     |
+----+-----+-------+------+----------+----------+

RESPONSE 响应命令
    0x00 代理服务器连接目标服务器成功
    0x01 代理服务器故障
    0x02 代理服务器规则集不允许连接
    0x03 网络无法访问
    0x04 目标服务器无法访问（主机名无效）
    0x05 连接目标服务器被拒绝
    0x06 TTL已过期
    0x07 不支持的命令
    0x08 不支持的目标服务器地址类型
    0x09 - 0xFF 未分配
RSV 保留字段
BND.ADDR 代理服务器连接目标服务器成功后的代理服务器IP
BND.PORT 代理服务器连接目标服务器成功后的代理服务器端口
*/

//send_socks5_request(cycle, e, "www.ip138.com", 80);

static bool_t network_event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t which) {
    /*成功连接服务器*/
    if (which & _CC_EVENT_CONNECTED_) {
        _tprintf(_T("%d connect to server.\n"), e->fd);
        return send_socks5_methods(cycle, e);
    }
    
    /*无法连接*/
    if (which & _CC_EVENT_DISCONNECT_) {
        _tprintf(_T("%d disconnect to server.\n"), e->fd);
        return false;
    }
    
    /*有数据可以读*/
    if (which & _CC_EVENT_READABLE_) {
        if (!_cc_event_recv(e)) {
            _tprintf(_T("TCP close %d\n"), e->fd);
            return false;
        }
        
        if (e->buffer) {
            e->buffer->r.bytes[e->buffer->r.length] = 0;
            e->buffer->r.length = 0;
            byte_t *s = e->buffer->r.bytes;
            switch(socks5_status) {
                case 0:{
                    if (*s == 0x05 && *(s + 1) == 0x02) {
                        socks5_status = 1;
                        return send_socks5_auth(cycle, e, "account", "123654aa");
                    }
                }
                    break;
                case 1: {
                    if (*s == 0x01 && *(s + 1) == 0x00) {
                        socks5_status = 2;
                        return send_socks5_connect(cycle, e, "2000019.ip138.com", 80);
                    }
                    break;
                }
                case 2:{
                    if (*s == 0x05 && *(s + 1) == 0x00) {
                        socks5_status = 3;
                        const char_t *http = "GET / HTTP/1.1\r\nHost: 2000019.ip138.com\r\nConnection: close\r\n\r\n";
                        return send_socks5(cycle, e, (byte_t*)http, (int32_t)(sizeof(char_t)*strlen(http)));
                    }
                    break;
                default:
                    printf("%s", e->buffer->r.bytes);
                }
            }
        }
        
        return true;
    }
    /*可写数据*/
    if (which & _CC_EVENT_WRITABLE_) {
        _cc_event_send(e, nullptr, 0);
        return true;
    }
    
    /*连接超时*/
    if (which & _CC_EVENT_TIMEOUT_) {
        _tprintf(_T("TCP timeout %d\n"), e->fd);
        return false;
    }
    
    return true;
}

static _cc_event_t *connect_server(const tchar_t* addr, uint16_t port) {
    struct sockaddr_in dest;
    _cc_socket_t fd = _CC_INVALID_SOCKET_;
    
    /*TCP*/
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (!fd){
        printf("socket %d\n", fd);
        return nullptr;
    }
    
    _cc_set_socket_nonblock(fd, true);
    
    _cc_inet_ipv4_addr(&dest, addr, port);
    
    return network_event.connect(&network_event, _CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_|_CC_EVENT_BUFFER_, fd,
                                        120000, network_event_callback, nullptr,
                                        (const _cc_sockaddr_t*)&dest, sizeof(struct sockaddr_in));
}

int main (int argc, char * const argv[]) {
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
    network_thread = _cc_thread(thread_running, "socks5", &network_event);
    
    /*连接到服务端口为8088*/
    if(connect_server(_T("47.96.85.231"), 3128) == nullptr) {
        _tprintf(_T("Unable to connect to the network port 3128\n"));
    }

    c = 0;
    /*等待用户输入q退出*/
    while((c = getchar()) != 'q') {
        _cc_sleep(1000);
    }
    
    /*释放资源*/
    keep_active = false;
    /*等待线程退出*/
    _cc_wait_thread(network_thread, nullptr);
    /*释放事件资源*/
    network_event.quit(&network_event);
    /*卸载系统网络*/
    _cc_uninstall_socket();
    
    return 0;
}
