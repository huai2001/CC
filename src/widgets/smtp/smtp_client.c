#include <libcc/widgets/smtp.h>
#include <libcc.h>
#include <locale.h>
#include <stdio.h>

_CC_API_PRIVATE(bool_t) network_event_close(_cc_event_cycle_t* cycle, _cc_event_t* e) {
    if (e->args)
        _cc_smtp_disconnected((_cc_smtp_t*)e->args);

    return true;
}

_CC_API_PRIVATE(bool_t) network_event_callback(_cc_event_cycle_t* cycle, _cc_event_t* e, const uint16_t which) {
    _cc_smtp_t* smtp = (_cc_smtp_t*)e->args;
    /*成功连接服务器*/
    if (which & _CC_EVENT_CONNECTED_) {
        _tprintf(_T("%d connect to server .\n"), e->fd);

        if (!_cc_smtp_connected(smtp)) {
            network_event_close(cycle, e);
            return false;
        }
        if (which == _CC_EVENT_CONNECTED_) {
            return true;  //
        }
    }

    /*无法连接*/
    if (which & _CC_EVENT_DISCONNECT_) {
        _tprintf(_T("%d disconnect to server.\n"), e->fd);

        network_event_close(cycle, e);
        return false;
    }

    /*有数据可以读*/
    if (which & _CC_EVENT_READABLE_) {
        if (!_cc_event_recv(e)) {
            _tprintf(_T("TCP close %d\n"), e->fd);
            network_event_close(cycle, e);
            return false;
        }

        if (smtp && smtp->resp.callback && e->buffer->r.length > 0) {
            _cc_event_rbuf_t *r = &e->buffer->r;
            int i;
            int start = 0;
            for (i = 0; i < r->length; i++) {
                if (r->bytes[i] == '\n') {
                    r->bytes[i] = 0;
                    smtp->resp.callback(smtp, (char_t*)&r->bytes[start], i - start)
                    start = i + 1;
                }
            }

            r->length -= start;
            if (r->length > 0) {
                memmove(r->bytes, &r->bytes[start], r->length);
            }
        }
    }

    /*可写数据*/
    if (which & _CC_EVENT_WRITABLE_) {
        _cc_event_sendbuf(e);
        return true;
    }

    /*连接超时*/
    if (which & _CC_EVENT_TIMEOUT_) {
        _tprintf(_T("TCP timeout %d\n"), e->fd);
        network_event_close(cycle, e);
        return false;
    }

    return true;
}

_CC_API_PRIVATE(bool_t) smtp_event_callback(_cc_smtp_t* smtp, uint16_t which) {
    switch (which) {
        case _CC_LIBSMTP_CONNECTED:
            printf("CC_LIBSMTP_CONNECTED\n");
            _cc_smtp_login(smtp, "yoouremail@163.com", "pass");
            return true;
        case _CC_LIBSMTP_LOGINED:
            printf("CC_LIBSMTP_LOGIN\n");
            _cc_smtp_from_to(smtp, "yoouremail@163.com", "libcc.cn@gmail.com");
            return true;
        case _CC_LIBSMTP_LOGOUT:
            printf("CC_LIBSMTP_LOGOUT\n");
            return false;
        case _CC_LIBSMTP_SEND_EMAIL:
            printf("CC_LIBSMTP_SEND_EMAIL\n");
            _cc_send_email(smtp, "nickname", "email subject.", "test email content!.");
            return true;
        case _CC_LIBSMTP_CONNECT_FAILED:
            printf("CC_LIBSMTP_CONNECT_FAILED\n");
            break;
        case _CC_LIBSMTP_LOGIN_USER_FAILED:
            printf("CC_LIBSMTP_LOGIN_USER_ERROR\n");
            break;
        case _CC_LIBSMTP_LOGIN_PASSWORD_FAILED:
            printf("CC_LIBSMTP_LOGIN_PASSWORD_ERROR\n");
            break;
        case _CC_LIBSMTP_LOGOUT_FAILED:
            printf("CC_LIBSMTP_LOGOUT_FAILED\n");
            break;
        case _CC_LIBSMTP_MAIL_FROM_FAILED:
            printf("CC_LIBSMTP_MAIL_FROM_FAILED\n");
            break;
        case _CC_LIBSMTP_RCPT_TO_FAILED:
            printf("CC_LIBSMTP_RCPT_TO_FAILED\n");
            break;
        case _CC_LIBSMTP_MAIL_DATA_FAILED:
            printf("CC_LIBSMTP_MAIL_DATA_FAILED\n");
            break;
        case _CC_LIBSMTP_SEND_EMAIL_SUCCESS:
            _cc_smtp_logout(smtp);
            printf("CC_LIBSMTP_SEND_EMAIL_SUCCESS\n");
            break;
        case _CC_LIBSMTP_SEND_EMAIL_FAILED:
            printf("CC_LIBSMTP_SEND_EMAIL_FAILED\n");
            break;
    }
    _cc_smtp_logout(smtp);
    return true;
}

bool_t smtp_client(_cc_smtp_t* smtp, tchar_t *host, uint16_t port) {
    struct sockaddr_in sa;
    if (smtp == nullptr) {
        return false;
    }

    bzero(smtp, sizeof(_cc_smtp_t));
    smtp->callback = smtp_event_callback;
    smtp->logined = false;
    smtp->mailtype = _CC_SMTP_HTML;

    /*连接到服务端口为21*/
    //_T("smtp.163.com")
    _cc_inet_ipv4_addr(&sa, host, port);
    smtp->ctrl.cycle = _cc_get_event_cycle();
    smtp->ctrl.e = _cc_tcp_connect(smtp->ctrl.cycle, _CC_EVENT_BUFFER_|_CC_EVENT_CONNECT_ | _CC_EVENT_TIMEOUT_,
        (_cc_sockaddr_t*)&sa, 60000, network_event_callback, smtp);
    if (smtp->ctrl.e == nullptr) {
        _cc_logger_error(_T("Unable to connect to the network port %s:%d\n"), host, port);
    }

    return true;
}
