#include <cc/widgets/smtp.h>
#include <libcc.h>
#include <locale.h>
#include <stdio.h>

static bool_t network_event_close(_cc_event_cycle_t* cycle,
                                             _cc_event_t* e) {
    if (e->args)
        _cc_smtp_disconnected((_cc_smtp_t*)e->args);

    return true;
}

static bool_t network_event_callback(_cc_event_cycle_t* cycle,
                                                _cc_event_t* e,
                                                const uint16_t events) {
    _cc_smtp_t* smtp = (_cc_smtp_t*)e->args;
    /*成功连接服务器*/
    if (events & _CC_EVENT_CONNECTED_) {
        _tprintf(_T("%d connect to server .\n"), e->fd);

        if (!_cc_smtp_connected(smtp)) {
            network_event_close(cycle, e);
            return false;
        }
        if (events == _CC_EVENT_CONNECTED_)
            return true;  //
    }

    /*无法连接*/
    if (events & _CC_EVENT_DISCONNECT_) {
        _tprintf(_T("%d disconnect to server.\n"), e->fd);

        network_event_close(cycle, e);
        return false;
    }

    /*有数据可以读*/
    if (events & _CC_EVENT_READABLE_) {
        int32_t len = 0;
        byte_t buf[512];
        len = _cc_recv(e->fd, buf, sizeof(buf));
        if (len <= 0) {
            _tprintf(_T("TCP close %d\n"), e->fd);
            network_event_close(cycle, e);
            return false;
        }

        buf[len] = 0;
        printf("%s\n", buf);

        if (smtp && smtp->resp.callback) {
            if (smtp->resp.callback(smtp, buf, len)) {
                return true;
            }
        }
        network_event_close(cycle, e);
        return false;
    }

    /*可写数据*/
    if (events & _CC_EVENT_WRITABLE_) {
        _cc_event_send(e, NULL, 0);
        return true;
    }

    /*连接超时*/
    if (events & _CC_EVENT_TIMEOUT_) {
        _tprintf(_T("TCP timeout %d\n"), e->fd);
        network_event_close(cycle, e);
        return false;
    }

    return true;
}

static bool_t smtp_event_callback(_cc_smtp_t* smtp,
                                             uint16_t events) {
    switch (events) {
        case _CC_LIBSMTP_CONNECTED:
            printf("CC_LIBSMTP_CONNECTED\n");
            _cc_smtp_login(smtp, "yoouremail@163.com", "pass");
            return true;
        case _CC_LIBSMTP_LOGINED:
            printf("CC_LIBSMTP_LOGIN\n");
            _cc_smtp_from_to(smtp, "yoouremail@163.com", "huai2011@163.com");
            return true;
        case _CC_LIBSMTP_LOGOUT:
            printf("CC_LIBSMTP_LOGOUT\n");
            return false;
        case _CC_LIBSMTP_SEND_EMAIL:
            printf("CC_LIBSMTP_SEND_EMAIL\n");
            _cc_send_email(smtp, "nickname", "email subject.",
                           "test email content!.");
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
    if (smtp == NULL) {
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
    smtp->ctrl.e = _cc_tcp_connect(smtp->ctrl.cycle, _CC_EVENT_CONNECT_ | _CC_EVENT_TIMEOUT_,
        (_cc_sockaddr_t*)&sa, 60000, network_event_callback, smtp);
    if (smtp->ctrl.e == NULL) {
        _cc_logger_error(_T("Unable to connect to the network port %s:%d\n"), host, port);
    }

    return true;
}
