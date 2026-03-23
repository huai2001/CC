#include <libcc/alloc.h>
#include <libcc/socket.h>

#define _CC_INET_ADDRSTRLEN_ 16
#define _CC_INET6_ADDRSTRLEN_ 46

#if 0
_CC_API_PRIVATE(bool_t) _get_remote_host(int family, const tchar_t *host, _cc_sockaddr_t *addr, _cc_socklen_t socklen) {
    int i;
    struct hostent *remoteHost;
    if ((remoteHost = gethostbyname(host)) == NULL) {
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
    rc = _cc_getaddrinfo(host, NULL, &hints, &addr_list);
    if (rc != 0 || addr_list == NULL) {
#ifdef EAI_SYSTEM
        _cc_logger_error("getaddrinfo Error: %s, %s", rc != EAI_SYSTEM ? gai_strerror(rc) : _cc_last_error(rc), host);
#else
        _cc_logger_error("getaddrinfo Error: %s, %s", _cc_last_error(rc), host);
#endif
        if (addr_list) {
            _cc_freeaddrinfo(addr_list);
        }
        return false;
    }

    for (cur = addr_list; cur != NULL; cur = cur->ai_next) {
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
    _cc_assert(addr != NULL);

    memset(addr, 0, sizeof(struct sockaddr_in));

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if (ip == NULL) {
        addr->sin_addr.s_addr = INADDR_ANY;
        return;
    }

    if (inet_pton(AF_INET, ip, (byte_t *)&addr->sin_addr.s_addr)) {
        return;
    }

    if (_get_remote_host(AF_INET, ip, (_cc_sockaddr_t *)addr, sizeof(struct sockaddr_in))) {
        addr->sin_port = htons(port);
    }
}

/**/
_CC_API_PUBLIC(void) _cc_inet_ipv6_addr(struct sockaddr_in6 *addr, const tchar_t *ip, int port) {
    _cc_assert(addr != NULL);

    memset(addr, 0, sizeof(struct sockaddr_in6));

    addr->sin6_family = AF_INET6;
    addr->sin6_port = htons(port);
    if (ip == NULL) {
        inet_pton(AF_INET6, _T("::"), (byte_t *)&addr->sin6_addr);
        return;
    }

    if (inet_pton(AF_INET6, ip, (byte_t *)&addr->sin6_addr)) {
        return;
    }

    if (_get_remote_host(AF_INET6, ip, (_cc_sockaddr_t *)addr, sizeof(struct sockaddr_in6))) {
        addr->sin6_port = htons(port);
    }
}

/**/
_CC_API_PUBLIC(bool_t) _cc_is_valid_ipv4_addr(const tchar_t *str, const tchar_t *endptr) {
    bool_t is_digit = false;
    int octets = 0;
    int v = 0;

    if (!str || !endptr || str >= endptr) {
        return false;
    }

    while (str < endptr) {
        int ch = *str++;
        if (_CC_ISDIGIT(ch)) {
            v = v * 10 + (ch - '0');
            if (v > 255) {
                return false;
            }
            is_digit = true;
        } else if (ch == '.' && is_digit) {
            if (++octets > 3) {
                return false;
            }
            v = 0;
            is_digit = false;
        } else {
            return false;
        }
    }
    return (octets == 3 && is_digit);
}

/**/
_CC_API_PUBLIC(bool_t) _cc_is_valid_ipv6_addr(const tchar_t *str, const tchar_t *endptr) {
    const tchar_t *double_colon = NULL;
    int segments = 0;
    int hex_count = 0;
    bool_t has_ipv4 = false;

    if (!str || !endptr || str == endptr) {
        return false;
    }
    
    // Handle leading ::
    if (*str == ':') {
        ++str;
        if (str >= endptr || *str != ':') {
            return false;
        }
        double_colon = str - 1;
        ++str;
    }
    
    while (str < endptr) {
        tchar_t ch;
        // Count hex digits in segment
        hex_count = 0;
        while (str < endptr && _CC_ISXDIGIT(*str) && hex_count < 4) {
            ++hex_count;
            ++str;
        }
        
        if (str >= endptr) {
            segments++;
            break;
        }

        ch = *str;
        if (ch == ':') {
            if (hex_count == 0) {
                if (double_colon) {
                    return false;
                }
                double_colon = str;
            } else {
                segments++;
            }
            str++;
        } else if (ch == '.' && hex_count > 0 && hex_count <= 3 && !has_ipv4) {
            // Validate embedded IPv4
            if (!_cc_is_valid_ipv4_addr(str - hex_count, endptr)) {
                return false;
            }
            has_ipv4 = true;
            break;
        } else {
            return false;
        }
    }
    
    if (double_colon) {
        return segments < 8 && !has_ipv4;
    }
    return segments == 8 || (has_ipv4 && segments == 6);
}