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
#include <cc/SSL.h>
#include <cc/rbtree.h>
#include <cc/string.h>
#ifdef __CC_MACOSX__
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#else
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#endif

#ifdef _CC_OPENSSL_HTTPS_
static SSL_CTX* _SSL_client_ctx = NULL;
static _cc_atomic32_t _SSL_init_refcount = 0;
static _cc_rbtree_t _SSL_CTX_active;
#endif

#define _SSL_CLIENT_ 1
#define _SSL_SERVER_ 2

typedef struct _cc_SSL_CTX {
    SSL_CTX* ctx;
    char_t* domain;
    uint32_t use_sslv2;
    uint32_t use_sslv3;
    //
    _cc_rbtree_iterator_t node;
} _cc_SSL_CTX_t;

struct _cc_SSL {
    SSL* ssl;
    SSL_CTX* ssl_ctx;
};

//
#define _SSL_Error()                                                \
    do {                                                            \
        static tchar_t err[10240];                                  \
        ERR_error_string_n(ERR_get_error(), err, _cc_countof(err)); \
        _cc_logger_error(_T("%s"), err);                            \
    } while (0)

void _SSL_quit(_cc_SSL_t* req) {
    /*SSL*/
    if (_cc_atomic32_dec_ref(&_SSL_init_refcount)) {
        ERR_free_strings();
        SSL_CTX_free(_SSL_client_ctx);
        _SSL_client_ctx = NULL;
    }
}

/**/
static SSL_CTX* _SSL_init(int flag) {
    SSL_CTX* ssl_ctx;
    const SSL_METHOD* ssl_method;
    /*SSL init*/
    if (_cc_atomic32_inc_ref(&_SSL_init_refcount)) {
        srand((unsigned)time(NULL));
        _CC_RB_INIT_ROOT(&_SSL_CTX_active);
#if OPENSSL_VERSION_NUMBER >= 0x10100003L
        OPENSSL_init_ssl(OPENSSL_INIT_SSL_DEFAULT, NULL);

        if (OPENSSL_init_ssl(OPENSSL_INIT_LOAD_CONFIG, NULL) == 0) {
            _cc_logger_error(_T("OPENSSL_init_ssl() failed!"));
            return NULL;
        }
        /*
         * OPENSSL_init_ssl() may leave errors in the error queue
         * while returning success
         */
        ERR_clear_error();
#else
        if (SSL_library_init() < 0) {
            _cc_logger_error(_T("SSL_library_init() failed!"));
            return NULL;
        }

        OpenSSL_add_all_algorithms();
        ERR_load_BIO_strings();
        ERR_load_crypto_strings();
        SSL_load_error_strings();

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
    }
    if (flag == _SSL_SERVER_) {
        ssl_method = SSLv23_server_method();
    } else if (flag == _SSL_CLIENT_) {
        ssl_method = SSLv23_client_method();
    }

    ssl_ctx = SSL_CTX_new(ssl_method);
    if (ssl_ctx == NULL) {
        _SSL_Error();
        return false;
    }

    RAND_poll();
    while (RAND_status() == 0) {
        unsigned short rand_ret = rand() % 65536;
        RAND_seed(&rand_ret, sizeof(rand_ret));
    }

    return ssl_ctx;
}
/*
static int32_t _X509_NAME_oneline(X509_NAME *name, char *buf, int32_t
sz) { BIO *bio = BIO_new(BIO_s_mem()); if (bio) { int32_t len =
X509_NAME_print_ex(bio, name, 0, XN_FLAG_ONELINE); BIO_gets(bio, buf, sz);
        BIO_free(bio);
        return len;
    }

    buf[0] = 0;
    return 0;
}

static bool_t _verifyX509(SSL* ssl) {
    char X509_name[256];
    X509 *client_cert = SSL_get_peer_certificate(ssl);
    if (client_cert == NULL){
        return false;
    }

    if (_X509_NAME_oneline(X509_get_subject_name(client_cert),
                            X509_name,
                            _cc_countof(X509_name)) == 0) {
        _SSL_Error();
        return false;
    }
    if (_X509_NAME_oneline(X509_get_issuer_name(client_cert),
                            X509_name,
                            _cc_countof(X509_name)) == 0){
        _SSL_Error();
        return false;
    }

    X509_free(client_cert);
    return true;
}
*/
static bool_t __server_init(_cc_SSL_CTX_t* cssl,
                                       const char_t* cert_key_file,
                                       const char_t* cert_chain_file) {
    SSL_CTX* ssl_ctx;
    ssl_ctx = _SSL_init(_SSL_SERVER_);
    if (ssl_ctx == NULL) {
        return false;
    }

    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, NULL);
    if (SSL_CTX_load_verify_locations(ssl_ctx, cert_chain_file,
                                      cert_key_file) <= 0) {
        _SSL_Error();
        return false;
    }

    if (SSL_CTX_set_default_verify_paths(ssl_ctx) != 1) {
        _SSL_Error();
        return false;
    }

    if (SSL_CTX_use_certificate_chain_file(ssl_ctx, cert_chain_file) <= 0) {
        _SSL_Error();
        return false;
    }

    //
    if (SSL_CTX_use_PrivateKey_file(ssl_ctx, cert_key_file, SSL_FILETYPE_PEM) <=
        0) {
        _SSL_Error();
        return false;
    }

    if (!SSL_CTX_check_private_key(ssl_ctx)) {
        _SSL_Error();
        return false;
    }
    /* client side options */
