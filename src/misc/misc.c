
#include <libcc/time.h>
#include <libcc/UTF.h>
#include "misc.c.h"

typedef struct _XML_entity {
    const tchar_t *pattern;
    byte_t length;
    tchar_t value;
} _XML_entity_t;

static _cc_syntax_error_t _cc_global_syntax_error = {NULL, 0};

#define _XML_NUM_ENTITIES_ 5
static const _XML_entity_t XML_entities[_XML_NUM_ENTITIES_] = {{_T("quot"), 4, _T('\"')},
                                                                    {_T("apos"), 4, _T('\'')},
                                                                    {_T("amp"), 3, _T('&')},
                                                                    {_T("lt"), 2, _T('<')},
                                                                    {_T("gt"), 2, _T('>')}};

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

tchar_t * _convert_text(_cc_sds_t sds, const tchar_t *input_ptr, const tchar_t *endpos) {
    size_t alloc_length = _cc_sds_available(sds);
    tchar_t *output_ptr = (tchar_t*)sds;
    tchar_t *output_endpos = output_ptr + alloc_length - 1; /* -1 for zero terminator */
    /* loop through the string literal */
    while (input_ptr < endpos && output_ptr < output_endpos) {
        if (*input_ptr != '\\') {
            *output_ptr++ = *input_ptr++;
        } else if (*input_ptr == _T('&')) {
            size_t i = 0;
            const tchar_t *p = (input_ptr + 1);
            const _XML_entity_t *entity = NULL;
            for (i = 0; i < _XML_NUM_ENTITIES_; i++) {
                const _XML_entity_t *tmp = &XML_entities[i];
                if (*tmp->pattern == *p && _tcsnicmp(tmp->pattern, p, tmp->length) == 0) {
                    entity = tmp;
                    break;
                }
            }

            if (entity) {
                *output_ptr++ = entity->value;
                /* +1 skip & */
                input_ptr += entity->length + 1;
            } else {
                *output_ptr++ = *input_ptr++;
            }
        } else {
            /* escape sequence */
            unsigned char sequence_length = 2;
            if ((endpos - input_ptr) < 1) {
                return NULL;
            }

            switch (input_ptr[1]) {
                case 'b':
                    *output_ptr++ = '\b';
                    break;
                case 'f':
                    *output_ptr++ = '\f';
                    break;
                case 'n':
                    *output_ptr++ = '\n';
                    break;
                case 'r':
                    *output_ptr++ = '\r';
                    break;
                case 't':
                    *output_ptr++ = '\t';
                    break;
                case '\"':
                case '\\':
                case '/':
                    *output_ptr++ = input_ptr[1];
                    break;
                /* UTF-16 literal */
                case 'u':{ 
                    sequence_length = _cc_convert_utf16_literal_to_utf8(&input_ptr, endpos, output_ptr, 
                        alloc_length - (output_ptr - (tchar_t*)sds));
                    if (sequence_length == 0) {
                        /* failed to convert UTF16-literal to UTF-8 */
                        return NULL;
                    }
                    break;
                }
                default:
                    return NULL;
            }
            input_ptr += sequence_length;
        }
    }

    /* zero terminate the output */
    *output_ptr = '\0';
    _cc_sds_set_length(sds, (size_t)(output_ptr - (tchar_t*)sds));

    return output_ptr;
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