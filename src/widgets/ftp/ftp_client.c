#include <libcc/widgets/ftp.h>
#include <libcc.h>
#include <locale.h>
#include <stdio.h>


#if defined(_WIN32)
#define SETSOCKOPT_OPTVAL_TYPE (const char*)
#else
#define SETSOCKOPT_OPTVAL_TYPE (void*)
#endif

_CC_API_PRIVATE(bool_t) ftp_event_callback(_cc_ftp_t* ftp, uint16_t which);

_CC_API_PRIVATE(bool_t) network_event_close(_cc_async_event_t* async,
                                             _cc_event_t* e) {
    if (e->args)
        _cc_ftp_disconnected((_cc_ftp_t*)e->args);

    return true;
}

_CC_API_PRIVATE(bool_t) network_event_pasv_callback(_cc_async_event_t* async,
                                                     _cc_event_t* e,
                                                     const uint16_t which) {
    /*成功连接服务器*/
    if (which & _CC_EVENT_CONNECTED_) {
        _tprintf(_T("%d connect to server.\n"), e->fd);
        e->buffer = _cc_alloc_event_buffer();
        _cc_ftp_list((_cc_ftp_t*)e->args, nullptr);
        if (which == _CC_EVENT_CONNECTED_)
            return true;
    }

    /*无法连接*/
    if (which & _CC_EVENT_DISCONNECT_) {
        _tprintf(_T("%d disconnect to server.\n"), e->fd);
        return false;
    }

    /*有数据可以读*/
    if (which & _CC_EVENT_READABLE_) {
        _cc_event_rbuf_t* rbuf = &e->buffer->r;

        if (!_cc_event_recv(e)) {
            _tprintf(_T("PORT TCP close %d\n"), e->fd);

            rbuf->bytes[rbuf->length - 1] = 0;
            printf("%s\n", (char_t*)rbuf->bytes);
            rbuf->length = 0;

            network_event_close(async, e);
            return false;
        }

        if (rbuf->length >= rbuf->limit) {
            rbuf->bytes[rbuf->length - 1] = 0;
            printf("%s\n", (char_t*)rbuf->bytes);
            rbuf->length = 0;
        }

        return true;
    }

    /*可写数据*/
    if (which & _CC_EVENT_WRITABLE_) {
        _ftp_send_command(e, nullptr, 0);
        return true;
    }

    /*连接超时*/
    if (which & _CC_EVENT_TIMEOUT_) {
        _tprintf(_T("TCP timeout %d\n"), e->fd);
        network_event_close(async, e);
        return false;
    }
    return true;
}

_CC_API_PRIVATE(bool_t) network_event_port_callback(_cc_async_event_t* async,
                                                     _cc_event_t* e,
                                                     const uint16_t which) {
    /*成功连接服务器*/
    if (which & _CC_EVENT_ACCEPT_) {
        _cc_ftp_t* ftp = (_cc_ftp_t*)e->args;
        _cc_socket_t fd = _CC_INVALID_SOCKET_;
        _cc_event_t* new_event;
        struct sockaddr_in remote_addr = {0};
        _cc_socklen_t remote_addr_len = sizeof(struct sockaddr_in);
        _cc_async_event_t* new_cycle = _cc_get_event_cycle();

        fd = _cc_event_accept(async, e, &remote_addr, &remote_addr_len);
        if (fd == _CC_INVALID_SOCKET_) {
            _cc_logger_error(_T("thread %d accept fail.\n"), _cc_get_thread_id(nullptr));
            return true;
        }

        new_event = new_cycle->attach(new_cycle, _CC_EVENT_TIMEOUT_ | _CC_EVENT_READABLE_ | _CC_EVENT_BUFFER_, fd, 30000, network_event_port_callback, ftp);
        if (!new_event) {
            _cc_logger_error(_T("thread %d attach socket (%d) event fial.\n"), _cc_get_thread_id(nullptr), fd);
            _cc_close_socket(fd);
            return true;
        }

        {
            struct sockaddr_in* remote_ip = (struct sockaddr_in*)&remote_addr;
            byte_t* ip_addr = (byte_t*)&remote_ip->sin_addr.s_addr;
            _cc_logger_debug(_T("TCP accept [%d,%d,%d,%d] fd:%d\n"), ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3], fd);
        }

        return _cc_ftp_bind_accept(ftp, new_cycle, new_event);
    }

    /*无法连接*/
    if (which & _CC_EVENT_DISCONNECT_) {
        _tprintf(_T("%d disconnect to server.\n"), e->fd);
        _cc_ftp_unbind_accept((_cc_ftp_t*)e->args);
        return false;
    }

    /*有数据可以读*/
    if (which & _CC_EVENT_READABLE_) {
        _cc_event_rbuf_t* rbuf = &e->buffer->r;

        if (!_cc_event_recv(e)) {
            _tprintf(_T("PORT TCP close %d\n"), e->fd);

            rbuf->bytes[rbuf->length - 1] = 0;
            printf("%s\n", (char_t*)rbuf->bytes);
            rbuf->length = 0;

            network_event_close(async, e);
            return false;
        }

        if (rbuf->length >= rbuf->limit) {
            rbuf->bytes[rbuf->length - 1] = 0;
            printf("%s\n", (char_t*)rbuf->bytes);
            rbuf->length = 0;
        }

        return true;
    }

    /*可写数据*/
    if (which & _CC_EVENT_WRITABLE_) {
        if (e->buffer) {
            if (!_cc_event_sendbuf(e)) {
                _cc_ftp_unbind_accept((_cc_ftp_t*)e->args);
                return false;
            }
        } else {
            _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        }
        return true;
    }

    /*连接超时*/
    if (which & _CC_EVENT_TIMEOUT_) {
        _tprintf(_T("TCP timeout %d\n"), e->fd);
        _cc_ftp_unbind_accept((_cc_ftp_t*)e->args);
        return false;
    which
    return true;
}

