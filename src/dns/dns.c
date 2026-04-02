#include <libcc/alloc.h>
#include <libcc/thread.h>
#include <libcc/event.h>
#include "dns.h"

// List of DNS Servers registered on the system
static struct in_addr dns_servers[DNS_SERVERS_MCOUNT];
static int dns_server_count = 0;

_CC_API_PRIVATE(int) dns_build_question(uint8_t *buf, const char_t *host, int type);
_CC_API_PRIVATE(void) dns_ipv4_addr(struct sockaddr_in *);

_CC_API_PRIVATE(uint8_t*) dns_read_name(uint8_t *, uint8_t *, uint8_t *, int *);
_CC_API_PRIVATE(uint8_t*) dns_read_rdata(uint8_t *, uint8_t *, uint8_t *, _cc_dns_record_t *);

_CC_API_PRIVATE(void) dns_cleanup_records(_cc_dns_record_t *records, uint16_t count) {
    uint16_t i;
    if (records == NULL) {
        return;
    }

    for (i = 0; i < count; i++) {
        _cc_dns_record_t *r = records + i;
        _cc_free(r->name);
        _cc_free(r->rdata);
        r->name = NULL;
        r->rdata = NULL;
    }
    _cc_free(records);
}
/*
 * This will convert 3www6google3com to www.google.com
 * got it :)
 * */
_CC_API_PRIVATE(int) _domain(char_t *buf, int dest_length) {
    int i, j, offset;
    for (i = 0; i < dest_length; i++) {
        offset = (int)buf[i];
        for (j = 0; j < offset; j++, i++) {
            buf[i] = buf[i + 1];
        }
        buf[i] = '.';
    }
    // remove the last dot
    buf[i - 1] = 0;
    return i;
}

_CC_API_PRIVATE(int) dns_build_question(uint8_t *buf, const char_t *host, int type) {
    int offset;
    int label_len = 0;
    struct QUESTION *q;
    char_t *p;
    char_t *dot;
    char_t *end;
    /*
     * This will convert www.google.com to 3www6google3com
     * got it :)
     * */
    p = (char_t *)buf;
    dot = p++;
    end = (char_t *)buf + 512;

    while (*host) {
        if (p >= end) {
            return -1;
        }

        if (*host == '.') {
            if (label_len <= 0 || label_len > 63) {
                return -1;
            }
            *dot = (p - dot) - 1;
            dot = p++;
            label_len = 0;
            host++;
            continue;
        }
        *p++ = *host++;
        label_len++;
    }

    if (label_len <= 0 || label_len > 63 || p >= end) {
        return -1;
    }

    // set the last dot
    *dot = (p - dot) - 1;
    *p++ = 0;

    offset = (int)(p - (char_t *)buf);

    if ((offset + (int)sizeof(struct QUESTION)) > 512) {
        return -1;
    }

    // fill it
    q = (struct QUESTION *)p;
    // type of the query , A , MX , CNAME , NS etc
    q->type = htons(type);
    // its internet (lol)
    q->classes = htons(DNS_CLASS_INET);

    return offset + sizeof(struct QUESTION);
}

_CC_API_PRIVATE(void) dump_type(const byte_t *rdata, const uint16_t type) {
    tchar_t addr_buf[128];
    switch (type) {
    case _CC_DNS_T_A_: {
        _cc_inet_ntop(AF_INET, (const pvoid_t)rdata, addr_buf, _cc_countof(addr_buf));
        _tprintf(_T("has IPv4 address : %s\n"), addr_buf);
    } break;
    case _CC_DNS_T_AAAA_: {
        _cc_inet_ntop(AF_INET6, (const pvoid_t)rdata, addr_buf, _cc_countof(addr_buf));
        _tprintf(_T("has IPv6 address : %s\n"), addr_buf);
    } break;
    case _CC_DNS_T_CNAME_: {
        // Canonical name for an alias
        printf("has alias name : %s\n", (char_t *)rdata);
    } break;
    default:
        puts("\n");
        break;
    }
}

