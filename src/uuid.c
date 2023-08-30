/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
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
#include <cc/string.h>
#include <cc/time.h>
#include <cc/uuid.h>
#include <cc/endian.h>
#include <sys/types.h>

static const tchar_t *fmt_lower = _T("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x");

static const tchar_t *fmt_upper = _T("%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X");

#ifdef __CC_WINDOWS__
#include <objbase.h>
/**/
void _cc_uuid(_cc_uuid_t *uuid) {
    CoCreateGuid((GUID *)uuid);
}

/**/
int32_t _cc_uuid_lower(_cc_uuid_t *uuid, tchar_t *buf, int32_t length) {
    GUID *guid;
    guid = (GUID *)uuid;

    return _sntprintf(buf, length, fmt_lower, guid->Data1, guid->Data2, guid->Data3, guid->Data4[0], guid->Data4[1],
                      guid->Data4[2], guid->Data4[3], guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

/**/
int32_t _cc_uuid_upper(_cc_uuid_t *uuid, tchar_t *buf, int32_t length) {
    GUID *guid;
    guid = (GUID *)uuid;

    return _sntprintf(buf, length, fmt_upper, guid->Data1, guid->Data2, guid->Data3, guid->Data4[0], guid->Data4[1],
                      guid->Data4[2], guid->Data4[3], guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

#else
/**/
struct uuid {
    uint32_t time_low;
    uint16_t time_mid;
    uint16_t time_hi_and_version;
    uint16_t clock_seq;
    uint8_t node[6];
};

/* Assume that the gettimeofday() has microsecond granularity */
#define MAX_ADJUSTMENT 10
/*
 * Offset between 15-Oct-1582 and 1-Jan-70
 */
#define TIME_OFFSET_HIGH 0x01B21DD2
#define TIME_OFFSET_LOW 0x13814000

static void random_get_bytes(void *buf, size_t nbytes) {
    size_t i;
    byte_t *cp = (byte_t *)buf;
    struct timeval tv;
    unsigned short uuid_rand_seed[3];

    gettimeofday(&tv, 0);
    srand((uint32_t)((_cc_getpid() << 16) ^ getuid() ^ tv.tv_sec ^ tv.tv_usec));

    uuid_rand_seed[0] = getpid() ^ (tv.tv_sec & 0xFFFF);
    uuid_rand_seed[1] = getppid() ^ (tv.tv_usec & 0xFFFF);
    uuid_rand_seed[2] = (tv.tv_sec ^ tv.tv_usec) >> 16;

    /* Crank the random number generator a few times */
    gettimeofday(&tv, 0);
    for (i = (tv.tv_sec ^ tv.tv_usec) & 0x1F; i > 0; i--) {
        rand();
    }

    for (i = 0; i < nbytes; i++) {
        *cp++ ^= (rand() >> 7) & 0xFF;
    }

    for (cp = buf, i = 0; i < nbytes; i++) {
        *cp++ ^= (jrand48(uuid_rand_seed) >> 7) & 0xFF;
    }
}

static int32_t _uuid_hex(const struct uuid *u, tchar_t *out, int32_t length, const tchar_t *fmt) {
    uint32_t time_low = _CC_SWAPBE32(u->time_low);
    uint16_t clock_seq = _CC_SWAPBE16(u->clock_seq);
    uint16_t time_mid = _CC_SWAPBE16(u->time_mid);
    uint16_t time_hi_and_version = _CC_SWAPBE16(u->time_hi_and_version);
    return (int32_t)_sntprintf(out, length, fmt, time_low, time_mid, time_hi_and_version,
                               clock_seq >> 8, clock_seq & 0xFF, u->node[0], u->node[1], u->node[2],
                               u->node[3], u->node[4], u->node[5]);
}
/**/
void _cc_uuid(_cc_uuid_t *u) {
    static struct timeval last = {0, 0};
    static uint16_t clock_seq = 0;
    static int32_t adjustment = 0;
    uint64_t clock_reg;
    uint32_t clock_mid;

    struct timeval tv;
    struct uuid *uu = (struct uuid *)u;

try_again:
    gettimeofday(&tv, 0);

    if ((tv.tv_sec < last.tv_sec) || ((tv.tv_sec == last.tv_sec) && (tv.tv_usec < last.tv_usec))) {
        clock_seq = (clock_seq + 1) & 0x3FFF;
        adjustment = 0;
        last = tv;
    } else if ((tv.tv_sec == last.tv_sec) && (tv.tv_usec == last.tv_usec)) {
        if (adjustment >= MAX_ADJUSTMENT) {
            goto try_again;
        }
        adjustment++;
    } else {
        adjustment = 0;
        last = tv;
    }

    clock_reg = tv.tv_usec * 10 + adjustment;
    clock_reg += ((uint64_t)tv.tv_sec) * 10000000;
    clock_reg += (((uint64_t)TIME_OFFSET_HIGH) << 32) + TIME_OFFSET_LOW;

    clock_mid = clock_reg >> 32;
    uu->time_low = (uint32_t)clock_reg;
    uu->clock_seq = ((uint16_t)clock_reg) | 0x8000;
    uu->time_mid = (uint16_t)clock_mid;
    uu->time_hi_and_version = ((clock_mid >> 16) & 0x0FFF) | 0x1000;

    random_get_bytes(uu->node, sizeof(uu->node));
}

/**/
int32_t _cc_uuid_lower(_cc_uuid_t *u, tchar_t *buf, int32_t length) {
    return _uuid_hex((struct uuid*)u, buf, length, fmt_lower);
}

/**/
int32_t _cc_uuid_upper(_cc_uuid_t *u, tchar_t *buf, int32_t length) {
    return _uuid_hex((struct uuid*)u, buf, length, fmt_upper);
}
#endif

#define __CC_TO_BYTE(CH, XX, OP)                                                                                       \
    do {                                                                                                               \
        if (XX <= _T('9')) {                                                                                           \
            CH OP (XX & 0x0F);                                                                                         \
        } else {                                                                                                       \
            CH OP ((XX & 0x0F) + 0x09);                                                                                \
        }                                                                                                              \
    } while (0)

void _cc_uuid_to_bytes(_cc_uuid_t *u, const tchar_t *buf) {
    byte_t ch = 0;
    byte_t *uu = (byte_t*)u;
    size_t i = 0, k = 0;

    if (_cc_unlikely(!buf)) {
        return;
    }

    while (k < 16 && buf[i]) {
        if (buf[i] == '-') {
            i++;
            continue;
        }

        __CC_TO_BYTE(ch, buf[i], =);
        ch <<= 4;
        __CC_TO_BYTE(ch, buf[i + 1], +=);
        uu[k++] = ch;

        i += 2;
    }
}