#ifndef _C_CC_HTTP2_H_INCLUDED_
#define _C_CC_HTTP2_H_INCLUDED_

#include "sds.h"
#include "rbtree.h"

/* frame types */
enum {
    _CC_HTTP2_FRAME_TYPE_DATA_           = 0x00,
    _CC_HTTP2_FRAME_TYPE_HEADERS_        = 0x01,
    _CC_HTTP2_FRAME_TYPE_PRIORITY_       = 0x02,
    _CC_HTTP2_FRAME_TYPE_RST_STREAM_     = 0x03,
    _CC_HTTP2_FRAME_TYPE_SETTINGS_       = 0x04,
    _CC_HTTP2_FRAME_TYPE_PUSH_PROMISE_   = 0x05,
    _CC_HTTP2_FRAME_TYPE_PING_           = 0x06,
    _CC_HTTP2_FRAME_TYPE_GOAWAY_         = 0x07,
    _CC_HTTP2_FRAME_TYPE_WINDOW_UPDATE_  = 0x08,
    _CC_HTTP2_FRAME_TYPE_CONTINUATION_   = 0x09,
    _CC_HTTP2_FRAME_TYPE_ALTSVC_         = 0x0a
};

/* settings identifiers */
enum {
    _CC_HTTP2_SETTINGS_HEADER_TABLE_SIZE_       = 0x01,
    _CC_HTTP2_SETTINGS_ENABLE_PUSH_             = 0x02,
    _CC_HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS_  = 0x03,
    _CC_HTTP2_SETTINGS_INITIAL_WINDOW_SIZE_     = 0x04,
    _CC_HTTP2_SETTINGS_MAX_FRAME_SIZE_          = 0x05,
    _CC_HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE_    = 0x06,
};

