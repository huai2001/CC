#include <stdio.h>
#include <libcc.h>
#include <libcc/widgets/json/json.h>

#define MAGIC_FIRST 0xBE
#define MAGIC_LAST 0xED
#define CHUNK _CC_16K_BUFFER_SIZE_

#pragma pack(1)
typedef struct wxapkg_header {
    byte_t first_mark;
    uint32_t info;
    uint32_t index_length;
    uint32_t body_length;
    byte_t last_mark;
} wxapkg_header_t;

typedef struct {
    tchar_t name[_CC_MAX_PATH_];
    tchar_t full_name[_CC_MAX_PATH_];
    uint32_t name_length;
    uint32_t offset;
    uint32_t size;
} file_entry_t;

#pragma pack()

static uint32_t file_count = 0;
static file_entry_t entries[100];

void en_wxapkg(const tchar_t *source_path, const tchar_t *dest_file, const _cc_json_t *files) {
    uint32_t i,big_endian;
    struct stat st;
    uint32_t body_length;
    uint32_t index_length;
    wxapkg_header_t header;
    file_entry_t *_entries;

    FILE *fp = _tfopen(dest_file, _T("wb"));
    if (fp == nullptr) {
        return;
    }

    _entries = _cc_calloc(files->length, sizeof(file_entry_t));
    body_length = 0;
    index_length = sizeof(uint32_t);
    for (i = 0; i < files->length; i++) {
        _cc_json_t *item = files->element.uni_array[i];
        file_entry_t *entry = _entries + i;
        entry->name_length = (int32_t)_cc_json_object_find_number(item,"file_name_length");
        memcpy(entry->name,_cc_json_object_find_string(item,"file_name"),entry->name_length * sizeof(tchar_t));
        entry->name[entry->name_length] = 0;
        _cc_fpath(entry->full_name, _cc_countof(entry->full_name), _T("%s%s"), source_path, entry->name);
    
        stat(entry->full_name, &st);
        entry->size = (uint32_t)st.st_size;
        entry->offset = body_length;
        body_length += entry->size;
        index_length += entry->name_length;
        index_length += 12;
    }

    // _cc_logger_debug(_T("open %s"), dest_file);
    // printf("index:%0.4x\n",_cc_swap32(index_length));
    // printf("body:%0.4x\n",_cc_swap32(body_length));

    header.first_mark = MAGIC_FIRST;
    header.info = 0;
    header.index_length = _cc_swap32(index_length);
    header.body_length = _cc_swap32(body_length);
    header.last_mark = MAGIC_LAST;
    
    fwrite(&header, sizeof(header), 1, fp);

    big_endian = _cc_swap32(files->length);
    fwrite(&big_endian, sizeof(uint32_t), 1, fp);

    index_length = index_length + sizeof(wxapkg_header_t);

    for (i = 0; i < files->length; i++) {
        file_entry_t *entry = _entries + i;
        big_endian = _cc_swap32(entry->name_length);
        fwrite(&big_endian,sizeof(uint32_t),1,fp);
        fwrite(&entry->name,sizeof(tchar_t),entry->name_length,fp);
        big_endian = _cc_swap32(entry->offset + index_length);
        fwrite(&big_endian,sizeof(uint32_t),1,fp);
        big_endian = _cc_swap32(entry->size);
        fwrite(&big_endian,sizeof(uint32_t),1,fp);
    }

    for (i = 0; i < files->length; i++) {
        file_entry_t *entry = _entries + i;
        size_t size = entry->size;
        FILE *rfp = _tfopen(entry->full_name, _T("rb"));

        if (rfp == nullptr) {
            _cc_logger_debug(_T("Couldn't open %s"), entry->name);
            continue;
        }

        fseek(fp, index_length + entry->offset, SEEK_SET);

        while (!feof(rfp) && size > 0) {
            byte_t out[CHUNK];
            size_t w;
            size_t l = 0;
            size_t r = size > CHUNK ? CHUNK : size;
            size_t rs = fread(out, sizeof(byte_t), r, rfp);
            if (rs < 0) {
                _cc_logger_debug(_T("error: %s read file fail!"), entry->name);
                break;
            }

            size -= rs;
            while ((w = fwrite(out + l, sizeof(byte_t), rs - l, fp)) > 0) {
                l += w;
                if (l == rs) {
                    break;
                }
            }
        }
        fclose(rfp);
    }
    _cc_free(_entries);
    fclose(fp);
}

