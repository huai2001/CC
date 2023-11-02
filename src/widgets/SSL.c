/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
 * All rights reserved.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/
#include <cc/alloc.h>
#include <cc/rbtree.h>
#include <cc/string.h>
#include <cc/widgets/SSL.h>

#ifdef _CC_OPENSSL_HTTPS_
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>


static _cc_atomic32_t _SSL_lock = 0;
static _cc_atomic32_t _SSL_init_refcount = 0;

struct _cc_SSL {
    SSL *handle;
    _cc_SSL_CTX_t *ctx;
};

struct _cc_SSL_CTX {
    SSL_CTX *handle;
    _cc_atomic32_t refcount;
};

#define _SSL_error(prefix) do {\
    _cc_logger_errorA("%s failed: %s", prefix, ERR_reason_error_string(ERR_get_error()));\
} while(0)

/*
static void _SSL_clear_error() {
    while (ERR_peek_error()) {
        _cc_logger_error(_T("ignoring stale global SSL error"));
    }
    ERR_clear_error();
}*/

static bool_t _SSL_only_init() {
    srand((unsigned)time(NULL));
#if OPENSSL_VERSION_NUMBER >= 0x10100003L
    OPENSSL_init_ssl(OPENSSL_INIT_SSL_DEFAULT, NULL);

    if (OPENSSL_init_ssl(OPENSSL_INIT_LOAD_CONFIG, NULL) == 0) {
        _SSL_error("OPENSSL_init_ssl");
        return false;
    }
    /*
     * OPENSSL_init_ssl() may leave errors in the error queue
     * while returning success
     */
    ERR_clear_error();
#else
/* Enable configuring OpenSSL using the standard openssl.cnf
 * OPENSSL_config()/OPENSSL_init_crypto() should be the first
 * call to the OpenSSL* library.
 *  - OPENSSL_config() should be used for OpenSSL versions < 1.1.0
 *  - OPENSSL_init_crypto() should be used for OpenSSL versions >= 1.1.0
 */
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    //OPENSSL_config(NULL);
#else
    //OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, NULL);
#endif

    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    SSL_load_error_strings();

    if (SSL_library_init() < 0) {
        _SSL_error("SSL_library_init");
        return false;
    }
#endif
#if OPENSSL_VERSION_NUMBER >= 0x0090800fL
#ifndef SSL_OP_NO_COMPRESSION
    {
        /*
         * Disable gzip compression in OpenSSL prior to 1.0.0 version,
         * this saves about 522K per connection.
         */
        int n;
        STACK_OF(SSL_COMP) * ssl_comp_methods;

        ssl_comp_methods = SSL_COMP_get_compression_methods();
        n = sk_SSL_COMP_num(ssl_comp_methods);

        while (n--) {
            sk_SSL_COMP_pop(ssl_comp_methods);
        }
    }
#endif
#endif
    if (!RAND_poll()) {
        _cc_logger_warin(_T("OpenSSL: Failed to seed random number generator."));
    }

    while (RAND_status() == 0) {
        unsigned short rand_ret = rand() % 65536;
        RAND_seed(&rand_ret, sizeof(rand_ret));
    }
    return true;
}

