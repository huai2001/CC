/**/
#include <libcc/smtp.h>

_CC_API_PRIVATE(bool_t) libsmtp_EHLO(_cc_smtp_t* smtp, const byte_t* buf, uint32_t length) {
    if (smtp->state != _CC_LIBSMTP_RESP_EHLO_) {
        return false;
    }
    
    if (buf[0] == '2' && buf[1] == '5' && buf[2] == '0') {
        if (buf[3] == 0x20) {
            smtp->state = _CC_LIBSMTP_RESP_PENDING_;
            return libsmtp_login(smtp);
        }
        return true;
    }
    return false;
}

_CC_API_PRIVATE(bool_t) sendEHLO(_cc_smtp_t* smtp) {
    char_t pcname[256] = {0};
    libsmtp_setup(smtp, _CC_LIBSMTP_RESP_EHLO_, libsmtp_EHLO);

    _cc_get_device_name(pcname, _cc_countof(pcname));

    return libsmtp_command(smtp, "EHLO %s\r\n", pcname);
}

/**/
_CC_API_PRIVATE(bool_t) libsmtp_connected(_cc_smtp_t* smtp, const byte_t* buf, uint32_t length) {
    if (smtp->state != _CC_LIBSMTP_RESP_CONNECTED_) {
        return false;
    }
    
    // 220
    smtp->state = _CC_LIBSMTP_RESP_PENDING_;
    if (buf[0] == '2' && buf[1] == '2' && buf[2] == '0') {
        return sendEHLO(smtp);
    }

    return false;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_smtp_connected(_cc_smtp_t* smtp) {
    if (smtp == nullptr) {
        return false;
    }
    smtp->io->w.off = 0;
    smtp->io->r.off = 0;
    libsmtp_setup(smtp, _CC_LIBSMTP_RESP_CONNECTED_, libsmtp_connected);
    return true;
}