#ifdef SSL_OP_MICROSOFT_SESS_ID_BUG
    SSL_CTX_set_options(ssl_ctx, SSL_OP_MICROSOFT_SESS_ID_BUG);
#endif

#ifdef SSL_OP_NETSCAPE_CHALLENGE_BUG
    SSL_CTX_set_options(ssl_ctx, SSL_OP_NETSCAPE_CHALLENGE_BUG);
#endif
    cssl->ctx = ssl_ctx;
    return true;
}

static int32_t _getSSL_CTX(_cc_rbtree_iterator_t* node,
                                      pvoid_t args) {
    _cc_SSL_CTX_t* r = _cc_upcast(node, _cc_SSL_CTX_t, node);
    return stricmp(r->domain, (const char_t*)args);
}

static int32_t _pushSSL_CTX(_cc_rbtree_iterator_t* left,
                                       _cc_rbtree_iterator_t* right) {
    _cc_SSL_CTX_t* l = _cc_upcast(left, _cc_SSL_CTX_t, node);
    _cc_SSL_CTX_t* r = _cc_upcast(right, _cc_SSL_CTX_t, node);

    return stricmp(l->domain, r->domain);
}

bool_t _SSL_init_server(const char_t* domain,
                        const char_t* cert_key_file,
                        const char_t* cert_chain_file) {
    _cc_SSL_CTX_t* ssl_ctx;
    _cc_rbtree_iterator_t* node;

    node = _cc_rbtree_get(&_SSL_CTX_active, (pvoid_t)domain, _getSSL_CTX);
    if (node == NULL) {
        ssl_ctx = (_cc_SSL_CTX_t*)_cc_malloc(sizeof(_cc_SSL_CTX_t));
        if (ssl_ctx == NULL) {
            return false;
        }
        bzero(ssl_ctx, sizeof(_cc_SSL_CTX_t));

        if (!__server_init(ssl_ctx, cert_key_file, cert_chain_file)) {
            _cc_free(ssl_ctx);
            return false;
        }

        ssl_ctx->domain = _cc_strdupA(domain);
        _cc_rbtree_push(&_SSL_CTX_active, &ssl_ctx->node, _pushSSL_CTX);
    } else {
        ssl_ctx = _cc_upcast(node, _cc_SSL_CTX_t, node);
    }

    // SSL_CTX_set_cipher_list(ssl_ctx->ctx, "RC5-MD5");
    /* client side options */
#ifdef SSL_OP_MICROSOFT_SESS_ID_BUG
    SSL_CTX_set_options(ssl_ctx->ctx, SSL_OP_MICROSOFT_SESS_ID_BUG);
#endif

#ifdef SSL_OP_NETSCAPE_CHALLENGE_BUG
    SSL_CTX_set_options(ssl_ctx->ctx, SSL_OP_NETSCAPE_CHALLENGE_BUG);
#endif

    /* server side options */
#ifdef SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG
    SSL_CTX_set_options(ssl_ctx->ctx, SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG);
#endif

#ifdef SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER
    SSL_CTX_set_options(ssl_ctx->ctx, SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER);
#endif

#ifdef SSL_OP_MSIE_SSLV2_RSA_PADDING
    /* this option allow a potential SSL 2.0 rollback (CAN-2005-2969) */
    SSL_CTX_set_options(ssl_ctx->ctx, SSL_OP_MSIE_SSLV2_RSA_PADDING);
#endif

#ifdef SSL_OP_SSLEAY_080_CLIENT_DH_BUG
    SSL_CTX_set_options(ssl_ctx->ctx, SSL_OP_SSLEAY_080_CLIENT_DH_BUG);
#endif

#ifdef SSL_OP_TLS_D5_BUG
    SSL_CTX_set_options(ssl_ctx->ctx, SSL_OP_TLS_D5_BUG);
#endif

#ifdef SSL_OP_TLS_BLOCK_PADDING_BUG
    SSL_CTX_set_options(ssl_ctx->ctx, SSL_OP_TLS_BLOCK_PADDING_BUG);
#endif

#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
    SSL_CTX_set_options(ssl_ctx->ctx, SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);
#endif

    SSL_CTX_set_options(ssl_ctx->ctx, SSL_OP_SINGLE_DH_USE);

#ifdef SSL_OP_NO_COMPRESSION
    SSL_CTX_set_options(ssl_ctx->ctx, SSL_OP_NO_COMPRESSION);
#endif

#ifdef SSL_MODE_RELEASE_BUFFERS
    SSL_CTX_set_mode(ssl_ctx->ctx, SSL_MODE_RELEASE_BUFFERS);
#endif

#ifdef SSL_MODE_NO_AUTO_CHAIN
    SSL_CTX_set_mode(ssl_ctx->ctx, SSL_MODE_NO_AUTO_CHAIN);
#endif

#ifdef SSL_MODE_AUTO_RETRY
    SSL_CTX_set_mode(ssl_ctx->ctx, SSL_MODE_AUTO_RETRY);
#endif
    return true;
}

