#include <libcc/smtp.h>
#include <libcc/crypto/base64.h>
/**/
void libsmtp_setup(_cc_smtp_t* smtp, uint16_t state, _cc_smtp_response_callback_t cb) {
    smtp->state = state;
    smtp->response_cb = cb;
}

bool_t libsmtp_command(_cc_smtp_t* smtp, const tchar_t *fmt, ...) {
    int32_t off,avail;
    va_list arg;
    _cc_io_buffer_t *io = (_cc_io_buffer_t *)smtp->io;

    _cc_assert(fmt != NULL);

    va_start(arg, fmt);
    avail = io->w.limit - io->w.off;
    off = vsnprintf((char*)(io->w.bytes + io->w.off), avail, fmt, arg);
    if (off > 0 && off < avail) {
        io->w.off += off;
    }
    va_end(arg);
    return true;
}

_CC_API_PRIVATE(bool_t) libsmtp_send_email_cb(_cc_smtp_t* smtp, const byte_t* buf, uint32_t length) {
    if (smtp->state != _CC_LIBSMTP_RESP_SEND_EMAIL_) {
        return false;
    }

    smtp->state = _CC_LIBSMTP_RESP_PENDING_;

    if (buf[0] == '2' && buf[1] == '5' && buf[2] == '0') {
        if (smtp->email) {
            _cc_free_email(smtp->email);
            smtp->email = NULL;
        }

        if (!_cc_list_empty(&smtp->emails)) {
            return libsmtp_from_to(smtp);
        }
        return _cc_smtp_logout(smtp);
    }
    return false;
}

/**/
_CC_API_PUBLIC(bool_t) libsmtp_send_email(_cc_smtp_t* smtp) {
    char_t date[128];

    int32_t length_of_content;
    int32_t required_length;

    char* content_type;
    time_t seconds = time(NULL);
    struct tm* m = gmtime(&seconds);

    _cc_io_buffer_t *io = smtp->io;
    _cc_email_t *email = smtp->email;
    _cc_sds_t from_name;


    _cc_assert(smtp != NULL);
    _cc_assert(email != NULL);

    if (smtp == NULL || email == NULL) {
        return false;
    }

    if (smtp->state != _CC_LIBSMTP_RESP_PENDING_) {
        return false;
    }

    if (email->mail_type == _CC_SMTP_HTML_) {
        content_type = "text/html;charset=UTF-8";
    } else {
        content_type = "text/plain;charset=UTF-8";
    }

    from_name = email->from_name ? email->from_name : smtp->from_name;

    libsmtp_setup(smtp, _CC_LIBSMTP_RESP_SEND_EMAIL_, libsmtp_send_email_cb);

    strftime(date, _cc_countof(date), "%a, %d %b %Y %H:%M:%S GMT", m);
    libsmtp_command(smtp, "FROM:\"%s\"<%s>\r\n"
                  "TO:<%s>\r\n"
                  "SUBJECT:%s\r\n"
                  "Date:%s\r\n"
                  "MIME-Version:1.0\r\n"
                  "X-Mailer:SMTP\r\n"
                  "Content-type: %s\r\n\r\n",
                  from_name, smtp->from, email->to, email->subject, date, content_type);

    length_of_content = (int32_t)_cc_sds_length(email->content);
    
    required_length = length_of_content + 5 + io->w.off;
    if (required_length > io->w.limit) {
        _cc_logger_error("email content too long. length:%d > limit:%d", length_of_content, io->w.limit - 5 - io->w.off);
        return false;
    }

    memcpy(&io->w.bytes[io->w.off], email->content, length_of_content);
    io->w.off += length_of_content;
    memcpy(&io->w.bytes[io->w.off], "\r\n.\r\n", 5);
    io->w.off += 5;

    return true;
}

