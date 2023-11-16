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
#include "http.h"

_CC_API_PRIVATE(void) random_masking(byte_t* buf, int32_t len) {
    int32_t i;
    for (i = 0; i < len; i++) {
        buf[i] = (byte_t)(rand() % 255) + 1;
    }
}

bool_t _cc_http_init_websocket(_cc_http_t* res) {
    int32_t len = 0;
    _cc_sha1_t ctx;
    byte_t results[1024];
    byte_t sha1_results[_CC_SHA1_DIGEST_LENGTH_];

    const tchar_t* websocket_key =
        _cc_find_kv(&res->request.headers, _T("Sec-WebSocket-Key"));
    if (websocket_key) {
        len = _sntprintf((tchar_t*)results,
                         _cc_countof(results) / sizeof(tchar_t), _T("%s%s"),
                         websocket_key,
                         _T("258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));
#ifdef _CC_UNICODE_
        {
            char_t utf8_buf[256];
            bzero(utf8_buf, sizeof(utf8_buf));
            len = _cc_utf16_to_utf8(
                (const uint16_t*)results, (const uint16_t*)(results + len),
                (uint8_t*)utf8_buf,
                (uint8_t*)(utf8_buf + _cc_countof(utf8_buf)), false);
            memcpy(results, utf8_buf, sizeof(results));
        }
#endif
        _cc_sha1_init(&ctx);
        _cc_sha1_update(&ctx, results, len);
        _cc_sha1_final(&ctx, sha1_results);
        _cc_base64_encode(sha1_results, _CC_SHA1_DIGEST_LENGTH_,
                          res->websocket.key, _cc_countof(res->websocket.key));
        res->websocket.status = 1;
        return true;
    }
    return false;
}

_CC_API_PRIVATE(void) inverted(byte_t* dest, byte_t* src, int32_t length) {
    int32_t i;
    for (i = 0; i < length; i++) {
        *(dest + i) = *(src + length - 1 - i);
    }
}

_CC_API_PRIVATE(bool_t) _websocket_decoding(_cc_event_rbuf_t* r,
                                  _websocket_frame_header_t* head) {
    int32_t rr = 2;

    if (r->length < 2) {
        return false;
    }

    /*read fin and op code*/
    head->fin = (r->buf[0] & 0x80) == 0x80;
    head->opcode = r->buf[0] & 0x0F;
    head->mask = (r->buf[1] & 0x80) == 0X80;

    /*get payload length*/
    head->payload_length = r->buf[1] & 0x7F;

    if (head->payload_length == 126) {
        rr += 2;
        if (r->length < rr) {
            return false;
        }
        head->payload_length = (r->buf[2] & 0xFF) << 8 | (r->buf[3] & 0xFF);

    } else if (head->payload_length == 127) {
        rr += 8;
        if (r->length < rr) {
            return false;
        }
        inverted((byte_t*)&(head->payload_length), &r->buf[2], 8);
    }

    /*read masking-key*/
    if (head->mask) {
        rr += 4;

        if (r->length < rr) {
            return false;
        }

        memcpy(head->masking, &r->buf[rr - 4], 4);
    } else if (r->length < rr) {
        return false;
    }

    r->length -= rr;
    if (r->length > 0) {
        memmove(&r->buf[0], &r->buf[rr], r->length);
    }

    return true;
}

void _websocket_umask(byte_t* data, int32_t len, char* mask) {
    int32_t i;
    for (i = 0; i < len; ++i) {
        *(data + i) ^= *(mask + (i & 0x3));
    }
}

int _cc_http_websocket_send(_cc_event_t* e,
                            byte_t opcode,
                            byte_t* data,
                            uint32_t length) {
    byte_t response_head[16];
    uint32_t head_length = 0;
    byte_t mask = 0x00;

    switch (opcode) {
        case WS_MINDATA:
            response_head[0] = 0x00;
            break;
        case WS_TXTDATA:
            response_head[0] = 0x81;
            break;
        case WS_BINDATA:
            response_head[0] = 0x82;
            break;
        case WS_DISCONNECT:
            response_head[0] = 0x88;
            break;
        case WS_PING:
            response_head[0] = 0x89;
            break;
        case WS_PONG:
            response_head[0] = 0x8A;
            break;
        default:
            break;
    }

    if (length < 126) {
        response_head[1] = (length & 0xFF) | mask;
        head_length = 2;
    } else if (length < 0xFFFF) {
        response_head[1] = 0x7E | mask;
        response_head[2] = (length >> 8 & 0xFF);
        response_head[3] = (length >> 0 & 0xFF);
        head_length = 4;
    } else {
        response_head[1] = 0x7F | mask;
        inverted((byte_t*)&response_head[2], (byte_t*)&length, 8);
        head_length = 12;
    }

    if (mask) {
        uint32_t i;
        byte_t masking[4];
        random_masking(masking, 4);
        response_head[head_length++] = masking[0];
        response_head[head_length++] = masking[1];
        response_head[head_length++] = masking[2];
        response_head[head_length++] = masking[3];

        for (i = 0; i < length; i++) {
            *(data + i) ^= masking[i & 0x3];
        }
    }

    if (_cc_event_send(e, response_head, head_length) < 0) {
        return -1;
    }

    if (_cc_event_send(e, data, length) < 0) {
        return -1;
    }
    return 0;
}

int _cc_http_websocket_recv(_cc_event_t* e,
                            bool_t (*callback)(_cc_event_t* e,
                                               int32_t opcode,
                                               byte_t* data,
                                               int32_t len)) {
    int32_t length;

    _cc_http_t* res = (_cc_http_t*)e->args;
    _cc_event_rbuf_t* r = &e->buffer->r;

    if (_websocket_decoding(r, &res->websocket.header) == false) {
        return 0;
    }

    length = (int32_t)res->websocket.header.payload_length;
    /*printf("fin=%d\nopcode=0x%X\nmask=%d\npayload_len=%llu\n",
    res->websocket.header.fin,
    res->websocket.header.opcode,
    res->websocket.header.mask,
    res->websocket.header.payload_length);*/
    switch (res->websocket.header.opcode) {
        case WS_DISCONNECT:
            return -1;
        case WS_PING:
            _cc_http_websocket_send(e, WS_PONG, NULL, 0);
            break;
        case WS_PONG:
            // printf("recv pong frame.\n");
            break;
    }

    if (r->length >= length) {
        _websocket_umask(r->buf, length, res->websocket.header.masking);
        //
        if (callback) {
            callback(e, res->websocket.header.opcode, r->buf, length);
        }

        r->length -= length;
        if (r->length > 0) {
            memmove(r->buf, (r->buf + length), r->length);
            return 1;
        }
    }

    return 0;
}