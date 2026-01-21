#include <libcc/crypto/base64.h>
#include <libcc/smtp.h>

_CC_API_PRIVATE(bool_t) copy_command(_cc_io_buffer_t *io, _cc_sds_t data) {
    size_t length = _cc_sds_length(data);
    if (length + 2 > io->w.limit) {
        return false;
    }

    memcpy((io->w.bytes + io->w.off), data, length);
    io->w.off += length;

    *(io->w.bytes + io->w.off++) = '\r'; 
    *(io->w.bytes + io->w.off++) = '\n';

    return true;
}

_CC_API_PRIVATE(bool_t) libsmtp_quit_user(_cc_smtp_t* smtp, const byte_t* buf, uint32_t length) {
    if (smtp->state != _CC_LIBSMTP_RESP_LOGOUT_) {
        return false;
    }

    smtp->state = _CC_LIBSMTP_RESP_PENDING_;

    if (buf[0] == '2' && buf[1] == '2' && buf[2] == '1') {
        smtp->is_logined = false;
        return true;
    }

    return false;
}

_CC_API_PRIVATE(bool_t) libsmtp_login_password(_cc_smtp_t* smtp, const byte_t* buf, uint32_t length) {
    if (smtp->state != _CC_LIBSMTP_RESP_LOGIN_PASSWORD_) {
        return false;
    }

    smtp->state = _CC_LIBSMTP_RESP_PENDING_;

    if (buf[0] == '2' && buf[1] == '3' && buf[2] == '5') {
        smtp->is_logined = true;
        if (!_cc_list_iterator_empty(&smtp->emails)) {
            return libsmtp_from_to(smtp);
        }
    }

    return false;
}

_CC_API_PRIVATE(bool_t) libsmtp_login_user(_cc_smtp_t* smtp, const byte_t* buf, uint32_t length) {
    if (smtp->state != _CC_LIBSMTP_RESP_LOGIN_USER_) {
        return false;
    }
    smtp->state = _CC_LIBSMTP_RESP_PENDING_;
    // 331 Please specify the password.
    if (buf[0] == '3' && buf[1] == '3' && buf[2] == '4') {
        libsmtp_setup(smtp, _CC_LIBSMTP_RESP_LOGIN_PASSWORD_,libsmtp_login_password);
        return copy_command(smtp->io,smtp->password);
    }
    return false;
}

_CC_API_PRIVATE(bool_t) libsmtp_auth_login(_cc_smtp_t* smtp, const byte_t* buf, uint32_t length) {
    if (smtp->state != _CC_LIBSMTP_RESP_AUTH_LOGIN_) {
        return false;
    }

    smtp->state = _CC_LIBSMTP_RESP_PENDING_;
    if (buf[0] == '3' && buf[1] == '3' && buf[2] == '4') {
        libsmtp_setup(smtp, _CC_LIBSMTP_RESP_LOGIN_USER_, libsmtp_login_user);
        return copy_command(smtp->io,smtp->user);
    }
    return false;
}

_CC_API_PRIVATE(bool_t) libsmtp_xauth2_login(_cc_smtp_t* smtp, const byte_t* buf, uint32_t length) {
    if (smtp->state != _CC_LIBSMTP_RESP_AUTH_LOGIN_) {
        return false;
    }

    smtp->state = _CC_LIBSMTP_RESP_PENDING_;
    if (buf[0] == '3' && buf[1] == '3' && buf[2] == '4') {
        libsmtp_setup(smtp, _CC_LIBSMTP_RESP_LOGIN_USER_, libsmtp_login_user);
        return copy_command(smtp->io,smtp->user);
    }
    return false;
}

_CC_API_PUBLIC(bool_t) libsmtp_login(_cc_smtp_t* smtp) {
    _cc_io_buffer_t *io;
    _cc_assert(smtp != NULL);
    _cc_assert(smtp->io != NULL);

    if (smtp == NULL) {
        return false;
    }

    if (smtp->io == NULL) {
        _cc_logger(_CC_LOG_LEVEL_ERROR_, "Not connected to SMTP server");
        return false;
    }

    if (smtp->state != _CC_LIBSMTP_RESP_PENDING_) {
        return false;
    }

    io = (_cc_io_buffer_t *)smtp->io;
    if (smtp->login_mode == _CC_SMTP_LOGIN_MODE_PLAIN_) {
        libsmtp_setup(smtp, _CC_LIBSMTP_RESP_AUTH_LOGIN_, libsmtp_auth_login);
        memcpy(io->w.bytes + io->w.off, "AUTH LOGIN\r\n", 12 * sizeof(char_t));
        io->w.off += 12;
        return true;
    } else {
        libsmtp_setup(smtp, _CC_LIBSMTP_RESP_AUTH_LOGIN_, libsmtp_xauth2_login);
        memcpy(io->w.bytes + io->w.off, "AUTH XOAUTH2\r\n", 14 * sizeof(char_t));
        io->w.off += 14;
        return true;
    }
    return false;
}

_CC_API_PUBLIC(bool_t) _cc_smtp_logout(_cc_smtp_t* smtp) {
    _cc_io_buffer_t *io;
    _cc_assert(smtp != NULL && smtp->io != NULL);

    if (smtp == NULL || smtp->io == NULL) {
        return false;
    }

    libsmtp_setup(smtp, _CC_LIBSMTP_RESP_LOGOUT_, libsmtp_quit_user);

    io = (_cc_io_buffer_t *)smtp->io;
    memcpy(io->w.bytes + io->w.off, (byte_t*)"QUIT\r\n", 6 * sizeof(char_t));
    io->w.off += 6;
    return true;
}