/**/
_cc_SSL_CTX_t *_SSL_init(bool_t is_client) {
    SSL_CTX *ssl_ctx;
    _cc_SSL_CTX_t *ctx;
    /*SSL init*/
    if (_cc_atomic32_inc_ref(&_SSL_init_refcount)) {
        _cc_lock(&_SSL_lock, 1, _CC_LOCK_SPIN_);
        if (!_SSL_only_init()) {
            _SSL_lock = 0;
            return NULL;
        }
        _SSL_lock = 0;
    }

    ctx = _cc_malloc(sizeof(_cc_SSL_CTX_t));
    _cc_lock(&_SSL_lock, 1, _CC_LOCK_SPIN_);
    ssl_ctx = SSL_CTX_new(is_client ? SSLv23_client_method() : SSLv23_server_method());
    _SSL_lock = 0;
    if (ssl_ctx == NULL) {
        _SSL_error("SSL_CTX_new");
        _cc_free(ctx);
        return NULL;
    }
    ctx->handle = ssl_ctx;
    ctx->refcount = 0;

    //SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

   /* client side options */
#ifdef SSL_OP_MICROSOFT_SESS_ID_BUG
   SSL_CTX_set_options(ssl_ctx, SSL_OP_MICROSOFT_SESS_ID_BUG);
#endif

#ifdef SSL_OP_NETSCAPE_CHALLENGE_BUG
   SSL_CTX_set_options(ssl_ctx, SSL_OP_NETSCAPE_CHALLENGE_BUG);
#endif

   /* server side options */
#ifdef SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG
   SSL_CTX_set_options(ssl_ctx, SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG);
#endif

#ifdef SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER
   SSL_CTX_set_options(ssl_ctx, SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER);
#endif

#ifdef SSL_OP_MSIE_SSLV2_RSA_PADDING
   /* this option allow a potential SSL 2.0 rollback (CAN-2005-2969) */
   SSL_CTX_set_options(ssl_ctx, SSL_OP_MSIE_SSLV2_RSA_PADDING);
#endif

#ifdef SSL_OP_SSLEAY_080_CLIENT_DH_BUG
   SSL_CTX_set_options(ssl_ctx, SSL_OP_SSLEAY_080_CLIENT_DH_BUG);
#endif

#ifdef SSL_OP_TLS_D5_BUG
   SSL_CTX_set_options(ssl_ctx, SSL_OP_TLS_D5_BUG);
#endif

#ifdef SSL_OP_TLS_BLOCK_PADDING_BUG
   SSL_CTX_set_options(ssl_ctx, SSL_OP_TLS_BLOCK_PADDING_BUG);
#endif

#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
   SSL_CTX_set_options(ssl_ctx, SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);
#endif

   SSL_CTX_set_options(ssl_ctx, SSL_OP_SINGLE_DH_USE);

#ifdef SSL_OP_NO_COMPRESSION
   SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_COMPRESSION);
#endif

#ifdef SSL_MODE_RELEASE_BUFFERS
   SSL_CTX_set_mode(ssl_ctx, SSL_MODE_RELEASE_BUFFERS);
#endif

#ifdef SSL_MODE_NO_AUTO_CHAIN
   SSL_CTX_set_mode(ssl_ctx, SSL_MODE_NO_AUTO_CHAIN);
#endif

#ifdef SSL_MODE_AUTO_RETRY
   SSL_CTX_set_mode(ssl_ctx, SSL_MODE_AUTO_RETRY);
#endif
    return ctx;
}

/**/
bool_t _SSL_free(_cc_SSL_t *ssl) {
    if (ssl->handle == NULL || ssl->ctx == NULL) {
        return true;
    }

    if (!SSL_in_init(ssl->handle) && ssl->ctx->handle) {
        _cc_lock(&_SSL_lock, 1, _CC_LOCK_SPIN_);
        SSL_CTX_remove_session(ssl->ctx->handle, SSL_get0_session(ssl->handle));
        _SSL_lock = 0;
    }

    _cc_atomic32_dec(&ssl->ctx->refcount);

    SSL_shutdown(ssl->handle);
    SSL_free(ssl->handle);

    _cc_free(ssl);


    ERR_clear_error();
    CRYPTO_cleanup_all_ex_data();
#if OPENSSL_VERSION_NUMBER < 0x10100003L
    EVP_cleanup();
    /*
#ifndef OPENSSL_NO_ENGINE
    ENGINE_cleanup();
#endif*/
#endif

    /*SSL*/
    return true;
}

void _SSL_quit(_cc_SSL_CTX_t *ctx) {
    if (_cc_atomic32_dec_ref(&_SSL_init_refcount)) {
        ERR_free_strings();
    }

    SSL_CTX_free(ctx->handle);
    ctx->handle = NULL;
    _cc_free(ctx);
}

