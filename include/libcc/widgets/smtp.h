/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
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

#ifndef _C_CC_WIDGETS_LIBSMTP_H_INCLUDED_
#define _C_CC_WIDGETS_LIBSMTP_H_INCLUDED_

#include "dylib.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_LIBSMTP_VERSION_INFO  "libSTMP Release 1.0,* Copyright 2018-2019 libcc.cn@gmail.com"

enum {
    _CC_SMTP_TEXT = 0,
    _CC_SMTP_HTML
};

enum {
    _CC_LIBSMTP_RESP_PENDING = 0,
    _CC_LIBSMTP_RESP_CONNECTED,
    _CC_LIBSMTP_RESP_EHLO,
    _CC_LIBSMTP_RESP_AUTH_LOGIN,
    _CC_LIBSMTP_RESP_LOGIN_USER,
    _CC_LIBSMTP_RESP_LOGIN_PASSWORD,
    _CC_LIBSMTP_RESP_LOGOUT,
    _CC_LIBSMTP_RESP_FROM,
    _CC_LIBSMTP_RESP_RCPT_TO,
    _CC_LIBSMTP_RESP_DATA,
    _CC_LIBSMTP_RESP_SEND_EMAIL,
};

enum {
    _CC_LIBSMTP_CONNECTED = 0,
    _CC_LIBSMTP_CONNECT_FAILED,
    _CC_LIBSMTP_EHLO,
    _CC_LIBSMTP_LOGINED,
    _CC_LIBSMTP_LOGIN_FAILED,
    _CC_LIBSMTP_LOGIN_USER_FAILED,
    _CC_LIBSMTP_LOGIN_PASSWORD_FAILED,
    _CC_LIBSMTP_LOGOUT,
    _CC_LIBSMTP_LOGOUT_FAILED,
    
    _CC_LIBSMTP_SEND_EMAIL,
    _CC_LIBSMTP_SEND_EMAIL_SUCCESS,
    
    _CC_LIBSMTP_MAIL_FROM_FAILED,
    _CC_LIBSMTP_RCPT_TO_FAILED,
    _CC_LIBSMTP_MAIL_DATA_FAILED,
    _CC_LIBSMTP_SEND_EMAIL_FAILED
};

typedef struct _cc_smtp _cc_smtp_t;
typedef struct _cc_smtp_resp _cc_smtp_resp_t;
/**/
typedef bool_t (*_cc_smtp_resp_callback_t)(_cc_smtp_t*, const byte_t *buf, uint32_t len);
typedef bool_t (*_cc_smtp_callback_t)(_cc_smtp_t*, uint16_t which);

/**/
struct _cc_smtp_resp {
    uint16_t flag;
    pvoid_t data;
    _cc_smtp_resp_callback_t callback;
};
    
/**/
struct _cc_smtp {
    bool_t logined;
        
    byte_t cmode;
    byte_t smode;
    byte_t mailtype;
        
    struct {
        _cc_event_t *e;
        _cc_event_cycle_t *cycle;
    } ctrl;
        
    _cc_smtp_resp_t resp;
    _cc_smtp_callback_t callback;
        
    char_t *user;
    char_t *password;
    char_t *from;
    char_t *to;
};

/**/
void libsmtp_set_error_info(const char_t *p, int32_t len);
/**/
void libsmtp_setup(_cc_smtp_t *smtp, uint16_t flag, _cc_smtp_resp_callback_t fn, pvoid_t data);
/**/
_CC_WIDGETS_API(bool_t) _cc_smtp_connected(_cc_smtp_t *smtp);
/**/
_CC_WIDGETS_API(bool_t) _cc_smtp_disconnected(_cc_smtp_t *smtp);
/**/
_CC_WIDGETS_API(bool_t) _cc_smtp_login(_cc_smtp_t *smtp, const char_t *user, const char_t *password);
/**/
_CC_WIDGETS_API(bool_t) _cc_smtp_logout(_cc_smtp_t *smtp);
/**/
_CC_WIDGETS_API(bool_t) _cc_smtp_from_to(_cc_smtp_t *smtp, const char_t *from, const char_t *to);
/**/
_CC_WIDGETS_API(bool_t) _cc_send_email(_cc_smtp_t *smtp, const char_t *from_name, const char_t *subject, const char_t *content);
/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_LIBSMTP_H_INCLUDED_ */


