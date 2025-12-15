#include <libcc/alloc.h>
#include <libcc/rbtree.h>
#include <libcc/string.h>
#include <libcc/atomic.h>
#include <libcc/OpenSSL.h>

#ifdef _CC_USE_OPENSSL_

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/pkcs12.h>


static _cc_atomic32_t _SSL_lock = 0;
static _cc_atomic32_t _SSL_init_refcount = 0;

struct _cc_SSL {
    SSL *handle;
    _cc_OpenSSL_t *ctx;
};

struct _cc_OpenSSL {
    SSL_CTX *handle;
    _cc_atomic32_t refcount;
};

#define _SSL_error(prefix) do {\
    _cc_loggerA_error("%s failed: %s", prefix, ERR_reason_error_string(ERR_get_error()));\
} while(0)

/*
_CC_API_PRIVATE(void) _SSL_clear_error() {
    while (ERR_peek_error()) {
        _cc_logger_error(_T("ignoring stale global SSL error"));
    }
    ERR_clear_error();
}*/

_CC_API_PRIVATE(bool_t) _SSL_only_init(void) {
#if (OPENSSL_INIT_LOAD_CONFIG && !defined LIBRESSL_VERSION_NUMBER)
    uint64_t opts = OPENSSL_INIT_LOAD_CONFIG;
    OPENSSL_INIT_SETTINGS  *init;
    init = OPENSSL_INIT_new();
    if (init == NULL) {
        _SSL_error("OPENSSL_INIT_new");
        return false;
    }
    #ifndef OPENSSL_NO_STDIO
    if (OPENSSL_INIT_set_config_appname(init, "libcc") == 0) {
        _SSL_error("OPENSSL_INIT_set_config_appname");
        return false;
    }
    #endif
    if (OPENSSL_init_ssl(opts, init) == 0) {
        _SSL_error("OPENSSL_init_ssl");
        return false;
    }

    OPENSSL_INIT_free(init);

    /*
     * OPENSSL_init_ssl() may leave errors in the error queue
     * while returning success
     */

    ERR_clear_error();

#else
    OPENSSL_no_config();
    OPENSSL_config("libcc");

    SSL_library_init();
    SSL_load_error_strings();

    OpenSSL_add_all_algorithms();

#endif
#ifndef SSL_OP_NO_COMPRESSION
    {
    /*
     * Disable gzip compression in OpenSSL prior to 1.0.0 version,
     * this saves about 522K per connection.
     */
    int n;
    STACK_OF(SSL_COMP) *ssl_comp_methods;

    ssl_comp_methods = SSL_COMP_get_compression_methods();
    n = sk_SSL_COMP_num(ssl_comp_methods);

    while (n--) {
        (void) sk_SSL_COMP_pop(ssl_comp_methods);
    }
    }
#endif

    // if (!RAND_poll()) {
    //     _cc_logger_warin(_T("OpenSSL: Failed to seed random number generator."));
    // }

    // while (RAND_status() == 0) {
    //     unsigned short rand_ret = rand() % 65536;
    //     RAND_seed(&rand_ret, sizeof(rand_ret));
    // }
    return true;
}
#if 0
static int verify_callback(int preverify_ok, X509_STORE_CTX *ctx) {
    if (!preverify_ok) {
        //X509 *cert = X509_STORE_CTX_get_current_cert(ctx);
        int err = X509_STORE_CTX_get_error(ctx);
        _cc_logger_warin(_T("Verify error: %s"), X509_verify_cert_error_string(err));
    }
    return preverify_ok;
}
#endif
/**/
_CC_API_PUBLIC(_cc_OpenSSL_t*) _SSL_init(uint32_t protocols) {
    SSL_CTX *ssl_ctx;
    _cc_OpenSSL_t *ctx;
    /*SSL init*/
    if (_cc_atomic32_inc_ref(&_SSL_init_refcount)) {
        _cc_lock(&_SSL_lock, 1, _CC_LOCK_SPIN_);
        if (!_SSL_only_init()) {
            _cc_unlock(&_SSL_lock);
            return nullptr;
        }
        _cc_unlock(&_SSL_lock);
    }

    ctx = _cc_malloc(sizeof(_cc_OpenSSL_t));
    _cc_lock(&_SSL_lock, 1, _CC_LOCK_SPIN_);
    ssl_ctx = SSL_CTX_new(TLS_method());
    //ssl_ctx = SSL_CTX_new(SSLv23_method());
    _cc_unlock(&_SSL_lock);

    if (ssl_ctx == nullptr) {
        _SSL_error("SSL_CTX_new");
        _cc_free(ctx);
        return nullptr;
    }

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

#if OPENSSL_VERSION_NUMBER >= 0x009080dfL
    /* only in 0.9.8m+ */
    SSL_CTX_clear_options(ssl_ctx, SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_NO_TLSv1);
#endif

    if (!(protocols & _CC_SSL_SSLv2_)) {
        SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2);
    }
    if (!(protocols & _CC_SSL_SSLv3_)) {
        SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv3);
    }
    if (!(protocols & _CC_SSL_TLSv1_)) {
        SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_TLSv1);
    }
