
#include <libcc/time.h>
#include <libcc/UTF.h>
#include "misc.c.h"

#define _XML_NUM_ENTITIES_ 5

typedef struct _XML_entity {
    const tchar_t *pattern;
    byte_t length;
    tchar_t value;
} _XML_entity_t;

static const _XML_entity_t XML_entities[_XML_NUM_ENTITIES_] = {{_T("quot;"), 5, _T('\"')}, {_T("apos;"), 5, _T('\'')},
                                                               {_T("amp;"),  4, _T('&')},  {_T("lt;"),   3, _T('<')}, 
                                                               {_T("gt;"),   3, _T('>')}};

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

_CC_API_PUBLIC(size_t) _cc_utf8_unescape_text(tchar_t *dst, tchar_t *dst_endptr, const tchar_t *ptr, const tchar_t *endptr) {
    tchar_t *dst_ptr = dst;
    /* Process each character in the input string */
    while (ptr < endptr && dst_ptr < dst_endptr) {
        if (*ptr != '\\') {
            /* Handle XML entities like &quot;, &amp;, &lt;, &gt;, &apos; */
            if (*ptr == _T('&')) {
                const _XML_entity_t *entity = NULL;
                const tchar_t *entity_start = ptr + 1;
                size_t remaining = (size_t)(endptr - entity_start);
                size_t i;

                /* Check each known XML entity */
                for (i = 0; i < _XML_NUM_ENTITIES_; i++) {
                    const _XML_entity_t *tmp = &XML_entities[i];
                    if (remaining >= tmp->length && *tmp->pattern == *entity_start && 
                        _tcsnicmp(tmp->pattern, entity_start, tmp->length) == 0) {
                        entity = tmp;
                        break;
                    }
                }

                if (entity) {
                    /* Replace entity with its character value */
                    *dst_ptr++ = entity->value;
                    ptr += entity->length + 1; /* Skip & and entity name */
                    continue;
                }
            }
            *dst_ptr++ = *ptr++;
        } else {
            const tchar_t *escape_start = ptr; /* Save backslash position */
            /* Handle C-style escape sequences */
            if (ptr + 1 >= endptr) {
                *dst_ptr++ = *ptr++;
                break;
            }

            ptr++; /* Skip backslash */

            switch (*ptr++) {
                case 'b':  *dst_ptr++ = '\b'; break;
                case 'f':  *dst_ptr++ = '\f'; break;
                case 'n':  *dst_ptr++ = '\n'; break;
                case 'r':  *dst_ptr++ = '\r'; break;
                case 't':  *dst_ptr++ = '\t'; break;
                case '\"': case '\\': case '/':
                    *dst_ptr++ = *(ptr - 1);
                    break;
                case 'u': {
                    int32_t res = _cc_convert_utf16_literal_to_utf8(&escape_start, endptr, dst_ptr, (size_t)(dst_endptr - dst_ptr));
                    if (res == 0) {
                        return 0;
                    }
                    dst_ptr += res;
                    ptr = escape_start;
                    break;
                }
                default:
                    *dst_ptr++ = '\\';
                    *dst_ptr++ = *(ptr - 1);
                    break;
            }
        }
    }

    /* Zero terminate the output buffer */
    *dst_ptr = '\0';
    return (size_t)(dst_ptr - dst);
}

bool_t _unescape_text(_cc_sds_t sds, const tchar_t *ptr, const tchar_t *endptr) {
    size_t alloc_length = _cc_sds_limit(sds);
    tchar_t *output_ptr = (tchar_t*)sds;
    size_t length = _cc_utf8_unescape_text(output_ptr, output_ptr + alloc_length - 1/* -1 for zero terminator */, ptr, endptr);
    if (length == 0) {
        return false;
    }
    _cc_sds_set_length(sds, length);
    return true;
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