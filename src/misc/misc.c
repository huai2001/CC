
#include <libcc/time.h>
#include <libcc/UTF.h>
#include "misc.c.h"

static _cc_syntax_error_t _cc_global_syntax_error = {NULL, 0};
void _cc_syntax_error(_cc_syntax_error_t *error) {
    _cc_global_syntax_error.content = error->content;
    _cc_global_syntax_error.position = error->position;
}

const tchar_t* _cc_get_syntax_error(void) {
    if (_cc_global_syntax_error.position) {
        return (_cc_global_syntax_error.content + _cc_global_syntax_error.position);
    }
    return _cc_global_syntax_error.content;
}

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

    if (rfc822_date == NULL) {
        return 0;
    }

    if (_cc_strptime(rfc822_date, _T("%a, %d %b %Y %H:%M:%S"), &ptm)) {
        return mktime(&ptm);
    }
    return 0;
}