_CC_API_PRIVATE(void) dump(const _cc_dns_t *dns) {
    // print answers
    int16_t i;
    _tprintf(_T("Answer Records : %d \n"), dns->header.answer);

    for ( i = 0; i < dns->header.answer; i++) {
        _cc_dns_record_t *r = dns->answers + i;
        _tprintf(_T("Name : %s TTL:%d "), r->name, r->ttl);
        dump_type(r->rdata, r->type);
    }

    // print authorities
    _tprintf(_T("Authoritive Records : %d \n"), dns->header.author);

    for ( i = 0; i < dns->header.author; i++) {
        _cc_dns_record_t *r = dns->authorities + i;
        _tprintf(_T("Name : %s TTL:%d "), r->name, r->ttl);
        dump_type(r->rdata, r->type);
    }

    // print additional resource records
    _tprintf(_T("Additional Records : %d \n"), dns->header.addition);

    for ( i = 0; i < dns->header.addition; i++) {
        _cc_dns_record_t *r = dns->additional + i;
        _tprintf(_T("Name : %s TTL:%d "), r->name, r->ttl);
        dump_type(r->rdata, r->type);
    }
}

_CC_API_PRIVATE(bool_t) _dns_response_callback(_cc_async_event_t *async, _cc_event_t *e, uint32_t which) {
    if (which & _CC_EVENT_READABLE_) {
        uint16_t i;
        int recv_len;
        uint8_t *reader;
        uint8_t *buffer_end;
        int offset;
        byte_t buffer[_CC_IO_BUFFER_SIZE_];
        struct sockaddr_in dest;
        socklen_t sa_len = (socklen_t)sizeof(struct sockaddr_in);
        _cc_dns_header_t *header;
        _cc_dns_record_t *record;
        _cc_dns_t *dns = (_cc_dns_t *)e->data;
        uint16_t header_flags;
        int16_t error_code = 0;

        printf("Receiving answer...\n");
        recv_len = recvfrom(e->fd, (char *)buffer, _cc_countof(buffer), 0, (struct sockaddr *)&dest, (socklen_t *)&sa_len);
        /* Check for valid DNS header */
        if (recv_len < 12) {
            return false;
        }
        buffer_end = buffer + recv_len;

        header = (_cc_dns_header_t *)buffer;
        /*
        rcode = header->flags & 15;
        z = (header->flags >> 4) & 7;
        ra = (header->flags >> 7) & 1;
        rd = (header->flags >> 8) & 1;
        tc = (header->flags >> 9) & 1;
        aa = (header->flags >> 10) & 1;
        opcode = (header->flags >> 11) & 15;
        type = (header->flags >> 15) & 1;
        */

        header_flags = _cc_swap16(header->flags);

        /* Check the truncated response flag first */
        if (header_flags & 0x0200) {
            return false;
        }

        printf("Now check the reply code:%d,%x\n", header_flags & 0x000F, header_flags);
        /* Now check the reply code */

        // switch (header->flags & 0x000F) {
        //     /* Format error with sent query */
        //     case DNS_R_FORMAT_ERROR:
        //         printf("Format error with sent query\n");
        //         error_code = _CC_DNS_ERR_BAD_FORMAT_;
        //         break;
        //     /* No such name exists */
        //     case DNS_R_NAME_ERROR:
        //         printf("No such name exists\n");
        //         error_code = _CC_DNS_ERR_NO_SUCH_NAME_;
        //         break;
        //     /* Server reports failure */
        //     case DNS_R_SERVER_FAILURE:
        //         printf("Server reports failure\n");
        //         error_code = _CC_DNS_ERR_SERVER_FAILURE_;
        //         break;
        //     /* Requested query not supported */
        //     case DNS_R_NOT_IMPLEMENTED:
        //         printf("Requested query not supported\n");
        //         error_code = _CC_DNS_ERR_NOT_IMPLEMENTED_;
        //         break;
        //     /* Server refused to handle query */
        //     case DNS_R_REFUSED:
        //         printf("Server refused to handle query\n");
        //         error_code = _CC_DNS_ERR_QUERY_REFUSED_;
        //         break;
        // }

        if (error_code != 0) {
            return false;
        }

        dns->error_code = error_code;

        /* Note that a root server will most likely ignore queries that
         * request recursive resolution. */
        dns->header.ident = _cc_swap16(header->ident);
        dns->header.flags = header_flags;
        dns->header.quests = _cc_swap16(header->quests);
        dns->header.answer = _cc_swap16(header->answer);
        dns->header.author = _cc_swap16(header->author);
        dns->header.addition = _cc_swap16(header->addition);

        printf("\nThe response contains : %d", recv_len);
        printf("\n %d Questions.", dns->header.quests);
        printf("\n %d Answers.", dns->header.answer);
        printf("\n %d Authoritative Servers.", dns->header.author);
        printf("\n %d Additional records.\n\n", dns->header.addition);

        // move ahead of the dns header and the query field
        offset = sizeof(_cc_dns_header_t);

        /* So far all is fine. Now parse response into lists of items */
        /* Assume that the question section remains unchanged and start
         * with the answers section instead. */
        for (i = 0; i < dns->header.quests; i++) {
            /* Read the Question Name */
#if 0
            char_t name[256];
            int name_length = 0;
            /* Read the length byte by byte until we get 0 which means end of name */
            while (*(buffer + offset) != 0) {
                if (name_length < _cc_countof(name)) {
                    name[name_length++] = (char_t) *(buffer + offset);
                }
                offset++;
            }
            offset++;
            name[name_length] = 0;
            name_length = _domain(name, name_length);
            printf("question name:%s\n", name);
#else
            while (offset < recv_len && buffer[offset] != 0) {
                offset++;
            }
            if (offset >= recv_len) {
                dns->error_code = _CC_DNS_ERR_BAD_FORMAT_;
                goto DNS_PARSE_FAILED;
            }
            offset++;
#endif
            /* Read the QTYPE and QCLASS */
            if ((offset + (int)sizeof(struct QUESTION)) > recv_len) {
                dns->error_code = _CC_DNS_ERR_BAD_FORMAT_;
                goto DNS_PARSE_FAILED;
            }
            offset += sizeof(struct QUESTION);
        }

        if (offset >= recv_len) {
            dns->error_code = _CC_DNS_ERR_BAD_FORMAT_;
            goto DNS_PARSE_FAILED;
        }
        reader = &buffer[offset];

        record = (_cc_dns_record_t *)_cc_calloc(dns->header.answer,sizeof(_cc_dns_record_t));
        dns->answers = record;
        // Start reading answers
        for (i = 0; i < dns->header.answer; i++) {
            _cc_dns_record_t *r = record + i;
            reader = dns_read_rdata(reader, buffer, buffer_end, r);
            if (reader == NULL) {
                dns->error_code = _CC_DNS_ERR_BAD_FORMAT_;
                goto DNS_PARSE_FAILED;
            }
        }

        record = (_cc_dns_record_t *)_cc_calloc(dns->header.author,sizeof(_cc_dns_record_t));
        dns->authorities = record;
        // read authorities
        for (i = 0; i < dns->header.author; i++) {
            _cc_dns_record_t *r = record + i;
            reader = dns_read_rdata(reader, buffer, buffer_end, r);
            if (reader == NULL) {
                dns->error_code = _CC_DNS_ERR_BAD_FORMAT_;
                goto DNS_PARSE_FAILED;
            }
        }

        record = (_cc_dns_record_t *)_cc_calloc(dns->header.addition,sizeof(_cc_dns_record_t));
        dns->additional = record;
        // read additional
        for (i = 0; i < dns->header.addition; i++) {
            _cc_dns_record_t *r = record + i;
            reader = dns_read_rdata(reader, buffer, buffer_end, r);
            if (reader == NULL) {
                dns->error_code = _CC_DNS_ERR_BAD_FORMAT_;
                goto DNS_PARSE_FAILED;
            }
        }
        dump(dns);
        return true;

DNS_PARSE_FAILED:
        dns_cleanup_records(dns->answers, dns->header.answer);
        dns_cleanup_records(dns->authorities, dns->header.author);
        dns_cleanup_records(dns->additional, dns->header.addition);
        dns->answers = NULL;
        dns->authorities = NULL;
        dns->additional = NULL;
        dns->header.answer = 0;
        dns->header.author = 0;
        dns->header.addition = 0;
        return false;
    }

    if (which & _CC_EVENT_TIMEOUT_) {
        return false;
    }

    return true;
}