_cc_SSL_t *_SSL_connect(_cc_SSL_CTX_t *ctx, _cc_event_cycle_t*cycle, _cc_event_t *e, const tchar_t *host, uint16_t port) {
    _cc_SSL_t *ssl;
    struct sockaddr_in sa;
#ifdef _CC_UNICODE_
    char host_utf8[256];
#endif

    ssl = _cc_malloc(sizeof(_cc_SSL_t));
    _cc_lock(&_SSL_lock, 1, _CC_LOCK_SPIN_);
    ssl->handle = SSL_new(ctx->handle);
    _SSL_lock = 0;
    if (ssl == NULL) {
        return NULL;
    }

    _cc_atomic32_inc(&ctx->refcount);
    ssl->ctx = ctx;
    _cc_inet_ipv4_addr(&sa, host, port);

    /*Open then socket*/
    e->fd = _cc_socket(AF_INET, _CC_SOCK_NONBLOCK_ | _CC_SOCK_CLOEXEC_ | SOCK_STREAM, 0);
    if (e->fd == -1) {
        _cc_logger_error(_T("socket fail:%s."), _cc_last_error(_cc_last_errno()));
        return NULL;
    }

    /* if we can't terminate nicely, at least allow the socket to be reused*/
    _cc_set_socket_reuseaddr(e->fd);
    /* Set the socket to non-blocking mode for accept() */
    _cc_set_socket_nonblock(e->fd, 1);

    SSL_set_fd(ssl->handle, e->fd);
    SSL_set_connect_state(ssl->handle);

#ifdef _CC_UNICODE_
    _cc_utf16_to_utf8((uint16_t*)host, (uint16_t*)(host + _tcslen(host)), (uint8_t*)host_utf8, (uint8_t*)host_utf8 + _cc_countof(host_utf8), FALSE);
    SSL_set_tlsext_host_name(ssl->handle, host_utf8);
#else
    SSL_set_tlsext_host_name(ssl->handle, host);
#endif

    if (cycle->driver.connect(cycle, e,(_cc_sockaddr_t*)&sa, sizeof(struct sockaddr_in))) {
        return ssl;
    }

    _cc_free(ssl);
    return NULL;
}

uint16_t _SSL_do_handshake(_cc_SSL_t *ssl) {
    int rs = SSL_do_handshake(ssl->handle);
    if (rs == 1) {
        return _CC_SSL_HS_ESTABLISHED_;
    }

    switch (SSL_get_error(ssl->handle, rs)) {
    case SSL_ERROR_ZERO_RETURN:
            _SSL_error(_T("The TLS/SSL connection has been closed. ")
                       _T("If the protocol version is SSL 3.0 or higher, this result ")
                       _T("code is returned ")
                       _T("only if a closure alert has occurred in the protocol, ")
                       _T("i.e. if the connection has been closed cleanly. ")
                       _T("Note that in this case SSL_ERROR_ZERO_RETURN does not ")
                       _T("necessarily indicate ")
                       _T("that the underlying transport has been closed."));
        break;
    case SSL_ERROR_WANT_READ:
        // Need to wait for a read event to continue to complete the
        // handshake
        return _CC_SSL_HS_WANT_READ_;
    case SSL_ERROR_WANT_WRITE:
        // Need to wait for a write event to continue to complete the
        // handshake
        return _CC_SSL_HS_WANT_WRITE_;
    case SSL_ERROR_WANT_CONNECT:
        return _CC_SSL_HS_WANT_WRITE_;
    case SSL_ERROR_WANT_ACCEPT:
        //_cc_logger_debug(_T("SSL_ERROR_WANT_ACCEPT: The operation did not
        // complete; the same TLS/SSL I/O function should be called again
        // later."));
        return _CC_SSL_HS_WANT_READ_;
    case SSL_ERROR_WANT_X509_LOOKUP:
            _SSL_error(_T("The operation did not complete because an application ")
                       _T("callback set by SSL_CTX_set_client_cert_cb() ")
                       _T("has asked to be called again. The TLS/SSL I/O function ")
                       _T("should be called again later."));
        break;
#ifdef SSL_ERROR_WANT_ASYNC
    case SSL_ERROR_WANT_ASYNC:
        // This will only occur if the mode has been set to SSL_MODE_ASYNC
        // using SSL_CTX_set_mode or SSL_set_mode and an asynchronous
        // capable engine is being used.
            _SSL_error(_T("The operation did not complete because an asynchronous ")
                       _T("engine is still processing data. "));
        break;
#endif
#ifdef SSL_ERROR_WANT_ASYNC_JOB
    case SSL_ERROR_WANT_ASYNC_JOB:
        // This will only occur if the mode has been set to SSL_MODE_ASYNC
        // using SSL_CTX_set_mode or SSL_set_mode and a maximum limit has
        // been set on the async job pool through a call to
        // ASYNC_init_thread.
            _SSL_error(_T("The asynchronous job could not be started because there ")
                       _T("were no async jobs available in the pool (see ")
                       _T("ASYNC_init_thread(3))."));
        break;
#endif
    case SSL_ERROR_SYSCALL: {
        int err = _cc_last_errno();
        if (_CC_EINTR_ == err) {
            break;
        } else {
            _cc_logger_error(_T("Some non-recoverable I/O error occurred. The OpenSSL ")
                             _T("error queue may contain more information on the ")
                             _T("error. ")
                             _T("For socket I/O on Unix systems, consult errno %d for ")
                             _T("details."), err);
        }
    } break;
    case SSL_ERROR_SSL:
        _SSL_error(_T("A failure in the SSL library occurred, usually a protocol ")
                   _T("error. The OpenSSL error queue contains more information ")
                   _T("on the error."));
        break;
    case SSL_ERROR_NONE:
        break;
    }
    return _CC_SSL_HS_ERROR_;
}

