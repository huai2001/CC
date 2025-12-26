#include <libcc/alloc.h>
#include <libcc/socket.h>
#include <libcc/string.h>
#include <libcc/url.h>
#include <libcc/UTF.h>

#ifndef __CC_WINDOWS__
#include <arpa/inet.h>
#else
#include <libcc/os/windows.h>
#endif

static tchar_t *_URL_PATH_ROOT_ = _T("/");

/*Scheme name*/
typedef struct _cc_url_scheme {
    uint32_t ident;
    uint32_t port;
    uint32_t value_length;
    tchar_t *value;
} _cc_url_scheme_t;

#define _URL_SCHEME_SUPPORTED_MAP(XX)                                                                                  \
    XX(HTTP)                                                                                                           \
    XX(HTTPS)                                                                                                          \
    XX(WS)                                                                                                             \
    XX(WSS)                                                                                                            \
    XX(FTP)                                                                                                            \
    XX(FTPS)                                                                                                           \
    XX(NNTP)                                                                                                           \
    XX(NEWS)                                                                                                           \
    XX(MYSQL)                                                                                                          \
    XX(SQLSERVER)                                                                                                      \
    XX(SQLITE)                                                                                                         \
    XX(EXCEL)                                                                                                          \
    XX(ORACLE)

const _cc_url_scheme_t _url_supported_schemes[] = {
#define XX(_CODE_) {_CC_SCHEME_##_CODE_##_, _CC_PORT_##_CODE_##_, sizeof(#_CODE_) - 1, _T(#_CODE_)},
    _URL_SCHEME_SUPPORTED_MAP(XX)
#undef XX
};

#define _alloc_url_field_data(S1, S2, D, U) do {                                                                       \
        U->D = _cc_sds_alloc((S2), (size_t)((S1) - (S2)));                                                             \
        (S2) = (S1);                                                                                                   \
    } while (0)

/**/
_CC_API_PRIVATE(bool_t) _url_exists_user_password(const tchar_t *s) {
    const tchar_t *p = s;
    while (*p && *p != _T('/')) {
        if (*p == _T('@')) {
            return true;
        }
        ++p;
    }
    return false;
}

/**/
_CC_API_PRIVATE(_cc_sds_t) _url_user_password_copy(const tchar_t *s, size_t length) {
    _cc_sds_t d = _cc_sds_alloc(NULL, length);
    length = _cc_raw_url_decode(s, (int32_t)length, d, (int32_t)length);
    _cc_sds_set_length(d, length);
    return d;
}

/* Returns the scheme type if the scheme is supported, or SCHEME_INVALID if not.
 */
_CC_API_PRIVATE(void) parse_url_scheme(_cc_url_t *u, const tchar_t *scheme, int32_t scheme_length) {
    int32_t i;

    u->scheme.ident = _CC_SCHEME_UNKNOWN_;
    u->port = 0;

    for (i = 0; i < _cc_countof(_url_supported_schemes); i++) {
        const _cc_url_scheme_t *r = &_url_supported_schemes[i];
        if (r->value_length == scheme_length && 0 == _tcsnicmp(scheme, r->value, r->value_length)) {
            u->scheme.ident = r->ident;
            u->scheme.value = r->value;
            u->port = r->port;
            return;
        }
    }

    u->scheme.value = _cc_sds_alloc(scheme, scheme_length);
    return;
}

#if 0
/* check host name, i.e. A-Z or 0-9 or -.:*/
_CC_API_PRIVATE(bool_t) is_valid_host_char(tchar_t chr) {
    return ( _CC_ISALPHA(chr) != 0 || _CC_ISDIGIT(chr) 
            || chr == _T('-') || chr == _T('.') || chr == _T(':') || chr == _T('_'));
}

/*check Host*/
_CC_API_PRIVATE(bool_t) is_valid_host(const tchar_t *_host) {
    _cc_assert(_host);
    if ( !_host ) {
        return false;
    }
    if ( _tcslen(_host) < 4 ) {
        return false;
    }

    /**/
    while (*_host) {
        if ( is_valid_host_char(*(_host++)) == false ) {
            return false;
        }
    }
    return true;
}
#endif

