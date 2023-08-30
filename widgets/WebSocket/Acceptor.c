#include "Acceptor.h"

static _cc_rwlock_t _Mir2AcceptorLock = 0;
static _cc_list_iterator_t _Mir2AcceptorLink[ServiceKind_MaxCount] = {0};
static _cc_list_iterator_t _Mir2AcceptorIdleLink = {0};

/**/
void _Mir2InitAcceptor(void) {
    int i;
    for (i = 0; i < ServiceKind_MaxCount; i++) {
        _cc_list_iterator_cleanup(&_Mir2AcceptorLink[i]);
    }
    _cc_list_iterator_cleanup(&_Mir2AcceptorIdleLink);

    _cc_rwlock_init(&_Mir2AcceptorLock);
}

/**/
void _Mir2QuitAcceptor(void) {
    int i;
    _cc_list_iterator_for_each(v, &_Mir2AcceptorIdleLink, {
        _Mir2Network_t *p = _cc_upcast(v, _Mir2Network_t, lnk);
        _cc_free(p);
    });

    for (i = 0; i < ServiceKind_MaxCount; i++) {
        _cc_list_iterator_for_each(v, &_Mir2AcceptorLink[i], {
            _Mir2Network_t *p = _cc_upcast(v, _Mir2Network_t, lnk);
            _cc_free(p);
        });
        _cc_list_iterator_cleanup(&_Mir2AcceptorLink[i]);
    }

    _cc_list_iterator_cleanup(&_Mir2AcceptorIdleLink);
}

/**/
_Mir2Network_t* _Mir2AllocAcceptor(byte_t classify, _cc_sockaddr_t *remoteAddr) {
    byte_t *addr;
    _Mir2Network_t* acceptor;
    _cc_list_iterator_t *iter;
    
    _cc_rwlock_wlock(&_Mir2AcceptorLock);
    iter = _cc_list_iterator_pop_front(&_Mir2AcceptorIdleLink);
    _cc_rwlock_unlock(&_Mir2AcceptorLock);

    if (iter == NULL) {
        acceptor = _CC_MALLOC(_Mir2Network_t);
    } else {
        acceptor = _cc_upcast(iter, _Mir2Network_t, lnk);
    }

    bzero(acceptor, sizeof(_Mir2Network_t));
    if (remoteAddr) {
        if (remoteAddr->sa_family == AF_INET) {
            struct sockaddr_in *a = (struct sockaddr_in *)remoteAddr;
            acceptor->ip.family = a->sin_family;
            addr = (byte_t*)&(a->sin_addr);
        } else {
            struct sockaddr_in6 *a = (struct sockaddr_in6 *)remoteAddr;
            acceptor->ip.family = a->sin6_family;
            addr = (byte_t*)&(a->sin6_addr);
        }
        _cc_inet_ntop(remoteAddr->sa_family, addr, acceptor->ip.data, MAX_IP_BUFFER);
    }


    acceptor->connectedNotify = NULL;
    acceptor->disconnectNotify = NULL;
    
    _cc_rwlock_wlock(&_Mir2AcceptorLock);
    _cc_list_iterator_push(&_Mir2AcceptorLink[classify], &acceptor->lnk);
    _cc_rwlock_unlock(&_Mir2AcceptorLock);

    return acceptor;
}
/**/
void _Mir2BindAcceptor(_Mir2Network_t *network,
                      uint8_t classify,
                      pvoid_t args) {
    if (classify != network->classify) {
        _cc_rwlock_wlock(&_Mir2AcceptorLock);
        _cc_list_iterator_swap(&_Mir2AcceptorLink[classify], &network->lnk);
        _cc_rwlock_unlock(&_Mir2AcceptorLock);
    }
    network->classify = classify;
    network->args = args;
}

/**/
void _Mir2FreeAcceptor(_Mir2Network_t *acceptor) {
    _cc_rwlock_wlock(&_Mir2AcceptorLock);
    _cc_list_iterator_swap(&_Mir2AcceptorIdleLink, &acceptor->lnk);
    _cc_rwlock_unlock(&_Mir2AcceptorLock);
}

/**/
void _Mir2Acceptor(uint8_t classify, void (*func)(_Mir2Network_t *, pvoid_t), pvoid_t args) {
    _Mir2Network_t *acceptor;

    _cc_rwlock_rlock(&_Mir2AcceptorLock);
    _cc_list_iterator_for_each(v, &_Mir2AcceptorLink[classify], {
        acceptor = _cc_upcast(v, _Mir2Network_t, lnk);
            func(acceptor, args);
    });
    _cc_rwlock_unlock(&_Mir2AcceptorLock);
}
