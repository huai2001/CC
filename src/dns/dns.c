#include <libcc/alloc.h>
#include <libcc/thread.h>
#include <libcc/event.h>
#include "dns.h"

_CC_API_PRIVATE(void) dns_cleanup_records(_cc_dns_record_t *records, uint16_t count) {
    uint16_t i;
    if (records == NULL) {
        return;
    }
    for (i = 0; i < count; i++) {
        _cc_dns_record_t *r = records + i;
        _cc_free(r->name);
        r->name = NULL;
        _cc_if_free(r->rdata);
    }
    _cc_free(records);
}

_CC_API_PRIVATE(bool_t) dns_skip_name(uint8_t *buffer, uint8_t *buffer_end, size_t *offset) {
    if (buffer == NULL || buffer_end == NULL || offset == NULL) {
        return false;
    }

    size_t pos = *offset;
    while (pos < (size_t)(buffer_end - buffer)) {
        uint8_t len = buffer[pos];
        if (len == 0) {
            *offset = pos + 1;
            return true;
        }

        if ((len & 0xC0) == 0xC0) {
            if (pos + 1 >= (size_t)(buffer_end - buffer)) {
                return false;
            }
            *offset = pos + 2;
            return true;
        }

        pos += (size_t)len + 1;
        if (pos > (size_t)(buffer_end - buffer)) {
            return false;
        }
    }

    return false;
}