void de_wxapkg(const tchar_t *full_name, const tchar_t *name, const tchar_t *save_path, _cc_json_t *json) {
    wxapkg_header_t header;
    file_entry_t *_entries;
    uint32_t i ;
    uint32_t file_count;
#ifdef _CC_UNICODE_
    char_t file_name[_CC_MAX_PATH_];
#endif
    _cc_json_t *item, *files;
    FILE *wf;
    FILE *fp = _tfopen(full_name, _T("rb"));
    if (fp == nullptr) {
        return;
    }

    fread(&header, sizeof(wxapkg_header_t), 1, fp);

    if (header.first_mark != MAGIC_FIRST || header.last_mark != MAGIC_LAST) {
        byte_t vimmwx[] = {'V','1','M','M','W','X'};
        if (memcmp(&header.first_mark, vimmwx, sizeof(vimmwx)) == 0) {
            _cc_logger_debug(_T("error: %s It is an encrypted wxapkg file!"), name);
            return;
        }
        _cc_logger_debug(_T("error: %s its not a wxapkg file!"), name);
        return;
    }

    header.index_length = _cc_swap32(header.index_length);
    header.body_length = _cc_swap32(header.body_length);

    fread(&file_count, sizeof(uint32_t), 1, fp);
    file_count = _cc_swap32(file_count);

    // _cc_logger_debug(_T("open %s"), name);
    // printf("index:%0.4x\n",_cc_swap32(header.index_length));
    // printf("body:%0.4x\n",_cc_swap32(header.body_length));
    
    _cc_json_add_string(json, "file_name", name,true);
    files = _cc_json_alloc_array("files",file_count);
    _cc_json_object_push(json, files, true);

    _entries = _cc_calloc(file_count, sizeof(file_entry_t));

    for (i = 0; i < file_count; i++) {
        uint32_t big_endian;
        file_entry_t *entry = _entries + i;
        fread(&big_endian, sizeof(uint32_t), 1, fp);
        entry->name_length = _cc_swap32(big_endian);

#ifdef _CC_UNICODE_
        fread(file_name, sizeof(char_t), entry->name_length, fp);
        _cc_utf8_to_utf16((uint8_t*)file_name, (uint8_t*)&file_name[entry->name_length], (uint16_t*)entry->name, (uint16_t*)&entry->name[_cc_countof(entry->name)], FALSE);
#else
        fread(entry->name, sizeof(char_t), entry->name_length, fp);;
#endif
        entry->name[entry->name_length] = 0;

        fread(&big_endian, sizeof(uint32_t), 1, fp);
        entry->offset = _cc_swap32(big_endian);
        fread(&big_endian, sizeof(uint32_t), 1, fp);
        entry->size = _cc_swap32(big_endian);

        item = _cc_json_alloc_object(_CC_JSON_OBJECT_,nullptr);
        _cc_json_add_string(item, "file_name", entry->name, true);
        _cc_json_add_number(item, "file_name_length", entry->name_length, true);
        _cc_json_array_push(files, item);

        _tprintf(_T("name:%s, offset:%d, size:%d\n"), entry->name, entry->offset, entry->size);

        _cc_fpath(entry->full_name, _cc_countof(entry->full_name), _T("%s%s"), save_path, entry->name);
        _cc_mkdir(entry->full_name);
    }

    for (i = 0; i < file_count; i++) {
        size_t size;
        file_entry_t *entry = _entries + i;
        wf = _tfopen(entry->full_name, _T("wb"));
        if (wf) {
            byte_t out[CHUNK];
            fseek(fp, entry->offset, SEEK_SET);
            size = entry->size;
            while (!feof(fp) && size > 0) {
                size_t w;
                size_t l = 0;
                size_t rs;
                size_t r = size > CHUNK ? CHUNK : size;
                rs = fread(out, sizeof(byte_t), r, fp);
                if (rs < 0) {
                    _cc_logger_debug(_T("error: %s read file fail!"), entry->name);
                    return;
                }

                size -= rs;
                while ((w = fwrite(out + l, sizeof(byte_t), rs - l, wf)) > 0) {
                    l += w;
                    if (l == rs) {
                        break;
                    }
                }
            }
            fclose(wf);
        }
    }
    fclose(fp);
    _cc_free(_entries);
}

