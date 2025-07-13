#include <libcc/widgets/ftp.h>

_CC_API_PRIVATE(bool_t) libftp_cwd(_cc_ftp_t* ftp,
                                    const byte_t* buf,
                                    uint32_t len) {
    if (ftp->resp.flag != _CC_LIBFTP_RESP_CWD) {
        return false;
    }
    // 250
    ftp->resp.flag = _CC_LIBFTP_RESP_PENDING;

    if (buf[0] == '2' && buf[1] == '5' && buf[2] == '0') {
        return ftp->callback(ftp, _CC_LIBFTP_CWD);
    }

    libftp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    ftp->callback(ftp, _CC_LIBFTP_CWD_FAILED);
    return false;
}

bool_t _cc_ftp_cwd(_cc_ftp_t* ftp, const char_t* path) {
    char_t cmd[_CC_MAX_PATH_];
    int32_t cmd_len = 0;

    _cc_assert(ftp != nullptr);
    _cc_assert(path != nullptr);

    if (ftp == nullptr || path == nullptr)
        return false;

    if (ftp->ctrl.e == nullptr) {
        _cc_logger_error(_T("Not connected to FTP server"));
        return false;
    }

    if (ftp->resp.flag != _CC_LIBFTP_RESP_PENDING) {
        return false;
    }

    libftp_setup(ftp, _CC_LIBFTP_RESP_CWD, libftp_cwd, nullptr);

    cmd_len = snprintf(cmd, _cc_countof(cmd), "CWD %s\r\n", path);

    _ftp_send_command(ftp->ctrl.e, cmd, cmd_len * sizeof(char_t));
    return true;
}
