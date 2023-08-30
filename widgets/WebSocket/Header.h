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
#ifndef _C_CC_HEADER_H_INCLUDED_
#define _C_CC_HEADER_H_INCLUDED_

#include <libcc.h>

#define _CC_COMMON_API(t) t

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

enum {
    ACCOUNT_STATUS_CONNECTING = 0,       //正在连接
    ACCOUNT_STATUS_ONLINE,               //在线状态
    ACCOUNT_STATUS_OFFLINE,              //断线状态
    ACCOUNT_STATUS_EXIT                  //退出状态
};

enum {
    LoginMode_Account = 1,
    LoginMode_Mobile,
    LoginMode_Wechat,
    LoginMode_QQ
};

enum {
    ServiceKind_None = 0,
    ServiceKind_Listen,
    ServiceKind_WebSocketListen,
    ServiceKind_WebSocketManager,
    ServiceKind_WebSocketConnector,
    ServiceKind_MaxCount
};


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_HEADER_H_INCLUDED_*/
