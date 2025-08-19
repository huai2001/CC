#include <libcc/widgets/ftp.h>

// static char *version = "libftp Release 1.0,* Copyright 2018-2019 .ftp";

static tchar_t libftp_error_buf[512];
/**/
const char_t* _cc_ftp_get_error(void) {
    return libftp_error_buf;
}

/**/
int32_t _ftp_send_command(_cc_event_t* e, const pvoid_t buf, int32_t size) {
    printf("Send Command:%s\n", (char_t*)buf);
    return _cc_event_send(e, buf, size);
}

/**/
void libftp_set_error_info(const char_t* p, int32_t len) {
    if (p && len > 0) {
        strncpy(libftp_error_buf, p, len);
        libftp_error_buf[_cc_countof(libftp_error_buf) - 1] = 0;
    }
}

void libftp_setup(_cc_ftp_t* ftp,
                  uint16_t flag,
                  _cc_ftp_resp_callback_t fn,
                  pvoid_t data) {
    ftp->resp.flag = flag;
    ftp->resp.callback = fn;
    ftp->resp.data = data;

    libftp_error_buf[0] = 0;
}

bool_t _cc_ftp_unbind_accept(_cc_ftp_t* ftp) {
    if (ftp->data.accept.e && ftp->data.accept.async) {
        ftp->data.accept.async->disconnect(ftp->data.accept.async, ftp->data.accept.e);
        ftp->data.accept.e = nullptr;
        ftp->data.accept.async = nullptr;
    }
    return true;
}

bool_t _cc_ftp_bind_accept(_cc_ftp_t* ftp,
                           _cc_async_event_t *async,
                           _cc_event_t* e) {
    _cc_ftp_unbind_accept(ftp);

    ftp->data.accept.e = e;
    ftp->data.accept.async = async;

    return true;
}
