#include "ini.c.h"

/* Skips spaces and comments as many as possible.*/
bool_t _INI_buf_jump_comments(_cc_sbuf_t* const buffer) {
    register const tchar_t* p = NULL;
    /*if ((buffer == NULL) || (buffer->content == NULL)) {
        return false;
    }*/
    while (_cc_sbuf_access(buffer)) {
        p = _cc_sbuf_offset(buffer);
        /*Whitespace characters.*/
        if (*p <= 32) {
            if (*p == _T(_CC_LF_)) {
                buffer->line++;
            }

            buffer->offset++;
        } else if (*p == _T(';') || *p == '#') {
            buffer->offset += 1;
            while (_cc_sbuf_access(buffer)) {
                if (*_cc_sbuf_offset(buffer) == _T(_CC_LF_)) {
                    buffer->offset++;
                    buffer->line++;
                    break;
                }
                buffer->offset++;
            }
        } else if (*p == _T('/')) {
            p++;
            /*double-slash comments, to end of line.*/
            if (*p == _T('/')) {
                buffer->offset += 2;
                while (_cc_sbuf_access(buffer)) {
                    if (*_cc_sbuf_offset(buffer) == _T(_CC_LF_)) {
                        buffer->offset++;
                        buffer->line++;
                        break;
                    }
                    buffer->offset++;
                }
                /*multiline comments.*/
            } else if (*p == _T('*')) {
                buffer->offset += 2;
                while (_cc_sbuf_access(buffer)) {
                    p = _cc_sbuf_offset(buffer);
                    if ((*p == _T('*') && *(p + 1) == _T('/'))) {
                        /*skip '*' and '/' */
                        buffer->offset += 2;
                        break;
                    }

                    if (*p == _T(_CC_LF_)) {
                        buffer->line++;
                    }

                    buffer->offset++;
                }
            } else
                break;
        } else
            break;
    }

    return _cc_sbuf_access(buffer);
}

_CC_API_PRIVATE(_cc_sds_t) _INI_read_name(_cc_sbuf_t* const buffer) {
    _cc_sds_t output = NULL;
    const tchar_t *start = _cc_sbuf_offset(buffer);
    const tchar_t *endpos = NULL;
    const tchar_t* p = start;

    endpos = buffer->content + buffer->length;
    while (p < endpos) {
        if (*p == ']' || *p == '=' || _CC_ISSPACE(*p)) {
            break;
        }
        p++;
    }

    if (p >= endpos) {
        return NULL;
    }
    /* -1 skip ']', '=', whitespace */
    endpos = p - 1;
    output = (_cc_sds_t)_cc_sds_alloc(start, (size_t)(endpos - start) + 1);
    buffer->offset = (size_t)(p - buffer->content);

    return output;
}

_CC_API_PRIVATE(bool_t) _INI_value_endflag(const tchar_t* p, const tchar_t quotes) {
    if (quotes != 0) {
        if (quotes == *p) {
            return true;
        }
        return false;
    }

    if (*p == _T(_CC_CR_) || *p == _T(_CC_LF_)) {
        return true;
    }

    /* comments */
    if (*p == _T(';') || (*p == _T('/') && *(p + 1) == _T('/')) || (*p == _T('/') && *(p + 1) == _T('*'))) {
        return true;
    }
    return false;
}

_CC_API_PRIVATE(bool_t) _INI_read_string(_cc_sbuf_t* const buffer, _cc_ini_t *item) {
    const tchar_t* p = _cc_sbuf_offset(buffer);
    const tchar_t* start;
    const tchar_t *endpos = NULL;

    size_t alloc_length = 0;
    size_t skipped_bytes = 0;

    tchar_t quotes = *p;

    int endflag = 0;

    endpos = buffer->content + buffer->length;

    if (quotes == _T('"') || quotes == _T('\'')) {
        start = ++p;
    } else {
        quotes = 0;
        while (p < endpos && _CC_ISSPACE(*p)) {
            p++;
        }
        start = p;
    }

    /* calculate approximate size of the output (overestimate) */
    while (p < endpos && !_INI_value_endflag(p, quotes)) {
        if (*p == _T('\\')) {
            /* prevent buffer overflow when last input character is a backslash */
            if ((p + 1) >= endpos) {
                return false;
            }
            skipped_bytes++;
            p++;
        }
        p++;
    }

    if (p > endpos) {
        return false;
    }

    if (quotes) {
        endpos = p - 1;
        while (start < endpos && _CC_ISSPACE(*(endpos - 1))) {
            endpos--;
        }
        endflag = 1;
    } else {
        endpos = p;
    }
    /* This is at most how much we need for the item->element.uni_string */
    alloc_length = (size_t)(endpos - start);

    item->element.uni_string = (_cc_sds_t)_cc_sds_alloc(NULL, (alloc_length - skipped_bytes + 1));
    if (_unescape_text(item->element.uni_string, start, endpos)) {
        buffer->offset = (size_t)(p - buffer->content) + endflag;
        return true;
    }

    return false;
}

