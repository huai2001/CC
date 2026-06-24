#include "dns.h"
#include <libcc/alloc.h>
#include <libcc/thread.h>
#include <libcc/event.h>

#define DNS_PORT 53
/*
 * This will convert 3www6google3com to www.google.com
 * got it :)
 * */
_CC_API_PRIVATE(int) dns_read_name(char_t *domain_name, size_t domain_name_length, uint8_t *buffer, size_t buffer_length) {
    int offset = 0;
    size_t pos = 0;
    if (domain_name == NULL || buffer == NULL || buffer_length == 0) {
        return -1;
    }
    while (offset < (int)buffer_length && pos < (int)domain_name_length) {
        uint8_t length = buffer[offset];
        if (length == 0) {
            domain_name[pos - 1] = '\0';
            return pos;
        }
        if (length > 63 || (offset + 1 + length) > (int)buffer_length) {
            return -1;
        }
        memcpy(domain_name + pos, buffer + offset + 1, length);
        pos += length;
        domain_name[pos++] = '.';
        offset += length + 1;
    }
    return -1;
}

bool_t _cc_dns_listen(void) {
    _cc_socket_t sockfd;
    struct sockaddr_in sa;
    int opt = 1;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return false;
    }

    _cc_inet_ipv4_addr(&sa, NULL, DNS_PORT);

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
    if (bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("Bind failed");
        return false;
    }
    printf("DNS Server listening on port %d...\n", DNS_PORT);
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        byte_t buffer[65535];
        int n = (int)recvfrom(sockfd, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len);
        if (n < 0) {
            perror("recvfrom failed");
            continue;
        }
        _cc_dns_header_t *dns_header = (_cc_dns_header_t *)buffer;
        printf("Received DNS query from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        tchar_t domain_name[512];
        int offset = sizeof(_cc_dns_header_t);
        int domain_length = dns_read_name(domain_name, 512, &buffer[offset], n - offset);

        printf("DNS request for %s\n", domain_name);
        struct QUESTION *q = (struct QUESTION *)&buffer[offset + domain_length + 1];
        printf("DNS Type: %d, Class: %d\n", ntohs(q->type), ntohs(q->classes));
    }
    return false;
}