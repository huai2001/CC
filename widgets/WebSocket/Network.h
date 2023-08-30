/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
 * All rights reserved.org>
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/
#ifndef _C_CC_NETWORK_H_INCLUDED_
#define _C_CC_NETWORK_H_INCLUDED_

#include "Header.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif
    
//IP缓存长度
#define MAX_IP_BUFFER           24
/*-------------------------------------------------------------------
0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               |Masking-key, if MASK set to 1  |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+
--------------------------------------------------------------------*/
enum {
    WS_MINDATA = 0x00,     // 0x0a continuation packet
    WS_TXTDATA = 0x01,     // 0x1a text packet
    WS_BINDATA = 0x02,     // 0x2a binary packet
    WS_DISCONNECT = 0x08,  // 0x8a disconnected packet
    WS_PING = 0x09,        // 0x9a ping packet
    WS_PONG = 0x0A,        // 0xAa pong packet
};

typedef struct _WebSocketFrameHeader {
    char fin;
    char opcode;
    char mask;
    unsigned long long payload_length;
    char masking[4];
    char_t base64Key[256];
} _WebSocketFrameHeader;

typedef struct _Mir2Network _Mir2Network_t;

typedef bool_t (*_Mir2NetworkDataNotify_t)(_Mir2Network_t *network, _cc_event_rbuf_t *r);
typedef bool_t (*_Mir2NetworkNotify_t)(_Mir2Network_t *network);
typedef bool_t (*_Mir2NetworkEventNotify_t)(_cc_event_t *e, const byte_t *, uint16_t);

struct _Mir2Network {
    //服务类型
    uint8_t classify;
    //状态
    uint8_t status;
    //监听的端口
    uint16_t port;
    //网络标识
    uint32_t socketId;

    _WebSocketFrameHeader header;

    //绑定的参数
    pvoid_t args;

    //IP信息
    struct {
        int family; //AF_INET or AF_INET6
        tchar_t data[MAX_IP_BUFFER];
    } ip;

    _Mir2NetworkDataNotify_t dataNotify;
    _Mir2NetworkNotify_t connectedNotify;
    _Mir2NetworkNotify_t disconnectNotify;

    _cc_list_iterator_t lnk;
};

_CC_COMMON_API(_cc_event_cycle_t*) _Mir2GetCycle(void); 
_CC_COMMON_API(bool_t) _Mir2NetworkStartup(int32_t services);
_CC_COMMON_API(bool_t) _Mir2NetworkStop(void);
_CC_COMMON_API(bool_t) _Mir2GetKeepActive(void);
/**/
_CC_COMMON_API(void) _Mir2NetworkBind(_cc_event_t *e,
                                      uint8_t classify,
                                      pvoid_t args);
/**/
_CC_COMMON_API(bool_t) _Mir2WebSocketSend(_Mir2Network_t *network, uint16_t command, byte_t* data, uint16_t length);
/**/
_CC_COMMON_API(bool_t) _Mir2WebSocketEventSend(_cc_event_t *e, uint16_t command, byte_t* data, uint16_t length);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_NETWORK_H_INCLUDED_*/
