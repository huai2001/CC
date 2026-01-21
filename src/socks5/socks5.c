#include <libcc/socks5.h>

/*
+----+-----------------+-------------+-----------------+------------+
|VER | USERNAME_LENGTH |  USERNAME   | PASSWORD_LENGTH |  PASSWORD  |
+----+-----------------+-------------+-----------------+------------+
| 1  |        1        |  1 to 255   |        1        |  1 to 255  |
+----+-----------------+-------------+-----------------+------------+
*/
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
bool_t _cc_socks5_response(_cc_socks5_t *socks, _cc_io_buffer_t *io) {
    byte_t *bytes = io->r.bytes;
    byte_t buf[128];
    size_t length;
    if (*bytes != 0x05) {
        return false;
    }
    switch (socks->state) {
        case 0 : { //shakehands
            socks->method = *(bytes + 2);
            socks->state = socks->method == 0x02 ? 1 : 3;

            *(io->w.bytes + io->w.off++) = 0x05;
            *(io->w.bytes + io->w.off++) = socks->method;
        }
        break;
        case 1: {//validate identity
            byte_t user_len = *(bytes + 1);
            byte_t pass_len = *(bytes + 1 + user_len);
            //validate
            socks->state = 3;
            *(io->w.bytes + io->w.off++) = 0x05;
            *(io->w.bytes + io->w.off++) = 0x00;
        }
        break;
        case 3: {
            byte_t atype = *(bytes + 3);
            byte_t rep = 0x07;
            switch (atype) {
                case _CC_SOCKS5_ADDRESS_TYPE_IPV4_: {
                    memcpy(buf, bytes + 4, 4);
                    length = 4;
                    memcpy(&socks->port, (bytes + 7), sizeof(uint16_t));
                }
                break;
                case _CC_SOCKS5_ADDRESS_TYPE_DOMAIN_: {
                    length = *(bytes + 4);
                    memcpy(buf, bytes + 5, length);
                    memcpy(&socks->port, (bytes + 5 + length), sizeof(uint16_t));
                }
                break;
                case _CC_SOCKS5_ADDRESS_TYPE_IPV6_: {
                    length = 16;
                    memcpy(buf, bytes + 4, 16);
                    memcpy(&socks->port, (bytes + 20), 16);
                }
                break;
            }
            buf[length] = 0;
        }
        break;
    }
}
/*

bool_t ProtocolRequest(byte_t *m, _cc_socks5_t *socks, _cc_event_t *e) {
    byte_t atype;
    byte_t rep = 0x07;
    uint16_t port;
    byte_t buf[128];
    byte_t domain_len;
    uint32_t ip;

    if (*m != 0x05) {
        return false;
    }
    atype = *(m + 3);

    switch (atype) {
    case _CC_SOCKS5_ADDRESS_TYPE_IPV4_:
        // socks->port = (uint16_t)((m + 7));
        memcpy(buf, m + 4, 4);
        memcpy(&socks->port, ((m + 7)), sizeof(uint16_t));
        break;
    case _CC_SOCKS5_ADDRESS_TYPE_DOMAIN_: {
        domain_len = *(m + 4);
        // socks->port = (uint16_t)((m + 5 + domain_len));
        memcpy(buf, m + 5, domain_len);
        memcpy(&socks->port, ((m + 5 + domain_len)), sizeof(uint16_t));
        buf[domain_len] = 0;

    } break;
    case _CC_SOCKS5_ADDRESS_TYPE_IPV6_: {
        // socks->port = (uint16_t)(*(m + 3 + 16));
        memcpy(buf, m + 3, 16);
        memcpy(&socks->port, ((m + 19)), sizeof(uint16_t));
        buf[16] = 0;

    } break;
    default:
        rep = 0x08;
        break;
    }

    if (rep == 0x07) {
        socks->port = _cc_swap16(socks->port);
        _cc_logger_debug("IP:%s, Port:%d.", buf, socks->port);
        if (atype == _CC_SOCKS5_ADDRESS_TYPE_IPV6_) {
            _cc_inet_ipv6_addr(&socks->addr_in6, (const tchar_t *)buf, socks->port);
        } else {
            _cc_inet_ipv4_addr(&socks->addr_in, (const tchar_t *)buf, socks->port);
        }
        rep = 0x00;
    }

    buf[0] = 0x05;
    buf[1] = rep;
    buf[2] = 0x00;
    buf[3] = 0x01;

    port = htons(8088);
    ip = inet_addr("127.0.0.1");

    memcpy(&buf[4], &ip, 4);
    memcpy(&buf[8], &port, 2);

    socks->e = _cc_tcp_connect(_cc_get_async_event(), _CC_EVENT_CONNECT_ | _CC_EVENT_TIMEOUT_ | _CC_EVENT_BUFFER_,
                               (_cc_sockaddr_t *)&socks->addr, 60000, _socks5_event_callback2, e);

    socks->status = 4;
    _cc_event_send(e, buf, 10);
    return true;
}
*/

bool_t _cc_socks5_listen(void) {

}
