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
#include "DataList.h"
/**/
void _Mir2WebSocketListen(uint16_t port,
                         _Mir2NetworkNotify_t connectedNotify,
                         _Mir2NetworkNotify_t disconnectNotify);

typedef struct _Token {
    uint32_t cn;
    uint32_t uid;
    char_t *sign;
}_Token_t;
/**/
bool_t _WebSocketData(_Mir2Network_t *ws, const byte_t* data, uint16_t length) {
    if (ws->classify == ServiceKind_None) {
        if (*data == '*') {
            char_t output[33];
            char_t sign[128];
            _Token_t token;
            size_t rc;
            token.sign = (char_t*)(data + 18);
            token.cn = _cc_hex8((tchar_t*)data + 1);
            token.uid = _cc_hex8((tchar_t*)data + 9);

            if (token.cn == 0 || token.uid == 0) {
                return false;
            } 

            rc = snprintf(sign, _cc_countof(sign), "%d%d6F1F8866CED4C7BF90BFD088C3F66DF4-POKER", token.cn, token.uid);
            _cc_md5(sign, rc, output);
            if (strncmp(output, token.sign, 33) == 0) {
                time_t now = time(NULL) - 1577808000;
                if (now > token.cn) {
                    return false;
                }

                _Mir2BindAcceptor(ws,ServiceKind_WebSocketManager,NULL);
                return true;
            }
            return false;
        } else if (_tcscmp((char_t*)data,"223BA3FAEB1738D30E0DDFD795A88A8B-WEPOKER") == 0) {
            _Mir2BindAcceptor(ws,ServiceKind_WebSocketConnector,NULL);
        } else {
            return false;
        }
        return true;
    }
    
    if (length > 0) {
        _Mir2AllocDataBuffer(data, length);
        printf("%s\n", data);
    }
    return true;
}

/**/
_CC_FORCE_INLINE_ bool_t _Mir2DisconnectNotify(_Mir2Network_t *acceptor) {
    if (acceptor == NULL) {
        return false;
    }
    
    if (acceptor->classify == ServiceKind_WebSocketConnector && acceptor->args) {
        //_Mir2LogoutAccount(acceptor);
    }
    return true;
}

/**/
_CC_FORCE_INLINE_ bool_t _Mir2AcceptNotify(_Mir2Network_t *acceptor) {
    return true;
}

int32_t threadDataRunning(_cc_thread_t* t, pvoid_t args);
/**/
void _Mir2WebSocketListener(uint16_t port) {
    _cc_thread_start(threadDataRunning, "Send DataBuffer", NULL);
    _Mir2WebSocketListen(port, _Mir2AcceptNotify, _Mir2DisconnectNotify);
}