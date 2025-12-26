#include "dns.h"
#include <libcc/alloc.h>
#include <libcc/thread.h>
#include <libcc/event.h>

_CC_API_PRIVATE(bool_t) _dns_response_callback(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
	if (which & _CC_EVENT_READABLE_) {
        struct sockaddr_in sa;
        int32_t n = 0;
//        int32_t res = 0;
//        int32_t offset = 0;
//        struct QUESTION *q;
//        _cc_dns_header_t *dns_header;
//        _cc_dns_question_t dns_question;
        byte_t buffer[_CC_IO_BUFFER_SIZE_];
        socklen_t sa_len = (socklen_t)sizeof(sa);

        n = (int32_t)recvfrom(e->fd, (char*)&buffer, _CC_IO_BUFFER_SIZE_, 0, (struct sockaddr *)&sa, &sa_len);
        
        if (n < sizeof(_cc_dns_header_t)) {
            return true;
        }
        /*
        dns_header = (_cc_dns_header_t*)&buffer;
        offset = sizeof(_cc_dns_header_t);
        dns_question.name = (char_t*)dns_read_name(&buffer[offset], buffer, &res);
        dns_question.name_length = res;
        offset += res;

        q = (struct QUESTION*)&buffer[offset];
        dns_question.type = ntohs(q->type);
        dns_question.classes = ntohs(q->classes);

        printf("DNS:%s\n", dns_question.name);

        _cc_free(dns_question.name);
        */
        return true;
    }
/*
    if (which & _CC_EVENT_TIMEOUT_) {
        return false;
    }

    if (which & _CC_EVENT_CLOSED_) {
        return false;
    }
*/
    return false;
}

bool_t _cc_dns_listen(void) {
    struct sockaddr_in sa;
    _cc_async_event_t *async = _cc_get_async_event();
    _cc_event_t *e = _cc_alloc_event(async, _CC_EVENT_ACCEPT_);
    if (e) {
        e->callback = _dns_response_callback;
        e->timeout = 60000;

        _cc_inet_ipv4_addr(&sa, NULL, 53);
        return _cc_tcp_listen(async, e, (_cc_sockaddr_t *)&sa, sizeof(struct sockaddr_in));
    }
    return false;
}
