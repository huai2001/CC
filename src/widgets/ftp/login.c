#include <cc/widgets/ftp.h>
#include <cc/alloc.h>

static bool_t libftp_quit_user(_cc_ftp_t* ftp,
                                          const byte_t* buf,
                                          uint32_t len) {
    if (ftp->resp.flag != _CC_LIBFTP_RESP_LOGOUT)
        return false;

    ftp->resp.flag = _CC_LIBFTP_RESP_PENDING;

    if (buf[0] == '2' && buf[1] == '2' && buf[2] == '1') {
        ftp->logined = false;
        return ftp->callback(ftp, _CC_LIBFTP_LOGOUT);
    }

    libftp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    ftp->callback(ftp, _CC_LIBFTP_LOGOUT_FAILED);
    return false;
}

static bool_t libftp_login_password(_cc_ftp_t* ftp,
                                               const byte_t* buf,
                                               uint32_t len) {
    if (ftp->resp.flag != _CC_LIBFTP_RESP_LOGIN_PASSWORD) {
        return false;
    }

    ftp->resp.flag = _CC_LIBFTP_RESP_PENDING;

    if (buf[0] == '2' && buf[1] == '3' && buf[2] == '0') {
        ftp->logined = true;
        return ftp->callback(ftp, _CC_LIBFTP_LOGINED);
    }

    libftp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    ftp->callback(ftp, _CC_LIBFTP_LOGIN_PASSWORD_FAILED);

    return false;
}

static bool_t libftp_login_user(_cc_ftp_t* ftp,
                                           const byte_t* buf,
                                           uint32_t len) {
    char_t cmd[256];
    int32_t cmd_len = 0;

    if (ftp->resp.flag != _CC_LIBFTP_RESP_LOGIN_USER)
        return false;

    ftp->resp.flag = _CC_LIBFTP_RESP_PENDING;
    // 331 Please specify the password.
    if ((buf[0] == '2' && buf[1] == '2' && buf[2] == '0') ||
        (buf[0] == '3' && buf[1] == '3' && buf[2] == '1')) {
        libftp_setup(ftp, _CC_LIBFTP_RESP_LOGIN_PASSWORD, libftp_login_password,
                     NULL);

        if (ftp->password) {
            cmd_len =
                _snprintf(cmd, _cc_countof(cmd), "PASS %s\r\n", ftp->password);
        } else {
            cmd_len = _snprintf(cmd, _cc_countof(cmd), "PASS \r\n");
        }

        _ftp_send_command(ftp->ctrl.e, cmd, cmd_len * sizeof(char_t));
        return true;
    }

    libftp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    ftp->callback(ftp, _CC_LIBFTP_LOGIN_USER_FAILED);
    return false;
}

bool_t _cc_ftp_login(_cc_ftp_t* ftp,
                     const char_t* user,
                     const char_t* password) {
    char_t cmd[256];
    int32_t cmd_len = 0;

    _cc_assert(ftp != NULL);
    _cc_assert(user != NULL);

    if (ftp == NULL)
        return false;

    if (ftp->ctrl.e == NULL) {
        _cc_logger_error(_T("Not connected to FTP server"));
        return false;
    }

    if (ftp->resp.flag != _CC_LIBFTP_RESP_PENDING) {
        return false;
    }

    if (user) {
        ftp->user = _cc_strdupA(user);
    }

    if (password) {
        ftp->password = _cc_strdupA(password);
    }
    libftp_setup(ftp, _CC_LIBFTP_RESP_LOGIN_USER, libftp_login_user, NULL);

    cmd_len = _snprintf(cmd, _cc_countof(cmd), "USER %s\r\n", ftp->user);
    _ftp_send_command(ftp->ctrl.e, cmd, cmd_len * sizeof(char_t));
    return true;
}

bool_t _cc_ftp_logout(_cc_ftp_t* ftp) {
    _cc_assert(ftp != NULL);

    if (ftp == NULL)
        return false;

    ftp->resp.flag = _CC_LIBFTP_RESP_LOGOUT;
    ftp->resp.callback = libftp_quit_user;

    _ftp_send_command(ftp->ctrl.e, "QUIT\r\n", 6);

    return true;
}
