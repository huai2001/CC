/*
 * Copyright (c) 2006 - 2018 QIU ZHONG HUAI <huai2011@163.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:

 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes GHO software, freely available from
 *    <https://github.com/huai2001/CC>".
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS`` AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "Acceptor.h"

/**/
bool_t _WebSocketData(_Mir2Network_t *ws, const byte_t* data, uint16_t length);
/**/
_CC_FORCE_INLINE_ void _WebSocketRandomMasking(byte_t* buf, int32_t len) {
    int32_t i;
    for (i = 0; i < len; i++) {
        buf[i] = (byte_t)(rand() % 255) + 1;
    }
}

/**/
_CC_FORCE_INLINE_ void _WebSocketInverted(byte_t* dest, byte_t* src, int32_t length) {
    int32_t i;
    for (i = 0; i < length; i++) {
        *(dest + i) = *(src + length - 1 - i);
    }
}

/**/
bool_t _Mir2WebSocketSend(_Mir2Network_t *network, uint16_t command, byte_t* data, uint16_t length) {
    _cc_event_t *e;

    e = _cc_get_event_by_id(network->socketId);
    if (e == NULL) {
        return false;
    }
    return _Mir2WebSocketEventSend(e, command, data, length);
}

/**/
bool_t _Mir2WebSocketEventSend(_cc_event_t *e, uint16_t command, byte_t* data, uint16_t length) {
    byte_t response[16];
    byte_t mask = 0x00;
    uint32_t head_length;

    switch (command) {
        case WS_MINDATA:
            response[0] = 0x00;
            break;
        case WS_TXTDATA:
            response[0] = 0x81;
            break;
        case WS_BINDATA:
            response[0] = 0x82;
            break;
        case WS_DISCONNECT:
            response[0] = 0x88;
            break;
        case WS_PING:
            response[0] = 0x89;
            break;
        case WS_PONG:
            response[0] = 0x8A;
            break;
        default:
            break;
    }

    if (length < 126) {
        response[1] = (length & 0xFF) | mask;
        head_length = 2;
    } else if (length < 0xFFFF) {
        response[1] = 0x7E | mask;
        response[2] = (length >> 8 & 0xFF);
        response[3] = (length >> 0 & 0xFF);
        head_length = 4;
    } else {
        response[1] = 0x7F | mask;
        _WebSocketInverted((byte_t*)&response[2], (byte_t*)&length, 8);
        head_length = 12;
    }

    if (mask) {
        uint32_t i;
        byte_t masking[4];
        _WebSocketRandomMasking(masking, 4);
        response[head_length++] = masking[0];
        response[head_length++] = masking[1];
        response[head_length++] = masking[2];
        response[head_length++] = masking[3];

        for (i = 0; i < length; i++) {
            *(data + i) ^= masking[i & 0x3];
        }
    }

    if (_cc_event_send(e, response, head_length) < 0) {
        return false;
    }

    if (length > 0 && _cc_event_send(e, data, length) < 0) {
        return false;
    }
    return true;
}

/**/
_CC_FORCE_INLINE_ int32_t _WebSocketDecoding(_cc_event_rbuf_t* r, _WebSocketFrameHeader* head) {
    int32_t hp;

    if (r->length < 2) {
        return 0;
    }

    /*read fin and op code*/
    head->fin = (r->buf[0] & 0x80) == 0x80;
    head->opcode = r->buf[0] & 0x0F;
    head->mask = (r->buf[1] & 0x80) == 0X80;

    /*get payload length*/
    head->payload_length = r->buf[1] & 0x7F;
    hp = 2;
    if (head->payload_length == 126) {
        hp += 2;
        if (r->length < hp) {
            return 0;
        }
        head->payload_length = (r->buf[2] & 0xFF) << 8 | (r->buf[3] & 0xFF);

    } else if (head->payload_length == 127) {
        hp += 8;
        if (r->length < hp) {
            return 0;
        }
        _WebSocketInverted((byte_t*)&(head->payload_length), &r->buf[2], 8);
    }

    /*read masking-key*/
    if (head->mask) {
        hp += 4;

        if (r->length < hp) {
            return 0;
        }

        memcpy(head->masking, &r->buf[hp - 4], 4);
    } else if (r->length < hp) {
        return 0;
    }

    return hp;
}

/**/
_CC_FORCE_INLINE_ void _WebSocketUmask(byte_t* data, int32_t len, char* mask) {
    int32_t i;
    for (i = 0; i < len; ++i) {
        *(data + i) ^= *(mask + (i & 0x3));
    }
}

