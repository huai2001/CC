/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
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
#include <stdio.h>
#include <libcc.h>
#include <locale.h>

enum {
    TYPE_CHAR = 1,TYPE_WCHAR,TYPE_BYTE,
    TYPE_INT8,TYPE_INT16,TYPE_INT32,TYPE_INT64,
    TYPE_UINT8,TYPE_UINT16,TYPE_UINT32,TYPE_UINT64,
    TYPE_FLOAT32,TYPE_FLOAT64,TYPE_STRUCT
};

typedef struct {
    tchar_t *name;
    int length;
    int type_id;
    tchar_t *base_name;
}type_list_t;

static type_list_t type_list [] = {
    {"char", sizeof("char")-1, TYPE_CHAR,"char"},
    {"wchar", sizeof("wchar")-1, TYPE_WCHAR,"wchar"},
    {"byte", sizeof("byte")-1, TYPE_BYTE,"byte_t"},
    {"int8", sizeof("int8")-1, TYPE_INT8,"int8_t"},
    {"int16", sizeof("int16")-1, TYPE_INT16,"int16_t"},
    {"int32", sizeof("int32")-1, TYPE_INT32,"int32_t"},
    {"int64", sizeof("int64")-1, TYPE_INT64,"intt64_t"},
    {"uint8", sizeof("uint8")-1, TYPE_UINT8, "uint8_t"},
    {"uint16", sizeof("uint16")-1, TYPE_UINT16,"uint16_t"},
    {"uint32", sizeof("uint32")-1, TYPE_UINT32,"uint32_t"},
    {"uint64", sizeof("uint64")-1, TYPE_UINT64,"uint64_t"},
    {"float32", sizeof("float32")-1, TYPE_FLOAT32,"float32_t"},
    {"float64", sizeof("float64")-1, TYPE_FLOAT64,"float64_t"},
    {"struct", sizeof("struct")-1, TYPE_STRUCT,"struct"}
};

typedef struct {
    tchar_t path[128];
    _cc_list_iterator_t lnk;
} tpl_file_t;

typedef struct {
    int type_id;
    int type_name_length;
    int name_length;
    int array_length;
    tchar_t type_name[128];
    tchar_t name[128];

    _cc_list_iterator_t variables;
    _cc_list_iterator_t lnk;
} tpl_t;

static _cc_list_iterator_t gRoot;
static _cc_list_iterator_t gFile;

static tpl_t *_tpl_create_struct(const tchar_t*name, size_t length) {
    tpl_t *item = _cc_malloc(sizeof(tpl_t));
    _cc_list_iterator_cleanup(&item->variables);
    item->type_id = TYPE_STRUCT;
    _tcsncpy(item->name, name, length);
    item->name[length] = 0;
    return item;
}

static bool_t _tpl_is_name(_cc_sbuf_tchar_t *buffer) {
    int c = *_cc_sbuf_offset(buffer);
    return _CC_ISDIGIT(c) || _CC_ISALPHA(c) || c == '_';
}

static bool_t _parser_number(_cc_sbuf_tchar_t *const buffer, tpl_t *item) {
    _cc_number_t num;
    const tchar_t *start = _cc_sbuf_offset(buffer);
    const tchar_t *s = _cc_to_number(start, &num);

    if (_cc_unlikely(s == NULL)) {
        return false;
    }

    if (num.vt == _CC_NUMBER_INT_) {
        item->array_length = (int32_t)num.v.uni_int;
    } else {
        return  false;
    }

    buffer->offset += (size_t)(s - start);

    return _cc_buf_jump_comments(buffer);
}

