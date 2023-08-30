#include <stdio.h>
#include <libcc.h>

#pragma pack(1)
typedef struct wxapkg_header {
    byte_t first_mark;
    uint32_t info;
    uint32_t index_length;
    uint32_t body_length;
    byte_t last_mark;
    uint32_t file_count;
} wxapkg_header_t;

#pragma pack()

#define CHUNK 1024 * 16
FILE *flog;
static int32_t positioning = 0;

void wxapkg(tchar_t *file, const tchar_t *save_path, const tchar_t *source_file) {
    wxapkg_header_t header;
    uint32_t i ;
    char_t file_name[_CC_MAX_PATH_];
    tchar_t szFilePath[_CC_MAX_PATH_];
    FILE *wf;
    FILE *fp = _tfopen(file, _T("rb"));
    if (fp == NULL) {
        return ;
    }
    //_ftprintf(flog,_T("[%s]\n"), file + positioning);
    _ftprintf(flog,_T("pc_wxapkg_decrypt.exe -wxid wx745f0f407ef0858b -in %s  -out %s\\%s\n"), file, save_path, file + positioning);

    fread(&header, sizeof(wxapkg_header_t), 1, fp);
    if (header.first_mark != 0xBE || header.last_mark != 0xED) {
        byte_t vimmwx[] = {'V','1','M','M','W','X'};
        if (memcmp(&header.first_mark, vimmwx, sizeof(vimmwx)) == 0) {
            _cc_logger_debug(_T("error: %s It is an encrypted wxapkg file!"), source_file);
            return ;
        }
        _cc_logger_debug(_T("error: %s its not a wxapkg file!"), source_file);
        return ;
    }
    header.file_count = _cc_swap32(header.file_count);

    for (i = 0; i < header.file_count; i++) {
        uint32_t name_length;
        uint32_t offset;
        uint32_t curr_offset;
        size_t size;
#ifdef _CC_UNICODE_
        wchar_t wfile_name[_CC_MAX_PATH_];
#endif
        fread(&name_length, sizeof(uint32_t), 1, fp);
        name_length = _cc_swap32(name_length);
           
        fread(file_name, sizeof(char_t), name_length, fp);
        file_name[name_length] = 0;
#ifdef _CC_UNICODE_
        _cc_utf8_to_utf16((uint8_t*)file_name, (uint8_t*)&file_name[name_length], (uint16_t*)wfile_name, (uint16_t*)&wfile_name[_CC_MAX_PATH_], FALSE);
        file = wfile_name;
#else
        file = file_name;
#endif
        fread(&offset, sizeof(uint32_t), 1, fp);
        fread(&size, sizeof(uint32_t), 1, fp);
        offset = _cc_swap32(offset);
        size = _cc_swap32(size);
        _tprintf(_T("{name:%s, offset:%ld, size:%ld\n"), file, offset, size);
        _ftprintf(flog,_T("{name:%s, offset:%ld, size:%ld\n"), file, offset, size);
        curr_offset = ftell(fp);

        _sntprintf(szFilePath, _cc_countof(szFilePath), _T("%s//%s"), save_path, file);
        _cc_realpath(szFilePath);
        _cc_mkdir(szFilePath);
        wf = _tfopen(szFilePath, _T("wb"));
        if (wf) {
            byte_t out[CHUNK];
            fseek(fp, offset, SEEK_SET);
            while (!feof(fp) && size > 0) {
                size_t writeSize;
                size_t left = 0;
                size_t readSize;
                size_t r = size > CHUNK ? CHUNK : size;
                readSize = fread(out, sizeof(byte_t), r, fp);
                size -= readSize;

                while ((writeSize = fwrite(out + left, sizeof(byte_t), readSize - left, wf)) > 0) {
                    left += writeSize;
                    if (left == readSize) {
                        break;
                    }
                }
                
                if (readSize != left) {
                    break;
                }
            }
            fclose(wf);
        }
        fseek(fp, curr_offset, SEEK_SET);
    }


    fclose(fp);
}

void finder(const tchar_t* source_path, const tchar_t* save_path) {
    tchar_t fpath[_CC_MAX_PATH_];
    DIR* dir;
    struct dirent* d;

    dir = opendir(source_path);
    if (dir == NULL) {
        return;
    }

    while ((d = readdir(dir)) != NULL) {
        //
        if (d->d_type == DT_DIR &&
            ((d->d_name[0] == '.' && d->d_name[1] == 0) ||
             (d->d_name[0] == '.' && d->d_name[1] == '.' &&
              d->d_name[2] == 0))) {
            continue;
        }

        _sntprintf(fpath, _cc_countof(fpath), _T("%s\\%s"), source_path, d->d_name);

        if (d->d_type == DT_DIR) {
            //finder(fpath, save_path);
        } else {
            _tprintf(_T("decoding:%s, %s\n"), fpath, fpath + positioning);
			wxapkg(fpath, save_path, fpath + positioning);
        }
    }
    closedir(dir);
}

int _tmain (int argc, tchar_t * const argv[]) {
    flog = _tfopen(_T("D:\\wx\\317\\wxapkg.log"), _T("w"));
    positioning = _tcslen(_T("D:\\wx\\wx745f0f407ef0858b\\316"));

    finder(_T("D:\\wx\\wx745f0f407ef0858b\\318"), _T("D:\\wx\\wx745f0f407ef0858b\\316"));

    return 0;
}
