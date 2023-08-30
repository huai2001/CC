/**/
#include <cc/alloc.h>
#include <cc/widgets/smtp.h>

static bool_t libsmtp_EHLO(_cc_smtp_t* smtp,
                                      const byte_t* buf,
                                      uint32_t len) {
    if (smtp->resp.flag != _CC_LIBSMTP_RESP_EHLO) {
        return false;
    }

    smtp->resp.flag = _CC_LIBSMTP_RESP_PENDING;

    if (buf[0] == '2' && buf[1] == '5' && buf[2] == '0') {
        smtp->callback(smtp, _CC_LIBSMTP_CONNECTED);
        return true;
    }

    libsmtp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    smtp->callback(smtp, _CC_LIBSMTP_CONNECT_FAILED);

    return false;
}

static bool_t sendEHLO(_cc_smtp_t* smtp) {
    char_t cmd[1024];
    char_t pcname[256] = {0};
    int32_t cmd_len = 0;
    libsmtp_setup(smtp, _CC_LIBSMTP_RESP_EHLO, libsmtp_EHLO, NULL);

    _cc_get_computer_name(pcname, _cc_countof(pcname));

    cmd_len = _snprintf(cmd, _cc_countof(cmd), "EHLO %s\r\n", pcname);
    _cc_event_send(smtp->ctrl.e, (byte_t*)cmd, cmd_len * sizeof(char_t));

    return true;
}

/**/
static bool_t libsmtp_connected(_cc_smtp_t* smtp,
                                           const byte_t* buf,
                                           uint32_t len) {
    if (smtp->resp.flag != _CC_LIBSMTP_RESP_CONNECTED) {
        return false;
    }

    // 220
    smtp->resp.flag = _CC_LIBSMTP_RESP_PENDING;

    if (buf[0] == '2' && buf[1] == '2' && buf[2] == '0') {
        return sendEHLO(smtp);
    }

    libsmtp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    smtp->callback(smtp, _CC_LIBSMTP_CONNECT_FAILED);
    return false;
}

/**/
bool_t _cc_smtp_connected(_cc_smtp_t* smtp) {
    if (smtp == NULL)
        return false;

    libsmtp_setup(smtp, _CC_LIBSMTP_RESP_CONNECTED, libsmtp_connected, NULL);

    smtp->logined = false;
    smtp->user = NULL;
    smtp->password = NULL;
    smtp->from = NULL;
    smtp->to = NULL;

    return true;
}

/**/
bool_t _cc_smtp_disconnected(_cc_smtp_t* smtp) {
    if (smtp == NULL)
        return false;

    if (smtp->user) {
        _cc_free(smtp->user);
        smtp->user = NULL;
    }

    if (smtp->password) {
        _cc_free(smtp->password);
        smtp->password = NULL;
    }

    if (smtp->from) {
        _cc_free(smtp->from);
        smtp->from = NULL;
    }

    if (smtp->to) {
        _cc_free(smtp->to);
        smtp->to = NULL;
    }

    return true;
}
