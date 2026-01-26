
#ifndef _C_CC_LIBSMTP_H_INCLUDED_
#define _C_CC_LIBSMTP_H_INCLUDED_

#include "sds.h"
#include "event.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _CC_LIBSMTP_VERSION_INFO  "libSTMP Release 1.0,* Copyright 2018-2025 libcc.cn@gmail.com"

enum {
    _CC_SMTP_TEXT_ = 0,
    _CC_SMTP_HTML_,
    _CC_SMTP_LOGIN_MODE_PLAIN_,
    _CC_SMTP_LOGIN_MODE_XOAUTH2
};

enum {
    _CC_LIBSMTP_RESP_PENDING_ = 0,
    _CC_LIBSMTP_RESP_CONNECTED_,
    _CC_LIBSMTP_RESP_EHLO_,
    _CC_LIBSMTP_RESP_AUTH_LOGIN_,
    _CC_LIBSMTP_RESP_LOGIN_USER_,
    _CC_LIBSMTP_RESP_LOGIN_PASSWORD_,
    _CC_LIBSMTP_RESP_FROM_,
    _CC_LIBSMTP_RESP_RCPT_TO_,
    _CC_LIBSMTP_RESP_DATA_,
    _CC_LIBSMTP_RESP_SEND_EMAIL_,
    _CC_LIBSMTP_RESP_LOGOUT_
};
/**/
typedef struct _cc_smtp _cc_smtp_t;
/**/
typedef bool_t (*_cc_smtp_response_callback_t)(_cc_smtp_t*, const byte_t *buf, uint32_t length);
/**/
typedef struct {
    byte_t mail_type;
    _cc_sds_t from_name;
    _cc_sds_t to;
    _cc_sds_t subject;
    _cc_sds_t content;
    _cc_list_t lnk;
} _cc_email_t;

/**/
struct _cc_smtp {
    byte_t login_mode;
    byte_t is_logined;

    uint16_t state;

    _cc_smtp_response_callback_t response_cb;

    _cc_io_buffer_t *io;

    _cc_sds_t from_name;
    _cc_sds_t from;
    _cc_sds_t user;
    _cc_sds_t password;

    _cc_email_t *email;
    _cc_atomic_lock_t lock;
    _cc_list_t emails;
};

/**/
void libsmtp_setup(_cc_smtp_t *smtp, uint16_t state, _cc_smtp_response_callback_t cb);
/**/
bool_t libsmtp_command(_cc_smtp_t* smtp, const tchar_t *fmt, ...);
/**/
bool_t libsmtp_login(_cc_smtp_t* smtp);
/**/
bool_t libsmtp_from_to(_cc_smtp_t* smtp);
/**/
bool_t libsmtp_send_email(_cc_smtp_t* smtp);

/**/
_CC_API_PUBLIC(_cc_smtp_t*) _cc_alloc_smtp(const char_t *from_name, const char_t *from);
/**/
_CC_API_PUBLIC(bool_t) _cc_smtp_set_login(_cc_smtp_t *smtp, byte_t login_mode, const char_t *user, const char_t *password);
/**/
_CC_API_PUBLIC(void) _cc_smtp_set_email(_cc_smtp_t *smtp, _cc_email_t *email);

/**/
_CC_API_PUBLIC(bool_t) _cc_smtp_connected(_cc_smtp_t* smtp);
/**/
_CC_API_PUBLIC(bool_t) _cc_smtp_logout(_cc_smtp_t *smtp);
/**/
_CC_API_PUBLIC(void) _cc_free_smtp(_cc_smtp_t *smtp);

/**/
_CC_API_PUBLIC(_cc_email_t*) _cc_alloc_email(byte_t mail_type, const char_t *from_name, const char_t *to);
/**/
_CC_API_PUBLIC(bool_t) _cc_set_email(_cc_email_t *email, const char_t *subject, const char_t *content);
/**/
_CC_API_PUBLIC(void) _cc_free_email(_cc_email_t *email);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CC_LIBSMTP_H_INCLUDED_ */