int _cc_dns_lookup(_cc_dns_t *dns, const char_t *host, int type) {
    uint8_t buf[DNS_BUFFER_SIZE];
    int offset;

    _cc_socket_t dns_sock;
    _cc_dns_header_t *header;

    struct sockaddr_in dest;

    dns->answers = NULL;
    dns->authorities = NULL;
    dns->additional = NULL;

    // UDP packet for DNS queries
    dns_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (dns_sock == _CC_INVALID_SOCKET_) {
        return _CC_DNS_ERR_SEE_ERRNO_;
    }

    dns_ipv4_addr(&dest);

    offset = sizeof(_cc_dns_header_t);

    // point to the query portion
    offset = dns_build_question(&buf[offset], host, type);
    if (offset < 0) {
        _cc_close_socket(dns_sock);
        return _CC_DNS_ERR_FORMAT_ERROR_;
    }
    offset += sizeof(_cc_dns_header_t);

    // Set the DNS structure to standard queries
    header = (_cc_dns_header_t *)&buf;
    memset(header, 0, sizeof(_cc_dns_header_t));
    header->ident = (uint16_t)htons(_cc_getpid());

    // Set standard codes and flags
    header->flags = htons(0x0100); // recursion desired (RD)
    header->quests = htons(1);

    if (offset > 512) {
        _cc_close_socket(dns_sock);
        printf("Question too big for UDP ('%d' bytes)\n", offset);
        return _CC_DNS_ERR_QUERY_TOO_LONG_;
    }

    printf("Sending Packet...\n");
    if (sendto(dns_sock, (char *)buf, offset, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
        _cc_close_socket(dns_sock);
        return _CC_DNS_ERR_SEE_ERRNO_;
    }

    {
        _cc_async_event_t *async = _cc_get_async_event();
        if (async == NULL) {
            _cc_close_socket(dns_sock);
            return _CC_DNS_ERR_SEE_ERRNO_;
        }

        _cc_event_t *e = _cc_alloc_event(async, _CC_EVENT_READABLE_ | _CC_EVENT_TIMEOUT_);
        if (e) {
            e->fd = dns_sock;
            e->callback = _dns_response_callback;
            e->timeout = 60000;
            e->data = (uintptr_t)dns;
            if (!async->attach(async, e)) {
                _cc_logger_error("thread %d attach socket (%d) event fial.", _cc_get_thread_id(NULL), dns_sock);
                _cc_free_event(async, e);
                return _CC_DNS_ERR_SEE_ERRNO_;
            }
            return 0;
        }
    }
    _cc_close_socket(dns_sock);
    return _CC_DNS_ERR_ENOMEM_;
}

void _cc_dns_free(_cc_dns_t *dns) {
    dump(dns);

    if (dns->answers) {
        dns_cleanup_records(dns->answers, dns->header.answer);
    }

    if (dns->authorities) {
        dns_cleanup_records(dns->authorities, dns->header.author);
    }

    if (dns->additional) {
        dns_cleanup_records(dns->additional, dns->header.addition);
    }
    dns->answers = NULL;
    dns->authorities = NULL;
    dns->additional = NULL;

    dns->header.answer = 0;
    dns->header.author = 0;
    dns->header.addition = 0;
}

/*
 *
 * */
_CC_API_PRIVATE(void) dns_ipv4_addr(struct sockaddr_in *addr) {
    _cc_assert(addr != NULL);

    memset(addr, 0, sizeof(struct sockaddr_in));

    addr->sin_family = AF_INET;
    addr->sin_port = 13568; // or htons(53);

    memcpy(&addr->sin_addr, &dns_servers[0], sizeof(addr->sin_addr));
}
/*
 *
 * */
_CC_API_PRIVATE(uint8_t*) dns_read_rdata(uint8_t *reader, uint8_t *buffer, uint8_t *buffer_end, _cc_dns_record_t *rescord) {
    int stop;
    struct R_DATA *r;

    if (reader == NULL || buffer_end == NULL || reader >= buffer_end) {
        return NULL;
    }

    rescord->name = (char_t *)dns_read_name(reader, buffer, buffer_end, &stop);
    if (rescord->name == NULL) {
        return NULL;
    }

    if ((buffer_end - reader) < stop) {
        return NULL;
    }
    reader += stop;

    if ((buffer_end - reader) < (int32_t)sizeof(struct R_DATA)) {
        return NULL;
    }

    r = (struct R_DATA *)(reader);
    // Read the TYPE, CLASS and TTL
    rescord->type = _cc_swap16(r->type);
    rescord->classes = _cc_swap16(r->classes);
    rescord->ttl = _cc_swap32(r->ttl);
    // Get the length of the Resource Data
    rescord->length = _cc_swap16(r->length);

    reader += sizeof(struct R_DATA);

    switch (rescord->type) {
    case _CC_DNS_T_CNAME_: {
        rescord->rdata = dns_read_name(reader, buffer, buffer_end, &stop);
        if (rescord->rdata == NULL) {
            return NULL;
        }

        if ((buffer_end - reader) < stop) {
            return NULL;
        }
        reader += stop;
    } break;
    default: {
        if ((buffer_end - reader) < rescord->length) {
            return NULL;
        }

        rescord->rdata = (uint8_t *)_cc_malloc(rescord->length);
        memcpy(rescord->rdata, (reader), rescord->length);
        reader += rescord->length;
    } break;
    }

    return reader;
}
/*
 *
 * */
_CC_API_PRIVATE(uint8_t*) dns_read_name(uint8_t *reader, uint8_t *buffer, uint8_t *buffer_end, int *count) {
    int offset;
    bool_t jumped = false;
    char_t name[256];
    int name_length = _cc_countof(name);
    int max_steps;

    int i = 0;

    if (reader == NULL || buffer == NULL || buffer_end == NULL || count == NULL) {
        return NULL;
    }

    max_steps = (int)(buffer_end - buffer);
    if (max_steps <= 0) {
        return NULL;
    }

    *count = 1;

    // read the names in 3www6google3com format
    while (reader < buffer_end && *reader != 0 && max_steps-- > 0) {
        if (*reader >= 192) {
            if ((reader + 1) >= buffer_end) {
                return NULL;
            }
            offset = (*reader) * 256 + *(reader + 1) - 49152; // 49152 = 11000000 00000000 ;)
            if (offset <= 0 || (buffer + offset) >= buffer_end) {
                return NULL;
            }
            reader = buffer + offset - 1;
            // we have jumped to another location so counting wont go up!
            jumped = true;
        } else if (i < name_length) {
            name[i++] = *reader;
        }

        reader++;

        if (!jumped) {
            // if we havent jumped to another location then we can count up
            *count += 1;
        }
    }

    if (reader >= buffer_end || max_steps <= 0) {
        return NULL;
    }

    // string complete
    if (name_length > i) {
        name[i] = 0;
        name_length = i;
    }

    if (jumped) {
        // number of steps we actually moved forward in the packet
        *count += 1;
    }

    // now convert 3www6google3com0 to www.google.com
    name_length = _domain(name, name_length);

    return (uint8_t *)_cc_sds_alloc((const char *)name, name_length);
}

/*
 * Get the DNS servers from /etc/resolv.conf file on Linux/unix
 * */
void _cc_dns_servers(const tchar_t *servers[], int count) {
#ifndef __CC_WINDOWS__
    FILE *fp;
    char line[200], *p;

    if ((fp = fopen("/etc/resolv.conf", "r"))) {
        while (fgets(line, 200, fp)) {
            if (line[0] == '#') {
                continue;
            }
            if (strncmp(line, "nameserver", 10) == 0) {
                p = strtok(line, " ");
                p = strtok(NULL, " ");
                // p now is the dns ip :)
                if (_cc_inet_pton(AF_INET, p, (byte_t *)&dns_servers[dns_server_count])) {
                    // printf("nameserver:%s\n", p);
                    if ((++dns_server_count) >= DNS_SERVERS_MCOUNT) {
                        break;
                    }
                }
            }
        }
        fclose(fp);
    } else {
        printf("Failed opening /etc/resolv.conf file \n");
    }
#endif

    if (count > 0 && servers) {
        int i;
        count = count > DNS_SERVERS_MCOUNT ? DNS_SERVERS_MCOUNT : count;

        for (i = 0; i < count; ++i) {
            if (_cc_inet_pton(AF_INET, servers[i], (byte_t *)&dns_servers[dns_server_count])) {
                // printf("nameserver:%s\n", servers[i]);
                dns_server_count++;
            }
        }
    }
}

#ifdef _TESTES_
int main(int argc, char *argv[]) {
    tchar_t *domain;
    tchar_t hostname[100];
    tchar_t *dns_servers_list[2] = {"114.114.114.114", "223.5.5.5"};

    _cc_dns_t dns;
    memset(&dns, 0, sizeof(_cc_dns_t));
    _cc_install_socket();

    /*Get the DNS servers from the resolv.conf file*/
    _cc_dns_servers((const tchar_t **)dns_servers_list, _cc_countof(dns_servers_list));

    if (argc <= 1) {
        // Get the hostname from the terminal
        printf("Enter Hostname to Lookup : ");
        scanf("%s", hostname);
        domain = hostname;
        _cc_dns_lookup(&dns, hostname, _CC_DNS_T_A_);
    } else {
        _cc_dns_lookup(&dns, argv[0], _CC_DNS_T_A_);
    }

    /*Now get the ip of this hostname , A record*/
    //_cc_dns_lookup(&dns, "www.baidu.com", _CC_DNS_T_A_);

    _cc_dns_free(&dns);

    _cc_uninstall_socket();
    return 0;
}

#endif
