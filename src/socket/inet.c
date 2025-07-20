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

#define _CC_INET_ADDRSTRLEN_ 16
#define _CC_INET6_ADDRSTRLEN_ 46

#if 0
_CC_API_PRIVATE(bool_t) _get_remote_host(int family, const tchar_t *host, _cc_sockaddr_t *addr, _cc_socklen_t socklen) {
    int i;
    struct hostent *remoteHost;
    if ((remoteHost = gethostbyname(host)) == nullptr) {
        return false;
    }
    i = 0;
    if (remoteHost->h_addrtype == family) {
        while (remoteHost->h_addr_list[i] != 0) {
            struct sockaddr_in *sa = (struct sockaddr_in *)addr;
            memcpy(&sa->sin_addr, remoteHost->h_addr_list[i++], sizeof(struct in_addr));
            return true;
        }
    }
    return false;
}
#else
_CC_API_PRIVATE(bool_t) _get_remote_host(int family, const tchar_t *host, _cc_sockaddr_t *addr, _cc_socklen_t socklen) {
    int rc;
    bool_t result = false;
    _cc_addrinfo_t hints, *addr_list, *cur;
    bzero(&hints, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = family;
    rc = _cc_getaddrinfo(host, nullptr, &hints, &addr_list);
    if (rc != 0 || addr_list == nullptr) {
#ifdef EAI_SYSTEM
        _cc_logger_error(_T("getaddrinfo Error: %s, %s"), rc != EAI_SYSTEM ? gai_strerror(rc) : _cc_last_error(rc), host);
#else
        _cc_logger_error(_T("getaddrinfo Error: %s, %s"), _cc_last_error(rc), host);
#endif
        if (addr_list) {
            _cc_freeaddrinfo(addr_list);
        }
        return false;
    }

    for (cur = addr_list; cur != nullptr; cur = cur->ai_next) {
        if (cur->ai_family == family) {
            memcpy(addr, cur->ai_addr, socklen);
            result = true;
            break;
        }
    }
    
    _cc_freeaddrinfo(addr_list);
    return result;
}
#endif
/**/
_CC_API_PUBLIC(void) _cc_inet_ipv4_addr(struct sockaddr_in *addr, const tchar_t *ip, int port) {
    _cc_assert(addr != nullptr);

    bzero(addr, sizeof(struct sockaddr_in));

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if (ip == nullptr) {
        addr->sin_addr.s_addr = INADDR_ANY;
        return;
    }

    if (_cc_inet_pton4(ip, (byte_t *)&addr->sin_addr.s_addr)) {
        return;
    }

    if (_get_remote_host(AF_INET, ip, (_cc_sockaddr_t *)addr, sizeof(struct sockaddr_in))) {
        addr->sin_port = htons(port);
    }
}

/**/
_CC_API_PUBLIC(void) _cc_inet_ipv6_addr(struct sockaddr_in6 *addr, const tchar_t *ip, int port) {
    _cc_assert(addr != nullptr);

    bzero(addr, sizeof(struct sockaddr_in6));

    addr->sin6_family = AF_INET6;
    addr->sin6_port = htons(port);
    if (ip == nullptr) {
        _cc_inet_pton6(_T("::"), (byte_t *)&addr->sin6_addr);
        return;
    }

    if (_cc_inet_pton6(ip, (byte_t *)&addr->sin6_addr)) {
        return;
    }

    if (_get_remote_host(AF_INET6, ip, (_cc_sockaddr_t *)addr, sizeof(struct sockaddr_in6))) {
        addr->sin6_port = htons(port);
    }
}

/**/
_CC_API_PUBLIC(bool_t) _cc_inet_ntop(int af, const byte_t *src, tchar_t *dst, int32_t size) {
    switch (af) {
    case AF_INET:
        return (_cc_inet_ntop4(src, dst, size));
    case AF_INET6:
        return (_cc_inet_ntop6(src, dst, size));
    }
    return false;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_inet_ntop4(const byte_t *src, tchar_t *dst, int32_t size) {
    int32_t l = (int32_t)_sntprintf(dst, size, _T("%u.%u.%u.%u"), src[0], src[1], src[2], src[3]);
    if (l <= 0 || l >= size) {
        return false;
    }

    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_inet_ntop6(const byte_t *src, tchar_t *dst, int32_t size) {
    /*
     * Note that int32_t and int16_t need only be "at least" large enough
     * to contain a value of the specified size.  On some systems, like
     * Crays, there is no such thing as an integer variable with 16 bits.
     * Keep this in mind if you think this function should have been coded
     * to use pointer overlays.  All the world's not a VAX.
     */
    tchar_t tmp[_CC_INET6_ADDRSTRLEN_], *tp;
    struct {
        int base, len;
    } best, cur;
    unsigned int words[sizeof(struct in6_addr) / sizeof(uint16_t)];
    int i;

    /*
     * Preprocess:
     *  Copy the input (bytewise) array into a wordwise array.
     *  Find the longest run of 0x00's in src[] for :: shorthanding.
     */
    bzero(words, sizeof words);
    for (i = 0; i < (int)sizeof(struct in6_addr); i++) {
        words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
    }

    best.base = -1;
    best.len = 0;
    cur.base = -1;
    cur.len = 0;

    for (i = 0; i < (int)_cc_countof(words); i++) {
        if (words[i] == 0) {
            if (cur.base == -1) {
                cur.base = i;
                cur.len = 1;
            } else {
                cur.len++;
            }
        } else {
            if (cur.base != -1) {
                if (best.base == -1 || cur.len > best.len) {
                    best = cur;
                }
                cur.base = -1;
            }
        }
    }
    if (cur.base != -1) {
        if (best.base == -1 || cur.len > best.len) {
            best = cur;
        }
    }
    if (best.base != -1 && best.len < 2) {
        best.base = -1;
    }

    /*
     * Format the result.
     */
    tp = tmp;
    for (i = 0; i < (int)_cc_countof(words); i++) {
        /* Are we inside the best run of 0x00's? */
        if (best.base != -1 && i >= best.base && i < (best.base + best.len)) {
            if (i == best.base) {
                *tp++ = _T(':');
            }
            continue;
        }
        /* Are we following an initial run of 0x00s or any real hex? */
        if (i != 0) {
            *tp++ = _T(':');
        }
        /* Is this address an encapsulated IPv4? */
        if (i == 6 && best.base == 0 &&
            (best.len == 6 || (best.len == 7 && words[7] != 0x0001) || (best.len == 5 && words[5] == 0xffff))) {
            if (_cc_inet_ntop4(src + 12, tp, (int32_t)(sizeof tmp - (tp - tmp))) == false) {
                return false;
            }

            tp += _tcslen(tp);
            break;
        }
        tp += _sntprintf(tp, (int32_t)(sizeof tmp - (tp - tmp)), _T("%x"), words[i]);
    }

    /* Was it a trailing run of 0x00's? */
    if (best.base != -1 && (best.base + best.len) == _cc_countof(words)) {
        *tp++ = _T(':');
    }

    *tp++ = _T('\0');

    /*
     * Check for overflow, copy, and we're done.
     */
    if ((int32_t)(tp - tmp) > size) {
        return false;
    }

    _tcsncpy(dst, tmp, size);
    dst[size - 1] = 0;

    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_inet_pton(int af, const tchar_t *src, byte_t *dst) {
    if (src == nullptr || dst == nullptr) {
        return false;
    }

    switch (af) {
    case AF_INET:
        return (_cc_inet_pton4(src, dst));
    case AF_INET6: {
        long len = 0;
        const tchar_t *p = nullptr;
        tchar_t tmp[_CC_INET6_ADDRSTRLEN_], *s;
        s = (tchar_t *)src;
        p = _tcschr(src, _T('%'));
        if (p != nullptr) {
            s = tmp;
            len = (long)(p - (tchar_t *)src);
            if (len > _CC_INET6_ADDRSTRLEN_ - 1) {
                return false;
            }
            memcpy(s, src, len);
            s[len] = '\0';
        }
        return _cc_inet_pton6(s, dst);
    }
    default:
        return false;
    }
    /* NOTREACHED */
}

/**/
_CC_API_PUBLIC(bool_t) _cc_inet_pton4(const tchar_t *src, byte_t *dst) {
    static const tchar_t digits[] = _T("0123456789");
    int saw_digit, octets, ch;
    byte_t tmp[sizeof(struct in_addr)], *tp;

    saw_digit = 0;
    octets = 0;
    *(tp = tmp) = 0;
    while ((ch = *src++) != '\0') {
        const tchar_t *pch;

        if ((pch = _tcschr(digits, ch)) != nullptr) {
            unsigned long nw = (unsigned long)(*tp * 10 + (pch - digits));

            if (saw_digit && *tp == 0) {
                return false;
            }
            if (nw > 255) {
                return false;
            }
            *tp = (byte_t)nw;
            if (!saw_digit) {
                if (++octets > 4) {
                    return false;
                }
                saw_digit = 1;
            }
        } else if (ch == _T('.') && saw_digit) {
            if (octets == 4) {
                goto IPV4_SUCCESS;
            }
            *++tp = 0;
            saw_digit = 0;
        } else {
            if (octets == 4) {
                goto IPV4_SUCCESS;
            }
            return false;
        }
    }

    if (octets < 4) {
        return false;
    }

IPV4_SUCCESS:
    memcpy(dst, tmp, sizeof(tmp));

    return true;
}

/**/
_CC_API_PUBLIC(bool_t) _cc_inet_pton6(const tchar_t *src, byte_t *dst) {
    byte_t tmp[sizeof(struct in6_addr)], *tp, *endp, *colonp;
    const tchar_t *xdigits, *curtok;
    int ch, seen_xdigits = 0;
    unsigned int val = 0;

    bzero((tp = tmp), sizeof tmp);
    endp = tp + sizeof tmp;
    colonp = nullptr;

    /* Leading :: requires some special handling. */
    if (*src == _T(':')) {
        if (*++src != _T(':')) {
            return false;
        }
    }

    curtok = src;
    while ((ch = *src++) != _T('\0')) {
        const tchar_t *pch;

        if ((pch = _tcschr((xdigits = _lower_xdigits), ch)) == nullptr) {
            pch = _tcschr((xdigits = _upper_xdigits), ch);
        }

        if (pch != nullptr) {
            val <<= 4;
            val |= (pch - xdigits);
            if (++seen_xdigits > 4) {
                return false;
            }

            continue;
        }
        if (ch == _T(':')) {
            curtok = src;
            if (!seen_xdigits) {
                if (colonp) {
                    return false;
                }
                colonp = tp;
                continue;
            } else if (*src == _T('\0')) {
                return false;
            }
            if (tp + sizeof(uint16_t) > endp) {
                return false;
            }
            *tp++ = (byte_t)(val >> 8) & 0xff;
            *tp++ = (byte_t)val & 0xff;
            seen_xdigits = 0;
            val = 0;
            continue;
        }
        if (ch == _T('.') && ((tp + sizeof(struct in_addr)) <= endp)) {
            if (_cc_inet_pton4(curtok, tp)) {
                tp += sizeof(struct in_addr);
                seen_xdigits = 0;
                break; /*%< '\\0' was seen by inet_pton4(). */
            }
        }
        return false;
    }
    if (seen_xdigits) {
        if (tp + sizeof(uint16_t) > endp) {
            return false;
        }

        *tp++ = (byte_t)(val >> 8) & 0xff;
        *tp++ = (byte_t)val & 0xff;
    }
    if (colonp != nullptr) {
        /*
         * Since some memmove()'s erroneously fail to handle
         * overlapping regions, we'll do the shift by hand.
         */
        const long n = (const long)(tp - colonp);
        long i;

        if (tp == endp) {
            return false;
        }
        for (i = 1; i <= n; i++) {
            endp[-i] = colonp[n - i];
            colonp[n - i] = 0;
        }
        tp = endp;
    }

    if (tp != endp) {
        return false;
    }

    memcpy(dst, tmp, sizeof tmp);
    return true;
}