_CC_API_PRIVATE(int) dns_build_question(uint8_t *buf, size_t length, const char_t *host, int type) {
    int offset;
    int label_length = 0;
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
    end = (char_t *)buf + length;

    while (*host) {
        if (p >= end) {
            return -1;
        }

        if (*host == '.') {
            if (label_length <= 0 || label_length > 63) {
                return -1;
            }
            *dot = (p - dot) - 1;
            dot = p++;
            label_length = 0;
            host++;
            continue;
        }
        *p++ = *host++;
        label_length++;
    }

    if (label_length <= 0 || label_length > 63 || p >= end) {
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

_CC_API_PRIVATE(void) dump(const _cc_dns_record_t *records, uint16_t count) {
    int16_t i;
    for ( i = 0; i < count; i++) {
        const _cc_dns_record_t *r = records + i;
        _tprintf(_T("Name : %s TTL:%d "), r->name, r->ttl);
        dump_type(r->rdata, r->type);
    }
}

/*
 *
 * */
_CC_API_PRIVATE(uint8_t*) dns_read_name(uint8_t *reader, uint8_t *buffer, uint8_t *buffer_end, int *count, uint16_t *length) {
    bool_t jumped = false;
    char_t name[256];

    int name_index = 0;
    int max_steps;
    uint8_t *ptr;

    if (reader == NULL || buffer == NULL || buffer_end == NULL || count == NULL) {
        return NULL;
    }

    if (reader >= buffer_end) {
        return NULL;
    }

    if (length) {
        *length = 0;
    }
    *count = 0;
    ptr = reader;
    max_steps = (int)(buffer_end - buffer);

    while (ptr < buffer_end && max_steps-- > 0) {
        uint8_t len = *ptr;

        if ((len & 0xC0) == 0xC0) {
            uint16_t offset;
            if (ptr + 1 >= buffer_end) {
                return NULL;
            }

            offset = (uint16_t)(((len & 0x3F) << 8) | *(ptr + 1));
            if (offset >= (uint16_t)(buffer_end - buffer)) {
                return NULL;
            }

            if (!jumped) {
                *count += 2;
            }
            ptr = buffer + offset;
            jumped = true;
            continue;
        }

        if (len == 0) {
            if (!jumped) {
                *count += 1;
            }
            break;
        }

        if (ptr + 1 + len > buffer_end) {
            return NULL;
        }

        if (name_index + len + 1 >= _cc_countof(name)) {
            return NULL;
        }

        memcpy(name + name_index, ptr + 1, len);
        name_index += len;
        name[name_index++] = '.';

        if (!jumped) {
            *count += 1 + len;
        }

        ptr += 1 + len;
    }

    if (ptr >= buffer_end) {
        return NULL;
    }

    if (name_index == 0) {
        name[0] = 0;
        return NULL;
    }

    name[name_index - 1] = 0;
    if (length) {
        *length = (uint16_t)(name_index - 1);
    }

    ptr = (uint8_t*)_cc_malloc(name_index);
    memcpy(ptr, name, (size_t)(name_index - 1));
    ptr[name_index - 1] = 0;
    return ptr;
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

    rescord->name = (char_t *)dns_read_name(reader, buffer, buffer_end, &stop, &rescord->name_length);
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
        rescord->rdata = dns_read_name(reader, buffer, buffer_end, &stop, &rescord->length);
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
 * Get the DNS servers from /etc/resolv.conf file on Linux/unix
 * */
// List of DNS Servers registered on the system
static struct in_addr dns_servers[DNS_SERVERS_MCOUNT];
static int dns_server_count = 0;

/* Answer modifier callback storage */
static bool_t (*g_answer_modifier)( _cc_dns_record_t *rec, void *ctx ) = NULL;
static void *g_answer_modifier_ctx = NULL;

_CC_API_PUBLIC(void) _cc_dns_set_answer_modifier(bool_t (*fn)(_cc_dns_record_t *rec, void *ctx), void *ctx) {
    g_answer_modifier = fn;
    g_answer_modifier_ctx = ctx;
}

/* write a domain name in label format (no compression) */
static int dns_write_name(uint8_t *out, size_t outlen, const char *name) {
    if (!out || !name) return -1;
    size_t pos = 0;
    const char *start = name;
    const char *p = name;
    while (1) {
        if (*p == '.' || *p == '\0') {
            size_t labellen = (size_t)(p - start);
            if (labellen > 63) return -1;
            if (pos + 1 + labellen > outlen) return -1;
            out[pos++] = (uint8_t)labellen;
            if (labellen) {
                memcpy(out + pos, start, labellen);
                pos += labellen;
            }
            if (*p == '\0') {
                if (pos + 1 > outlen) return -1;
                out[pos++] = 0;
                break;
            }
            p++;
            start = p;
            continue;
        }
        p++;
    }
    return (int)pos;
}

/* Rebuild a DNS response containing header, original question(s) and provided answers.
   This implementation writes names without compression. Returns length on success, -1 on error. */
_CC_API_PUBLIC(int) _cc_dns_rebuild_response(uint8_t *outbuf, size_t outlen, uint8_t *orig_resp, size_t orig_len, _cc_dns_record_t *answers, uint16_t answer_count, uint16_t author_count, uint16_t add_count, size_t question_offset) {
    if (!outbuf || !orig_resp || orig_len < sizeof(_cc_dns_header_t)) return -1;
    if (question_offset > orig_len) return -1;

    /* copy header */
    if (outlen < sizeof(_cc_dns_header_t)) return -1;
    memcpy(outbuf, orig_resp, sizeof(_cc_dns_header_t));
    _cc_dns_header_t *oh = (_cc_dns_header_t *)outbuf;

    /* set counts */
    oh->answer = htons(answer_count);
    oh->author = htons(author_count);
    oh->addition = htons(add_count);

    size_t pos = sizeof(_cc_dns_header_t);

    /* copy questions from original packet (header..question_offset) */
    size_t qlen = question_offset - sizeof(_cc_dns_header_t);
    if (question_offset < sizeof(_cc_dns_header_t)) return -1;
    if (pos + qlen > outlen) return -1;
    memcpy(outbuf + pos, orig_resp + sizeof(_cc_dns_header_t), qlen);
    pos += qlen;

    /* now append answers */
    for (uint16_t i = 0; i < answer_count; i++) {
        _cc_dns_record_t *r = answers + i;
        /* write name */
        int wrote = dns_write_name(outbuf + pos, outlen - pos, r->name ? r->name : "");
        if (wrote < 0) return -1;
        pos += (size_t)wrote;

        /* R_DATA struct */
        if (pos + sizeof(struct R_DATA) > outlen) return -1;
        struct R_DATA rd;
        rd.type = htons(r->type);
        rd.classes = htons(r->classes);
        rd.ttl = htonl(r->ttl);
        rd.length = htons(r->length);
        memcpy(outbuf + pos, &rd, sizeof(struct R_DATA));
        pos += sizeof(struct R_DATA);

        /* rdata */
        if (r->type == _CC_DNS_T_CNAME_) {
            /* r->rdata is a textual name; write as labels */
            int wn = dns_write_name(outbuf + pos, outlen - pos, (const char *)r->rdata);
            if (wn < 0) return -1;
            pos += (size_t)wn;
        } else {
            if (pos + r->length > outlen) return -1;
            memcpy(outbuf + pos, r->rdata, r->length);
            pos += r->length;
        }
    }

    return (int)pos;
}

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

int _cc_dns_question(uint8_t *question, size_t length, const char_t *host, int type) {
    int offset;

    _cc_dns_header_t *header;

    offset = sizeof(_cc_dns_header_t);

    // point to the query portion
    offset = dns_build_question(question + offset, length, host, type);
    if (offset < 0) {
        return _CC_DNS_ERR_FORMAT_ERROR_;
    }
    offset += sizeof(_cc_dns_header_t);
    if (offset > (int)length) {
        return _CC_DNS_ERR_QUERY_TOO_LONG_;
    }

    // Set the DNS structure to standard queries
    header = (_cc_dns_header_t *)question;
    memset(header, 0, sizeof(_cc_dns_header_t));
    header->ident = (uint16_t)htons(_cc_getpid());

    // Set standard codes and flags
    header->flags = htons(0x0100); // recursion desired (RD)
    header->quests = htons(1);

    if (offset > 512) {
        printf("Question too big for UDP ('%d' bytes)\n", offset);
        return _CC_DNS_ERR_QUERY_TOO_LONG_;
    }

    return offset;
}

int _cc_dns_response(uint8_t *response, size_t length) {
    _cc_dns_header_t *header;
    _cc_dns_record_t *additions = NULL;
    _cc_dns_record_t *answers = NULL;
    _cc_dns_record_t *authors = NULL;
    uint16_t header_flags;
    size_t offset;
    uint8_t *reader;
    int i;

    if (response == NULL || length < sizeof(_cc_dns_header_t)) {
        return _CC_DNS_ERR_BAD_FORMAT_;
    }

    header = (_cc_dns_header_t *)response;
    header_flags = _cc_swap16(header->flags);
    uint16_t rcode = header_flags & 0x000F;

    if (header_flags & 0x0200) {
        return _CC_DNS_ERR_REPLY_TRUNCATED_;
    }

    if (rcode != DNS_R_NO_ERROR) {
        switch (rcode) {
        case DNS_R_FORMAT_ERROR:
            return _CC_DNS_ERR_BAD_FORMAT_;
        case DNS_R_NAME_ERROR:
            return _CC_DNS_ERR_NO_SUCH_NAME_;
        case DNS_R_SERVER_FAILURE:
            return _CC_DNS_ERR_SERVER_FAILURE_;
        case DNS_R_NOT_IMPLEMENTED:
            return _CC_DNS_ERR_NOT_IMPLEMENTED_;
        case DNS_R_REFUSED:
            return _CC_DNS_ERR_QUERY_REFUSED_;
        default:
            return _CC_DNS_ERR_BAD_FORMAT_;
        }
    }

    printf("Now check the reply code:%d,%x\n", rcode, header_flags);

    uint16_t quests = _cc_swap16(header->quests);
    uint16_t answer = _cc_swap16(header->answer);
    uint16_t author = _cc_swap16(header->author);
    uint16_t addition = _cc_swap16(header->addition);

    printf("\nThe response contains : %ld", length);
    printf("\n %d Questions.", quests);
    printf("\n %d Answers.", answer);
    printf("\n %d Authoritative Servers.", author);
    printf("\n %d Additional records.\n\n", addition);

    // move ahead of the dns header and the query field
    offset = sizeof(_cc_dns_header_t);

    /* So far all is fine. Now parse response into lists of items */
    /* Assume that the question section remains unchanged and start
        * with the answers section instead. */
    for (i = 0; i < (int)quests; i++) {
        if (!dns_skip_name(response, response + length, &offset)) {
            return _CC_DNS_ERR_BAD_FORMAT_;
        }

        if (offset + sizeof(struct QUESTION) > length) {
            return _CC_DNS_ERR_BAD_FORMAT_;
        }
        offset += sizeof(struct QUESTION);
    }

    if (offset > length) {
        return _CC_DNS_ERR_BAD_FORMAT_;
    }

    reader = response + offset;

    if (answer > 0) {
        answers = (_cc_dns_record_t *)_cc_calloc(answer, sizeof(_cc_dns_record_t));
        for (i = 0; i < answer; i++) {
            _cc_dns_record_t *r = answers + i;
            reader = dns_read_rdata(reader, response, response + length, r);
            if (reader == NULL) {
                goto DNS_PARSE_FAILED;
            }
        }
    }
    _tprintf(_T("Answer Records : %u \n"), answer);
    dump(answers, answer);

    /* apply modifier callback if set */
    if (g_answer_modifier) {
        for (i = 0; i < (int)answer; i++) {
            g_answer_modifier(answers + i, g_answer_modifier_ctx);
        }
    }

    if (author > 0) {
        authors = (_cc_dns_record_t *)_cc_calloc(author, sizeof(_cc_dns_record_t));
        for (i = 0; i < author; i++) {
            _cc_dns_record_t *r = authors + i;
            reader = dns_read_rdata(reader, response, response + length, r);
            if (reader == NULL) {
                goto DNS_PARSE_FAILED;
            }
        }
    }
    _tprintf(_T("Authoritative Records : %u \n"), author);
    dump(authors, author);

    if (addition > 0) {
        additions = (_cc_dns_record_t *)_cc_calloc(addition, sizeof(_cc_dns_record_t));
        for (i = 0; i < addition; i++) {
            _cc_dns_record_t *r = additions + i;
            reader = dns_read_rdata(reader, response, response + length, r);
            if (reader == NULL) {
                goto DNS_PARSE_FAILED;
            }
        }
    }
    _tprintf(_T("Additional Records : %u \n"), addition);
    dump(additions, addition);

    /* attempt to rebuild response and overwrite original if fits */
    if (answer > 0) {
        uint8_t *tmp = (_cc_malloc(65536));
        if (tmp) {
            int rebuilt = _cc_dns_rebuild_response(tmp, 65536, response, length, answers, answer, author, addition, offset);
            if (rebuilt > 0 && (size_t)rebuilt <= length) {
                memcpy(response, tmp, (size_t)rebuilt);
            }
            _cc_free(tmp);
        }
    }
    dns_cleanup_records(answers, answer);
    dns_cleanup_records(authors, author);
    dns_cleanup_records(additions, addition);

    return 1;


DNS_PARSE_FAILED:
    dns_cleanup_records(answers, answer);
    dns_cleanup_records(authors, author);
    dns_cleanup_records(additions, addition);
    return _CC_DNS_ERR_BAD_FORMAT_;
}
