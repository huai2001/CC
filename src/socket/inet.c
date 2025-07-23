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

    if (inet_pton(AF_INET, ip, (byte_t *)&addr->sin_addr.s_addr)) {
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