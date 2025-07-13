#include <libcc/widgets/ftp.h>

_CC_API_PRIVATE(bool_t) libftp_list(_cc_ftp_t* ftp,
                                     const byte_t* buf,
                                     uint32_t len) {
    if (ftp->resp.flag != _CC_LIBFTP_RESP_LIST)
        return false;

    /**/
    if (buf[0] == '1' && buf[1] == '5' && buf[2] == '0') {
        return ftp->callback(ftp, _CC_LIBFTP_LIST_WAITING);
    }

    ftp->resp.flag = _CC_LIBFTP_RESP_PENDING;

    if (buf[0] == '2' && buf[1] == '2' && buf[2] == '6') {
        return ftp->callback(ftp, _CC_LIBFTP_LIST);
    }

    libftp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    ftp->callback(ftp, _CC_LIBFTP_LIST_FAILED);
    return false;
}

bool_t _cc_ftp_list(_cc_ftp_t* ftp, const char_t* path) {
    _cc_assert(ftp != nullptr);
    char_t cmd[_CC_MAX_PATH_];
    int32_t cmd_len = 0;

    if (ftp == nullptr)
        return false;

    if (ftp->ctrl.e == nullptr) {
        _cc_logger_error(_T("Not connected to FTP server"));
        return false;
    }

    if (ftp->resp.flag != _CC_LIBFTP_RESP_PENDING) {
        return false;
    }

    libftp_setup(ftp, _CC_LIBFTP_RESP_LIST, libftp_list, nullptr);

    if (path == nullptr) {
        _ftp_send_command(ftp->ctrl.e, "LIST\r\n", 6 * sizeof(char_t));
    } else {
        cmd_len = _snprintf(cmd, _cc_countof(cmd), "LIST %s\r\n", path);
        _ftp_send_command(ftp->ctrl.e, cmd, cmd_len * sizeof(char_t));
    }

    return true;
}