#ifdef SSL_OP_NO_TLSv1_1
    SSL_CTX_clear_options(ssl_ctx, SSL_OP_NO_TLSv1_1);
    if (!(protocols & _CC_SSL_TLSv1_1_)) {
        SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_TLSv1_1);
    }
#endif
#ifdef SSL_OP_NO_TLSv1_2
    SSL_CTX_clear_options(ssl_ctx, SSL_OP_NO_TLSv1_2);
    if (!(protocols & _CC_SSL_TLSv1_2_)) {
        SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_TLSv1_2);
    } 
#endif
#ifdef SSL_OP_NO_TLSv1_3
    SSL_CTX_clear_options(ssl_ctx, SSL_OP_NO_TLSv1_3);
    if (!(protocols & _CC_SSL_TLSv1_3_)) {
        SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_TLSv1_3);
    }
#endif

#ifdef SSL_CTX_set_min_proto_version
    SSL_CTX_set_min_proto_version(ssl_ctx, 0);
    SSL_CTX_set_max_proto_version(ssl_ctx, TLS1_2_VERSION);
#endif

#ifdef TLS1_3_VERSION
    SSL_CTX_set_min_proto_version(ssl_ctx, 0);
    SSL_CTX_set_max_proto_version(ssl_ctx, TLS1_3_VERSION);
#endif

#ifdef SSL_OP_NO_COMPRESSION
    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_COMPRESSION);
#endif

#ifdef SSL_OP_NO_TX_CERTIFICATE_COMPRESSION
    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_TX_CERTIFICATE_COMPRESSION);
    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_RX_CERTIFICATE_COMPRESSION);
#endif

#ifdef SSL_OP_NO_ANTI_REPLAY
    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_ANTI_REPLAY);
#endif

#ifdef SSL_OP_NO_CLIENT_RENEGOTIATION
    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_CLIENT_RENEGOTIATION);
#endif

#ifdef SSL_OP_IGNORE_UNEXPECTED_EOF
    SSL_CTX_set_options(ssl_ctx, SSL_OP_IGNORE_UNEXPECTED_EOF);
#endif

#ifdef SSL_MODE_RELEASE_BUFFERS
    SSL_CTX_set_mode(ssl_ctx, SSL_MODE_RELEASE_BUFFERS);
#endif

#ifdef SSL_MODE_NO_AUTO_CHAIN
    SSL_CTX_set_mode(ssl_ctx, SSL_MODE_NO_AUTO_CHAIN);
#endif

    SSL_CTX_set_read_ahead(ssl_ctx, 1);
    SSL_CTX_set_cipher_list(ssl_ctx, "HIGH:!aNULL:!MD5:!RC4");

    ctx->handle = ssl_ctx;
    ctx->refcount = 0;
    return ctx;
}

_CC_API_PUBLIC(void) _SSL_quit(_cc_OpenSSL_t *ctx) {
    if (_cc_atomic32_dec_ref(&_SSL_init_refcount)) {
    #if OPENSSL_VERSION_NUMBER < OPENSSL_VERSION_1_1_0
        CONF_modules_free();
        ENGINE_cleanup();
        EVP_cleanup();
        CRYPTO_cleanup_all_ex_data();
        ERR_free_strings();
    #if OPENSSL_VERSION_NUMBER >= OPENSSL_VERSION_1_0_2
        SSL_COMP_free_compression_methods();
    #endif
    #endif
    }

    SSL_CTX_free(ctx->handle);
    ctx->handle = nullptr;
    _cc_free(ctx);
}

