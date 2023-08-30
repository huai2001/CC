#include <cc/widgets/ftp.h>

static bool_t libftp_mkd(_cc_ftp_t* ftp,
                                    const byte_t* buf,
                                    uint32_t len) {
    if (ftp->resp.flag != _CC_LIBFTP_RESP_MKDIR) {
        return false;
    }

    ftp->resp.flag = _CC_LIBFTP_RESP_PENDING;

    if ((buf[0] == '2' && buf[1] == '5' && buf[2] == '7')) {
        return ftp->callback(ftp, _CC_LIBFTP_MKDIR);
    }

    libftp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    ftp->callback(ftp, _CC_LIBFTP_MKDIR_FAILED);
    return false;
}

bool_t _cc_ftp_mkdir(_cc_ftp_t* ftp, const char_t* path) {
    char_t cmd[_CC_MAX_PATH_];
    int32_t cmd_len = 0;

    _cc_assert(ftp != NULL);
    _cc_assert(path != NULL);

    if (ftp == NULL || path == NULL)
        return false;

    if (ftp->ctrl.e == NULL) {
        _cc_logger_error(_T("Not connected to FTP server"));
        return false;
    }

    if (ftp->resp.flag != _CC_LIBFTP_RESP_PENDING) {
        return false;
    }

    libftp_setup(ftp, _CC_LIBFTP_RESP_MKDIR, libftp_mkd, NULL);

    cmd_len = snprintf(cmd, _cc_countof(cmd), "MKD %s\r\n", path);
    _ftp_send_command(ftp->ctrl.e, cmd, cmd_len * sizeof(char_t));

    return true;
}
