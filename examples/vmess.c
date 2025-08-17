#include <stdio.h>
#include <libcc.h>
#include <locale.h>

const char_t *KDFSaltConstAuthIDEncryptionKey             = "AES Auth ID Encryption";
const char_t *KDFSaltConstAEADRespHeaderLenKey            = "AEAD Resp Header Len Key";
const char_t *KDFSaltConstAEADRespHeaderLenIV             = "AEAD Resp Header Len IV";
const char_t *KDFSaltConstAEADRespHeaderPayloadKey        = "AEAD Resp Header Key";
const char_t *KDFSaltConstAEADRespHeaderPayloadIV         = "AEAD Resp Header IV";
const char_t *KDFSaltConstVMessAEADKDF                    = "VMess AEAD KDF";
const char_t *KDFSaltConstVMessHeaderPayloadAEADKey       = "VMess Header AEAD Key";
const char_t *KDFSaltConstVMessHeaderPayloadAEADIV        = "VMess Header AEAD Nonce";
const char_t *KDFSaltConstVMessHeaderPayloadLengthAEADKey = "VMess Header AEAD Key_Length";
const char_t *KDFSaltConstVMessHeaderPayloadLengthAEADIV  = "VMess Header AEAD Nonce_Length";
const char_t *vmess_uuid = "c48619fe-8f02-49e0-b9e9-edf763e17e21";

enum {
    SecurityType_AES128_CFB         = 0x01,
    SecurityType_AES128_GCM         = 0x03,
    SecurityType_CHACHA20_POLY1305  = 0x04,
    SecurityType_NONE               = 0x05
};

enum {
    AddressType_IPv4    = 0x01,
    AddressType_Domain  = 0x02,
    AddressType_IPv6    = 0x03,
};

enum {
	RequestCommandTCP = 0x01,
	RequestCommandUDP = 0x02,
	RequestCommandMux = 0x03
};

/*
 https://note.tonycrane.cc/web/protocol/vmess/
（ 1 字节）版本号 ver：始终为 1
（16 字节）数据部分加密 iv：随机生成，供数据部分加密使用（后也称请求 iv）
（16 字节）数据部分加密 key：随机生成，供数据部分加密使用（后也称请求 key）
（ 1 字节）响应认证 V：随机生成，用于匹配响应
（ 1 字节）选项 opt
        .......S：是否使用标准格式数据流（一般均为 1）
        ......R.：已弃用
        .....M..：数据部分及响应是否开启 mask（后面会详细解释）
        ....P...：数据部分及响应是否开启 padding
（ 1 字节）P 与 Sec
        （前 4 bit）余量 P：在校验码前添加的字节数
        （后 4 bit）加密方式 Sec：对于数据部分及响应使用的加密方式（此处文档有误）
        0x1：使用 AES-128-CFB 算法（少用）
        0x3：使用 AES-128-GCM 算法
        0x4：使用 ChaCha20-Poly1305 算法
        0x5：不加密（少用）
（ 1 字节）保留，默认为 0x00
（ 1 字节）指令 cmd：为 0x01 时使用 TCP、为 0x02 时使用 UDP
（ 2 字节）端口号 port：2 字节大端格式的整型端口号
（ 1 字节）地址类型 T：为 0x01 到 0x03
（ ? 字节）地址 A：
        当 T == 0x01 时：A 为 4 字节 IPv4 地址
        当 T == 0x02 时：A 为 1 字节的长度 L 后接 L 字节的域名
        当 T == 0x03 时：A 为 16 字节 IPv6 地址
（ P 字节）随机值：随机填充，长度由前面的 P 决定
（ 4 字节）校验码 F：指令部分除校验码以外所有内容的 fnv1a 哈希值
*/

#pragma pack(1)

typedef struct _vmess_user {
    //认证信息
    byte_t authorized[16];
    //用户UUID
    byte_t uuid[16];
    //加密数据IV
    byte_t iv[16];
    //加密数据KEY
    byte_t key[16];
} _vmess_user_t;

