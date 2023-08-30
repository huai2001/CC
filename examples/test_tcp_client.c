#include <stdio.h>
#include <libcc.h>
#include <locale.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <cc/widgets/SSL.h>

byte_t c = 0;
time_t start_time = 0;
_cc_event_cycle_t cycle;

static _cc_SSL_t *ssl;
static _cc_SSL_CTX_t *ssl_ctx;

int32_t fn_thread(_cc_thread_t *thrd, void* param) {
    while(c!='q')
        _cc_event_wait((_cc_event_cycle_t *)param, 100);

    return 1;
}

uint32_t getIP(const _cc_event_t* e) {
    struct sockaddr_in addr;
    _cc_socklen_t len = sizeof(struct sockaddr_in);
    if(getpeername(e->fd, (struct sockaddr *)&addr, &len) == -1) {
        int32_t err = _cc_last_errno();
        _cc_logger_error(_T("discovery client information failed, fd=%d, errno=%d(%#x).\n"), e->fd, err, err);
        return 0;
    }
    return (addr.sin_addr.s_addr);
}

static bool_t _Send(_cc_event_t *e, const char_t *str) {
    return _SSL_send(ssl, (byte_t*)str, (uint32_t)strlen(str));
}

static bool_t _event_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t events) {
    if(events & _CC_EVENT_CONNECT_){
        _tprintf(_T(" connect to server!\n"));
        /**/
        while (1) {
            int rc = _SSL_do_handshake(ssl);
            if (rc == _CC_SSL_HS_ESTABLISHED_) {
                _Send(e,"GET /get_data HTTP/1.1\r\nHost: pc28yx.com\r\nUser-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/110.0.0.0 Safari/537.36\r\n\r\n");
                break;
            } else if (rc == _CC_SSL_HS_WANT_WRITE_ || rc == _CC_SSL_HS_WANT_READ_) {
                
            } else {
                return false;
            }
        }
    }
    
    if (events & _CC_EVENT_DISCONNECT_) {
        _tprintf(_T("TCP Close - %d\n"), e->fd);

        return false;
    }
    /**/
    if (events & _CC_EVENT_READABLE_) {
        char_t buf[_CC_IO_BUFFER_SIZE_];
        uint32_t length = _SSL_read(ssl, (byte_t*)buf, _CC_IO_BUFFER_SIZE_);
        if(length < 0) {
            _tprintf(_T("TCP close Data %d\n"),e->fd);
            return false;
        }
        if (length == 0) {
            return true;
        }
        buf[length] = 0;
        printf("%s\n", buf);

        return true;
    }

    if (events & _CC_EVENT_WRITABLE_) {
        /*if (_cc_event_sendbuf(e) < 0) {
            _tprintf(_T(" Fail to send, error = %d\n"), _cc_last_errno());
            return false;   
        }*/
    }
    
    if (events & _CC_EVENT_TIMEOUT_) {
        _tprintf(_T("Timeout - %d\n"), e->fd);
        return false;
    }

    return true;
}
struct _cc_SSL_CTX {
    SSL_CTX *handle;
    _cc_atomic32_t refcount;
};

int main (int argc, char * const argv[]) {
    _cc_event_t *e;
    setlocale( LC_CTYPE, "chs" );
    _cc_install_socket();
    

    if(_cc_init_event_poller(&cycle) == false){
        return 0;
    }
    ssl_ctx = _SSL_init(true);
    if (ssl_ctx == NULL) {
        _tprintf(_T("SSL_CTX_new failed.\n"));
        return -1;
    }
    
    _cc_thread_start(fn_thread, "test-tcp-client", &cycle);

    e = _cc_alloc_event(&cycle,  _CC_EVENT_CONNECT_|_CC_EVENT_TIMEOUT_|_CC_EVENT_BUFFER_);
    if (e == NULL) {
        cycle.driver.quit(&cycle);
        return -1;
    }
    e->callback = _event_callback;
    e->timeout = 10000;
    ssl = _SSL_connect(ssl_ctx, &cycle, e, "pc28yx.com", 443);
    if (ssl == NULL) {
        _cc_free_event(&cycle, e);
    }
    while((c = getchar()) != 'q') {
        _cc_sleep(100);
    }
    cycle.driver.quit(&cycle);
    _cc_uninstall_socket();
    
    return 0;
}
