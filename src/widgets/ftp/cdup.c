#include <cc/widgets/ftp.h>

static bool_t libftp_cdup(_cc_ftp_t* ftp,
                                     const byte_t* buf,
                                     uint32_t len) {
    if (ftp->resp.flag != _CC_LIBFTP_RESP_CDUP) {
        return false;
    }
    // 250
    ftp->resp.flag = _CC_LIBFTP_RESP_PENDING;
    if (buf[0] == '2' && buf[1] == '5' && buf[2] == '0') {
        ftp->callback(ftp, _CC_LIBFTP_CWD);
    }

    libftp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    ftp->callback(ftp, _CC_LIBFTP_CWD_FAILED);
    return false;
}

bool_t _cc_ftp_cdup(_cc_ftp_t* ftp) {
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

    libftp_setup(ftp, _CC_LIBFTP_RESP_CDUP, libftp_cdup, NULL);

    _ftp_send_command(ftp->ctrl.e, "CDUP\r\n", 6 * sizeof(char_t));
    return true;
}