typedef struct _vmess {
    //HEADER
    //0
    //版本号 Ver
    byte_t version;
    //1
    //数据加密 IV
    byte_t iv[16];
    //17
    //数据加密 Key
    byte_t key[16];	
    //33
    //响应认证 V
    byte_t v;
    //34
    //选项 Opt
    byte_t option;
    //35
    //余量 P 加密方式 Sec
    byte_t security;
    //36
    //保留
    byte_t reserve;
    //37
    //指令 Cmd
    byte_t command;
    //38
    //端口 Port
    uint16_t port;
    //40
    //地址类型 T
    byte_t address_type;
    //41 {1 + 16 + 16 + 1 + 1 + 1 + 1 + 1 + 2 + 1} + 1 + MAX{255} + MAX{15} + 4
    //N地址 A + P随机值 + 4校验F
    byte_t data[277];
    //实际长度
    uint32_t length;
    //VMESS 用户信息
    _vmess_user_t user;
} _vmess_t;

#pragma pack()

void dump(const char *name, unsigned char *digest, unsigned int digest_size)
{
    int i;
    printf("%s:", name);
    for (i = 0; i < (int) digest_size ; i++) {
       printf("%02X", digest[i]);
    }
    putchar('\n');
}

void _build_vmess_aead_header(_cc_uuid_t *u, byte_t *data, uint32_t length) {
    //create authid
    uint64_t timestamp;
    uint32_t zero;
    byte_t buf[8];
    //信息为当前的 UTC 时间（Unix 时间戳，精确到秒）
    timestamp = time(nullptr);
    timestamp = _cc_swap64(timestamp);
    memcpy(buf, &timestamp, sizeof(uint64_t));
    buf[4] = rand() % 256;
    buf[5] = rand() % 256;
    buf[6] = rand() % 256;
    buf[7] = rand() % 256;
    
    zero = _cc_crc32(buf, 8);
    zero = _cc_swap32(zero);
}

void _build_vmess_user(_vmess_user_t *user) {
    _cc_md5_t c;
    _cc_hmac_t *hmac;
    uint64_t timestamp;

    // the key of AES-128-CFB encrypter
    // Key：MD5(UUID + []byte('c48619fe-8f02-49e0-b9e9-edf763e17e21'))
    _cc_md5_init(&c);
    _cc_md5_update(&c, user->uuid, 16);
    _cc_md5_update(&c, (const byte_t*)vmess_uuid, 36);
    _cc_md5_final(&c, user->key);

    //信息为当前的 UTC 时间（Unix 时间戳，精确到秒）上下随机浮动 30 秒，然后表示为 8 字节大端格式
    timestamp = time(nullptr) - (rand() % 31) + (rand() % 31);
    timestamp = _cc_swap64(timestamp);

    // the iv of AES-128-CFB encrypter
    // IV：MD5(X + X + X + X)，X = []byte(timestamp.now) (8 bytes, Big Endian)
    _cc_md5_init(&c);
    _cc_md5_update(&c, (byte_t*)&timestamp, sizeof(uint64_t));
    _cc_md5_update(&c, (byte_t*)&timestamp, sizeof(uint64_t));
    _cc_md5_update(&c, (byte_t*)&timestamp, sizeof(uint64_t));
    _cc_md5_update(&c, (byte_t*)&timestamp, sizeof(uint64_t));
    _cc_md5_final(&c, user->iv);

    //16 字节认证信息
    //密钥为十六字节的 uuid + timestamp
    _cc_hmac_t *hmac = _cc_hmac_alloc(_CC_HMAC_MD5_);
    if (hmac == nullptr) {
        return;
    }

    _cc_hmac_init(hmac, user->uuid, 16);
    _cc_hmac_update(hmac, (byte_t*)&timestamp, sizeof(uint64_t));
    _cc_hmac_final(hmac, user->authorized, 16);
    _cc_hmac_free(hmac);
}

