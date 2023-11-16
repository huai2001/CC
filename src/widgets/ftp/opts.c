#include <cc/widgets/ftp.h>

_CC_API_PRIVATE(bool_t) libftp_opts(_cc_ftp_t* ftp,
                                     const byte_t* buf,
                                     uint32_t len) {
    uint16_t flag = ftp->resp.flag;
    ftp->resp.flag = _CC_LIBFTP_RESP_PENDING;

    switch (flag) {
        case _CC_LIBFTP_RESP_OPTS_DATATYPE: {
            if (buf[0] == '2' && buf[1] == '0' && buf[2] == '0') {
                ftp->callback(ftp, _CC_LIBFTP_OPTS_DATATYPE);
                return true;
            }
        } break;
        case _CC_LIBFTP_RESP_OPTS_SYST:
            return true;
        case _CC_LIBFTP_RESP_OPTS_UTF8: {
            if (buf[0] == '2' && buf[1] == '0' && buf[2] == '0') {
                ftp->callback(ftp, _CC_LIBFTP_OPTS_UTF8);
                return true;
            }
        } break;
        case _CC_LIBFTP_RESP_OPTS_PASV: {
            int32_t addr[6];
            if (buf[0] == '2' && buf[1] == '2' && buf[2] == '7') {
                char_t* cp = strchr((const char_t*)buf, '(');
                printf("%s\n", buf);
                if (cp && cp++) {
                    sscanf(cp, "%u,%u,%u,%u,%u,%u", &addr[0], &addr[1],
                           &addr[2], &addr[3], &addr[4], &addr[5]);
                    bzero(&ftp->sa, sizeof(ftp->sa));
                    ftp->sa.sa_family = AF_INET;
                    ftp->sa.sa_data[2] = addr[0];
                    ftp->sa.sa_data[3] = addr[1];
                    ftp->sa.sa_data[4] = addr[2];
                    ftp->sa.sa_data[5] = addr[3];
                    ftp->sa.sa_data[0] = addr[4];
                    ftp->sa.sa_data[1] = addr[5];

                    ftp->callback(ftp, _CC_LIBFTP_OPTS_PASV);
                    return true;
                }
            }
            break;
        }
        case _CC_LIBFTP_RESP_OPTS_PORT: {
            if (buf[0] == '2' && buf[1] == '0' && buf[2] == '0') {
                ftp->callback(ftp, _CC_LIBFTP_OPTS_PORT);
                return true;
            }

            libftp_set_error_info((const char_t*)buf, len / sizeof(char_t));
            ftp->callback(ftp, _CC_LIBFTP_OPTS_PORT_FAILED);
            return false;
        }
    }

    libftp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    ftp->callback(ftp, _CC_LIBFTP_OPTS_FAILED);
    return false;
}

bool_t _cc_ftp_opts_port_passive(_cc_ftp_t* ftp) {
    if (ftp->cmode == _CC_LIBFTP_PASSIVE) {
        libftp_setup(ftp, _CC_LIBFTP_RESP_OPTS_PASV, libftp_opts, NULL);
        _ftp_send_command(ftp->ctrl.e, "PASV\r\n", 6 * sizeof(char_t));
        return true;
    } else {
        socklen_t l = sizeof(ftp->sa);
        if (getsockname(ftp->ctrl.e->fd, &ftp->sa, &l) < 0) {
            return false;
        }
        ftp->callback(ftp, _CC_LIBFTP_OPTS_PASV);
        return true;
    }
    return false;
}

bool_t _cc_ftp_opts_datatype(_cc_ftp_t* ftp) {
    char_t cmd[256];
    int32_t cmd_len = 0;

    _cc_assert(ftp != NULL);

    if (ftp == NULL)
        return false;

    if (ftp->ctrl.e == NULL) {
        _cc_logger_error(_T("Not connected to FTP server"));
        return false;
    }

    if (ftp->resp.flag != _CC_LIBFTP_RESP_PENDING) {
        return false;
    }
    libftp_setup(ftp, _CC_LIBFTP_RESP_OPTS_DATATYPE, libftp_opts, NULL);

    cmd_len = _snprintf(cmd, _cc_countof(cmd), "TYPE %c\r\n", ftp->smode);
    _ftp_send_command(ftp->ctrl.e, cmd, cmd_len * sizeof(char_t));

    return true;
}

bool_t _cc_ftp_opts_utf8(_cc_ftp_t* ftp) {
    _cc_assert(ftp != NULL);

    if (ftp == NULL)
        return false;

    if (ftp->ctrl.e == NULL) {
        _cc_logger_error(_T("Not connected to FTP server"));
        return false;
    }

    if (ftp->resp.flag != _CC_LIBFTP_RESP_PENDING) {
        return false;
    }

    libftp_setup(ftp, _CC_LIBFTP_RESP_OPTS_UTF8, libftp_opts, NULL);

    _ftp_send_command(ftp->ctrl.e, "OPTS UTF8 ON\r\n", 14 * sizeof(char_t));
    return true;
}

bool_t _cc_ftp_open_port(_cc_ftp_t* ftp) {
    char_t cmd[256];
    int32_t len = 0;
    int32_t err = 0;
    _cc_assert(ftp != NULL);
    _cc_socklen_t l = 0;
    if (ftp == NULL)
        return false;

    if (ftp->ctrl.e == NULL) {
        _cc_logger_error(_T("Not connected to FTP server"));
        return false;
    }

    if (ftp->resp.flag != _CC_LIBFTP_RESP_PENDING) {
        return false;
    }

    l = sizeof(ftp->sa);
    if (getsockname(ftp->data.e->fd, &ftp->sa, &l) < 0) {
        _cc_logger_error(_T("getsockname error(%d) %s"), err,
                         _cc_last_error(err));
        return false;
    }

    len = snprintf(
        cmd, _cc_countof(cmd), "PORT %d,%d,%d,%d,%d,%d\r\n",
        (unsigned char)ftp->sa.sa_data[2], (unsigned char)ftp->sa.sa_data[3],
        (unsigned char)ftp->sa.sa_data[4], (unsigned char)ftp->sa.sa_data[5],
        (unsigned char)ftp->sa.sa_data[0], (unsigned char)ftp->sa.sa_data[1]);

    libftp_setup(ftp, _CC_LIBFTP_RESP_OPTS_PORT, libftp_opts, NULL);

    _ftp_send_command(ftp->ctrl.e, cmd, len);
    return true;
}
