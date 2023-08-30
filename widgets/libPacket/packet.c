#include "packet.h"
#include <cc/logger.h>
#include <zlib/zlib.h>
#include "quicklz.h"

//////////////////////////////////////////////////////////////////////////////////
static int32_t __decompress(byte_t* input,
                            byte_t* output,
                            int32_t packet_length,
                            _cc_tcp_header_t* header) {
    byte_t* by = NULL;
    byte_t compressed[_CC_PACKET_SIZE_];

#if _COMPRESS_QLZ
    qlz_state_decompress state_decompress;
    int32_t csize = 0;
#else
    uLong ulen = _CC_PACKET_SIZE_;
#endif

    if (_CC_ISSET_BIT(_CC_PACKET_KIND_MAPPED_, header->kind) != 0) {
        if (_cc_decrypt_mapping(compressed, input, packet_length) !=
            header->crc) {
            return -1;
        }
        by = compressed;
    } else {
        by = input;
    }

#if _COMPRESS_QLZ
    /*QLZ head size need 9-byte*/
    if (packet_length < 9)
        return -2;

    csize = (int32_t)qlz_size_compressed((const char*)by);
    if (csize != packet_length) {
        return -2;
    }

    bzero(&state_decompress, sizeof(qlz_state_decompress));
    packet_length =
        (int32_t)qlz_decompress((char*)by, (char*)output, &state_decompress);
    if (packet_length <= 0) {
        return -3;
    }
#else
    if (uncompress(output, &ulen, (byte_t*)by, packet_length) != Z_OK) {
        return -3;
    }
    _CC_SET_BIT(_CC_PACKET_KIND_COMPRESS_, header->kind);
    packet_length = (int32_t)ulen;
#endif

    return packet_length;
}
/**/
int32_t _cc_tcp_decrypt(const byte_t* data,
                        int32_t data_length,
                        _cc_tcp_t* buf) {
    int32_t packet_length = 0;
    _cc_tcp_header_t* header = NULL;

    if (data_length < sizeof(_cc_tcp_header_t)) {
        return 0;
    }

    header = (_cc_tcp_header_t*)data;

    packet_length = header->length;

    if (packet_length < sizeof(_cc_tcp_header_t)) {
        return -1;
    } else if (packet_length > data_length) {
        if (packet_length > _CC_PACKET_BUFFER_) {
            return -2;
        }

        return 0;
    }

    if (_CC_ISSET_BIT(_CC_PACKET_KIND_SOURCE_ | _CC_PACKET_KIND_COMPRESS_ |
                          _CC_PACKET_KIND_MAPPED_,
                      header->kind) == 0) {
        return -3;
    }

    data_length = packet_length;
    packet_length = packet_length - sizeof(_cc_tcp_header_t);
    memcpy(&buf->header, header, sizeof(_cc_tcp_header_t));

    if (header->kind == _CC_PACKET_KIND_SOURCE_) {
        memcpy(buf->buffer, (byte_t*)(data + sizeof(_cc_tcp_header_t)),
               packet_length);
        buf->header.length = packet_length;
        return data_length;
    }

    if (_CC_ISSET_BIT(_CC_PACKET_KIND_COMPRESS_, header->kind) != 0) {
        packet_length = __decompress((byte_t*)(data + sizeof(_cc_tcp_header_t)),
                                     buf->buffer, packet_length, header);
    } else if (_CC_ISSET_BIT(_CC_PACKET_KIND_MAPPED_, header->kind) != 0) {
        if (_cc_decrypt_mapping(buf->buffer,
                                (byte_t*)(data + sizeof(_cc_tcp_header_t)),
                                packet_length) != header->crc) {
            return -4;
        }
    }

    buf->header.length = packet_length;
    /**/
    return data_length;
}

_CC_FORCE_INLINE_ _cc_tcp_header_t* _cc_tcp_encrypt_header(byte_t major,
                                                           byte_t minor,
                                                           _cc_tcp_t* buf) {
    _cc_tcp_header_t* header = NULL;
    /**/
    bzero(buf, sizeof(_cc_tcp_t));
    /**/
    header = &buf->header;
    header->kind = _CC_PACKET_KIND_SOURCE_;
    header->length = sizeof(_cc_tcp_header_t);
    header->crc = 0x00;
    header->major = major;
    header->minor = minor;

    return header;
}

int32_t _cc_tcp_encrypt(byte_t major,
                        byte_t minor,
                        const byte_t* data,
                        int32_t data_length,
                        byte_t kind,
                        _cc_tcp_t* buf) {
    _cc_tcp_header_t* header;
    _cc_assert(buf != NULL);
    if (buf == NULL) {
        return 0;
    }

    header = _cc_tcp_encrypt_header(major, minor, buf);

    if (data_length <= 0 || !data) {
        return sizeof(_cc_tcp_header_t);
    }

    if (kind == _CC_PACKET_KIND_SOURCE_) {
        memcpy(buf->buffer, data, data_length);
        header->length += data_length;
        return header->length;
    }

    if (_CC_ISSET_BIT(_CC_PACKET_KIND_COMPRESS_, kind) != 0) {
#if _COMPRESS_QLZ
        qlz_state_compress state_compress;
        bzero(&state_compress, sizeof(qlz_state_compress));
        data_length = (int32_t)qlz_compress(data, (char*)buf->buffer,
                                            data_length, &state_compress);
        _CC_SET_BIT(_CC_PACKET_KIND_COMPRESS_, header->kind);
#else
        uLong clen = compressBound(data_length);
        if (compress(buf->buffer, &clen, data, data_length) != Z_OK) {
            memcpy(buf->buffer, data, data_length);
        } else {
            _CC_SET_BIT(_CC_PACKET_KIND_COMPRESS_, header->kind);
            data_length = clen;
        }
#endif
        data = buf->buffer;
    }

    header->length += data_length;
    if (_CC_ISSET_BIT(_CC_PACKET_KIND_MAPPED_, kind) != 0) {
        header->crc = _cc_encrypt_mapping(buf->buffer, data, data_length);
        _CC_SET_BIT(_CC_PACKET_KIND_MAPPED_, header->kind);
    }

    return header->length;
}

bool_t _cc_tcp_event_dispatch(_cc_event_t* e,
                              bool_t(fncall)(_cc_event_t*,
                                             byte_t major,
                                             byte_t minor,
                                             const byte_t*,
                                             uint16_t)) {
    _cc_tcp_t buf;
    _cc_event_buffer_t* rw = e->buffer;
    bzero(&buf, sizeof(_cc_tcp_t));

    while (rw->r.length > 0) {
        int32_t data_left = 0;
        int32_t data_length = _cc_tcp_decrypt(rw->r.buf, rw->r.length, &buf);
        if (data_length == 0) {
            break;
        }

        if (data_length < 0) {
            _cc_logger_error("decryptTCPPacket error:buffer.r.length=%d,code:%d",
                             rw->r.length, data_length);
            return false;
        }

        data_left = (rw->r.length - data_length);
        if (data_left > 0) {
            memmove(rw->r.buf, rw->r.buf + data_length, data_left);
            rw->r.length = data_left;
        } else {
            rw->r.length = 0;
        }

        if (fncall(e, buf.header.major, buf.header.minor, buf.buffer,
                   buf.header.length) == false) {
            return false;
        }
    }
    return true;
}
