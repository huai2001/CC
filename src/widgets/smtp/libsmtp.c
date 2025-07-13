
#include <libcc/widgets/smtp.h>

static tchar_t libsmtp_error_buf[512];
/**/
const char_t* _cc_smtp_get_error(void) {
    return libsmtp_error_buf;
}

/**/
void libsmtp_set_error_info(const char_t* p, int32_t len) {
    if (p && len > 0) {
        strncpy(libsmtp_error_buf, p, len);
        libsmtp_error_buf[_cc_countof(libsmtp_error_buf) - 1] = 0;
    }
}

/**/
void libsmtp_setup(_cc_smtp_t* smtp, uint16_t flag, _cc_smtp_resp_callback_t fn, pvoid_t data) {
    smtp->resp.flag = flag;
    smtp->resp.callback = fn;
    smtp->resp.data = data;

    libsmtp_error_buf[0] = 0;
}

_CC_API_PRIVATE(bool_t) libsmtp_send_email(_cc_smtp_t* smtp, const byte_t* buf, uint32_t len) {
    if (smtp->resp.flag != _CC_LIBSMTP_RESP_SEND_EMAIL) {
        return false;
    }

    smtp->resp.flag = _CC_LIBSMTP_RESP_PENDING;

    if (buf[0] == '2' && buf[1] == '5' && buf[2] == '0') {
        smtp->callback(smtp, _CC_LIBSMTP_SEND_EMAIL_SUCCESS);
        return true;
    }

    libsmtp_set_error_info((const char_t*)buf, len / sizeof(char_t));
    smtp->callback(smtp, _CC_LIBSMTP_SEND_EMAIL_FAILED);
    return false;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_send_email(_cc_smtp_t* smtp,
                      const char_t* from_name,
                      const char_t* subject,
                      const char_t* content) {
    char_t head[10240];
    uint32_t len = 0;
    char_t date[128];

    char* content_type = "text/plain;charset=UTF-8";
    time_t seconds = time(nullptr);
    struct tm* m = gmtime(&seconds);

    _cc_assert(smtp != nullptr);
    _cc_assert(subject != nullptr);
    _cc_assert(content != nullptr);

    if (smtp == nullptr)
        return false;

    if (smtp->ctrl.e == nullptr) {
        _cc_logger_error(_T("Not connected to SMTP server"));
        return false;
    }

    if (smtp->resp.flag != _CC_LIBSMTP_RESP_PENDING) {
        return false;
    }
    if (smtp->mailtype == _CC_SMTP_HTML) {
        content_type = "text/html;charset=UTF-8";
    }

    strftime(date, _cc_countof(date), "%a, %d %b %Y %H:%M:%S GMT", m);
    len = _snprintf(head, _cc_countof(head),
                  "FROM:\"%s\"<%s>\r\n"
                  "TO:<%s>\r\n"
                  "SUBJECT:%s\r\n"
                  "Date:%s\r\n"
                  "MIME-Version:1.0\r\n"
                  "X-Mailer:SMTP\r\n"
                  "Content-type: %s\r\n"
                  "\r\n",
                  from_name, smtp->from, smtp->to, subject, date, content_type);

    libsmtp_setup(smtp, _CC_LIBSMTP_RESP_SEND_EMAIL, libsmtp_send_email, nullptr);

    _cc_event_send(smtp->ctrl.e, (byte_t*)head, len * sizeof(char_t));
    _cc_event_send(smtp->ctrl.e, (byte_t*)content,
                   strlen(content) * sizeof(char_t));
    _cc_event_send(smtp->ctrl.e, (byte_t*)"\r\n.\r\n", 5);

    return true;
}