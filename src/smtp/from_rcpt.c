#include <libcc/smtp.h>

_CC_API_PRIVATE(bool_t) libsmtp_data(_cc_smtp_t* smtp, const byte_t* buf, uint32_t length) {
    if (smtp->state != _CC_LIBSMTP_RESP_DATA_) {
        return false;
    }

    smtp->state = _CC_LIBSMTP_RESP_PENDING_;

    if (buf[0] == '3' && buf[1] == '5' && buf[2] == '4') {
        return libsmtp_send_email(smtp);
    }
    return false;
}

_CC_API_PRIVATE(bool_t) libsmtp_rcpt_to(_cc_smtp_t* smtp, const byte_t* buf, uint32_t length) {
    _cc_io_buffer_t *io = smtp->io;
    if (smtp->state != _CC_LIBSMTP_RESP_RCPT_TO_) {
        return false;
    }
    smtp->state = _CC_LIBSMTP_RESP_PENDING_;

    if (buf[0] == '2' && buf[1] == '5' && buf[2] == '0') {
        libsmtp_setup(smtp, _CC_LIBSMTP_RESP_DATA_, libsmtp_data);
        memcpy(io->w.bytes + io->w.off, "DATA\r\n", 6);
        io->w.off += 6;
        return true;
    }
    return false;
}

_CC_API_PRIVATE(bool_t) libsmtp_mail_from(_cc_smtp_t* smtp, const byte_t* buf, uint32_t length) {
    _cc_assert(smtp != NULL);
    _cc_assert(smtp->io != NULL);
    _cc_assert(smtp->email != NULL);
    if (smtp->state != _CC_LIBSMTP_RESP_FROM_) {
        return false;
    }
    if (smtp->email == NULL) {
        return false;
    }

    smtp->state = _CC_LIBSMTP_RESP_PENDING_;

    if (buf[0] == '2' && buf[1] == '5' && buf[2] == '0') {
        libsmtp_setup(smtp, _CC_LIBSMTP_RESP_RCPT_TO_, libsmtp_rcpt_to);
        return libsmtp_command(smtp, "RCPT TO:<%s>\r\n", smtp->email->to);
    }
    return false;
}

_CC_API_PUBLIC(bool_t) libsmtp_from_to(_cc_smtp_t* smtp) {
    _cc_assert(smtp != NULL);
    _cc_assert(smtp->io != NULL);
    _cc_assert(smtp->from != NULL);

    if (smtp->state != _CC_LIBSMTP_RESP_PENDING_) {
        return false;
    }

    if (smtp->from == NULL) {
        return false;
    }

    if (smtp->email == NULL) {
        _cc_spin_lock(&smtp->lock);
        _cc_list_t *lnk =  _cc_list_pop(&smtp->emails);
        _cc_unlock(&smtp->lock);
        if (lnk == &smtp->emails) {
            return false;
        }
        smtp->email = _cc_upcast(lnk, _cc_email_t, lnk);
    }

    libsmtp_setup(smtp, _CC_LIBSMTP_RESP_FROM_, libsmtp_mail_from);

    return libsmtp_command(smtp, "MAIL FROM:<%s>\r\n", smtp->from);
}
