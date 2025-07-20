/*
 * Copyright .Qiu<huai2011@163.com>. and other libcc contributors.
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
#ifndef _C_CC_WSHEADER_H_INCLUDED_
#define _C_CC_WSHEADER_H_INCLUDED_

#include "dylib.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/*
+---------------+---------------+---------------+---------------+
         0               1               2               3
 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 
+-+-+-+-+-------+-+-------------+---------------+---------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               |Masking-key, if MASK set to 1  |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+
FIN (Bit 1): Indicates whether it is the last frame of the message (1 = end frame, 0 = there are subsequent frames)
RSV1-3 (1 bit): Reserved, must be 0
Opcode _WS_OPCODE_
-------------------------------------------------
-------------------*/

#define _WS_MASK_SIZE_          4
#define _WS_MAX_HEADER_         14

enum _WS_STATUS_ {
    WS_DATA_OK = 0,
    WS_DATA_PARTIAL,
    WS_DATA_ERROR
};

enum _WS_OPCODE_ {
    WS_OP_MIN          = 0x00,     // continuation
    WS_OP_TXT          = 0x01,     // text
    WS_OP_BINARY       = 0x02,     // binary
    WS_OP_JSON         = 0x03,     // JSON
    WS_OP_XML          = 0x04,     // XML
    //WS_OP_?          = 0x05,     //
    //WS_OP_?          = 0x06,     //
    //WS_OP_?          = 0x07,     //
    WS_OP_DISCONNECT   = 0x08,     // disconnected
    WS_OP_PING         = 0x09,     // ping
    WS_OP_PONG         = 0x0A,     // pong
    //WS_OP_?          = 0x0B,     //
    //WS_OP_?          = 0x0C,     //
    //WS_OP_?          = 0x0D,     //
    //WS_OP_?          = 0x0E,     //
    //WS_OP_?          = 0x0F,     //
    //.....................
    //RSV1-3 Unless the extension agreement is activated
    //WS_OP_?          = 0x7F      //0x10 ... 0x7F

};

typedef struct _WSHeader {
    byte_t fin;
    byte_t rsv;
    byte_t opc;
    int64_t offset;
    int64_t length;
    int64_t payload;
    byte_t *mask;
    byte_t *bytes;
} _WSHeader_t;

/**/
_CC_WIDGETS_API(void) _WSMask(byte_t *data, int64_t length, byte_t *mask);
/**/
_CC_WIDGETS_API(int32_t) _WSHeader(byte_t *header, byte_t operation, int64_t length, byte_t *mask);
/**/
_CC_WIDGETS_API(int32_t) _WSReverseHeader(byte_t *header, byte_t operation, int64_t length, byte_t *mask);
/**/
_CC_WIDGETS_API(int) _WSRead(_WSHeader_t *header);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*_C_CC_WSHEADER_H_INCLUDED_*/