_CC_API_PRIVATE(bool_t) _INI_read(_cc_ini_t* root, _cc_sbuf_t* const buffer) {
    while (_INI_buf_jump_comments(buffer) && *_cc_sbuf_offset(buffer) == '[') {
        _cc_ini_t* section;
        _cc_sds_t name;
        /* skip [ */
        buffer->offset++;

        if (!_INI_buf_jump_comments(buffer)) {
            buffer->offset--;
            return false;
        }

        name = _INI_read_name(buffer);
        if ((*_cc_sbuf_offset(buffer) != _T(']'))) {
            _cc_sds_free(name);
            return false;
        }

        /* skip ] */
        buffer->offset++;
        if (!_INI_buf_jump_comments(buffer)) {
            _cc_sds_free(name);
            buffer->offset--;
            return false;
        }

        section = _INI_push(&root->element.uni_object, name, _CC_INI_SECTION_);

        do {
            _cc_ini_t *item;
            name = _INI_read_name(buffer);

            if (!_INI_buf_jump_comments(buffer)) {
                buffer->offset--;
                _cc_sds_free(name);
                return false;
            }

            if ((*_cc_sbuf_offset(buffer) != _T('='))) {
                _cc_sds_free(name);
                return false;
            }

            /* skip = */
            buffer->offset++;

            if (!_INI_buf_jump_comments(buffer)) {
                buffer->offset--;
                _cc_sds_free(name);
                return false;
            }

            item = _INI_push(&section->element.uni_object, name, _CC_INI_STRING_);
            if (!_INI_read_string(buffer, item)) {
                return false;
            }
            
            _INI_buf_jump_comments(buffer);

            if (*_cc_sbuf_offset(buffer) == _T('[')) {
                break;
            }

        } while(_cc_sbuf_access(buffer));
    }

    return true;
}

_CC_API_PUBLIC(_cc_ini_t*) _cc_ini_parser(_cc_sbuf_t* const buffer) {
    _cc_ini_t* root = NULL;
    _cc_syntax_error_t local_error;

    local_error.content = NULL;
    local_error.position = 0;

    root = _INI_alloc(_CC_INI_SECTION_);
    if (_INI_read(root, buffer)) {
        return root;
    }

    local_error.content = buffer->content;
    if (buffer->offset < buffer->length) {
        local_error.position = buffer->offset;
    } else if (buffer->length > 0) {
        local_error.position = buffer->length - 1;
    }

    /*reset error position*/
    _cc_syntax_error(&local_error);

    _cc_free_ini(root);
    return NULL;
}

_CC_API_PUBLIC(_cc_ini_t*) _cc_ini_from_file(const tchar_t* file_name) {
    _cc_sbuf_t buffer;
    _cc_ini_t* item = NULL;
    _cc_buf_t buf;

    if (!_cc_buf_from_file(&buf, file_name)) {
        return NULL;
    }

    buffer.content = (tchar_t*)buf.bytes;
    buffer.length = buf.length / sizeof(tchar_t);
    buffer.offset = 0;
    buffer.line = 1;
    buffer.depth = 0;

    item = _cc_ini_parser(&buffer);

    _cc_free_buf(&buf);

    return item;
}

_CC_API_PUBLIC(_cc_ini_t*) _cc_parse_ini(const tchar_t* src, size_t length) {
    _cc_sbuf_t buffer;
    buffer.content = src;
    buffer.length = length;
    buffer.offset = 0;
    buffer.line = 1;
    buffer.depth = 0;

    return _cc_ini_parser(&buffer);
}
