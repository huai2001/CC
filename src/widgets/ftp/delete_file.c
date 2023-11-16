#include <cc/widgets/ftp.h>

_CC_API_PRIVATE(bool_t) libftp_delete_file(_cc_ftp_t* ftp,
                                            const byte_t* buf,
                                            uint32_t len) {
    if (ftp->resp.flag != _CC_LIBFTP_RESP_DEL_FILE) {
        return false;
    }
    ftp->resp.flag = _CC_LIBFTP_RESP_PENDING;
    // 250
    if (buf[0] == '2' && buf[1] == '5' && buf[2] == '0') {
        ftp->callback(ftp, _CC_LIBFTP_DEL_FILE);
    }

    libftp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    ftp->callback(ftp, _CC_LIBFTP_DEL_FILE_FAILED);
    return false;
}

bool_t _cc_ftp_del_file(_cc_ftp_t* ftp, const char_t* file) {
    char_t cmd[_CC_MAX_PATH_];
    int32_t cmd_len = 0;

    _cc_assert(ftp != NULL);
    _cc_assert(file != NULL);

    if (ftp == NULL || file == NULL)
        return false;

    if (ftp->ctrl.e == NULL) {
        _cc_logger_error(_T("Not connected to FTP server"));
        return false;
    }

    if (ftp->resp.flag != _CC_LIBFTP_RESP_PENDING) {
        return false;
    }

    libftp_setup(ftp, _CC_LIBFTP_RESP_DEL_FILE, libftp_delete_file, NULL);

    if (file) {
        cmd_len = snprintf(cmd, _cc_countof(cmd), "DELETE %s\r\n", file);
    } else {
        cmd_len = snprintf(cmd, _cc_countof(cmd), "MDELETE *\r\n");
    }

    _ftp_send_command(ftp->ctrl.e, cmd, cmd_len * sizeof(char_t));
    return true;
}
