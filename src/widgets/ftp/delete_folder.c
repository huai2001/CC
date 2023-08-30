#include <cc/widgets/ftp.h>

static bool_t libftp_delete_folder(_cc_ftp_t* ftp,
                                              const byte_t* buf,
                                              uint32_t len) {
    if (ftp->resp.flag != _CC_LIBFTP_RESP_DEL_FOLDER) {
        return false;
    }

    ftp->resp.flag = _CC_LIBFTP_RESP_PENDING;

    // 250
    if (buf[0] == '2' && buf[1] == '5' && buf[2] == '0') {
        ftp->callback(ftp, _CC_LIBFTP_DEL_FOLDER);
    }

    libftp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    ftp->callback(ftp, _CC_LIBFTP_DEL_FOLDER_FAILED);
    return false;
}

bool_t _cc_ftp_del_folder(_cc_ftp_t* ftp, const char_t* folder) {
    char_t cmd[_CC_MAX_PATH_];
    int32_t cmd_len = 0;

    _cc_assert(ftp != NULL);
    _cc_assert(folder != NULL);

    if (ftp == NULL || folder == NULL)
        return false;

    if (ftp->ctrl.e == NULL) {
        _cc_logger_error(_T("Not connected to FTP server"));
        return false;
    }

    if (ftp->resp.flag != _CC_LIBFTP_RESP_PENDING) {
        return false;
    }
    libftp_setup(ftp, _CC_LIBFTP_RESP_DEL_FOLDER, libftp_delete_folder, NULL);

    cmd_len = snprintf(cmd, _cc_countof(cmd), "RMD %s\r\n", folder);

    _ftp_send_command(ftp->ctrl.e, cmd, cmd_len * sizeof(char_t));
    return true;
}
