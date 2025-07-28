/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
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
#include <libcc/alloc.h>
#include <libcc/socket/socket.h>
#include <libcc/string.h>
#include <libcc/url.h>
#include <libcc/UTF.h>

#ifndef __CC_WINDOWS__
#include <arpa/inet.h>
#else
#include <libcc/core/windows.h>
#endif

/* POST PORT*/
#define DEFAULT_HTTP_PORT 80
#define DEFAULT_NNTP_PORT 119
#define DEFAULT_NEWS_PORT 119
#define DEFAULT_HTTPS_PORT 443
#define DEFAULT_FTP_PORT 21
#define DEFAULT_MYSQL_PORT 3306
#define DEFAULT_SQLSERVER_PORT 1433
#define DEFAULT_ORACLE_PORT 1521
#define DEFAULT_FTPS_PORT 21

static tchar_t *_URL_PATH_ROOT_ = _T("/");

#define _alloc_url_field_data(S1, S2, D, U)                                                                            \
    do {                                                                                                               \
        U->D = _cc_tcsndup((S2), (size_t)((S1) - (S2)));                                                               \
        if (_cc_unlikely(nullptr == U->D)) {                                                                              \
            _cc_free_url(U);                                                                                           \
            return nullptr;                                                                                               \
        }                                                                                                              \
        (S2) = (S1);                                                                                                   \
    } while (0)

/*Scheme name*/
typedef struct _cc_url_scheme {
    uint32_t ident;
    uint32_t port;
    uint32_t value_len;
    tchar_t *value;
} _cc_url_scheme_t;

#define _URL_SCHEME_SUPPORTED_MAP(XX)                                                                                  \
    XX(HTTPS)                                                                                                          \
    XX(HTTP)                                                                                                           \
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

/**/
_CC_API_PRIVATE(bool_t) _url_exists_user_password(const tchar_t *s) {
    const tchar_t *p = s;

    while (*p != '\0' && *p != _T('/')) {
        if (*p == _T('@')) {
            return true;
        }
        ++p;
    }

    return false;
}

/**/
_CC_API_PRIVATE(tchar_t *) _url_user_password_copy(const tchar_t *s, size_t len) {
    tchar_t *d = (tchar_t *)_cc_malloc(sizeof(tchar_t) * (len + 1));
    _cc_raw_url_decode(s, (int32_t)len, d, (int32_t)len);
    return d;
}

/* Returns the scheme type if the scheme is supported, or SCHEME_INVALID if not.
 */
