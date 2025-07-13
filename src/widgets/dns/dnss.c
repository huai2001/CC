/*
 * Copyright libcc.cn@gmail.com. and other libCC contributors.
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
#include "dns.h"

_CC_API_PRIVATE(bool_t) _dns_response_callback(_cc_event_cycle_t *cycle, _cc_event_t *e, const uint16_t which) {
	if (which & _CC_EVENT_READABLE_) {
        struct sockaddr_in sa;
        int32_t n = 0, res = 0;
        int32_t offset = 0;
        struct QUESTION *q;
        _cc_dns_header_t *dns_header;
        _cc_dns_question_t dns_question;
        byte_t buffer[_CC_IO_BUFFER_SIZE_];
        socklen_t sa_len = (socklen_t)sizeof(sa);

        n = (int32_t)recvfrom(e->fd, (char*)&buffer, _CC_IO_BUFFER_SIZE_, 0, (struct sockaddr *)&sa, &sa_len);
        
        if (n < sizeof(_cc_dns_header_t)) {
            return true;
        }
        
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
        
        return true;
    }
/*
    if (which & _CC_EVENT_TIMEOUT_) {
        return false;
    }

    if (which & _CC_EVENT_DISCONNECT_) {
        return false;
    }
*/
    return false;
}

bool_t _cc_dns_listen(void) {
    struct sockaddr_in sa;
    _cc_socket_t io_fd;
    _cc_event_cycle_t *cycle = _cc_get_event_cycle();

    io_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (io_fd == -1) {
        _cc_logger_error(_T("socket err:%s"), _cc_last_error(_cc_last_errno()));
        return false;
    }

    _cc_inet_ipv4_addr(&sa, nullptr, 53);
    if (bind(io_fd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
        _cc_logger_error(_T("bing port error:%s"), _cc_last_error(_cc_last_errno()));
        return false;
    }

    if (cycle->attach(cycle, _CC_EVENT_READABLE_, io_fd, 0, _dns_response_callback, nullptr) == nullptr) {
        _cc_logger_error(_T("thread %d add socket (%d) event fial."), _cc_get_thread_id(nullptr), io_fd);
        return false;
    }
    return true;
}
