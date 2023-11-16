#include <cc/widgets/ftp.h>
#include <cc/alloc.h>

typedef struct _libftp_rename {
    char_t* src;
    char_t* dst;
} _libftp_rename_t;

_CC_API_PRIVATE(_libftp_rename_t*) libftp_alloc_rename(const char_t* src,
                                                        const char_t* dst) {
    _libftp_rename_t* rn = (_libftp_rename_t*)_cc_malloc(sizeof(_libftp_rename_t));

    rn->src = _cc_strdupA(src);
    rn->dst = _cc_strdupA(dst);

    return rn;
}

_CC_API_PRIVATE(void) libftp_free_rename(_libftp_rename_t* rn) {
    if (rn == NULL)
        return;

    if (rn->src)
        _cc_free(rn->src);

    if (rn->dst)
        _cc_free(rn->dst);

    _cc_free(rn);
}

_CC_API_PRIVATE(bool_t) libftp_rename_RNTO(_cc_ftp_t* ftp,
                                            const byte_t* buf,
                                            uint32_t len) {
    if (ftp->resp.flag != _CC_LIBFTP_RESP_RENAME_FILE) {
        return false;
    }

    ftp->resp.flag = _CC_LIBFTP_RESP_PENDING;

    if (buf[0] == '2' && buf[1] == '5' && buf[2] == '0') {
        ftp->callback(ftp, _CC_LIBFTP_RENAME);
        libftp_free_rename((_libftp_rename_t*)ftp->resp.data);
        return true;
    }

    libftp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    ftp->callback(ftp, _CC_LIBFTP_RENAME_FAILED);
    libftp_free_rename((_libftp_rename_t*)ftp->resp.data);
    return false;
}

_CC_API_PRIVATE(bool_t) libftp_rename_RNFR(_cc_ftp_t* ftp,
                                            const byte_t* buf,
                                            uint32_t len) {
    char_t cmd[_CC_MAX_PATH_];
    int32_t cmd_len = 0;
    _libftp_rename_t* rn = (_libftp_rename_t*)ftp->resp.data;

    if (ftp->resp.flag != _CC_LIBFTP_RESP_RENAME_FILE) {
        return false;
    }

    if (rn == NULL)
        return false;

    ftp->resp.flag = _CC_LIBFTP_RESP_PENDING;

    if (buf[0] == '3' && buf[1] == '5' && buf[2] == '0') {
        libftp_setup(ftp, _CC_LIBFTP_RESP_RENAME_FILE, libftp_rename_RNTO, rn);

        cmd_len =
            (int32_t)snprintf(cmd, _cc_countof(cmd), "RNTO %s\r\n", rn->dst);

        _ftp_send_command(ftp->ctrl.e, cmd, cmd_len * sizeof(char_t));
        return true;
    }

    libftp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    ftp->callback(ftp, _CC_LIBFTP_RENAME_FAILED);
    libftp_free_rename((_libftp_rename_t*)ftp->resp.data);
    return false;
}

bool_t _cc_ftp_rename_file(_cc_ftp_t* ftp,
                           const char_t* src,
                           const char_t* dst) {
    char_t cmd[_CC_MAX_PATH_];
    int32_t cmd_len = 0;
    _libftp_rename_t* rn = NULL;

    _cc_assert(ftp != NULL);
    _cc_assert(src != NULL);
    _cc_assert(dst != NULL);

    if (ftp == NULL || src == NULL || dst == NULL) {
        return false;
    }

    if (ftp->ctrl.e == NULL) {
        _cc_logger_error(_T("Not connected to FTP server"));
        return false;
    }

    if (ftp->resp.flag != _CC_LIBFTP_RESP_PENDING) {
        return false;
    }

    rn = libftp_alloc_rename(src, dst);
    if (rn == NULL)
        return false;

    libftp_setup(ftp, _CC_LIBFTP_RESP_RENAME_FILE, libftp_rename_RNFR, rn);

    cmd_len = snprintf(cmd, _cc_countof(cmd), "RNFR %s\r\n", rn->src);

    _ftp_send_command(ftp->ctrl.e, cmd, cmd_len * sizeof(char_t));
    return true;
}