/**/
bool_t _SSL_free(_cc_SSL_t* req) {
    if (req->ssl == NULL) {
        return true;
    }

    if (!SSL_in_init(req->ssl)) {
        SSL_CTX_remove_session(req->ssl_ctx, SSL_get0_session(req->ssl));
    }

    SSL_shutdown(req->ssl);
    SSL_free(req->ssl);

    _cc_free(req);

    ERR_clear_error();
    CRYPTO_cleanup_all_ex_data();
#if OPENSSL_VERSION_NUMBER < 0x10100003L
    EVP_cleanup();
    /*
#ifndef OPENSSL_NO_ENGINE
    ENGINE_cleanup();
#endif*/
#endif

    _SSL_quit(req);
    return true;
}

_cc_SSL_t* _SSL_accept(const tchar_t* domain, _cc_socket_t fd) {
    _cc_SSL_t* req;
    _cc_SSL_CTX_t* ssl_ctx;
    _cc_rbtree_iterator_t* node;

    node = _cc_rbtree_get(&_SSL_CTX_active, (pvoid_t)domain, _getSSL_CTX);
    if (node == NULL) {
        return NULL;
    } else {
        ssl_ctx = _cc_upcast(node, _cc_SSL_CTX_t, node);
    }

    req = (_cc_SSL_t*)_cc_malloc(sizeof(_cc_SSL_t));
    if (req == NULL) {
        return NULL;
    }
    bzero(req, sizeof(_cc_SSL_t));

    req->ssl_ctx = ssl_ctx->ctx;
    req->ssl = NULL;

    req->ssl = SSL_new(req->ssl_ctx);
    if (req->ssl == NULL) {
        _SSL_Error();
        _cc_free(req);
        return false;
    }

    SSL_clear(req->ssl);
    SSL_set_fd(req->ssl, fd);
    SSL_set_accept_state(req->ssl);

    return req;
}

_cc_SSL_t* _SSL_connect(_cc_socket_t fd) {
    _cc_SSL_t* req;

    _SSL_client_ctx = _SSL_init(_SSL_CLIENT_);
    if (_SSL_client_ctx == NULL) {
        return false;
    }

    req = (_cc_SSL_t*)_cc_malloc(sizeof(_cc_SSL_t));
    if (req == NULL) {
        return NULL;
    }

    bzero(req, sizeof(_cc_SSL_t));
    req->ssl_ctx = _SSL_client_ctx;
    req->ssl = SSL_new(_SSL_client_ctx);
    if (req->ssl == NULL) {
        _SSL_Error();
        return false;
    }

    if (SSL_set_fd((SSL*)req->ssl, (int)fd) == 0) {
        _SSL_Error();
        return false;
    }

    SSL_set_connect_state(req->ssl);

    return req;
}