void finder(const tchar_t* source_path, size_t offset) {
    tchar_t fpath[_CC_MAX_PATH_];
    size_t length;
    DIR* dir;
    struct dirent* d;

    dir = opendir(source_path);
    if (dir == nullptr) {
        return;
    }

    while ((d = readdir(dir)) != nullptr) {
        //
        if (d->d_type == DT_DIR &&
            ((d->d_name[0] == '.' && d->d_name[1] == 0) ||
             (d->d_name[0] == '.' && d->d_name[1] == '.' &&
              d->d_name[2] == 0))) {
            continue;
        }

        length = _sntprintf(fpath, _cc_countof(fpath), _T("%s\\%s"), source_path, d->d_name);

        if (d->d_type == DT_DIR) {
            finder(fpath,offset);
        } else {
            file_entry_t *entry = &entries[file_count++];
            memcpy(entry->name, fpath + offset, (length - offset)  * sizeof(tchar_t));
            memcpy(entry->full_name, fpath, length  * sizeof(tchar_t));
        }

        if (file_count >= _cc_countof(entries)) {
            break;
        }
    }
    closedir(dir);
}
/*Print the usage message.*/
static int print_usage(void) {
    return fprintf(stdout,
                   _T("Usage:wxapkg [-e,-d]... [-f[File],-r[dir]]...-o[File]\n"));
}

int main(int argc, char* const argv[]) {
    int i;
    const char* src = nullptr;
    const char* dest = nullptr;
    int m = 0,r = 0;

    tchar_t path[_CC_MAX_PATH_];
    const tchar_t *file_name;
    const _cc_json_t *files;
    _cc_buf_t *buf;
    _cc_json_t *root;
    _cc_json_t *item;

    if (argc <= 3) {
        print_usage();
        return 0;
    }

    for (i = 0; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
                case 'e':
                case 'E':
                    m = 1;
                    break;
                case 'd':
                case 'D':
                    m = 2;
                    break;
                case 'f':
                case 'F':
                    if (argv[i][2] == 0) {
                        if ((i + 1) < argc) {
                            src = argv[i + 1];
                        }
                    } else {
                        src = &argv[i][2];
                    }
                    r = 0;
                    break;
                case 'r':
                case 'R':
                    if (argv[i][2] == 0) {
                        if ((i + 1) < argc) {
                            src = argv[++i];
                        }
                    } else {
                        src = &argv[i][2];
                    }
                    r = 1;
                    break;
                case 'o':
                case 'O':
                    if (argv[i][2] == 0) {
                        if ((i + 1) < argc) {
                            dest = argv[++i];
                        }
                    } else {
                        dest = &argv[i][2];
                    }
                    break;
            }
        }
    }

    if (m == 0 || src == nullptr || dest == nullptr) {
        print_usage();
        return 1;
    }

    if (m == 1) {
        _cc_fpath(path, _cc_countof(path), _T("%s\\wxapkg.json"), src);
        root = _cc_json_from_file(path);
        if (root == nullptr) {
            return 0;
        }
        for (i = 0; i < root->length; i++) {
            item = root->element.uni_array[i];
            file_name = _cc_json_object_find_string(item, "file_name");
            files = _cc_json_object_find_array(item,"files");
            if (files) {
                _cc_fpath(path, _cc_countof(path), _T("%s%s"), dest, file_name);
                _cc_mkdir(path);
                en_wxapkg(src, path, files);
            }
        }
    } else if (m == 2) {
        file_count = 0;
        if (r) {
            finder(src, _tcslen(src));
        } else {
            const tchar_t *p = _tcsrchr(src,'\\');
            if (p == nullptr) {
                p = _tcsrchr(src,'/');
            }

            if (p == nullptr) {
                p = src;
            }

            file_entry_t *entry = &entries[file_count++];
            _tcsncpy(entry->name, p, _cc_countof(entry->name));
            _tcsncpy(entry->full_name, src, _cc_countof(entry->full_name));
            entry->name[_cc_countof(entry->name) - 1] = 0;
            entry->full_name[_cc_countof(entry->full_name) - 1] = 0;
        }

        root = _cc_json_alloc_array(nullptr, 32);
        for (i = 0; i < file_count; i++) {
            file_entry_t *entry = &entries[i];
            item = _cc_json_alloc_object(_CC_JSON_OBJECT_,nullptr);
            de_wxapkg(entry->full_name, entry->name, dest, item);
            _cc_json_array_push(root, item);
        }

        buf = _cc_json_dump(root);
        if (buf) {
            FILE *fp;
            _cc_fpath(path, _cc_countof(path), _T("%s\\wxapkg.json"), dest);
            fp = _tfopen(path,"w");
            if (fp) {
                fwrite(buf->bytes, sizeof(byte_t), buf->length, fp);
                fclose(fp);
            }
        }
    }
    return 0;
}