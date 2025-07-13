#include <libcc/widgets/ip_locator.h>
#include <libcc/logger.h>
#include <stdio.h>

#define REDIRECT_MODE1 0x01
#define REDIRECT_MODE2 0x02

/**
 *  convert 1-4 bytes to an integer, little-endian.
 */
_CC_API_PRIVATE(uint32_t) bytes2integer(const byte_t* ip, int count) {
    int i;
    uint32_t res;

    if (count < 1 || count > 4) {
        return 0;
    }

    res = ip[0];
    for (i = 0; i < count; i++) {
        res |= ((uint32_t)ip[i]) << (8 * i);
    }

    return res;
}

/**
 *  read 'len' of bytes to 'buf', from 'fp' at 'offset'
 */
_CC_API_PRIVATE(int32_t) read_from_file(FILE* fp,
                                         uint32_t offset,
                                         byte_t* buf,
                                         int len) {
    int32_t rlen = 0;
    if (fseek(fp, offset, SEEK_SET)) {
        _cc_logger_error(_T("fseek() error in read_from_file()..."));
        memset(buf, 0, len);
        return 0;
    }

    rlen = (int32_t)fread(buf, sizeof(byte_t), len, fp);
    if (rlen != len) {
        _cc_logger_error(_T("Fail read %d bytes in read_from_file()..."), len);
        memset(buf, 0, len);
        return 0;
    }

    rlen = 0;
    while (buf[rlen]) {
        rlen++;
    }
    return rlen;
}

_CC_API_PRIVATE(int32_t) get_area_addr(FILE* fp,
                                        uint32_t offset,
                                        byte_t* result,
                                        int32_t len) {
    int32_t result_len;
    byte_t buf[4];
    uint32_t p = 0;

    read_from_file(fp, offset, buf, 4);

    if (REDIRECT_MODE1 == buf[0] || REDIRECT_MODE2 == buf[0]) {
        p = bytes2integer(buf + 1, 3);
        if (p) {
            result_len = read_from_file(fp, p, result, len);
        } else {
            *result = 0;
            result_len = 0;
        }
    } else {
        result_len = read_from_file(fp, offset, result, len);
    }

    return result_len;
}

_CC_API_PRIVATE(int32_t) get_addr(FILE* fp,
                                   uint32_t offset,
                                   byte_t* addr,
                                   uint32_t len) {
    byte_t buf[4];
    uint32_t country_offset;
    byte_t country_addr[256];
    byte_t area_addr[256];
    int32_t country_addr_len = 0;
    int32_t area_addr_len = 0;

    read_from_file(fp, offset + 4, buf, 4);

    if (REDIRECT_MODE1 == buf[0]) {
        country_offset = bytes2integer(buf + 1, 3);
        read_from_file(fp, country_offset, buf, 4);

        if (REDIRECT_MODE2 == buf[0]) {
            country_addr_len =
                read_from_file(fp, bytes2integer(buf + 1, 3), country_addr,
                               _cc_countof(country_addr));
            area_addr_len = get_area_addr(fp, country_offset + 4, area_addr,
                                          _cc_countof(area_addr));
        } else {
            country_addr_len = read_from_file(fp, country_offset, country_addr,
                                              _cc_countof(country_addr));
            area_addr_len =
                get_area_addr(fp, country_offset + country_addr_len + 1,
                              area_addr, _cc_countof(area_addr));
        }
    } else if (REDIRECT_MODE2 == buf[0]) {
        read_from_file(fp, offset + 4 + 1, buf, 3);

        country_offset = bytes2integer(buf, 3);

        country_addr_len = read_from_file(fp, country_offset, country_addr,
                                          _cc_countof(country_addr));
        area_addr_len =
            get_area_addr(fp, offset + 8, area_addr, _cc_countof(area_addr));
    } else {
        country_addr_len = read_from_file(fp, offset + 4, country_addr,
                                          _cc_countof(country_addr));
        area_addr_len = get_area_addr(fp, offset + 4 + country_addr_len + 1,
                                      area_addr, _cc_countof(area_addr));
    }

    if (len > (uint32_t)(country_addr_len + area_addr_len + 1)) {
        memcpy(addr, country_addr, country_addr_len);
        memcpy(addr + country_addr_len, " ", 1);
        memcpy(addr + country_addr_len + 1, area_addr, area_addr_len);
        *(addr + country_addr_len + area_addr_len + 1) = 0;
        return country_addr_len + area_addr_len + 1;
    }
    return 0;
}

_CC_API_PRIVATE(void) set_ip_range(int rec_no, _cc_ip_locator_t* f) {
    byte_t buf[7];
    uint32_t offset = 0;

    if (f == nullptr) {
        return;
    }

    offset = f->first_index + rec_no * 7;

    read_from_file(f->fp, offset, buf, 7);

    f->cur_start_ip = bytes2integer(buf, 4);
    f->cur_end_ip_offset = bytes2integer(buf + 4, 3);

    read_from_file(f->fp, f->cur_end_ip_offset, buf, 4);

    f->cur_end_ip = bytes2integer(buf, 4);
}

/**
 *  get IP location, return 0 if error
 */
_CC_API_PRIVATE(int32_t) _ip_locator_get_ip_addr(_cc_ip_locator_t* f,
                                                  uint32_t ip,
                                                  byte_t* addr,
                                                  int32_t len) {
    uint32_t M, L, R;

    /* search for right range */
    L = 0;
    R = f->record_count - 1;
    while (L < R - 1) {
        M = (L + R) / 2;
        set_ip_range(M, f);

        if (ip == f->cur_start_ip) {
            L = M;
            break;
        }

        if (ip > f->cur_start_ip) {
            L = M;
        } else {
            R = M;
        }
    }

    set_ip_range(L, f);

    /* version infomation, the last item */
    if ((ip & 0xffffff00) == 0xffffff00) {
        set_ip_range(R, f);
    }

    *addr = '\0';
    if (f->cur_start_ip <= ip && ip <= f->cur_end_ip) {
        return get_addr(f->fp, f->cur_end_ip_offset, addr, len);
    }

    return 0;
}

_CC_API_PRIVATE(int32_t) _ip_locator_get_version(_cc_ip_locator_t* f,
                                                  byte_t* version,
                                                  int32_t len) {
    /* the last item is the version information. */
    return _ip_locator_get_ip_addr(f, 0xffffff00, version, len);
}

_CC_API_PRIVATE(void) _ip_locator_quit(_cc_ip_locator_t* f) {
    if (f && f->fp) {
        fclose(f->fp);
    }
}

bool_t _cc_init_ip_locator(_cc_ip_locator_t* f, const char_t* path) {
    byte_t buf[4];

    if ((f->fp = fopen(path, "rb")) == nullptr) {
        _cc_logger_error(_T("Unable to open file: %s"), path);
        return false;
    }

    /* read the file header, just 8 bytes */
    read_from_file(f->fp, 0, buf, 4);
    f->first_index = bytes2integer(buf, 4);

    read_from_file(f->fp, 4, buf, 4);
    f->last_index = bytes2integer(buf, 4);

    f->record_count = (f->last_index - f->first_index) / 7 + 1;
    f->query = _ip_locator_get_ip_addr;
    f->get_version = _ip_locator_get_version;
    f->quit = _ip_locator_quit;
    return true;
}
