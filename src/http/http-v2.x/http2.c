#include "libcc/http2.h"

/* Build a frame header and write it */
_CC_API_PUBLIC(int32_t) _cc_http2_frame_header(byte_t *hdr, uint8_t type, uint8_t flags, uint32_t stream_id, uint32_t payload) {
    hdr[0] = (payload >> 16) & 0xFF;
    hdr[1] = (payload >> 8) & 0xFF;
    hdr[2] = payload & 0xFF;

    hdr[3] = type;
    hdr[4] = flags;

    hdr[5] = (stream_id >> 24) & 0x7F; /* top bit must be zero */
    hdr[6] = (stream_id >> 16) & 0xFF;
    hdr[7] = (stream_id >> 8) & 0xFF;
    hdr[8] = stream_id & 0xFF;

    return _CC_HTTP2_FRAME_HEADER_SIZE_;
}