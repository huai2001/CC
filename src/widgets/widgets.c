#include <cc/alloc.h>
#include <cc/event/event.h>

/**/
bool_t _cc_event_writef(_cc_event_t *e, const char_t *fmt, ...) {
    char_t buf[_CC_IO_BUFFER_SIZE_];
    int32_t off = 0;
    va_list arg;

    va_start(arg, fmt);
    off = _vsnprintf(buf, _cc_countof(buf), fmt, arg);
    va_end(arg);

    if (off > 0) {
        return _cc_event_send(e, (byte_t*)buf, off);
    }
    return false;
}