/*parser url*/
_CC_API_PRIVATE(_cc_url_t *) _parser_url(_cc_url_t *u, const tchar_t *url) {
    const tchar_t *cursor, *ptr;
    const tchar_t *user_name = NULL, *user_password = NULL;
    const tchar_t *endpos;
    _cc_assert(u && url);
    /*init url*/
    bzero(u, sizeof(_cc_url_t));
    cursor = url;

    if (*cursor == _T('/')) {
        goto URL_PRASE_PATH_PARAMS;
    }
    /*
     * <scheme>:<scheme-specific-part>
     * <scheme>:= [a-z\+\-\.]+
     * upper case = lower case for resiliency
     */
    /*Read scheme */
    ptr = _tcschr(cursor, _T(':'));

    /* End of the string */
    if (_cc_likely(ptr && *(ptr + 1) == '/' && *(ptr + 2) == '/')) {
        /**/
        parse_url_scheme(u, cursor, (int32_t)(ptr - cursor));
        /* skip "://" */
        cursor = (ptr + 3);
    } else {
        /*not found the character*/
        u->scheme.ident = _url_supported_schemes[0].ident;
        u->scheme.value = _url_supported_schemes[0].value;
        u->port = _url_supported_schemes[0].port;
    }
    /*not host*/
    if (*cursor == _T('/')) {
        goto URL_PRASE_PATH_PARAMS;
    }

    /*Check if the user (and password) are specified. */
    if (_url_exists_user_password(cursor)) {
        ptr = cursor;
        do {
            /* UserName and Password are specified */
            if (_T(':') == *ptr) {
                user_name = ptr;
            } else if (_T('@') == *ptr) {
                if (user_name == NULL) {
                    user_name = ptr;
                } else {
                    user_password = ptr;
                }
                break;
            }
            ptr++;
            /* End of <UserName>:<Password> specification */
        } while (*ptr && *ptr != _T('/'));

        /*User and Password specification*/
        if (user_name) {
            /* Get the username */
            u->username = _url_user_password_copy(cursor, (size_t)(user_name - cursor));

            /*user_name++ skip :*/
            ++user_name;

            /*Get the password*/
            if (user_password) {
                u->password = _url_user_password_copy(user_name, (size_t)(user_password - user_name));
                /*++user_password skip @*/
                cursor = (++user_password);
            } else {
                cursor = user_name;
            }
        }
    }

    /* Proceed on by delimiters with reading host */
    ptr = cursor;
    if (_T('[') == *ptr) {
        u->ipv6 = true;
        while (*ptr) {
            /* End of IPv6 address. */
            if (_T(']') == *ptr) {
                break;
            }
            ptr++;
        }
    } else {
        while (*ptr) {
            /* Port number is specified. */
            if (_T(':') == *ptr || _T('/') == *ptr) {
                break;
            }
            ptr++;
        }
    }

    /* Get the host */
    if (u->ipv6) {
        /* Skip IPv6 ++'[ */
        cursor++;
        _alloc_url_field_data(ptr, cursor, host, u);
        /* Skip IPv6 ++']' */
        cursor++;
    } else {
        _alloc_url_field_data(ptr, cursor, host, u);
    }

    /* Read port number */
    if (_T(':') == *cursor) {
        uint16_t port;
        /* Skip ':' */
        ptr = (cursor + 1);
        port = 0;
        while (_CC_ISDIGIT(*ptr)) {
            port = (port * 10) + (*ptr++ - '0');
        }
        if (port > 0) {
            u->port = port;
        }
        /* Proceed current pointer */
        cursor = ptr;
    }

    /* End of the string */
    if (_T('\0') == *cursor) {
        u->request = _URL_PATH_ROOT_;
        u->path = _URL_PATH_ROOT_;
        return u;
    }

    /* Skip '/' */
    if (_T('/') != *cursor) {
        _cc_free_url(u);
        return NULL;
    }

URL_PRASE_PATH_PARAMS:
    endpos = (cursor + _tcslen(cursor));
    u->request = _cc_sds_alloc(cursor,(size_t)(endpos - cursor));

    /* Parse request*/
    ptr = cursor;
    while (ptr < endpos) {
        if (_T('#') == *ptr || _T('?') == *ptr) {
            break;
        }
        ptr++;
    }
    /* Get the path*/
    _alloc_url_field_data(ptr, cursor, path, u);

    /* Is query specified? */
    if (_T('?') == *cursor) {
        /* Skip '?' */
        ptr = (cursor++);
        while (ptr < endpos && _T('#') != *ptr) {
            ptr++;
        }
        /* Read query */
        _alloc_url_field_data(ptr, cursor, query, u);
    }

    /* Is fragment specified? */
    if (_T('#') == *cursor && (cursor + 1) < endpos) {
        ++cursor;
        /*Get the Fragment*/
        u->fragment = _cc_sds_alloc(cursor, (size_t)(endpos - cursor));
    }

    return u;
}

