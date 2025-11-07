#include <libcc/socket.h>

_CC_API_PRIVATE(_cc_socket_t) _tcp_socket(void) {
#if _CC_USE_WSASOCKET_
    return (_cc_socket_t)WSASocket(AF_INET, SOCK_STREAM, 0, 0, 0, 0);
#else
    return (_cc_socket_t)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
}

/* oh, the humanity! */
_CC_API_PUBLIC(int) _cc_pipe(_cc_socket_t filedes[2]) {
    static int id = 0;
    FD_SET rs;
    _cc_socket_t ls;
    struct timeval socktm;
    struct sockaddr_in pa;
    struct sockaddr_in la;
    struct sockaddr_in ca;
    int nrd;
    int ll = sizeof(la);
    int lc = sizeof(ca);
    unsigned long bm = 1;
    int uid[2];
    int iid[2];

    _cc_socket_t *rd = &filedes[0];
    _cc_socket_t *wr = &filedes[1];

    /* Create the unique socket identifier
     * so that we know the connection originated
     * from us.
     */
    uid[0] = _cc_getpid();
    uid[1] = id++;
    if ((ls = _tcp_socket()) == INVALID_SOCKET) {
        return _cc_last_errno();
    }

    _cc_inet_ipv4_addr(&pa, _T("127.0.0.1"), 0);

    if (bind(ls, (SOCKADDR *)&pa, sizeof(pa)) == SOCKET_ERROR) {
        goto PIPE_FAIL;
    }

    if (getsockname(ls, (SOCKADDR *)&la, &ll) == SOCKET_ERROR) {
        goto PIPE_FAIL;
    }

    if (listen(ls, 1) == SOCKET_ERROR) {
        goto PIPE_FAIL;
    }

    if ((*wr = (_cc_socket_t)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        goto PIPE_FAIL;
    }

    if (connect(*wr, (SOCKADDR *)&la, sizeof(la)) == SOCKET_ERROR) {
        goto PIPE_FAIL;
    }

    if (send(*wr, (char *)uid, sizeof(uid), 0) != sizeof(uid)) {
        goto PIPE_FAIL;
    }

    if (ioctlsocket(ls, FIONBIO, &bm) == SOCKET_ERROR) {
        goto PIPE_FAIL;
    }

    for (;;) {
        int ns;
        /* Listening socket is nonblocking by now.
         * The accept should create the socket
         * immediatelly because we are connected already.
         * However on buys systems this can take a while
         * until winsock gets a chance to handle the events.
         */
        FD_ZERO(&rs);
        FD_SET(ls, &rs);

        socktm.tv_sec  = 1;
        socktm.tv_usec = 0;

        if ((ns = select(0, &rs, nullptr, nullptr, &socktm)) == SOCKET_ERROR) {
            /* Accept still not signaled */
            Sleep(100);
            continue;
        }

        if (ns == 0) {
            /* No connections in the last second */
            continue;
        }

        if ((*rd = (_cc_socket_t)accept(ls, (SOCKADDR *)&ca, &lc)) == INVALID_SOCKET) {
            goto PIPE_FAIL;
        }

        /* Verify the connection by reading the send identification.
         */
        nrd = _cc_recv(*rd, (byte_t*)iid, (int32_t)sizeof(iid));
        if (nrd == sizeof(iid)) {
            if (memcmp(uid, iid, sizeof(uid)) == 0) {
                /* Wow, we recived what we send.
                 * Put read side of the pipe to the blocking
                 * mode and return.
                 */
                bm = 0;
                if (ioctlsocket(*rd, FIONBIO, &bm) == SOCKET_ERROR) {
                    goto PIPE_FAIL;
                }
                break;
            }
        } else if (nrd == SOCKET_ERROR) {
            goto PIPE_FAIL;
        }
        _cc_close_socket(*rd);
    }

    /* We don't need the listening socket any more */
    _cc_close_socket(ls);
    return 0;

PIPE_FAIL:
    /* Don't leak resources */
    if (*rd != INVALID_SOCKET)
        _cc_close_socket(*rd);
    if (*wr != INVALID_SOCKET)
        _cc_close_socket(*wr);

    *rd = INVALID_SOCKET;
    *wr = INVALID_SOCKET;
    _cc_close_socket(ls);
    return -1;
}

#undef pipe
#define pipe(filedes) _cc_pipe(filedes)