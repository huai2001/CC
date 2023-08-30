/**/
#include <cc/alloc.h>
#include <cc/widgets/ftp.h>

static bool_t libftp_connected(_cc_ftp_t* ftp,
                                          const byte_t* buf,
                                          uint32_t len) {
    if (ftp->resp.flag != _CC_LIBFTP_RESP_CONNECTED) {
        return false;
    }

    // 220
    ftp->resp.flag = _CC_LIBFTP_RESP_PENDING;

    if (buf[0] == '2' && buf[1] == '2' && buf[2] == '0') {
        ftp->callback(ftp, _CC_LIBFTP_CONNECTED);
        return true;
    }

    libftp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    ftp->callback(ftp, _CC_LIBFTP_CONNECT_FAILED);
    return false;
}

bool_t _cc_ftp_connected(_cc_ftp_t* ftp) {
    if (ftp == NULL)
        return false;

    libftp_setup(ftp, _CC_LIBFTP_RESP_CONNECTED, libftp_connected, NULL);

    ftp->logined = false;
    ftp->user = NULL;
    ftp->password = NULL;

    return true;
}

bool_t _cc_ftp_disconnected(_cc_ftp_t* ftp) {
    if (ftp == NULL)
        return false;

    if (ftp->user) {
        _cc_free(ftp->user);
        ftp->user = NULL;
    }

    if (ftp->password) {
        _cc_free(ftp->password);
        ftp->password = NULL;
    }
    return true;
}