_CC_API_PRIVATE(void) parse_url_scheme(_cc_url_t *u, const tchar_t *scheme, int32_t scheme_len) {
    int32_t i;

    u->scheme.ident = _CC_SCHEME_UNKNOWN_;
    u->scheme.value = nullptr;
    u->port = 0;

    for (i = 0; i < _cc_countof(_url_supported_schemes); i++) {
        const _cc_url_scheme_t *r = &_url_supported_schemes[i];
        if (r->value_len == scheme_len && 0 == _tcsnicmp(scheme, r->value, r->value_len)) {
            u->scheme.ident = r->ident;
            u->scheme.value = r->value;
            u->port = r->port;
            return;
        }
    }

    u->scheme.value = _cc_tcsndup(scheme, scheme_len);
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

/*create url*/
_CC_API_PRIVATE(_cc_url_t *) _create_url(_cc_url_t *u, const tchar_t *url) {
    const tchar_t *curstr, *tmpstr;
    const tchar_t *user_name = nullptr, *user_password = nullptr;

    _cc_assert(u && url);
    /*init url*/
    bzero(u, sizeof(_cc_url_t));
    curstr = url;

    if (*curstr == _T('/')) {
        goto URL_PRASE_PATH_PARAMS;
    }
    /*
     * <scheme>:<scheme-specific-part>
     * <scheme>:= [a-z\+\-\.]+
     * upper case = lower case for resiliency
     */
    /*Read scheme */
    tmpstr = _tcschr(curstr, _T(':'));

    /* End of the string */
    if (_cc_likely(tmpstr && *(tmpstr + 1) == '/' && *(tmpstr + 2) == '/')) {
        /**/
        parse_url_scheme(u, curstr, (int32_t)(tmpstr - curstr));
        /* skip "://" */
        curstr = (tmpstr + 3);
    } else {
        /*not found the character*/
        u->scheme.ident = _url_supported_schemes[0].ident;
        u->scheme.value = _url_supported_schemes[0].value;
        u->port = _url_supported_schemes[0].port;
    }

    /*Check if the user (and password) are specified. */
    if (_url_exists_user_password(curstr)) {
        tmpstr = curstr;
        do {
            /* UserName and Password are specified */
            if (_T(':') == *tmpstr) {
                user_name = tmpstr;
            } else if (_T('@') == *tmpstr) {
                if (user_name == nullptr) {
                    user_name = tmpstr;
                } else {
                    user_password = tmpstr;
                }
                break;
            }
            tmpstr++;
            /* End of <UserName>:<Password> specification */
        } while (*tmpstr && *tmpstr != _T('/'));

        /*User and Password specification*/
        if (user_name) {
            /* Get the username */
            u->username = _url_user_password_copy(curstr, (size_t)(user_name - curstr));

            /*user_name++ skip :*/
            ++user_name;

            /*Get the password*/
            if (user_password) {
                u->password = _url_user_password_copy(user_name, (size_t)(user_password - user_name));
                /*++user_password skip @*/
                curstr = (++user_password);
            } else {
                curstr = user_name;
            }
        }
    }

    /* Proceed on by delimiters with reading host */
    tmpstr = curstr;
    if (_T('[') == *tmpstr) {
        u->ipv6 = true;
        while (*tmpstr) {
            /* End of IPv6 address. */
            if (_T(']') == *tmpstr) {
                break;
            }
            tmpstr++;
        }
    } else {
        while (*tmpstr) {
            /* Port number is specified. */
            if (_T(':') == *tmpstr || _T('/') == *tmpstr) {
                break;
            }
            tmpstr++;
        }
    }

    /* Get the host */
    if (u->ipv6) {
        /* Skip IPv6 ++'[ */
        curstr++;
        _alloc_url_field_data(tmpstr, curstr, host, u);
        /* Skip IPv6 ++']' */
        curstr++;
    } else {
        _alloc_url_field_data(tmpstr, curstr, host, u);
    }

    /* Read port number */
    if (_T(':') == *curstr) {
        uint16_t port;
        /* Skip ':' */
        tmpstr = (curstr + 1);
        port = 0;
        while (_CC_ISDIGIT(*tmpstr)) {
            port = (port * 10) + (*tmpstr++ - '0');
        }
        if (port > 0) {
            u->port = port;
        }
        /* Proceed current pointer */
        curstr = tmpstr;
    }

    /* End of the string */
    if (_T('\0') == *curstr) {
        u->request = _URL_PATH_ROOT_;
        u->path = _URL_PATH_ROOT_;
        return u;
    }

    /* Skip '/' */
    if (_T('/') != *curstr) {
        _cc_free_url(u);
        return nullptr;
    }

URL_PRASE_PATH_PARAMS:
    u->request = _cc_tcsdup(curstr);
    if (_cc_unlikely(u->request == nullptr)) {
        _cc_free_url(u);
        return nullptr;
    }
    /* Parse request*/
    tmpstr = curstr;
    while (*tmpstr) {
        if (_T('#') == *tmpstr || _T('?') == *tmpstr) {
            break;
        }
        tmpstr++;
    }
    /* Get the path*/
    _alloc_url_field_data(tmpstr, curstr, path, u);

    /* Is query specified? */
    if (_T('?') == *curstr) {
        /* Skip '?' */
        tmpstr = (curstr++);
        while (*tmpstr && _T('#') != *tmpstr) {
            tmpstr++;
        }
        /* Read query */
        _alloc_url_field_data(tmpstr, curstr, query, u);
    }

    /* Is fragment specified? */
    if (_T('#') == *curstr) {
        /*Get the Fragment*/
        u->fragment = _cc_tcsdup((++curstr));
    }
    return u;
}

_CC_API_PUBLIC(bool_t) _cc_parse_url(_cc_url_t *u, const tchar_t *url) {
    if (_create_url(u, url) == nullptr) {
        return false;
    }
    return true;
}

/*create url*/
_CC_API_PUBLIC(_cc_url_t *) _cc_create_url(const tchar_t *url) {
    _cc_url_t *u = _CC_MALLOC(_cc_url_t);
    if (_create_url(u, url) == nullptr) {
        _cc_free(u);
        return nullptr;
    }
    return u;
}
/**/
_CC_API_PUBLIC(bool_t) _cc_free_url(_cc_url_t *url) {
    if (url->request != nullptr && url->request != _URL_PATH_ROOT_) {
        _cc_free(url->request);
        url->request = nullptr;
    }

    if (url->path != nullptr && url->path != _URL_PATH_ROOT_) {
        _cc_free(url->path);
        url->path = nullptr;
    }

    _cc_safe_free(url->host);
    _cc_safe_free(url->query);
    _cc_safe_free(url->fragment);
    _cc_safe_free(url->username);
    _cc_safe_free(url->password);

    if (url->scheme.value && url->scheme.ident == _CC_SCHEME_UNKNOWN_) {
        _cc_free(url->scheme.value);
        url->scheme.value = nullptr;
    }
    return true;
}
/**/
_CC_API_PUBLIC(bool_t) _cc_destroy_url(_cc_url_t **url) {
    if (_cc_likely(url && *url)) {
        _cc_free_url(*url);
        _cc_free((*url));
        (*url) = nullptr;

        return true;
    }
    return false;
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