_CC_API_PUBLIC(bool_t) _cc_parse_url(_cc_url_t *u, const tchar_t *url) {
    if (_parser_url(u, url) == NULL) {
        return false;
    }
    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_free_url(_cc_url_t *url) {
    if (_cc_unlikely(url == NULL)) {
        return true;
    }
    if (url->request && url->request != _URL_PATH_ROOT_) {
        _cc_sds_free(url->request);
        url->request = NULL;
    }

    if (url->path && url->path != _URL_PATH_ROOT_) {
        _cc_sds_free(url->path);
        url->path = NULL;
    }

    if (url->host) {
        _cc_sds_free(url->host);
        url->host = NULL;
    }

    if (url->query) {
        _cc_sds_free(url->query);
        url->query = NULL;
    }

    if (url->fragment) {
        _cc_sds_free(url->fragment);
        url->fragment = NULL;
    }

    if (url->username) {
        _cc_sds_free(url->username);
        url->username = NULL;
    }

    if (url->password) {
        _cc_sds_free(url->password);
        url->password = NULL;
    }

    if (url->scheme.value && url->scheme.ident == _CC_SCHEME_UNKNOWN_) {
        _cc_sds_free(url->scheme.value);
        url->scheme.value = NULL;
    }
    return true;
}

/* {{{ url_encode
 */

_CC_API_PUBLIC(int32_t) _cc_url_encode(const tchar_t *src, int32_t src_len, tchar_t *dst, int32_t dst_len) {
    register int c;
    register int32_t x, y;

    if (_cc_unlikely(src_len == 0 || dst_len == 0)) {
        return 0;
    }

    for (x = 0, y = 0; (src_len-- && y < dst_len); x++) {
        c = (int)src[x];
#ifdef _CC_UNICODE_
        if (c > 0xff) {
            y += _sntprintf(dst + y, dst_len - y, _T("%%u%04X"), c);
        } else
#endif
        if (c == 0x20) {
            *(dst + y++) = _T('+');
        } else if ((c < '0' && c != '-' && c != '.') || (c < 'A' && c > '9') || (c > 'Z' && c < 'a' && c != '_') ||
                   (c > 'z')) {
            *(dst + y++) = _T('%');
            *(dst + y++) = _lower_xdigits[(uchar_t)c >> 4];
            *(dst + y++) = _lower_xdigits[(uchar_t)c & 15];
        } else {
            *(dst + y++) = c;
        }
    }

    *(dst + y) = 0;

    return y;
}
/* }}} */

/* {{{ url_decode
 */
_CC_API_PUBLIC(int32_t) _cc_url_decode(const tchar_t *src, int32_t src_len, tchar_t *dst, int32_t dst_len) {
    register int32_t i = 0;
    const tchar_t *s = src;
    const tchar_t *e = src + src_len;
    while (s < e && dst_len > i) {
        if (*s == _T('+')) {
            *(dst + i++) = 0x20;
            s++;
            continue;
        }

        if (*s == _T('%')) {
            if (*(s + 1) == 'u' && e >= (s + 6)) {
                int32_t convert_bytes;
                /*skip %u*/
                s += 2;
                convert_bytes = _cc_convert_utf16_literal_to_utf8(&s, e, dst + i, dst_len - i);
                if (_cc_unlikely(convert_bytes == 0)) {
                    return 0;
                }
                i += convert_bytes;
                continue;
            }

            if (_istxdigit((int32_t) * (s + 1)) && _istxdigit((int)*(s + 2))) {
                *(dst + i++) = (tchar_t)_cc_hex2((s + 1));
                s += 3;
                continue;
            }
        }
        *(dst + i++) = *s++;
    }

    *(dst + i) = 0;

    return i;
}
/* }}} */

/* {{{ raw_url_encode
 */
_CC_API_PUBLIC(int32_t) _cc_raw_url_encode(const tchar_t *src, int32_t src_len, tchar_t *dst, int32_t dst_len) {
    register int c;
    register int32_t x, y;

    if (_cc_unlikely(src_len == 0 || dst_len == 0)) {
        return 0;
    }

    for (x = 0, y = 0; (src_len-- && y < dst_len); x++) {
        c = (int)src[x];
#ifdef _CC_UNICODE_
        if (c > 0xff) {
            y += _sntprintf(dst + y, dst_len - y, _T("%%u%04X"), c);
        } else
#endif
            if ((c < '0' && c != '-' && c != '.') || (c < 'A' && c > '9') || (c > 'Z' && c < 'a' && c != '_') ||
                (c > 'z' && c != '~')) {
            *(dst + y++) = _T('%');
            *(dst + y++) = _lower_xdigits[(uchar_t)c >> 4];
            *(dst + y++) = _lower_xdigits[(uchar_t)c & 15];
        } else {
            *(dst + y++) = c;
        }
    }
    *(dst + y) = 0;

    return y;
}
/* }}} */

/* {{{ raw_url_decode
 */
_CC_API_PUBLIC(int32_t) _cc_raw_url_decode(const tchar_t *src, int32_t src_len, tchar_t *dst, int32_t dst_len) {
    register int32_t i = 0;
    const tchar_t *s = src;
    const tchar_t *e = src + src_len;
    while (s < e && dst_len > i) {
        if (*s == _T('%')) {
            if (*(s + 1) == 'u' && e >= (s + 6)) {
                /*skip %u*/
                int32_t convert_bytes;
                s += 2;
                convert_bytes = _cc_convert_utf16_literal_to_utf8(&s, e, dst + i, dst_len - i);
                if (_cc_unlikely(convert_bytes == 0)) {
                    return 0;
                }
                i += convert_bytes;
                continue;
            }

            if (_istxdigit((int32_t) * (s + 1)) && _istxdigit((int)*(s + 2))) {
                *(dst + i++) = (tchar_t)_cc_hex2((s + 1));
                s += 3;
                continue;
            }
        }
        *(dst + i++) = *s++;
    }

    *(dst + i) = 0;

    return i;
}
/* }}} */