/**/
_CC_API_PUBLIC(bool_t) _SSL_free(_cc_SSL_t *ssl) {
    if (ssl->handle == nullptr || ssl->ctx == nullptr) {
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
    return true;
}

/**/
_CC_API_PUBLIC(_cc_SSL_t*) _SSL_alloc(_cc_OpenSSL_t* ctx) {
    _cc_SSL_t *ssl = _cc_malloc(sizeof(_cc_SSL_t));
    
    static _cc_atomic64_t session_id = 0;
    uint64_t session_id_context = _cc_atomic64_inc(&session_id);
    SSL_CTX_set_session_id_context(ctx->handle, (byte_t*)&session_id_context, sizeof(session_id_context));
    
    _cc_lock(&_SSL_lock, 1, _CC_LOCK_SPIN_);
    ssl->handle = SSL_new(ctx->handle);
    _SSL_lock = 0;

    if (ssl->handle == nullptr) {
        _cc_free(ssl);
        return nullptr;
    }

    ssl->ctx = ctx;

    return ssl;
}

_CC_API_PRIVATE(int) _ssl_pkey_password_callback(char *buf, int size, int rwflag, void *userdata) {
    const char *password = (const char *)userdata;
    int password_length;
    if (rwflag) {
        _cc_logger_alert(_T("_ssl_pkey_password_callback() is called for encryption,rwflag:%d"), rwflag);
        return 0;
    }
    if (password == nullptr) {
        return 0;
    }
    password_length = (int)_tcslen(password);
    if (password_length > size) {
        _cc_logger_warin(_T("password is truncated to %d bytes, password_length:%d > size:%d"), size, password_length, size);
        password_length = size;
    }
    memcpy(buf, password, password_length);
    buf[password_length] = '\0';
    return password_length;
}

_CC_API_PUBLIC(bool_t) _SSL_setup(_cc_OpenSSL_t *ssl,
                                const tchar_t *cert_file,
                                const tchar_t *key_file,
                                const tchar_t *key_password) {
    if (key_password) {
        SSL_CTX_set_default_passwd_cb_userdata(ssl->handle, (void*)key_password);
        SSL_CTX_set_default_passwd_cb(ssl->handle, _ssl_pkey_password_callback);
    }

    if (SSL_CTX_use_certificate_chain_file(ssl->handle, cert_file) <= 0) {
        _cc_logger_error(_T("Failed to load certificate file: %s SSL_CTX_use_certificate_file failed: %s"), cert_file, ERR_reason_error_string(ERR_get_error()));
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(ssl->handle, key_file, SSL_FILETYPE_PEM) <= 0) {
        _cc_logger_error(_T("Failed to load private key file: %s SSL_CTX_use_PrivateKey_file failed: %s"), key_file,ERR_reason_error_string(ERR_get_error()));
        return false;
    }

    if (!SSL_CTX_check_private_key(ssl->handle)) {
        _cc_logger_error(_T("SSL_CTX_check_private_key failed: %s\n"), ERR_reason_error_string(ERR_get_error()));
        return false;
    }

    return true;
}
/*
static int _SSL_sni_callback(SSL *ssl, int *ad, void *arg) {
    _cc_OpenSSL_t *ctx = (_cc_OpenSSL_t*)arg;
    const char *servername = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
    if (!servername) {
        return SSL_TLSEXT_ERR_NOACK;
    }
    ctx->sni_callback(ssl, servername);
    return SSL_TLSEXT_ERR_OK;
}

_CC_API_PUBLIC(bool_t) _SSL_setup_sni_callback(_cc_OpenSSL_t *ssl,  callback) {
    SSL_CTX_set_tlsext_servername_callback(ssl->handle, _SSL_sni_callback);
    SSL_CTX_set_tlsext_servername_arg(ssl->handle, ssl);
    return true;
}*/

_CC_API_PUBLIC(bool_t) _SSL_setup_pkcs12(_cc_OpenSSL_t *ssl,
                                const tchar_t *pkcs12_file,
                                const tchar_t *password) {
    PKCS12 *p12;
    EVP_PKEY *key = nullptr;
    X509 *cert = nullptr;
    STACK_OF(X509) *ca = nullptr;

    FILE *fp = _tfopen(pkcs12_file, _T("rb"));
    if (fp == nullptr) {
        _cc_logger_error(_T("Failed to open PKCS12 file: %s"), pkcs12_file);
        return false;
    }
    
    p12 = d2i_PKCS12_fp(fp, nullptr);
    fclose(fp);

    if (p12 == nullptr) {
        _cc_logger_error(_T("Failed to read PKCS12 file: %s"), pkcs12_file);
        return false;
    } 

    if (PKCS12_parse(p12, password, &key, &cert, &ca) <= 0) {
        _cc_logger_error(_T("Failed to parse PKCS12 file: %s, %s"), pkcs12_file,ERR_reason_error_string(ERR_get_error()));
        PKCS12_free(p12);
        return false;
    }

    if (SSL_CTX_use_certificate(ssl->handle, cert) <= 0) {
        _cc_logger_error(_T("SSL_CTX_use_certificate failed:%s"),ERR_reason_error_string(ERR_get_error()));
        sk_X509_pop_free(ca, X509_free);
        PKCS12_free(p12);
        return false;
    }

    if (SSL_CTX_use_PrivateKey(ssl->handle, key) <= 0) {
        _cc_logger_error(_T("SSL_CTX_use_PrivateKey failed: %s"),ERR_reason_error_string(ERR_get_error()));
        X509_free(cert);
        sk_X509_pop_free(ca, X509_free);
        PKCS12_free(p12);
        return false;
    }

    X509_free(cert);
    EVP_PKEY_free(key);
    sk_X509_pop_free(ca, X509_free);
    PKCS12_free(p12);
    return true;
}


_CC_API_PUBLIC(void) _SSL_set_host_name(_cc_SSL_t *ssl, const tchar_t *host, size_t length) {
#ifdef _CC_UNICODE_
    char host_utf8[256];
#endif
#ifdef _CC_UNICODE_
    _cc_utf16_to_utf8((uint16_t *)host, (uint16_t *)(host + length), (uint8_t *)host_utf8,
                      (uint8_t *)host_utf8 + _cc_countof(host_utf8));
    SSL_set_tlsext_host_name(ssl->handle, host_utf8);
#else
    SSL_set_tlsext_host_name(ssl->handle, host);
    _CC_UNUSED(length);
#endif
}

/**/
_CC_API_PUBLIC(int) _SSL_set_alpn_protos(_cc_SSL_t *ssl, const unsigned char *protos, unsigned int protos_len) {
    return SSL_set_alpn_protos(ssl->handle, protos, protos_len);
}

_CC_API_PRIVATE(uint8_t) _SSL_Error(SSL *handle,const char *fn) {
	switch (SSL_get_error(handle, 0)) {
        case SSL_ERROR_WANT_READ:
            return _CC_SSL_HS_WANT_READ_;
        case SSL_ERROR_WANT_WRITE:
            return _CC_SSL_HS_WANT_WRITE_;
		case SSL_ERROR_SYSCALL:{
            int err = _cc_last_errno();
            if ((err == _CC_EINTR_ || err == _CC_EAGAIN_)) {
                return _CC_SSL_HS_SYSCALL_WOULDBLOCK_;
            }
			_cc_logger_error(_T("SSL_get_error failed:%s."), _cc_last_error(_cc_last_errno()));
			return _CC_SSL_HS_ERROR_;
		}
	}
	_SSL_error(fn);
    return _CC_SSL_HS_ERROR_;
}

_CC_API_PUBLIC(_cc_SSL_t*) _SSL_accept(_cc_OpenSSL_t *ctx, _cc_socket_t fd) {
    _cc_SSL_t *ssl = _SSL_alloc(ctx);

    if (ssl) {
		SSL_set_fd(ssl->handle, (int)fd);
		SSL_set_accept_state(ssl->handle);
    }
	return ssl;
}

/**/
_CC_API_PUBLIC(_cc_SSL_t*) _SSL_connect(_cc_OpenSSL_t *ctx, _cc_socket_t fd) {
    _cc_SSL_t *ssl = _SSL_alloc(ctx);
    if (ssl) {
		SSL_set_fd(ssl->handle, (int)fd);
		SSL_set_connect_state(ssl->handle);
	}
	return ssl;
}

/**/
_CC_API_PUBLIC(uint8_t) _SSL_do_handshake(_cc_SSL_t* ssl) {
    ERR_clear_error();
    if (SSL_do_handshake(ssl->handle) == 1) {
		return _CC_SSL_HS_ESTABLISHED_;
    }
    return _SSL_Error(ssl->handle, _CC_FUNC_);
}

/**/
_CC_API_PUBLIC(int32_t) _SSL_send(_cc_SSL_t *ssl, const byte_t *buf, int32_t length) {
    int32_t rc = 0;
	_cc_assert(buf != nullptr);
	_cc_assert(length > 0);

    ERR_clear_error();
    rc = (int32_t)SSL_write(ssl->handle, (char *)buf, length);
    if (rc <= 0) {
		if (_SSL_Error(ssl->handle, _CC_FUNC_) != _CC_SSL_HS_ERROR_) {
			return 0;
		}
    }
    return rc;
}

/**/
_CC_API_PUBLIC(int32_t) _SSL_read(_cc_SSL_t *ssl, byte_t *buf, int32_t length) {
    int32_t rc;

    ERR_clear_error();
    rc = (int32_t)SSL_read(ssl->handle, buf, length);
    if (rc > 0) {
        return rc;
    } else if (rc == 0) {
        return -1;
    }
	if (_SSL_Error(ssl->handle, _CC_FUNC_) != _CC_SSL_HS_ERROR_) {
		return 0;
	}
    return rc;
}

#endif /*OpenSSL*/