void _build_vmess_header(_vmess_t *vmess, const byte_t *addr, uint16_t len, uint16_t port) {
#ifndef AEAD
    _cc_md5_t c;
#else
    _cc_sha256_t c;
#endif
    uint32_t fnv1a;
    byte_t i;
    byte_t P = (rand() % 0x0F) + 1;
    byte_t buf[33];

    for (i = 0; i < 33; i++) {
        buf[i] = (rand() % 256);
    }

    vmess->length = 0;
    vmess->v = buf[32]; //responseHeader
    vmess->security = (P << 4) | vmess->security;// P(4bit) and Sec(4bit)
    vmess->reserve = 0;

#ifndef AEAD
    //数据加密key
    _cc_md5_init(&c);
    _cc_md5_update(&c, &buf[0], 16);
    _cc_md5_final(&c, vmess->key);

    //数据加密iv
    _cc_md5_init(&c);
    _cc_md5_update(&c, &buf[16], 16);
    _cc_md5_final(&c, vmess->iv);
#else
    //数据加密key
    _cc_sha256_init(&c);
    _cc_sha256_update(&c, &buf[0], 16);
    _cc_sha256_final(&c, vmess->key);

    //数据加密iv
    _cc_sha256_init(&c);
    _cc_sha256_update(&c, &buf[16], 16);
    _cc_sha256_final(&c, vmess->iv);
#endif
    // target
    vmess->port = _cc_swap16(port);
    if (vmess->address_type == AddressType_Domain) {
        vmess->data[0] = (byte_t)len;
        for (i = 0; i < len; i++) {
            vmess->data[i + 1] = addr[i];
        }
        len += 1;
    } else {
        memcpy(vmess->data, addr, len);
    }

    vmess->length += len;

    // padding
    for (i = 0; i < P; i++) {
        vmess->data[vmess->length++] = (rand() % 256) & 0xff;
    }

    // F
    fnv1a = _cc_hash_fnv1a_32((byte_t*)vmess, vmess->length + 41);
    fnv1a = _cc_swap32(fnv1a);

    memcpy(&vmess->data[vmess->length], &fnv1a, sizeof(uint32_t));
    vmess->length += sizeof(uint32_t);
    vmess->length += 41;
}

byte_t c = 0;
time_t start_time = 0;
_cc_event_cycle_t network_event;
_cc_thread_t *network_thread = nullptr;

int32_t fn_thread(_cc_thread_t *thrd, void* param) {
    while(c!='q')
        _cc_event_wait((_cc_event_cycle_t *)param, 100);

    return 1;
}

uint32_t getIP(const _cc_event_t* e) {
    struct sockaddr_in addr;
    _cc_socklen_t len = sizeof(struct sockaddr_in);
    if(getpeername(e->fd, (struct sockaddr *)&addr, &len) == -1) {
        int32_t err = _cc_last_errno();
        _cc_logger_error(_T("discovery client information failed, fd=%d, errno=%d(%#x).\n"), e->fd, err, err);
        return 0;
    }
    return (addr.sin_addr.s_addr);
}

