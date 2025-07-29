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
#include "ini.c.h"

/* Skips spaces and comments as many as possible.*/
bool_t _INI_buf_jump_comments(_cc_sbuf_t* const buffer) {
    register const tchar_t* p = nullptr;
    /*if ((buffer == nullptr) || (buffer->content == nullptr)) {
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

_CC_API_PRIVATE(tchar_t*) _INI_read_name(_cc_sbuf_t* const buffer) {
    tchar_t* output = nullptr;
    const tchar_t *start = _cc_sbuf_offset(buffer);
    const tchar_t *endpos = nullptr;
    const tchar_t* p = start;

    endpos = buffer->content + buffer->length;
    while (p < endpos) {
        if (*p == ']' || *p == '=' || _CC_ISSPACE(*p)) {
            break;
        }
        p++;
    }

    if (p >= endpos) {
        return nullptr;
    }
    /* -1 skip ']', '=', whitespace */
    endpos = p - 1;
    output = (tchar_t*)_cc_tcsndup(start, (size_t)(endpos - start) + 1);
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
    const tchar_t *endpos = nullptr;

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

    endpos = p - 1;
    if (quotes) {
        while (start < endpos && _CC_ISSPACE(*(endpos - 1))) {
            endpos--;
        }
        endflag = 1;
    }

    /* This is at most how much we need for the item->element.uni_string */
    alloc_length = (size_t)(endpos - start);

    item->element.uni_string = (tchar_t*)_cc_malloc(sizeof(tchar_t) * (alloc_length - skipped_bytes));
    endpos = _convert_text(item->element.uni_string, alloc_length, start, endpos);
    if (endpos) {
        item->length = (endpos - item->element.uni_string);
        
        buffer->offset = (size_t)(p - buffer->content) + endflag;
        return true;
    }

    return false;
}

_CC_API_PRIVATE(bool_t) _INI_read(_cc_ini_t* root, _cc_sbuf_t* const buffer) {
    while (_INI_buf_jump_comments(buffer) && *_cc_sbuf_offset(buffer) == '[') {
        _cc_ini_t* section;
        tchar_t *name;
        /* skip [ */
        buffer->offset++;

        if (!_INI_buf_jump_comments(buffer)) {
            buffer->offset--;
            return false;
        }

        name = _INI_read_name(buffer);
        if ((*_cc_sbuf_offset(buffer) != _T(']'))) {
            return false;
        }

        /* skip ] */
        buffer->offset++;
        if (!_INI_buf_jump_comments(buffer)) {
            buffer->offset--;
            return false;
        }

        section = _INI_push(&root->element.uni_object, name, _CC_INI_SECTION_);

        do {
            _cc_ini_t *item;
            name = _INI_read_name(buffer);

            if (!_INI_buf_jump_comments(buffer)) {
                buffer->offset--;
                _cc_free(name);
                return false;
            }

            if ((*_cc_sbuf_offset(buffer) != _T('='))) {
                _cc_free(name);
                return false;
            }

            /* skip = */
            buffer->offset++;

            if (!_INI_buf_jump_comments(buffer)) {
                buffer->offset--;
                _cc_free(name);
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
    _cc_ini_t* root = nullptr;
    _cc_syntax_error_t local_error;

    local_error.content = nullptr;
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
    return nullptr;
}

_CC_API_PUBLIC(_cc_ini_t*) _cc_ini_from_file(const tchar_t* file_name) {
    _cc_sbuf_t buffer;
    _cc_ini_t* item = nullptr;

    byte_t* content = nullptr;
    size_t offset = 0;
    _cc_buf_t buf;

    if (!_cc_buf_from_file(&buf,file_name)) {
        return nullptr;
    }
    content = buf.bytes;

    /*----BOM----
    EF BB BF = UTF-8
    FE FF 00 = UTF-16, big-endian
    FF FE    = UTF-16, little-endian

    00 00 FE FF = UTF-32, big-endian
    FF FE 00 00 = UTF-32, little-endian
    */

    /*UTF8 BOM */
    if (*content == 0xEF && *(content + 1) == 0xBB && *(content + 2) == 0xBF) {
        offset = 3;
        /*UTF-32 BOM */
    } else if ((*content == 0x00 && *(content + 1) == 0x00 &&
                *(content + 2) == 0xFE && *(content + 3) == 0xFF) ||
               (*content == 0xFF && *(content + 1) == 0xFE &&
                *(content + 2) == 0x00 && *(content + 3) == 0x00)) {
        offset = 4;
        /*UTF-16 BOM big-endian*/
    } else if (*content == 0xFE && *(content + 1) == 0xFF &&
               *(content + 2) == 0x00) {
        offset = 3;
        /*UTF-16 BOM little-endian*/
    } else if (*content == 0xFF && *(content + 1) == 0xFE) {
        offset = 2;
    }

    buffer.content = (tchar_t*)(content + offset);
    buffer.length = (buf.length - offset) / sizeof(tchar_t);
    buffer.offset = 0;
    buffer.line = 1;
    buffer.depth = 0;

    item = _cc_ini_parser(&buffer);

    _cc_free_buf(&buf);

    return item;
}

_CC_API_PUBLIC(_cc_ini_t*) _cc_parse_ini(const tchar_t* src) {
    _cc_sbuf_t buffer;
    buffer.content = src;
    buffer.length = _tcslen(src);
    buffer.offset = 0;
    buffer.line = 1;
    buffer.depth = 0;

    return _cc_ini_parser(&buffer);
}
