#include <libcc/widgets/widgets.h>

/* Render seconds since 1970 as an RFC822 date string.  Return
** a pointer to that string in a static buffer.
*/
tchar_t* get_rfc822_date(time_t t) {
    struct tm* ptm;
    static tchar_t str_date[128];
    ptm = gmtime(&t);
    _tcsftime(str_date, _cc_countof(str_date), _T("%a, %d %b %Y %H:%M:%S GMT"), ptm);
    return str_date;
}
/*
** Parse an RFC822-formatted timestamp as we'd expect from HTTP and return
** a Unix epoch time. <= zero is returned on failure.
*/
time_t get_rfc822_time(const tchar_t* rfc822_date) {
    struct tm ptm;

    if (rfc822_date == nullptr) {
        return 0;
    }

    if (_cc_strptime(rfc822_date, _T("%a, %d %b %Y %H:%M:%S"), &ptm)) {
        return mktime(&ptm);
    }
    return 0;
}

_CC_API_PUBLIC(bool_t) _cc_event_writef(_cc_event_t *e, const tchar_t *fmt, ...) {
    _cc_event_wbuf_t *wbuf;
    int32_t fmt_length, empty_length;
    tchar_t *ptr;
    va_list arg;

    _cc_assert(fmt != nullptr);
    _cc_assert(e != nullptr);
    _cc_assert(e->buffer != nullptr);

    if (e->buffer == nullptr) {
        return false;
    }

    va_start(arg, fmt);
    wbuf = &e->buffer->w;

    _cc_spin_lock(&wbuf->lock);
    ptr = (tchar_t*)&(wbuf->bytes[wbuf->length]);

    empty_length = (wbuf->limit - wbuf->length);

    va_start(arg, fmt);
    /* If the first attempt to append fails, resize the buffer appropriately
     * and try again */
    while (true) {
        /* fmt_length is the length of the string required, excluding the
         * trailing nullptr */
        fmt_length = _vsntprintf(ptr, empty_length, fmt, arg);

#ifdef __CC_WINDOWS__
        if (fmt_length == -1) {
            fmt_length = _vsntprintf(nullptr, 0, fmt, arg);
        }
#endif
        if (fmt_length <= 0) {
            goto WRITE_FAIL;
        }

        /* SUCCESS */
        if (fmt_length < empty_length) {
            break;
        }
        
        _cc_alloc_event_wbuf(wbuf, fmt_length);
        ptr = (tchar_t*)&(wbuf->bytes[wbuf->length]);
    }
    va_end(arg);
    _cc_unlock(&wbuf->lock);

    if (_CC_ISSET_BIT(_CC_EVENT_WRITABLE_, e->flags)) {
        _cc_event_cycle_t *cycle = _cc_get_event_cycle_by_id(e->round);
        _CC_SET_BIT(_CC_EVENT_WRITABLE_, e->flags);
        cycle->reset(cycle, e);
    }
    return true;

WRITE_FAIL:
    _cc_unlock(&wbuf->lock);
    return false;
}

/**/
_CC_API_PUBLIC(void) _widget_open_syslog(byte_t facility, const tchar_t *ip, const uint16_t port) {
    _cc_open_syslog(facility, "libcc.widgets.dll", ip, port);
}

/**/
_CC_API_PUBLIC(void) _widget_close_syslog(void) {
    _cc_close_syslog();
}