_CC_API_PRIVATE(bool_t) network_event_callback(_cc_async_event_t* async,
                                                _cc_event_t* e,
                                                const uint16_t which) {
    /*成功连接服务器*/
    if (which & _CC_EVENT_CONNECTED_) {
        _cc_ftp_t* ftp = (_cc_ftp_t*)e->args;
        _tprintf(_T("%d connect to server .\n"), e->fd);
        ftp->ctrl.e = e;

        if (!_cc_ftp_connected(ftp)) {
            network_event_close(async, e);
            return false;
        }

        if (which == _CC_EVENT_CONNECTED_)
            return true;
    }

    /*无法连接*/
    if (which & _CC_EVENT_DISCONNECT_) {
        _tprintf(_T("%d disconnect to server.\n"), e->fd);

        network_event_close(async, e);
        return false;
    }

    /*有数据可以读*/
    if (which & _CC_EVENT_READABLE_) {
        _cc_ftp_t* ftp = (_cc_ftp_t*)e->args;
        _cc_event_rbuf_t* rbuf = &e->buffer->r;
        if (!_cc_event_recv(e)) {
            _tprintf(_T("TCP close %d\n"), e->fd);
            network_event_close(async, e);
            return false;
        }

        if (!strstr((char_t*)rbuf->bytes, _CC_CRLF_)) {
            return true;
        }

        rbuf->bytes[rbuf->length] = 0;
        if (ftp && ftp->resp.callback) {
            if (ftp->resp.callback(ftp, rbuf->bytes, rbuf->length)) {
                rbuf->length = 0;
                return true;
            }
        }
        network_event_close(async, e);
        return false;
    }

    /*可写数据*/
    if (which & _CC_EVENT_WRITABLE_) {
        _ftp_send_command(e, nullptr, 0);
        return true;
    }

    /*连接超时*/
    if (which & _CC_EVENT_TIMEOUT_) {
        _tprintf(_T("TCP timeout %d\n"), e->fd);
        network_event_close(async, e);
        return false;
    }

    return true;
}

/**/
bool_t _cc_ftp_tcp_listen(_cc_ftp_t* ftp) {
    struct linger lng = {0, 0};
    _cc_async_event_t *async = nullptr;
    _cc_socket_t fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        return false;
    }

    if (_cc_set_socket_reuseaddr(fd) == -1) {
        _cc_close_socket(fd);
        return false;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, SETSOCKOPT_OPTVAL_TYPE & lng, sizeof(lng)) == -1) {
        _cc_close_socket(fd);
        return false;
    }

    /* port */
    ftp->sa.sa_data[0] = 0;
    ftp->sa.sa_data[1] = 0;

    if (bind(fd, (struct sockaddr*)&ftp->sa, sizeof(ftp->sa)) < 0) {
        int32_t err = _cc_last_errno();
        _cc_logger_error(_T("socket bind port(%d) error(%d) %s"), 0, err, _cc_last_error(err));
        _cc_close_socket(fd);
        return false;
    }

    if (listen(fd, SOMAXCONN) < 0) {
        int32_t err = _cc_last_errno();
        _cc_logger_error(_T("socket listen port(%d) error(%d) %s"), 0, err, _cc_last_error(err));
        _cc_close_socket(fd);
        return false;
    }

    ftp->data.e = async->attach(async, _CC_EVENT_ACCEPT_, fd, 60000, network_event_port_callback, ftp);
    if (ftp->data.e) {
        _cc_set_socket_nonblock(fd, true);
        return true;
    }

    _cc_close_socket(fd);
    return false;
}

