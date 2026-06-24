#include <libcc.h>
#include <libcc/dns.h>

#define DNS_SERVERS_MCOUNT 10
#define DNS_BUFFER_SIZE 65536

int main(int argc, char *argv[]) {
    int c;
    _cc_alloc_async_event(0, NULL);
    _cc_dns_listen();    

    while ((c = getchar()) != 'q') {
        _cc_sleep(100);
    }

    _cc_free_async_event();
    return 0;
}

/*
uint8_t buf[DNS_BUFFER_SIZE];
int offset;

int main(int argc, char *argv[]) {
    struct sockaddr_in dest;
    int length;
    _cc_socket_t io_fd = _CC_INVALID_SOCKET_;
    if (!_cc_install_socket()) {
        printf("Failed to initialize socket library\n");
        return 1;
    }

    io_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(io_fd == -1) {
        printf("socket err\n");
        return 1;
    }
    _cc_inet_ipv4_addr(&dest, "114.114.114.114", 53);
    length = _cc_dns_question(buf, sizeof(buf), "www.google.com", _CC_DNS_T_AAAA_);
    if (length <= 0) {
        printf("Failed to build DNS question\n");
        return 1;
    }

    printf("Sending Packet...\n");
    if (sendto(io_fd, (char *)buf, length, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
        return 0;
    }

    printf("Receiving answer...\n");
    byte_t recv_buffer[_CC_IO_BUFFER_SIZE_];
    socklen_t dest_len = (socklen_t)sizeof(struct sockaddr_in);
    int bytes_recv = recvfrom(io_fd, (char *)recv_buffer, _cc_countof(recv_buffer), 0, (struct sockaddr *)&dest, (socklen_t *)&dest_len);
    if (bytes_recv < 0) {
        // 检查是否是超时错误
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("Timeout: No response from server within %d seconds.\n", 5);
        } else {
            perror("Recv failed");
        }
        return 1;
    }
    _cc_dns_response(recv_buffer, (size_t)bytes_recv);
    _cc_uninstall_socket();
    return 0;
}*/