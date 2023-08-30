#include <libcc.h>

typedef struct _cc_http_proxy {
    bool_t parsing;
    bool_t finished;
    bool_t keep_alive;

    uint32_t length;
    _cc_event_t *e;
} _cc_http_proxy_t;

static void _proxy_free(_cc_http_proxy_t *proxy) {
    if (proxy) {
        if (proxy->e) {
            _cc_event_change_flag(NULL, proxy->e, _CC_EVENT_DISCONNECT_);
        }
        _cc_free(proxy);
    }
}

static bool_t _proxy_event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t events) {
    if (events & _CC_EVENT_ACCEPT_) {
        _cc_socket_t fd;
        _cc_sockaddr_t remote_addr = {0};
        _cc_socklen_t remote_addr_len = sizeof(_cc_sockaddr_t);
        _cc_event_cycle_t *cycle_new = _cc_get_event_cycle();
        _cc_http_proxy_t *proxy = (_cc_http_proxy_t*)_cc_malloc(sizeof(_cc_http_proxy_t));

        proxy->parsing = true;
        proxy->finished = false;
        proxy->length = 0;

        fd = _cc_event_accept(cycle, e, &remote_addr, &remote_addr_len);
        if (fd == _CC_INVALID_SOCKET_) {
            _cc_logger_debug(_T("thread %d accept fail %s."), _cc_get_thread_id(NULL), _cc_last_error(_cc_last_errno()));
            return true;
        }
        
        _cc_set_socket_nonblock(fd, 1);

        if (cycle_new->driver.add(cycle_new, _CC_EVENT_TIMEOUT_|_CC_EVENT_READABLE_|_CC_EVENT_BUFFER_, fd, 
                                  e->timeout, network_event_callback, proxy) == NULL) {
            _cc_logger_debug(_T("thread %d add socket (%d) event fial."), _cc_get_thread_id(NULL), fd);
            _cc_free(proxy);
            return true;
        }

        return true;
    }
    
    if (events & _CC_EVENT_DISCONNECT_) {
        //_cc_logger_debug(_T("%d disconnect to client."), e->fd);
        _proxy_free((_cc_http_proxy_t*)e->args);
        return false;
    }
    
    if (events & _CC_EVENT_READABLE_) {
        _cc_event_buffer_t* rw = e->buffer;
        _cc_http_proxy_t* proxy = (_cc_http_proxy_t*)e->args;
        if (!_cc_event_recv(e)) {
            //_cc_logger_debug(_T("%d close to client."), e->fd);
            _proxy_free(e->args);
            return false;
        }
        if (rw->r.length <= 0) {
            return true;
        }
        rw->r.buf[rw->r.length - 1] = 0;

        //_http_request_too_large
        if (proxy->parsing) {
            char_t* content_length;
            char_t* proxy_connection;
            _cc_number_t number;
            char_t* finality = strstr(rw->r.buf, "\r\n\r\n");
            if (finality == NULL) {
                return true;
            }
            proxy_connection = strstr(rw->r.buf, "Proxy-Connection");
            if (proxy_connection) {
                while(proxy_connection) {
                    if (*proxy_connection != ' ' && *proxy_connection != ':') {
                        break;
                    }
                    proxy_connection++;
                }
            } else {
                _proxy_free(e->args);
                return false;
            }

            if (*proxy_connection == '\r' || *proxy_connection == '\n') {
                _proxy_free(e->args);
                return false;
            }

            if (strncmp("Upgrade", line, 7) == 0) {
                
            }

            content_length = strstr(rw->r.buf, "Content-Length");
            if (content_length) {
                while(content_length) {
                    if (*content_length != ' ' && *content_length != ':') {
                        break;
                    }
                    content_length++;
                }
            }



            if (*content_length == '\r' || *content_length == '\n') {
                _proxy_free(e->args);
                return false;
            }
            _cc_to_number(content_length, &number);
            proxy->length = number.uni_int;
            proxy->finality = true;

            _cc_tcp_connect();
        }


        _cc_sleep(1);
        return true;
    }
    
    if (events & _CC_EVENT_WRITABLE_) {
        int32_t off = _cc_event_sendbuf(e);
        if (off < 0) {
            return false;
        }

        if ((e->flags & _CC_EVENT_WRITABLE_) == 0 && e->flags & _CC_EVENT_DISCONNECT_) {
            return false;
        }
    }
    
    if (events & _CC_EVENT_TIMEOUT_) {
        return false;
    }
    
    return true;
}


bool_t _cc_proxy_listen(uint16_t port) {
    struct sockaddr_in sa;
    _cc_event_cycle_t *cycle = _cc_get_event_cycle();
    _cc_inet_ipv4_addr(&sa, NULL, port);

    return _cc_tcp_listen(cycle,(_cc_sockaddr_t*)&sa, 60000, _proxy_event_callback, NULL) != NULL;
}