/**/
int32_t _SSL_sendbuf(_cc_SSL_t *ssl, _cc_event_t *e) {
    _cc_event_wbuf_t *wbuf;
    int32_t off;
    if (e->buffer == NULL) {
        _cc_logger_error(_T("No write cache was created. e->buffer == NULL"));
        return -1;
    }

    wbuf = &e->buffer->w;
    if (wbuf->r == wbuf->w) {
        _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        return 0;
    }

    _cc_spin_lock(&wbuf->wlock);
    off = _SSL_send(ssl, wbuf->buf + wbuf->r, wbuf->w - wbuf->r);
    if (off) {
        wbuf->r += off;
        if (wbuf->r == wbuf->w) {
            _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        }
    } else if (off < 0) {
        _CC_UNSET_BIT(_CC_EVENT_WRITABLE_, e->flags);
    }
    _cc_spin_unlock(&wbuf->wlock);
    return off;
}

/**/
int32_t _SSL_send(_cc_SSL_t *ssl, const pvoid_t buf, int32_t len) {
    int32_t rc = 0;

    if (buf == NULL) {
        return true;
    }
    ERR_clear_error();

    rc = (int32_t)SSL_write(ssl->handle, (char *)buf, len);
    if (rc <= 0) {
        switch (SSL_get_error(ssl->handle, rc)) {
        case SSL_ERROR_WANT_READ:
            if (_SSL_do_handshake(ssl) == _CC_SSL_HS_ESTABLISHED_) {
                return 0;
            }
            break;
        case SSL_ERROR_WANT_WRITE:
            if (_SSL_do_handshake(ssl) == _CC_SSL_HS_ESTABLISHED_) {
                return 0;
            }
            break;
        case SSL_ERROR_ZERO_RETURN:
            _cc_logger_debug(_T("_SSL_send:The SSL connection is securely closed"));
            break;
        case SSL_ERROR_SYSCALL: {
            int err = _cc_last_errno();
            if ((rc == 0 && err == 0) || (err == _CC_EINTR_ || err == _CC_EAGAIN_)) {
                return 0;
            }
            _SSL_error("_SSL_send");
            rc = -1;
        }
        break;
        }
    }

    return rc;
}

int32_t _SSL_read(_cc_SSL_t *ssl, pvoid_t buf, int32_t len) {
    int32_t rc;
    ERR_clear_error();
    rc = (int32_t)SSL_read(ssl->handle, buf, len);
    if (rc == 0) {
        return -1;
    }
    
    if (rc < 0) {
        switch (SSL_get_error(ssl->handle, rc)) {
        case SSL_ERROR_WANT_READ:
            if (_SSL_do_handshake(ssl) == _CC_SSL_HS_ESTABLISHED_) {
                return 0;
            }
            break;
        case SSL_ERROR_WANT_WRITE:
            if (_SSL_do_handshake(ssl) == _CC_SSL_HS_ESTABLISHED_) {
                return 0;
            }
            break;
        case SSL_ERROR_ZERO_RETURN:
            _cc_logger_debug(_T("_SSL_send:The SSL connection is securely closed"));
            break;
        case SSL_ERROR_SYSCALL: {
            int err = _cc_last_errno();
            if ((err == _CC_EINTR_ || err == _CC_EAGAIN_)) {
                return true;
            }
            _SSL_error("SSL_read");
            break;
        }
        default:
            _SSL_error("SSL_read");
        }
    }
    return rc;
}
/**/
bool_t _SSL_event_read(_cc_SSL_t *ssl, _cc_event_t *e) {
    int32_t rc;
    _cc_event_buffer_t *rw = e->buffer;
    rc = _SSL_read(ssl, (pvoid_t)(rw->r.buf + rw->r.length), _CC_IO_BUFFER_SIZE_ - rw->r.length);
    if (rc > 0) {
        rw->r.length += rc;
        return true;
    }
    return rc == 0;
}
#endif