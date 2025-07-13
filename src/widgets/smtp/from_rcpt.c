
#include <libcc/alloc.h>
#include <libcc/widgets/smtp.h>

_CC_API_PRIVATE(bool_t) libsmtp_data(_cc_smtp_t* smtp, const byte_t* buf, uint32_t len) {
    if (smtp->resp.flag != _CC_LIBSMTP_RESP_DATA)
        return false;

    smtp->resp.flag = _CC_LIBSMTP_RESP_PENDING;

    if (buf[0] == '3' && buf[1] == '5' && buf[2] == '4') {
        smtp->callback(smtp, _CC_LIBSMTP_SEND_EMAIL);
        return true;
    }

    libsmtp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    smtp->callback(smtp, _CC_LIBSMTP_MAIL_DATA_FAILED);
    return false;
}

_CC_API_PRIVATE(bool_t) libsmtp_rcpt_to(_cc_smtp_t* smtp, const byte_t* buf, uint32_t len) {
    if (smtp->resp.flag != _CC_LIBSMTP_RESP_RCPT_TO)
        return false;

    smtp->resp.flag = _CC_LIBSMTP_RESP_PENDING;

    if (buf[0] == '2' && buf[1] == '5' && buf[2] == '0') {
        libsmtp_setup(smtp, _CC_LIBSMTP_RESP_DATA, libsmtp_data, nullptr);
        _cc_event_send(smtp->ctrl.e, (byte_t*)"DATA\r\n", 6 * sizeof(char_t));
        return true;
    }

    libsmtp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    smtp->callback(smtp, _CC_LIBSMTP_RCPT_TO_FAILED);
    return false;
}

_CC_API_PRIVATE(bool_t) libsmtp_mail_from(_cc_smtp_t* smtp, const byte_t* buf, uint32_t len) {
    char_t cmd[256];
    int32_t cmd_len = 0;

    if (smtp->resp.flag != _CC_LIBSMTP_RESP_FROM)
        return false;

    smtp->resp.flag = _CC_LIBSMTP_RESP_PENDING;

    if (buf[0] == '2' && buf[1] == '5' && buf[2] == '0') {
        libsmtp_setup(smtp, _CC_LIBSMTP_RESP_RCPT_TO, libsmtp_rcpt_to, nullptr);
        return _cc_event_writef(smtp->ctrl.e, "RCPT TO:<%s>\r\n", smtp->to);
    }

    libsmtp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    smtp->callback(smtp, _CC_LIBSMTP_MAIL_FROM_FAILED);
    return false;
}

_CC_API_PUBLIC(bool_t) _cc_smtp_from_to(_cc_smtp_t* smtp, const char_t* from, const char_t* to) {
    _cc_assert(smtp != nullptr);
    _cc_assert(from != nullptr && to != nullptr);

    if (smtp->ctrl.e == nullptr) {
        _cc_logger_error(_T("Not connected to SMTP server"));
        return false;
    }

    if (smtp->resp.flag != _CC_LIBSMTP_RESP_PENDING) {
        return false;
    }

    smtp->from = _cc_strdupA(from);
    smtp->to = _cc_strdupA(to);

    libsmtp_setup(smtp, _CC_LIBSMTP_RESP_FROM, libsmtp_mail_from, nullptr);

    return _cc_event_writef(smtp->ctrl.e, "MAIL FROM:<%s>\r\n", smtp->from);
}