/* frame flags */
enum {
    _CC_HTTP2_FRAME_FLAG_NO_             = 0x00,
    _CC_HTTP2_FRAME_FLAG_ACK_            = 0x01,
    _CC_HTTP2_FRAME_FLAG_END_STREAM_     = 0x01,
    _CC_HTTP2_FRAME_FLAG_END_HEADERS_    = 0x04,
    _CC_HTTP2_FRAME_FLAG_PADDED_         = 0x08,
    _CC_HTTP2_FRAME_FLAG_PRIORITY_       = 0x20,
};
/*
+-------+-----------------------------+---------------+
| Index | Header Name                 | Header Value  |
+-------+-----------------------------+---------------+
| 1     | :authority                  |               |
| 2     | :method                     | GET           |
| 3     | :method                     | POST          |
| 4     | :path                       | /             |
| 5     | :path                       | /index.html   |
| 6     | :scheme                     | http          |
| 7     | :scheme                     | https         |
| 8     | :status                     | 200           |
| 9     | :status                     | 204           |
| 10    | :status                     | 206           |
| 11    | :status                     | 304           |
| 12    | :status                     | 400           |
| 13    | :status                     | 404           |
| 14    | :status                     | 500           |
| 15    | accept-charset              |               |
| 16    | accept-encoding             | gzip, deflate |
| 17    | accept-language             |               |
| 18    | accept-ranges               |               |
| 19    | accept                      |               |
| 20    | access-control-allow-origin |               |
| 21    | age                         |               |
| 22    | allow                       |               |
| 23    | authorization               |               |
| 24    | cache-control               |               |
| 25    | content-disposition         |               |
| 26    | content-encoding            |               |
| 27    | content-language            |               |
| 28    | content-length              |               |
| 29    | content-location            |               |
| 30    | content-range               |               |
| 31    | content-type                |               |
| 32    | cookie                      |               |
| 33    | date                        |               |
| 34    | etag                        |               |
| 35    | expect                      |               |
| 36    | expires                     |               |
| 37    | from                        |               |
| 38    | host                        |               |
| 39    | if-match                    |               |
| 40    | if-modified-since           |               |
| 41    | if-none-match               |               |
| 42    | if-range                    |               |
| 43    | if-unmodified-since         |               |
| 44    | last-modified               |               |
| 45    | link                        |               |
| 46    | location                    |               |
| 47    | max-forwards                |               |
| 48    | proxy-authenticate          |               |
| 49    | proxy-authorization         |               |
| 50    | range                       |               |
| 51    | referer                     |               |
| 52    | refresh                     |               |
| 53    | retry-after                 |               |
| 54    | server                      |               |
| 55    | set-cookie                  |               |
| 56    | strict-transport-security   |               |
| 57    | transfer-encoding           |               |
| 58    | user-agent                  |               |
| 59    | vary                        |               |
| 60    | via                         |               |
| 61    | www-authenticate            |               |
+-------+-----------------------------+---------------+
*/
enum {
    _CC_HTTP2_INDEXED_AUTHORITY_            = 0x01,
    _CC_HTTP2_INDEXED_METHOD_GET_           = 0x02,
    _CC_HTTP2_INDEXED_METHOD_POST_          = 0x03,
    _CC_HTTP2_INDEXED_PATH_                 = 0x04,
    _CC_HTTP2_INDEXED_PATH_INDEX_           = 0x05,
    _CC_HTTP2_INDEXED_SCHEME_HTTP_          = 0x06,
    _CC_HTTP2_INDEXED_SCHEME_HTTPS_         = 0x07,
    _CC_HTTP2_INDEXED_STATUS_200_           = 0x08,
    _CC_HTTP2_INDEXED_STATUS_204_           = 0x09,
    _CC_HTTP2_INDEXED_STATUS_206_           = 0x0a,
    _CC_HTTP2_INDEXED_STATUS_304_           = 0x0b,
    _CC_HTTP2_INDEXED_STATUS_400_           = 0x0c,
    _CC_HTTP2_INDEXED_STATUS_404_           = 0x0d,
    _CC_HTTP2_INDEXED_STATUS_500_           = 0x0e,
    _CC_HTTP2_INDEXED_ACCEPT_CHARSET_       = 0x0f,
    _CC_HTTP2_INDEXED_ACCEPT_ENCODING_      = 0x10,
    _CC_HTTP2_INDEXED_ACCEPT_LANGUAGE_      = 0x11,
    _CC_HTTP2_INDEXED_ACCEPT_RANGES_        = 0x12,
    _CC_HTTP2_INDEXED_ACCEPT_               = 0x13,
    _CC_HTTP2_INDEXED_ACCEPT_ORIGIN_        = 0x14,
    _CC_HTTP2_INDEXED_AGE_                  = 0x15,
    _CC_HTTP2_INDEXED_ALLOW_                = 0x16,
    _CC_HTTP2_INDEXED_AUTHORIZATION_        = 0x17,
    _CC_HTTP2_INDEXED_CACHE_CONTROL_        = 0x18,
    _CC_HTTP2_INDEXED_CONTENT_DISPOSITION_  = 0x19,
    _CC_HTTP2_INDEXED_CONTENT_ENCODING_     = 0x1a,
    _CC_HTTP2_INDEXED_CONTENT_LANGUAGE_     = 0x1b,
    _CC_HTTP2_INDEXED_CONTENT_LENGTH_       = 0x1c,
    _CC_HTTP2_INDEXED_CONTENT_LOCATION_     = 0x1d,
    _CC_HTTP2_INDEXED_CONTENT_RANGE_        = 0x1e,
    _CC_HTTP2_INDEXED_CONTENT_TYPE_         = 0x1f,
    _CC_HTTP2_INDEXED_COOKIE_               = 0x20,
    _CC_HTTP2_INDEXED_DATE_                 = 0x21,
    _CC_HTTP2_INDEXED_ETAG_                 = 0x22,
    _CC_HTTP2_INDEXED_EXPECT_               = 0x23,
    _CC_HTTP2_INDEXED_EXPIRES_              = 0x24,
    _CC_HTTP2_INDEXED_FROM_                 = 0x25,
    _CC_HTTP2_INDEXED_HOST_                 = 0x26,
    _CC_HTTP2_INDEXED_IF_MATCH_             = 0x27,
    _CC_HTTP2_INDEXED_IF_MODIFIED_SINCE_    = 0x28,
    _CC_HTTP2_INDEXED_IF_NONE_MATCH_        = 0x29,
    _CC_HTTP2_INDEXED_IF_RANGE_             = 0x2a,
    _CC_HTTP2_INDEXED_IF_UNMODIFIED_SINCE_  = 0x2b,
    _CC_HTTP2_INDEXED_LAST_MODIFIED_        = 0x2c,
    _CC_HTTP2_INDEXED_LINK_                 = 0x2d,
    _CC_HTTP2_INDEXED_LOCATION_             = 0x2e,
    _CC_HTTP2_INDEXED_MAX_FORWARDS_         = 0x2f,
    _CC_HTTP2_INDEXED_PROXY_AUTHENTICATE_   = 0x30,
    _CC_HTTP2_INDEXED_PROXY_AUTHORIZATION_  = 0x31,
    _CC_HTTP2_INDEXED_RANGE_                = 0x32,
    _CC_HTTP2_INDEXED_REFERER_              = 0x33,
    _CC_HTTP2_INDEXED_REFRESH_              = 0x34,
    _CC_HTTP2_INDEXED_RETRY_AFTER_          = 0x35,
    _CC_HTTP2_INDEXED_SERVER_               = 0x36,
    _CC_HTTP2_INDEXED_SET_COOKIE_           = 0x37,
    _CC_HTTP2_INDEXED_STRICT_TRANSPORT_SEC_ = 0x38,
    _CC_HTTP2_INDEXED_TRANSFER_ENCODING_    = 0x39,
    _CC_HTTP2_INDEXED_USER_AGENT_           = 0x3a,
    _CC_HTTP2_INDEXED_VARY_                 = 0x3b,
    _CC_HTTP2_INDEXED_VIA_                  = 0x3c,
    _CC_HTTP2_INDEXED_WWW_AUTHENTICATE_     = 0x3d
};