/**/
_CC_FORCE_INLINE_ void _WebSocketSecKey(char_t *secKey, _Mir2Network_t *ws) {
    _cc_sha1_t ctx;
    int32_t len;
    byte_t results[1024];
    byte_t sha1Results[_CC_SHA1_DIGEST_LENGTH_];

    len = _snprintf((char_t*)results, _cc_countof(results), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", secKey);

    _cc_sha1_init(&ctx);
    _cc_sha1_update(&ctx, results, len);
    _cc_sha1_final(&ctx, sha1Results);
    _cc_base64_encode(sha1Results, _CC_SHA1_DIGEST_LENGTH_,
                      ws->header.base64Key, _cc_countof(ws->header.base64Key));
}

/**/
_CC_FORCE_INLINE_ bool_t _WebSocketResponseHeader(_cc_event_t *e, _Mir2Network_t *ws) {
    int length;
    char_t headers[1024];
    length = _snprintf(headers, _cc_countof(headers),
                        "HTTP/1.1 101 Switching Protocols\r\n"
                        "Connection: Upgrade\r\n"
                        "Upgrade: websocket\r\n"
                        "Sec-WebSocket-Protool: echo\r\n"
                        "Sec-WebSocket-Accept: %s\r\n\r\n",
                        ws->header.base64Key);

    if (_cc_event_send(e, (byte_t*)headers, sizeof(char_t) * length) == -1) {
        return false;
    }

    ws->status = 101;

    printf("%s", headers);
    return true;
}

/**/
_CC_FORCE_INLINE_ void _WebSocketLine(_Mir2Network_t *ws, char_t *line, int length) {
    printf("%s\n", line);

    if (*line == 'C' && strncmp("Connection", line, 10) == 0) {
        if (strstr(line + 10, "Upgrade")) {
            ws->status++;
        }
        return;
    }

    if (*line == 'U' && strncmp("Upgrade", line, 7) == 0) {
        if (strstr(line + 7, "websocket")) {
            ws->status++;
        }
        return;
    }

    if (*line == 'S' && strncmp("Sec-WebSocket-Key", line, 17) == 0) {
        line += 18;
        while(line) {
            if (*line != ' ' && *line != ':') {
                break;
            }
            line++;
        }

        _WebSocketSecKey(line, ws);
        ws->status++;
        return;
    }
}

/**/
_CC_FORCE_INLINE_ bool_t _WebSocketHeader(_Mir2Network_t *ws, _cc_event_rbuf_t* r) {
    int i, c;
    int start;

    i = 0;
    c = (int)r->length - 1;
    start = 0;
    r->buf[r->length] = 0;

    while (i < c) {
        if (r->buf[i] == '\r' && r->buf[i + 1] == '\n') {
            if (i == start) {
                start = i + 2;
                //3 + 97 == 100
                ws->status += 97;
                break;
            }

            r->buf[i] = 0;
            _WebSocketLine(ws, (char_t*)&r->buf[start], i - start);
            start = i += 2;
            continue;
        }
        i++;
    }

    i = r->length - start;
    if (i > 0) {
        memmove(r->buf, &r->buf[start], i);
    }
    r->length = i;
    return true;
}

/**/
_CC_FORCE_INLINE_ int _WebSocketPackage(_Mir2Network_t* ws, _cc_event_rbuf_t* r) {
    int32_t length;
    int32_t offset;
    offset = _WebSocketDecoding(r, &ws->header);
    if (offset == 0) {
        return 0;
    }

    length = (int32_t)ws->header.payload_length;
    /*printf("fin=%d\nopcode=0x%X\nmask=%d\npayload_len=%llu\n",
    ws->header.fin,
    ws->header.opcode,
    ws->header.mask,
    ws->header.payload_length);*/
    switch (ws->header.opcode) {
        case WS_DISCONNECT:
            return -1;
        case WS_PING:
            _Mir2WebSocketSend(ws, WS_PONG, NULL, 0);
            break;
        case WS_PONG:
            printf("recv pong frame.\n");
            break;
    }
    
    if (length > 0) {
        r->buf[r->length] = 0;
        _WebSocketUmask(&r->buf[offset], length, ws->header.masking);

        if (!_WebSocketData(ws, &r->buf[offset], length)) {
            return -1;
        }
    } 

    r->length -= (length + offset);
    if (r->length > 0) {
        memmove(r->buf, (r->buf + length + offset), r->length);
        return 1;
    }
    r->length = 0;
    return 0;
}

/**/
_CC_FORCE_INLINE_ bool_t _WebSocketNotify(_cc_event_t *e) {
    _Mir2Network_t *ws = (_Mir2Network_t*)e->args;
    _cc_event_rbuf_t *r;
    if (ws == NULL || e->buffer == NULL) {
        return false;
    }

    r = &e->buffer->r;

    if (ws->status == 101) {
        do {
            int rc = _WebSocketPackage(ws, r);
            if (rc == 0) {
                break;
            }

            if (rc == -1) {
                return false;
            }
        } while (1);
        return true;
    } 

    if (ws->status <= 3) {
        if (_WebSocketHeader(ws, r) == false) {
            return false;
        }
    }

    if (ws->status == 100) {
        return _WebSocketResponseHeader(e, ws);
    }

    return false;
}

/**/
_CC_FORCE_INLINE_ bool_t _Mir2DisconnectNotify(_cc_event_t *e) {
    _Mir2Network_t *acceptor;
    if (e == NULL || e->args == NULL) {
        return false;
    }
    acceptor = (_Mir2Network_t *)e->args;
    
    e->args = NULL;

    if (acceptor->disconnectNotify) {
        acceptor->disconnectNotify(acceptor);
    }

    _Mir2FreeAcceptor(acceptor);
    return true;
}

/**/
_CC_FORCE_INLINE_ void _Mir2NewAcceptor(_cc_event_cycle_t *cycle, _cc_event_t *e) {
    _cc_socket_t fd = _CC_INVALID_SOCKET_;
    _cc_sockaddr_t remoteAddr = {0};
    _cc_socklen_t remoteAddrLen = sizeof(_cc_sockaddr_t);
    _cc_event_cycle_t *newCycle;
    _cc_event_t *newEvent;
    _Mir2Network_t *newAcceptor;
    _Mir2Network_t* acceptor = (_Mir2Network_t*)e->args;

    fd = _cc_event_accept(cycle, e, &remoteAddr, &remoteAddrLen);
    if (fd == _CC_INVALID_SOCKET_) {
        _cc_logger_error(_T("thread %d accept fail %s.\n"), _cc_get_thread_id(NULL), _cc_last_error(_cc_last_errno()));
        return ;
    }

    newAcceptor = _Mir2AllocAcceptor(ServiceKind_None, &remoteAddr);
    if (newAcceptor == NULL) {
        _cc_close_socket(fd);
        return ;
    }

    newAcceptor->connectedNotify = acceptor->connectedNotify;
    newAcceptor->disconnectNotify = acceptor->disconnectNotify;

    newCycle = _Mir2GetCycle();
    if (newCycle == NULL) {
        newCycle = cycle;
    }

    newEvent = _cc_alloc_event(newCycle, _CC_EVENT_TIMEOUT_|_CC_EVENT_READABLE_|_CC_EVENT_BUFFER_, fd, e->timeout, e->callback, newAcceptor);
    newAcceptor->socketId = newEvent->ident;

    if (newAcceptor->connectedNotify == NULL || 
        newAcceptor->connectedNotify(newAcceptor) == false) {
        _cc_cleanup_event(newCycle, newEvent);
        _Mir2FreeAcceptor(newAcceptor);
        return ;
    }

    if (newCycle->driver.attach(newCycle, newEvent) == NULL) {
        _cc_logger_error(_T("thread %d add socket (%d) event fial."), _cc_get_thread_id(NULL), fd);
        _Mir2FreeAcceptor(newAcceptor);
    }
}

/**/
_CC_FORCE_INLINE_ bool_t _Mir2WebSocketAcceptorCallback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t events) {
    if (events & _CC_EVENT_ACCEPT_) {
        _Mir2NewAcceptor(cycle, e);
        return true;
    }
    
    if (events & _CC_EVENT_READABLE_) {
        if (!_cc_event_recv(e)) {
            _Mir2DisconnectNotify(e);
            _cc_event_force_disconnect(e);
            return false;
        }

        if (!_WebSocketNotify(e)) {
            _Mir2DisconnectNotify(e);
            return false;
        }
    }
    
    if (events & _CC_EVENT_WRITABLE_) {
        if (e->buffer) {
            if (_CC_EVENT_WBUF_HAS_DATA(e->buffer)) {
                if (!_cc_event_sendbuf(e)) {
                    _Mir2DisconnectNotify(e);
                    return false;
                }
            }
            if (!_CC_EVENT_WBUF_HAS_DATA(e->buffer)) {
                if (e->flags & _CC_EVENT_DISCONNECT_) {
                    _Mir2DisconnectNotify(e);
                    return false;
                }
                _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
            }
        } else {
            _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        }
    }
    
    if (events & _CC_EVENT_TIMEOUT_) {
        if (_Mir2WebSocketEventSend(e, WS_PING, NULL, 0)) {
            return true;
        }
        
        _Mir2DisconnectNotify(e);
        return false;
    }
    
    if (events & _CC_EVENT_DISCONNECT_) {
        _Mir2DisconnectNotify(e);
        return false;
    }

    return true;
}

/**/
void _Mir2WebSocketListen(uint16_t port,
                         _Mir2NetworkNotify_t connectedNotify,
                         _Mir2NetworkNotify_t disconnectNotify) {
    struct sockaddr_in sa;
    _Mir2Network_t *acceptor;

    acceptor = _Mir2AllocAcceptor(ServiceKind_WebSocketListen, NULL);
    if (acceptor == NULL) {
        return ;
    }

    acceptor->connectedNotify = connectedNotify;
    acceptor->disconnectNotify = disconnectNotify;

    _cc_inet_ipv4_addr(&sa, NULL, port);
    _cc_tcp_listen(_Mir2GetCycle(), (_cc_sockaddr_t*)&sa, 1000 * 60 * 5, _Mir2WebSocketAcceptorCallback, acceptor);
}