static bool_t _tpl_read_fields(_cc_sbuf_tchar_t *buffer, _cc_list_iterator_t *root) {
    int i;
    while (_cc_sbuf_access(buffer)) {
        tpl_t *child = _cc_malloc(sizeof(tpl_t));
        size_t offset = buffer->offset;
        size_t length;
        
        _cc_list_iterator_cleanup(&child->variables);
        
        _cc_first_index_of(buffer->offset, buffer->length, _tpl_is_name(buffer));
        if (offset == buffer->offset) {
            return false;
        }
        
        child->type_name_length = (int)(buffer->offset - offset);
        _tcsncpy(child->type_name, (buffer->content + offset), child->type_name_length);
        child->type_name[child->type_name_length] = 0;
        
        child->type_id = 0;
        for (i = 0; i < _cc_countof(type_list); i++) {
            type_list_t *t = &type_list[i];
            if (_tcsnicmp(t->name, child->type_name, child->type_name_length) == 0) {
                child->type_id = t->type_id;
                if (t->type_id != TYPE_STRUCT) {
                    _tcsncpy(child->type_name, t->base_name, _cc_countof(child->type_name));
                    child->type_name[_cc_countof(child->type_name) - 1] = 0;
                }
                break;
            }
        }
        
        if (!_cc_buf_jump_comments(buffer)) {
            return false;
        }
        
        if (child->type_id == TYPE_STRUCT) {
            if (*_cc_sbuf_offset(buffer) == '{') {
                tpl_t*item;
                buffer->offset++;
                if (!_cc_buf_jump_comments(buffer)) {
                    return false;
                }
                length = _sntprintf(child->type_name, _cc_countof(child->type_name), _T("anonymity_%p_%ld"), child, time(NULL));
                item = _tpl_create_struct(child->type_name, length);
                
                if (!_tpl_read_fields(buffer, &item->variables)) {
                    return false;
                }
                _cc_list_iterator_push_front(&gRoot, &item->lnk);
            } else {
                offset = buffer->offset;
                _cc_first_index_of(buffer->offset, buffer->length, _tpl_is_name(buffer));
                if (offset == buffer->offset) {
                    return false;
                }
                child->name_length = (int)(buffer->offset - offset);
                _tcsncpy(child->type_name, (buffer->content + offset), child->name_length);
                child->type_name[_cc_countof(child->type_name) - 1] = 0;
                
                if (!_cc_buf_jump_comments(buffer)) {
                    return false;
                }
            }
        }
        
        offset = buffer->offset;
        _cc_first_index_of(buffer->offset, buffer->length, _tpl_is_name(buffer));
        if (offset == buffer->offset) {
            return false;
        }
        
        child->name_length = (int)(buffer->offset - offset);
        _tcsncpy(child->name, (buffer->content + offset), child->name_length);
        child->name[child->name_length] = 0;
        
        if (!_cc_buf_jump_comments(buffer)) {
            return false;
        }
        child->array_length = 0;
        if (*_cc_sbuf_offset(buffer) == '[') {
            buffer->offset++;
            if (!_parser_number(buffer, child)) {
                return false;
            }
            if (*_cc_sbuf_offset(buffer) != ']') {
                return false;
            }
            buffer->offset++;
            
            if (!_cc_buf_jump_comments(buffer)) {
                return false;
            }
        }
        
        if (*_cc_sbuf_offset(buffer) != ';') {
            return false;
        }
        
        buffer->offset++;
        
        if (!_cc_buf_jump_comments(buffer)) {
            return false;
        }
        
        //printf("%s %s\n", child->type_name, child->name);
        _cc_list_iterator_push(root, &child->lnk);
        
        if (*_cc_sbuf_offset(buffer) == '}') {
            buffer->offset++;
            _cc_buf_jump_comments(buffer);
            return true;
        }
    }
    
    return false;
}

static bool_t _tpl_read_struct(_cc_sbuf_tchar_t *buffer) {
    tpl_t *item;
    size_t offset = buffer->offset;
    if (!_cc_buf_jump_comments(buffer)) {
        return false;
    }

    _cc_first_index_of(buffer->offset, buffer->length, _tpl_is_name(buffer));
    if (offset == buffer->offset) {
        return false;
    }
    item = _tpl_create_struct((buffer->content + offset), (buffer->offset - offset));
    //printf("struct %s\n", item->name);

    if (!_cc_buf_jump_comments(buffer)) {
        return false;
    }

    if (*_cc_sbuf_offset(buffer) != '{') {
        return false;
    }

    buffer->offset += 1;
    if (!_cc_buf_jump_comments(buffer)) {
        return false;
    }
    _cc_list_iterator_push(&gRoot, &item->lnk);
    
    return _tpl_read_fields(buffer, &item->variables);
}

