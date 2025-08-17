#include <stdio.h>
#include <libcc.h>
#include <locale.h>

#define ICMP_ECHO       8                 //发送Ping请求时的ICMP报文类型  
#define ICMP_ECHOREPLY  0                 //接收Ping回复时的ICMP报文类型  
#define ICMP_TIMEOUT    11                //ICMP超时报文类型  
#define ICMP_MIN        8                 //Minimum 8-byte ICMP packet (header)  
#define MAX_PACKET      1024              //Max ICMP packet size  
#define DEICMP_PACKSIZE 44                //Defaut ICMP PACKET SIZE


char        lpdest[16];                   //用来存放目的IP地址  
uint64_t    cStartTickCount;              //用来存放发送包的起始时间  

#pragma pack(1)

typedef struct _ICMPHeader {
    byte_t itype;
    byte_t icode;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} _ICMPHeader_t;

#pragma pack()

void fillICMPData(char *data, int size) {
    _ICMPHeader_t* icmpHeader;
    icmpHeader = (_ICMPHeader_t*)data;
    icmpHeader->itype = ICMP_ECHO;
    icmpHeader->icode = 0;
    icmpHeader->id = _cc_getpid();
    icmpHeader->checksum = 0;
    icmpHeader->seq = 0;

    memset(data + sizeof(_ICMPHeader_t), 'E', size - sizeof(_ICMPHeader_t));
}

uint16_t checksum(uint16_t *buffer, int size) {
    unsigned long sum = 0;  
    while(size > 1) {  
        sum += *buffer++;  
        size -= sizeof(uint16_t);  
    }  
    if (size) {
        sum += *(uint16_t *)buffer;  
    }
    sum = (sum >> 16) + (sum & 0xffff);  
    sum += (sum >> 16);  

    return (uint16_t)(~sum);
}

int DecodeIPHeader(char *buf, int bytes, struct sockaddr_in *from) {
    _ICMPHeader_t *icmpHeader;
    uint64_t tick;
    static int count = 1;
    uint16_t iphdrlen;

    if (buf == nullptr) {
        printf("%2d:\t\t***.***.***.***\t\tRequest timed out.\n",count++);  
        return 1;
    }

    tick = _cc_get_ticks();
    iphdrlen = (buf[0] & 0x0f) * 4;

    icmpHeader = (_ICMPHeader_t*)(buf + iphdrlen);
    if (bytes < (iphdrlen + ICMP_MIN)) {
        printf("Too few bytes from %s\n",inet_ntoa(from->sin_addr)); 
        return 0;
    }

    if (icmpHeader->itype == ICMP_TIMEOUT && icmpHeader->icode == 0) {
        printf("%2d:\t\t%-15s\t\t%4dms\n",count++,inet_ntoa(from->sin_addr),tick-cStartTickCount);      
        return 0;   
    } else if (icmpHeader->itype == ICMP_ECHOREPLY && icmpHeader->id == _cc_getpid()) {
        printf("%2d:\t\t%-15s\t\t%4dms\n",count++,inet_ntoa(from->sin_addr),tick-cStartTickCount);  
        printf("Trace complete!\n");  
        return 1;
    }

    printf("%2d:        Destination host is unreachable!\n",count++);  
    return 1;
}

int main (int argc, char * const argv[]) {
    _cc_socket_t sockRaw;
    struct sockaddr_in dest, from;
    int i,bread;
    _cc_socklen_t fromlen = sizeof(from);
    int timeout = 1000,ret;
    char icmp_data[MAX_PACKET],recvbuf[MAX_PACKET];
    uint16_t seq_no = 0;

    _cc_install_socket();

    strcpy(lpdest,"baidu.com");
    sockRaw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if ( sockRaw < 0 ) {
        printf("socket fail: %s\n", _cc_last_error(_cc_last_errno()));
        return 0;
    }
    
    //创建套接字   
    //对锁定套接字设置超时  
    _cc_set_socket_timeout(sockRaw, timeout);
    //解析目标地址，将主机名转化为IP地址 
    _cc_inet_ipv4_addr(&dest, lpdest, 0);

    memset(icmp_data,0,MAX_PACKET);

    //_ICMPHeader_t
    fillICMPData((char*)icmp_data,DEICMP_PACKSIZE);  
    printf("Hop\t\tIP Address\t\tTime elapsed\n");  

    //开始发送/接收ICMP报文  
    for (i = 1; i <= 255; i++) {  
        int bwrote;  
        //设置IP包的生存期  
        ret = setsockopt(sockRaw,IPPROTO_IP,IP_TTL,(char *)&i,sizeof(int));  
        if (ret == _CC_SOCKET_ERROR_)  {  
            printf("setsockopt(IP_TTL)\n");  
        }
        ((_ICMPHeader_t *)icmp_data)->checksum = 0;
        ((_ICMPHeader_t *)icmp_data)->seq = seq_no++;     //Sequence number of ICMP packets  
        ((_ICMPHeader_t *)icmp_data)->checksum = checksum((uint16_t *)icmp_data,DEICMP_PACKSIZE);  
        //发送ICMP包请求查询  
        cStartTickCount = _cc_get_ticks();  
        bwrote = sendto(sockRaw,icmp_data,DEICMP_PACKSIZE,0,(struct sockaddr *)&dest,sizeof(dest));
        if (bwrote <= 0) {
            continue;
        }
   
        //接收ICMP回复包  
        bread = recvfrom(sockRaw,recvbuf,MAX_PACKET,0,(struct sockaddr *)&from, &fromlen);  
        if (bread > 0) {
            if (DecodeIPHeader(recvbuf,bread,&from))  
                break;  
        }
        //_cc_sleep(10);
    }  
  
    if(sockRaw!=_CC_INVALID_SOCKET_)  
        _cc_close_socket(sockRaw);

    system("pause");  
    return 0;
}