uint16_t _SSL_do_handshake(_cc_SSL_t* req) {
    int rs;

    rs = SSL_do_handshake(req->ssl);
    if (rs == 1) {
        return _CC_SSL_HS_ESTABLISHED_;
    }

    switch (SSL_get_error(req->ssl, rs)) {
        case SSL_ERROR_ZERO_RETURN:
            _cc_logger_error(
                _T("The TLS/SSL connection has been closed. ")
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
            //complete; the same TLS/SSL I/O function should be called again
            //later."));
            return _CC_SSL_HS_WANT_READ_;
        case SSL_ERROR_WANT_X509_LOOKUP:
            _cc_logger_error(
                _T("The operation did not complete because an application ")
                _T("callback set by SSL_CTX_set_client_cert_cb() ")
                _T("has asked to be called again. The TLS/SSL I/O function ")
                _T("should be called again later."));
            break;
#ifdef SSL_ERROR_WANT_ASYNC
        case SSL_ERROR_WANT_ASYNC:
            // This will only occur if the mode has been set to SSL_MODE_ASYNC
            // using SSL_CTX_set_mode or SSL_set_mode and an asynchronous
            // capable engine is being used.
            _cc_logger_error(
                _T("The operation did not complete because an asynchronous ")
                _T("engine is still processing data. "));
            break;
#endif
#ifdef SSL_ERROR_WANT_ASYNC_JOB
        case SSL_ERROR_WANT_ASYNC_JOB:
            // This will only occur if the mode has been set to SSL_MODE_ASYNC
            // using SSL_CTX_set_mode or SSL_set_mode and a maximum limit has
            // been set on the async job pool through a call to
            // ASYNC_init_thread.
            _cc_logger_error(
                _T("The asynchronous job could not be started because there ")
                _T("were no async jobs available in the pool (see ")
                _T("ASYNC_init_thread(3))."));
            break;
#endif
        case SSL_ERROR_SYSCALL: {
            int err = _cc_last_errno();
            if (_CC_EINTR_ == err) {
                break;
            } else {
                _cc_logger_error(
                    _T("Some non-recoverable I/O error occurred. The OpenSSL ")
                    _T("error queue may contain more information on the ")
                    _T("error. ")
                    _T("For socket I/O on Unix systems, consult errno %d for ")
                    _T("details."),
                    err);
            }
        } break;
        case SSL_ERROR_SSL:
            _cc_logger_error(
                _T("A failure in the SSL library occurred, usually a protocol ")
                _T("error. The OpenSSL error queue contains more information ")
                _T("on the error."));
            break;
        case SSL_ERROR_NONE:
            break;
    }
    return _CC_SSL_HS_ERROR;
}

/**/
int32_t _SSL_send(_cc_SSL_t* req, const pvoid_t buf, int32_t len) {
    int32_t sent = 0;

    if (buf == NULL) {
        return true;
    }

    sent = (int32_t)SSL_write(req->ssl, (char*)buf, len);
    if (sent <= 0) {
        int _SSL_error = SSL_get_error(req->ssl, sent);
        switch (_SSL_error) {
            case SSL_ERROR_WANT_READ:
                if (_SSL_do_handshake(req) == _CC_SSL_HS_ESTABLISHED_) {
                    return true;
                }
                break;
            case SSL_ERROR_WANT_WRITE:
                if (_SSL_do_handshake(req) == _CC_SSL_HS_ESTABLISHED_) {
                    return true;
                }
                break;
            case SSL_ERROR_ZERO_RETURN:
                _cc_logger_debug(
                    _T("_SSL_send:The SSL connection is securely closed"));
                break;
            case SSL_ERROR_SYSCALL: {
                int err = _cc_last_errno();
                if ((sent == 0 && err == 0) ||
                    (err == _CC_EINTR_ || err == _CC_EAGAIN_)) {
                    return 0;
                }
                _cc_logger_error(_T("SSL_read() error code:%d,%s"), sent, err,
                                 _cc_last_error(err));
            } break;
        }
    }

    return sent;
}

/**/
bool_t _SSL_recv(_cc_event_t* e, _cc_SSL_t* req) {
    int32_t rc = 0;
    _cc_event_buffer_t* rw = e->buffer;

    rc = (int32_t)SSL_read(req->ssl, (pvoid_t)(rw->r.buf + rw->r.length),
                           _CC_IO_BUFFER_SIZE_ - rw->r.length);
    if (rc < 0) {
        int _SSL_error = SSL_get_error(req->ssl, rc);
        switch (_SSL_error) {
            case SSL_ERROR_WANT_READ:
                if (_SSL_do_handshake(req) == _CC_SSL_HS_ESTABLISHED_) {
                    return true;
                }
                break;
            case SSL_ERROR_WANT_WRITE:
                if (_SSL_do_handshake(req) == _CC_SSL_HS_ESTABLISHED_) {
                    return true;
                }
                break;
            case SSL_ERROR_ZERO_RETURN:
                _cc_logger_debug(
                    _T("_SSL_send:The SSL connection is securely closed"));
                break;
            case SSL_ERROR_SYSCALL: {
                int err = _cc_last_errno();
                if ((err == _CC_EINTR_ || err == _CC_EAGAIN_)) {
                    return true;
                }
                _cc_logger_error(_T("SSL_read() error code:%d,%s"), rc, err,
                                 _cc_last_error(err));
            } break;
            default:
                _cc_logger_error(
                    _T("SSL_read() error code %d, see SSL_get_error() manual ")
                    _T("for error code detail."),
                    _SSL_error);
        }
    } else if (rc > 0) {
        rw->r.length += rc;
        return true;
    }
    return false;
}