void sendVmess(_cc_event_t *e) {
    _vmess_t vmess;

    bzero(&vmess, sizeof(vmess));
    vmess.version = 2;
    vmess.option = 1;
    vmess.command = RequestCommandTCP;
    vmess.security = SecurityType_NONE;//SecurityType_AES128_CFB;
    vmess.address_type = AddressType_Domain;

    _cc_uuid_to_bytes(&vmess.user.uuid, _T("c8f4b563-498c-4fe9-a8f9-02665f0afeb1"));
    _build_vmess_user(&vmess.user);

    _build_vmess_header(&vmess, (const byte_t *)"pinterest.com", sizeof("pinterest.com"), 80);

    {
        char_t *http;
        size_t iv_off = 0;
        uint16_t len = 0;
        uint16_t len2 = 0;
        byte_t iv[16] = {0};

        byte_t output[10240];
        _cc_aes_t aes;
        memcpy(iv, vmess.user.iv, 16);

        _cc_aes_init(&aes);
        _cc_aes_setkey_enc(&aes, (byte_t*)vmess.user.key, 128);
        _cc_aes_crypt_cfb128(&aes, _CC_AES_ENCRYPT_, (byte_t*)&vmess, vmess.length, &iv_off, iv, output);

        _cc_event_send(e, vmess.user.authorized, 16);
        _cc_event_send(e, output, vmess.length);

        _cc_sleep(1000);
        http = "GET / HTTP/1.1\r\nHost: %s\r\nUser-Agent: Mozilla/5.0 (iPod; CPU iPhone OS 10_0 like Mac OS X) AppleWebKit/602.1.38 (KHTML, like Gecko) Version/10.0 Mobile/14A300 Safari/602.1\r\nAccept: */*\r\n\r\n";
        len = snprintf(output, 10240, http,  "pinterest.com");
        //len = (uint16_t)strlen(http);
/*
        iv_off = 0;
        memcpy(iv, vmess.iv, 16);
        _cc_aes_init(&aes);
        _cc_aes_setkey_enc(&aes, (byte_t*)vmess.key, 128);
        _cc_aes_crypt_cfb128(&aes, _CC_AES_ENCRYPT_, (byte_t*)http, len, &iv_off, iv, output);
*/
        len2 = _cc_swap16(len);
        _cc_event_send(e,  (byte_t*)&len2, sizeof(uint16_t));
        _cc_event_send(e, (byte_t*)output, len);
    }
}

static bool_t network_event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t which) {
    if(which & _CC_EVENT_CONNECT_){
        sendVmess(e);
        _cc_logger_debug(_T("Connect to server!\n"));
        return true;
    }
    
    if (which & _CC_EVENT_DISCONNECT_) {
        _tprintf(_T("TCP Close - %d\n"), e->fd);

        return false;
    }
    /**/
    if (which & _CC_EVENT_READABLE_) {
        char_t buf[_CC_IO_BUFFER_SIZE_];
        int32_t length;
        length = _cc_recv(e->fd, (byte_t*)buf, _CC_IO_BUFFER_SIZE_);
        if(length <= 0) {
            _cc_logger_debug(_T("TCP close Data %d\n"),e->fd);
            return false;
        }
        buf[length] = 0;
        printf("%s\n", buf);

        return true;
    }

    if (which & _CC_EVENT_WRITABLE_) {
        if (_cc_event_send(e,nullptr,0) < 0) {
            _cc_logger_debug(_T(" Fail to send, error = %d\n"), _cc_last_errno());
            return false;   
        }
    }
    
    if (which & _CC_EVENT_TIMEOUT_) {
        _cc_logger_debug(_T("Timeout - %d\n"), e->fd);
        return true;
    }

    return true;
}

int main (int argc, char * const argv[]) {
    _cc_event_t *e;
    struct sockaddr_in sa;
    _cc_uuid_t uuid;

    setlocale( LC_CTYPE, "chs" );
    _cc_install_socket();

    _cc_uuid(&uuid);
    dump("GUID", uuid, sizeof(uuid));
    _cc_uuid_to_bytes(&uuid, _T("c8f4b563-498c-4fe9-a8f9-02665f0afeb1"));
    dump("ToBytes", uuid, sizeof(uuid));

    if(_cc_init_event_poller(&network_event) == false){
        return 0;
    }

    //39.108.173.196
    _cc_inet_ipv4_addr(&sa, _T("127.0.0.1"), 10606);
    network_thread = _cc_thread(fn_thread, _T("net-time"), &network_event);

    e = _cc_tcp_connect(&network_event, 
    _CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_|_CC_EVENT_BUFFER_, (_cc_sockaddr_t*)&sa,
    60 * 5 * 1000, network_event_callback, nullptr);
    
    while((c = getchar()) != 'q') {
        _cc_sleep(100);
    }
    
    _cc_wait_thread(network_thread, nullptr);
    network_event.quit(&network_event);
    _cc_uninstall_socket();

    return 0;
}