_CC_API_PRIVATE(bool_t) ftp_event_callback(_cc_ftp_t* ftp, uint16_t which) {
    switch (which) {
        case _CC_LIBFTP_CONNECTED:
            printf("CC_LIBFTP_CONNECTED OK\n");
            _cc_ftp_login(ftp, "username", "password");
            break;
        case _CC_LIBFTP_CONNECT_FAILED:
            printf("CC_LIBFTP_CONNECT_FAILED\n");
            break;
        case _CC_LIBFTP_LOGIN_USER_FAILED:
            printf("CC_LIBFTP_LOGIN_USER_FAILED\n");
            break;
        case _CC_LIBFTP_LOGIN_PASSWORD_FAILED:
            printf("CC_LIBFTP_LOGIN_PASSWORD_FAILED\n");
            break;
        case _CC_LIBFTP_LOGINED:
            printf("CC_LIBFTP_LOGINED OK\n");
            //_cc_ftp_opts_utf8(ftp);
            //_cc_ftp_opts_datatype(ftp);
            _cc_ftp_opts_port_passive(ftp);
            break;
        case _CC_LIBFTP_LOGOUT:
            printf("CC_LIBFTP_LOGOUT OK\n");
            return false;
        case _CC_LIBFTP_LOGOUT_FAILED:
            printf("CC_LIBFTP_LOGOUT_FAILED\n");
            break;
        case _CC_LIBFTP_OPTS_UTF8:
            printf("_CC_LIBFTP_OPTS_UTF8 OK\n");
            break;
        case _CC_LIBFTP_OPTS_DATATYPE:
            printf("_CC_LIBFTP_OPTS_DATATYPE OK\n");
            break;
        case _CC_LIBFTP_OPTS_PASV:
            printf("_CC_LIBFTP_OPTS_PASV OK\n");
            /**/
            if (ftp->cmode == _CC_LIBFTP_PORT) {
                if (_cc_ftp_tcp_listen(ftp)) {
                    _cc_ftp_open_port(ftp);
                }
            } else {
                ftp->data.async = _cc_get_event_cycle();
                ftp->data.e = _cc_tcp_connect(ftp->data.async, _CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_|_CC_EVENT_BUFFER_,
                    (_cc_sockaddr_t*)&ftp->sa, 60000, network_event_pasv_callback, ftp);
            }
            break;
        case _CC_LIBFTP_OPTS_FAILED:
            printf("CC_LIBFTP_OPTS_FAILED\n");
            _cc_ftp_logout(ftp);
            break;
        case _CC_LIBFTP_LIST_FAILED:
            printf("_CC_LIBFTP_LIST_FAILED\n");
            _cc_ftp_logout(ftp);
            break;
        case _CC_LIBFTP_OPTS_PORT:
            printf("CC_LIBFTP_OPTS_PORT OK\n");
            _cc_ftp_list(ftp, nullptr);
            break;
        case _CC_LIBFTP_CWD:
            printf("CC_LIBFTP_CWD OK\n");
            _cc_ftp_list(ftp, nullptr);
            break;
        case _CC_LIBFTP_LIST_WAITING:
            printf("CC_LIBFTP_LIST_WAITING\n");
            break;
        case _CC_LIBFTP_LIST:
            printf("CC_LIBFTP_LIST OK\n");
            _cc_sleep(1000);
            _cc_ftp_cwd(ftp, "./testes");
            break;
        case _CC_LIBFTP_MKDIR:
            printf("CC_LIBFTP_MKDIR OK\n");
            break;
        case _CC_LIBFTP_MKDIR_FAILED:
            printf("CC_LIBFTP_MKDIR_FAILED\n");
            break;
    }
    return true;
}

bool_t ftp_client(_cc_ftp_t* ftp, tchar_t *host, uint16_t port) {
    struct sockaddr_in sa;
    _cc_async_event_t *async = _cc_get_event_cycle();
    if (ftp == nullptr) {
        return false;
    }

    bzero(ftp, sizeof(_cc_ftp_t));
    ftp->callback = ftp_event_callback;
    ftp->cmode = _CC_LIBFTP_PORT;  //_CC_LIBFTP_PASSIVE;//_CC_LIBFTP_PORT;
    ftp->smode = _CC_LIBFTP_TEXT;
    ftp->logined = false;

    _cc_inet_ipv4_addr(&sa, host, port);
    if (_cc_tcp_connect(async, _CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_|_CC_EVENT_BUFFER_, (_cc_sockaddr_t*)&sa, 60000, network_event_callback, ftp) == nullptr) {
        _tprintf(_T("Unable to connect to the network port %s:%d\n"), host, port);
    }
    return true;
}