bool_t _tpl_read(_cc_sbuf_tchar_t *buffer) {
    register const tchar_t *p = _cc_sbuf_offset(buffer);
    /*if (_tcsnicmp("include", p, sizeof("include")) == 0) {
        buffer->offset += sizeof("include");
        //读取包含文件
        return _tpl_read_include(_cc_sbuf_tchar_t *buffer, _cc_tpl_t *item);
    }
*/
    if (_tcsnicmp("struct", p, sizeof("struct")-1) == 0) {
        buffer->offset += sizeof("struct");
        return _tpl_read_struct(buffer);
    }

    return true;
}

bool_t findStruct(const tchar_t* type_name) {
    _cc_list_iterator_t *it;
    _cc_list_iterator_for(it, &gRoot) {
        tpl_t *item = _cc_upcast(it, tpl_t, lnk);
        if (_tcscmp(item->name, type_name) == 0) {
            return true;
        }
    }
    return false;
}

void write_file_header(_cc_list_iterator_t *root, FILE *fp, int dep) {
    _cc_list_iterator_t *it;
    int i = 0;
    _cc_list_iterator_for(it, root) {
        tpl_t *item = _cc_upcast(it, tpl_t, lnk);
        for (i = 0; i < dep; i++) {
            fputc('\t',fp);
        }
        if (item->type_id == TYPE_STRUCT) {
            if (item->variables.next != &item->variables) {
                fprintf(fp, "struct %s {\n", item->name);
                write_file_header(&item->variables, fp, dep+1);
                for (i = 0; i < dep; i++) {
                    fputc('\t',fp);
                }
                fprintf(fp, "};\n\n");
                fprintf(fp, "bool_t %s_WriteBuffer(WriteBuffer_t *buffer, struct %s *object);\n", item->name, item->name);
                fprintf(fp, "bool_t %s_ReadBuffer(ReadBuffer_t *buffer, struct %s *object);\n\n", item->name, item->name);
            } else {
                if (item->array_length > 0) {
                    fprintf(fp, "struct %s %s[%d];\n", item->type_name, item->name, item->array_length);
                } else {
                    fprintf(fp, "struct %s %s;\n", item->type_name, item->name);
                }
            }
        } else {
            if (item->array_length > 0) {
                fprintf(fp, "%s %s[%d];\n", item->type_name, item->name, item->array_length);
            }else {
                fprintf(fp, "%s %s;\n", item->type_name, item->name);
            }
        }
    }
}