// RFC 7540
// Error Codes
enum {
    _CC_HTTP2_ERROR_NO_ERROR_               = 0x0,
    _CC_HTTP2_ERROR_PROTOCOL_ERROR_         = 0x1,
    _CC_HTTP2_ERROR_INTERNAL_ERROR_         = 0x2,
    _CC_HTTP2_ERROR_FLOW_CONTROL_ERROR_     = 0x3,
    _CC_HTTP2_ERROR_SETTINGS_TIMEOUT_       = 0x4,
    _CC_HTTP2_ERROR_STREAM_CLOSED_          = 0x5,
    _CC_HTTP2_ERROR_FRAME_SIZE_ERROR_       = 0x6,
    _CC_HTTP2_ERROR_REFUSED_STREAM_         = 0x7,
    _CC_HTTP2_ERROR_CANCEL_                 = 0x8,
    _CC_HTTP2_ERROR_COMPRESSION_ERROR_      = 0x9,
    _CC_HTTP2_ERROR_CONNECT_ERROR_          = 0xA,
    _CC_HTTP2_ERROR_ENHANCE_YOUR_CALM_      = 0xB,
    _CC_HTTP2_ERROR_INADEQUATE_SECURITY_    = 0xC,

    _CC_HTTP2_ERROR_CUSTOM_BASE_            = 0xD
};

#define _CC_HTTP2_MAX_WINDOW_SIZE_                  0x7FFFFFFF

#define _CC_HTTP2_INITIAL_WINDOW_SIZE_              65535
#define _CC_HTTP2_DEFAULT_WINDOW_SIZE_              65535

#define _CC_HTTP2_FRAME_HEADER_SIZE_                9

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _cc_http2_frame_header {
    uint32_t length;
    uint8_t type;
    uint8_t flags;
    uint32_t stream_id;
} _cc_http2_frame_header_t;

/* Build a frame header and write it */
_CC_API_PUBLIC(int32_t) _cc_http2_frame_header(byte_t *hdr, uint8_t type, uint8_t flags, uint32_t stream_id, uint32_t payload);
#ifdef __cplusplus
}
#endif

#endif