_CC_API_PUBLIC(_cc_smtp_t *) _cc_alloc_smtp(const char_t *from_name, const char_t *from) {
    _cc_smtp_t *smtp = (_cc_smtp_t *)_cc_malloc(sizeof(_cc_smtp_t));
    smtp->io = _cc_alloc_io_buffer(_CC_16K_BUFFER_SIZE_);

    smtp->user = NULL;
    smtp->password = NULL;
    smtp->from = _cc_sds_alloc(from, 0);
    smtp->from_name = _cc_sds_alloc(from_name, 0);
    smtp->is_logined = false;
    smtp->login_mode = _CC_SMTP_LOGIN_MODE_PLAIN_;
    smtp->state = _CC_LIBSMTP_RESP_PENDING_;
    smtp->response_cb = NULL;

    smtp->lock = 0;
    smtp->email = NULL;
    _cc_list_cleanup(&smtp->emails);

    return smtp;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_smtp_set_login(_cc_smtp_t *smtp, byte_t login_mode, const char_t *user, const char_t *password) {

    if (smtp->user) {
        _cc_sds_free(smtp->user);
        smtp->user = NULL;
    }

    if (smtp->password) {
        _cc_sds_free(smtp->password);
        smtp->password = NULL;
    }
    smtp->is_logined = false;
    if (login_mode == _CC_SMTP_LOGIN_MODE_PLAIN_) {
        if (user) {
            size_t length = strlen(user);
            size_t base64_length = sizeof(char_t) * _CC_BASE64_EN_LEN(length);
            smtp->user = _cc_sds_alloc(NULL, base64_length);
            base64_length = _cc_base64_encode((byte_t*)user, length, smtp->user, base64_length);
            length = strlen(smtp->user);
            _cc_sds_set_length(smtp->user, base64_length);
        }
        if (password) {
            size_t length = strlen(password);
            size_t base64_length = sizeof(char_t) * _CC_BASE64_EN_LEN(length);
            smtp->password = _cc_sds_alloc(NULL, base64_length);
            base64_length = _cc_base64_encode((byte_t*)password, length, smtp->password, base64_length);
            length = strlen(smtp->password);
            _cc_sds_set_length(smtp->password, base64_length);
        }
        smtp->login_mode =  _CC_SMTP_LOGIN_MODE_PLAIN_;
        return true;
    } else {
        char_t auth[1024 * 4];
        size_t length = snprintf(auth,_cc_countof(auth), "user=%s%cauth=Bearer %s%c%c", user, 0x01, password, 0x01, 0x01);
        size_t base64_length = sizeof(char_t) * _CC_BASE64_EN_LEN(length);
        smtp->user = _cc_sds_alloc(NULL, base64_length);
        length = _cc_base64_encode((byte_t*)auth, length, smtp->password, base64_length);
        _cc_sds_set_length(smtp->user, base64_length);
        smtp->password = NULL;
        smtp->login_mode =  _CC_SMTP_LOGIN_MODE_XOAUTH2;
        return true;
    }
    return false;
}

/**/
_CC_API_PUBLIC(_cc_email_t*) _cc_alloc_email(byte_t mail_type, const char_t *from_name, const char_t *to) {
    _cc_email_t *email = (_cc_email_t *)_cc_malloc(sizeof(_cc_email_t));
    email->to = _cc_sds_alloc(to,0);
    email->mail_type = mail_type;
    if (from_name) {
        email->from_name = _cc_sds_alloc(from_name,0);
    } else {
        email->from_name = NULL;
    }
    return email;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_set_email(_cc_email_t *email, const char_t *subject, const char_t *content) {
    if (subject) {
        email->subject = _cc_sds_alloc(subject, 0);
    } else {
        email->subject = NULL;
    }
    if (content) {
        email->content = _cc_sds_alloc(content, 0);
    } else {
        email->content = NULL;
    }
    return true;
}

/**/
_CC_API_PUBLIC(void) _cc_smtp_set_email(_cc_smtp_t *smtp, _cc_email_t *email) {
    _cc_spin_lock(&smtp->lock);
    _cc_list_push(&smtp->emails, &email->lnk);
    _cc_unlock(&smtp->lock);
}

/**/
_CC_API_PUBLIC(void) _cc_free_email(_cc_email_t *email) {
    if (email->subject) {
        _cc_sds_free(email->subject);
    }

    if (email->content) {
        _cc_sds_free(email->content);
    }

    if (email->to) {
        _cc_sds_free(email->to);
    }

    if (email->from_name) {
        _cc_sds_free(email->from_name);
    }

    _cc_free(email);
}

/**/
_CC_API_PUBLIC(void) _cc_free_smtp(_cc_smtp_t *smtp) {
    _cc_free_io_buffer(smtp->io);
    if (smtp->user) {
        _cc_sds_free(smtp->user);
    }

    if (smtp->password) {
        _cc_sds_free(smtp->password);
    }

    if (smtp->from) {
        _cc_sds_free(smtp->from);
    }

    if (smtp->from_name) {
        _cc_sds_free(smtp->from_name);
    }

    if (smtp->email) {
        _cc_free_email(smtp->email);
    }

    _cc_list_for_each(it, &smtp->emails,{
        _cc_free_email(_cc_upcast(it,_cc_email_t, lnk));
    });

    _cc_free(smtp);
}
