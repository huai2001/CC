/**/
#include <libcc/alloc.h>
#include <libcc/widgets/smtp.h>

_CC_API_PRIVATE(bool_t) libsmtp_EHLO(_cc_smtp_t* smtp, const byte_t* buf, uint32_t len) {
    if (smtp->resp.flag != _CC_LIBSMTP_RESP_EHLO) {
        return false;
    }

    smtp->resp.flag = _CC_LIBSMTP_RESP_PENDING;

    if (buf[0] == '2' && buf[1] == '5' && buf[2] == '0') {
        smtp->callback(smtp, _CC_LIBSMTP_CONNECTED);
        return true;
    }

    libsmtp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    return smtp->callback(smtp, _CC_LIBSMTP_CONNECT_FAILED);
}

_CC_API_PRIVATE(bool_t) sendEHLO(_cc_smtp_t* smtp) {
    char_t pcname[256] = {0};
    libsmtp_setup(smtp, _CC_LIBSMTP_RESP_EHLO, libsmtp_EHLO, nullptr);

    _cc_get_computer_name(pcname, _cc_countof(pcname));

    return _cc_event_writef(smtp->ctrl.e, "EHLO %s\r\n", pcname);
}

/**/
_CC_API_PRIVATE(bool_t) libsmtp_connected(_cc_smtp_t* smtp, const byte_t* buf, uint32_t len) {
    if (smtp->resp.flag != _CC_LIBSMTP_RESP_CONNECTED) {
        return false;
    }

    // 220
    smtp->resp.flag = _CC_LIBSMTP_RESP_PENDING;

    if (buf[0] == '2' && buf[1] == '2' && buf[2] == '0') {
        return sendEHLO(smtp);
    }

    libsmtp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    return smtp->callback(smtp, _CC_LIBSMTP_CONNECT_FAILED);
}

/**/
_CC_API_PUBLIC(bool_t) _cc_smtp_connected(_cc_smtp_t* smtp) {
    if (smtp == nullptr) {
        return false;
    }

    libsmtp_setup(smtp, _CC_LIBSMTP_RESP_CONNECTED, libsmtp_connected, nullptr);

    smtp->logined = false;
    smtp->user = nullptr;
    smtp->password = nullptr;
    smtp->from = nullptr;
    smtp->to = nullptr;

    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_smtp_disconnected(_cc_smtp_t* smtp) {
    if (smtp == nullptr) {
        return false;
    }

    if (smtp->user) {
        _cc_free(smtp->user);
        smtp->user = nullptr;
    }

    if (smtp->password) {
        _cc_free(smtp->password);
        smtp->password = nullptr;
    }

    if (smtp->from) {
        _cc_free(smtp->from);
        smtp->from = nullptr;
    }

    if (smtp->to) {
        _cc_free(smtp->to);
        smtp->to = nullptr;
    }

    return true;
}
