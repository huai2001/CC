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
#ifndef _C_CCNET_PACKET_H_INCLUDED_
#define _C_CCNET_PACKET_H_INCLUDED_

#include <cc/event/event.h>
#include <cc/types.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define _COMPRESS_QLZ 0
#define _COMPRESS_ZLIB 0

//////////////////////////////////////////////////////////////////////////////////
#define _CC_PACKET_KIND_SOURCE_ 0x01
#define _CC_PACKET_KIND_MAPPED_ 0x02
#define _CC_PACKET_KIND_ENCRYPT_ 0x04
#define _CC_PACKET_KIND_COMPRESS_ 0x08

#define _CC_PACKET_BUFFER_ 5120

#if _COMPRESS_QLZ
#define _CC_PACKET_SIZE_ (_CC_PACKET_BUFFER_ - sizeof(_cc_tcp_header_t) - 9)
#else
#define _CC_PACKET_SIZE_ (_CC_PACKET_BUFFER_ - sizeof(_cc_tcp_header_t))
#endif
//////////////////////////////////////////////////////////////////////////////////
// TCP

#pragma pack(1)

typedef struct _cc_tcp_header {
    byte_t kind;
    byte_t crc;
    uint16_t length;
    byte_t major;
    byte_t minor;
} _cc_tcp_header_t;

typedef struct _cc_tcp {
    _cc_tcp_header_t header;
    byte_t buffer[_CC_PACKET_SIZE_];
} _cc_tcp_t;

#pragma pack()
//////////////////////////////////////////////////////////////////////////////////

/**/
byte_t _cc_encrypt_mapping(byte_t* encrypt,
                           const byte_t* source,
                           int32_t source_size);
/**/
byte_t _cc_decrypt_mapping(byte_t* source,
                           const byte_t* encrypt,
                           int32_t encrypt_size);

//////////////////////////////////////////////////////////////////////////////////
/**/
int32_t _cc_tcp_encrypt(byte_t major,
                        byte_t minor,
                        const byte_t* data,
                        int32_t data_length,
                        byte_t kind,
                        _cc_tcp_t* buffer);
/**/
int32_t _cc_tcp_decrypt(const byte_t* data,
                        int32_t data_length,
                        _cc_tcp_t* buffer);

bool_t _cc_tcp_event_dispatch(_cc_event_t* e,
                              bool_t(fn)(_cc_event_t*,
                                         byte_t major,
                                         byte_t minor,
                                         const byte_t*,
                                         uint16_t));

//////////////////////////////////////////////////////////////////////////////////

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _C_CCNET_PACKET_H_INCLUDED_ */
