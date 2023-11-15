#include <cc/base64.h>
#include <cc/alloc.h>
#include <cc/widgets/smtp.h>

_CC_API_PRIVATE(bool_t) libsmtp_quit_user(_cc_smtp_t* smtp,
                                           const byte_t* buf,
                                           uint32_t len) {
    if (smtp->resp.flag != _CC_LIBSMTP_RESP_LOGOUT)
        return false;

    smtp->resp.flag = _CC_LIBSMTP_RESP_PENDING;

    if (buf[0] == '2' && buf[1] == '2' && buf[2] == '1') {
        smtp->logined = false;
        return smtp->callback(smtp, _CC_LIBSMTP_LOGOUT);
    }

    libsmtp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    smtp->callback(smtp, _CC_LIBSMTP_LOGOUT_FAILED);
    return false;
}

_CC_API_PRIVATE(bool_t) libsmtp_login_password(_cc_smtp_t* smtp,
                                                const byte_t* buf,
                                                uint32_t len) {
    if (smtp->resp.flag != _CC_LIBSMTP_RESP_LOGIN_PASSWORD) {
        return false;
    }

    smtp->resp.flag = _CC_LIBSMTP_RESP_PENDING;

    if (buf[0] == '2' && buf[1] == '3' && buf[2] == '5') {
        smtp->logined = true;
        return smtp->callback(smtp, _CC_LIBSMTP_LOGINED);
    }

    libsmtp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    smtp->callback(smtp, _CC_LIBSMTP_LOGIN_PASSWORD_FAILED);

    return false;
}

_CC_API_PRIVATE(bool_t) libsmtp_login_user(_cc_smtp_t* smtp,
                                            const byte_t* buf,
                                            uint32_t len) {
    char_t cmd[256];
    int32_t cmd_len = 0;

    if (smtp->resp.flag != _CC_LIBSMTP_RESP_LOGIN_USER)
        return false;

    smtp->resp.flag = _CC_LIBSMTP_RESP_PENDING;
    // 331 Please specify the password.
    if (buf[0] == '3' && buf[1] == '3' && buf[2] == '4') {
        libsmtp_setup(smtp, _CC_LIBSMTP_RESP_LOGIN_PASSWORD,
                      libsmtp_login_password, NULL);
        cmd_len = _snprintf(cmd, _cc_countof(cmd), "%s\r\n", smtp->password);
        _cc_event_send(smtp->ctrl.e, (byte_t*)cmd, cmd_len * sizeof(char_t));
        return true;
    }

    libsmtp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    smtp->callback(smtp, _CC_LIBSMTP_LOGIN_USER_FAILED);
    return false;
}

_CC_API_PRIVATE(bool_t) libsmtp_auth_login(_cc_smtp_t* smtp,
                                            const byte_t* buf,
                                            uint32_t len) {
    char_t cmd[256];
    int32_t cmd_len = 0;

    if (smtp->resp.flag != _CC_LIBSMTP_RESP_AUTH_LOGIN)
        return false;

    smtp->resp.flag = _CC_LIBSMTP_RESP_PENDING;
    if (buf[0] == '3' && buf[1] == '3' && buf[2] == '4') {
        cmd_len = _snprintf(cmd, _cc_countof(cmd), "%s\r\n", smtp->user);
        libsmtp_setup(smtp, _CC_LIBSMTP_RESP_LOGIN_USER, libsmtp_login_user,
                      NULL);
        _cc_event_send(smtp->ctrl.e, (byte_t*)cmd, cmd_len * sizeof(char_t));
        return true;
    }

    libsmtp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    smtp->callback(smtp, _CC_LIBSMTP_LOGIN_FAILED);
    return false;
}

bool_t _cc_smtp_login(_cc_smtp_t* smtp,
                      const char_t* user,
                      const char_t* password) {
    _cc_assert(smtp != NULL);
    _cc_assert(user != NULL);

    if (smtp == NULL)
        return false;

    if (smtp->ctrl.e == NULL) {
        _cc_logger_error(_T("Not connected to SMTP server"));
        return false;
    }

    if (smtp->resp.flag != _CC_LIBSMTP_RESP_PENDING) {
        return false;
    }

    if (user) {
        int32_t ulen = (int32_t)strlen(user);
        int32_t xlen = sizeof(char_t) * _CC_BASE64_EN_LEN(ulen);
        smtp->user = (char_t*)_cc_malloc(xlen);
        if (smtp->user == NULL) {
            return false;
        }
        _cc_base64_encode((byte_t*)user, ulen, smtp->user, xlen);
    }

    if (password) {
        int32_t ulen = (int32_t)strlen(password);
        int32_t xlen = sizeof(char_t) * _CC_BASE64_EN_LEN(ulen);
        smtp->password = (char_t*)_cc_malloc(xlen);
        if (smtp->password == NULL) {
            _cc_free(smtp->user);
            smtp->user = NULL;
            return false;
        }
        _cc_base64_encode((byte_t*)password, ulen, smtp->password, xlen);
    }

    libsmtp_setup(smtp, _CC_LIBSMTP_RESP_AUTH_LOGIN, libsmtp_auth_login, NULL);
    _cc_event_send(smtp->ctrl.e, (byte_t*)"AUTH LOGIN\r\n", 12 * sizeof(char_t));

    return true;
}

bool_t _cc_smtp_logout(_cc_smtp_t* smtp) {
    _cc_assert(smtp != NULL);

    if (smtp == NULL)
        return false;

    libsmtp_setup(smtp, _CC_LIBSMTP_RESP_LOGOUT, libsmtp_quit_user, NULL);

    _cc_event_send(smtp->ctrl.e, (byte_t*)"QUIT\r\n", 6 * sizeof(char_t));

    return true;
}