void write_file_body(_cc_list_iterator_t *root, FILE *fp, int dep) {
    _cc_list_iterator_t *it;
    int i = 0;
    _cc_list_iterator_for(it, root) {
        tpl_t *item = _cc_upcast(it, tpl_t, lnk);
        for (i = 0; i < dep; i++) {
            fputc('\t',fp);
        }
        if (item->type_id == TYPE_STRUCT) {
            if (item->variables.next != &item->variables) {
                fprintf(fp, "bool_t %s_WriteBuffer(WriteBuffer_t *buffer, struct %s *object) {\n", item->name, item->name);
                write_file_body(&item->variables, fp, dep+1);
                fprintf(fp, "}\n\n");
            } else if (findStruct(item->type_name)) {
                fprintf(fp, "%s_WriteBuffer(buffer, &object->%s);\n",item->type_name, item->name);
            } else {
                fprintf(fp, "ByteBuffer_WriteByte(buffer, &object->%s, sizeof(struct %s));\n",item->name, item->type_name);
            }
        } else {
            switch (item->type_id) {
                case TYPE_BYTE:
                    break;
                case TYPE_CHAR:
                    if (item->array_length > 0) {
                        fprintf(fp, "ByteBuffer_WriteString(buffer, object->%s, %d);\n",item->name, item->array_length);
                    } else {
                        fprintf(fp, "ByteBuffer_WriteInt8(buffer, &object->%s);\n",item->name);
                    }
                    break;
                case TYPE_WCHAR:
                    if (item->array_length > 0) {
                        fprintf(fp, "ByteBuffer_WriteByte(buffer, object->%s, sizeof(wchar) * %d);\n",item->name, item->array_length);
                    } else {
                        fprintf(fp, "ByteBuffer_WriteInt8(buffer, &object->%s);\n",item->name);
                    }
                    break;
                case TYPE_UINT8:
                case TYPE_INT8:
                    if (item->array_length > 0) {
                        fprintf(fp, "ByteBuffer_WriteByte(buffer, object->%s, %d);\n",item->name, item->array_length);
                    } else {
                        fprintf(fp, "ByteBuffer_WriteInt8(buffer, &object->%s);\n",item->name);
                    }
                    break;
                case TYPE_UINT16:
                case TYPE_INT16:
                    if (item->array_length > 0) {
                        fprintf(fp, "ByteBuffer_WriteByte(buffer, object->%s, sizeof(int16_t) * %d);\n",item->name, item->array_length);
                    } else {
                        fprintf(fp, "ByteBuffer_WriteInt16(buffer, &object->%s);\n",item->name);
                    }
                    break;
                case TYPE_UINT32:
                case TYPE_INT32:
                    if (item->array_length > 0) {
                        fprintf(fp, "ByteBuffer_WriteByte(buffer, object->%s, sizeof(int32_t) * %d);\n",item->name, item->array_length);
                    } else {
                        fprintf(fp, "ByteBuffer_WriteInt32(buffer, &object->%s);\n",item->name);
                    }
                    break;
                case TYPE_UINT64:
                case TYPE_INT64:
                    if (item->array_length > 0) {
                        fprintf(fp, "ByteBuffer_WriteByte(buffer, object->%s, sizeof(int64_t) * %d);\n",item->name, item->array_length);
                    } else {
                        fprintf(fp, "ByteBuffer_WriteInt64(buffer, &object->%s);\n",item->name);
                    }
                    break;
            }
        }
    }
}

void write_file(const tchar_t *file_name, _cc_list_iterator_t *root) {
    int dep = 0;
    FILE *fp = _tfopen(file_name, "w");
    if (fp == NULL) {
        return;
    }
    write_file_header(root, fp, dep);
    fclose(fp);
}
void write_file2(const tchar_t *file_name, _cc_list_iterator_t *root) {
    int dep = 0;
    FILE *fp = _tfopen(file_name, "w");
    if (fp == NULL) {
        return;
    }
    write_file_body(root, fp, dep);
    fclose(fp);
}


bool_t _tpl_open(const tchar_t *file_name) {
    _cc_sbuf_tchar_t buffer;
    tpl_file_t *file = _cc_malloc(sizeof(tpl_file_t));
    _cc_buf_t *buf = _cc_load_buf(file_name);
    if (buf == NULL) {
        return false;
    }

    _tcsncpy(file->path, file_name, _cc_countof(file->path));
    file->path[_cc_countof(file->path) - 1] = 0;

#ifdef _CC_UNICODE_
    _cc_buf_utf8_to_utf16(buf, offset);
    buffer.content = (tchar_t *)buf->bytes;
    buffer.length = _cc_buf_length(buf) / sizeof(tchar_t);
#else
    buffer.content = (tchar_t *)(buf->bytes);
    buffer.length = _cc_buf_length(buf) / sizeof(tchar_t);
#endif

    buffer.offset = 0;
    buffer.line = 1;
    buffer.depth = 0;

    while (_cc_sbuf_access(&buffer)) {
        if (!_cc_buf_jump_comments(&buffer)) {
            return true;
        }
        
        if (_tpl_read(&buffer) == false) {
            return false;
        }
    }
    
    write_file2("/Users/QIU/Github/Mir2Server/Packets/CMD_DBGame.proto.c", &gRoot);
    write_file("/Users/QIU/Github/Mir2Server/Packets/CMD_DBGame.proto.h", &gRoot);

    return true;
}


int main (int argc, char* argv[]) {
    _cc_list_iterator_cleanup(&gRoot);
    _cc_list_iterator_cleanup(&gFile);
    _tpl_open("/Users/QIU/Github/Mir2Server/Packets/CMD_DBGame.tpl");
    return 0;
}
