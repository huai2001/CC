/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
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
#include <libcc/alloc.h>
#include <libcc/string.h>
#include <libcc/url.h>
#include <libcc/widgets/url_request.h>


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

/* frame flags */
enum {
    _CC_HTTP2_FRAME_FLAG_NO_             = 0x00,
    _CC_HTTP2_FRAME_FLAG_ACK_            = 0x01,
    _CC_HTTP2_FRAME_FLAG_END_STREAM_     = 0x01,
    _CC_HTTP2_FRAME_FLAG_END_HEADERS_    = 0x04,
    _CC_HTTP2_FRAME_FLAG_PADDED_         = 0x08,
    _CC_HTTP2_FRAME_FLAG_PRIORITY_       = 0x20,
};

#define _CC_HTTP2_FRAME_HEADER_SIZE_    9

