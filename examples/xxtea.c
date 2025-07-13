#include <libcc/alloc.h>
#include <libcc/base64.h>
#include <libcc/buf.h>
#include <libcc/dirent.h>
#include <libcc/md5.h>
#include <libcc/xxtea.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

byte_t* keys = (byte_t*)"a3a2836d-0a32-1c";
#if 0
int main (int argc, char * const argv[]) {
    const char *text = "Hello World! 你好，中国！";
    size_t len;
    size_t outlen = 0;

    tchar_t base64_en[256];
    byte_t *encrypt_data = _cc_xxtea_encrypt((byte_t*)text, (int32_t)strlen(text), keys, &len);

    outlen = _cc_base64_encode((byte_t*)encrypt_data, (int32_t)(len * sizeof(tchar_t)), base64_en, 256);
    printf("%s\n", base64_en);
    byte_t* decrypt_data = _cc_xxtea_decrypt(encrypt_data, len, keys, &len);
    if (strncmp(text, (char*)decrypt_data, len) == 0) {
        printf("success:%s!\n",decrypt_data);
    }
    else {
        printf("fail!\n");
    }
    free(encrypt_data);
    free(decrypt_data);
    return 0;
}
#endif

#define CHUNK 16384

_CC_FORCE_INLINE_ uint64_t fileCheck(const tchar_t* fileName, tchar_t* output) {
    byte_t t = 0;
    const tchar_t hex[] = _T("0123456789abcdef");
    byte_t md[_CC_MD5_DIGEST_LENGTH_];
    byte_t buf[CHUNK];
    int32_t i;
    _cc_md5_t c;
    FILE* fp = _tfopen(fileName, _T("rb"));
    uint64_t fileSize;

    if (fp == nullptr)
        return 0;

    _cc_md5_init(&c);

    fseek(fp, 0, SEEK_SET);

    fileSize = 0;
    while ((i = (int32_t)fread(buf, sizeof(byte_t), _cc_countof(buf), fp))) {
        _cc_md5_update(&c, buf, (unsigned long)i);
        fileSize += i;
    }
    _cc_md5_final(&c, &(md[0]));

    fclose(fp);

    if (output) {
        int index = 0;
        for (i = 0; i < _CC_MD5_DIGEST_LENGTH_; i++) {
            t = md[i];
            output[index++] = hex[t / 16];
            output[index++] = hex[t % 16];
        }
        output[index] = _T('\0');
    }
    return fileSize;
}

_CC_FORCE_INLINE_ bool_t _mk(const tchar_t* path) {
    int32_t i = 0;
    const tchar_t* cp = nullptr;
    tchar_t cpath[_CC_MAX_PATH_];

    //
    cp = path;
    /* Skip the first / */
    if (*cp == _CC_T_SLASH_C_) {
        cpath[i++] = *cp++;
    }

    /**/
    while (*cp) {
        if (*cp == _CC_T_SLASH_C_) {
            cpath[i] = 0;
            if (_taccess(cpath, 0) != 0) {
                _tmkdir(cpath);
            }
        }
        cpath[i++] = *cp++;
    }
    return true;
}

#define CHUNK_SOURCE 1024
#define CHUNK_DEST 16384
_CC_FORCE_INLINE_ _cc_buf_t* _gzip_def(const tchar_t* source_file,
                                       int level,
                                       size_t file_size) {
    int res, flush;
    z_stream strm;
    _cc_buf_t* buf;

    byte_t source[CHUNK_SOURCE];
    byte_t dest[CHUNK_DEST];

    FILE* fp;

    fp = _tfopen(source_file, _T("rb"));
    if (fp == nullptr) {
        return nullptr;
    }

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    // res = deflateInit(&strm, level);
    res = deflateInit2(&strm, level, Z_DEFLATED, MAX_WBITS + 16, MAX_MEM_LEVEL,
                       Z_DEFAULT_STRATEGY);
    if (res != Z_OK) {
        return nullptr;
    }

    buf = _cc_create_buf(file_size);
    if (buf == nullptr) {
        goto DEF_FIAL;
    }
    /* compress until end of file */
    do {
        strm.avail_in = (uint32_t)fread(source, 1, CHUNK_SOURCE, fp);
        if (ferror(fp)) {
            goto DEF_FIAL;
        }

        flush = feof(fp) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = (Bytef*)source;

        do {
            strm.avail_out = CHUNK_DEST;
            strm.next_out = (Bytef*)dest;
            /* no bad return value */
            res = deflate(&strm, flush);
            _cc_assert(res != Z_STREAM_ERROR);
            _cc_buf_append(buf, dest, CHUNK_DEST - strm.avail_out);
        } while (strm.avail_out == 0);
        /* all input will be used */
        _cc_assert(strm.avail_in == 0);

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    /* stream will be complete */
    _cc_assert(res == Z_STREAM_END);

    /* clean up and return */
    (void)deflateEnd(&strm);
    fclose(fp);

    return buf;

DEF_FIAL:
    /* clean up and return */
    (void)deflateEnd(&strm);
    fclose(fp);

    if (buf) {
        _cc_destroy_buf(&buf);
    }

    return nullptr;
}

_CC_FORCE_INLINE_ int _gzip_inf(const tchar_t* dest_file,
                                byte_t* source,
                                size_t length) {
    int res;
    size_t have, left = 0;
    size_t bytes_write, left_write;
    z_stream strm;
    byte_t out[CHUNK_DEST];
    FILE* fp;

    fp = _tfopen(dest_file, _T("wb"));
    if (fp == nullptr) {
        return Z_DATA_ERROR;
    }

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    res = inflateInit2(&strm, 47);
    if (res != Z_OK) {
        return res;
    }

    /* decompress until deflate stream ends or end of file */
    do {
        have = (length - left);
        strm.next_in = (Bytef*)source + left;
        strm.avail_in = have > CHUNK_SOURCE ? CHUNK_SOURCE : have;
        _cc_assert(strm.avail_in != 0);
        left += strm.avail_in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK_DEST;
            strm.next_out = (Bytef*)out;
            res = inflate(&strm, Z_NO_FLUSH);
            /* state not clobbered */
            _cc_assert(res != Z_STREAM_ERROR);
            switch (res) {
                case Z_NEED_DICT:
                    res = Z_DATA_ERROR; /* and fall through */
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    goto INF_FIAL;
                    break;
            }

            have = CHUNK_DEST - strm.avail_out;
            left_write = 0;
            do {
                bytes_write = fwrite(out + left_write, sizeof(byte_t),
                                     have - left_write, fp);
                if (bytes_write <= 0) {
                    goto INF_FIAL;
                }
                left_write += bytes_write;
            } while (have != left_write);
        } while (strm.avail_out == 0);
        /* done when inflate() says it's done */
    } while (res != Z_STREAM_END);

INF_FIAL:
    /* clean up and return */
    (void)inflateEnd(&strm);
    fclose(fp);
    return res == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

void _xxtea_decrypt_file(const tchar_t* source_path, const tchar_t* save_path) {
    _cc_buf_t* fdata;
    byte_t* output;
    size_t output_length;

    fdata = _cc_buf_from_file(source_path);

    if (fdata) {
        output = _cc_xxtea_decrypt(fdata->bytes, fdata->length, keys,
                                   &output_length);
        if (output) {
            int res = _gzip_inf(save_path, output, output_length);
            if (res != Z_OK) {
                printf("_gzip_inf fail. %s\n", source_path);
            } else {
                printf("successfully. %s\n", save_path);
            }
            _cc_free(output);
        } else {
            printf("_cc_xxtea_decrypt fail. %s\n", source_path);
        }
        _cc_destroy_buf(&fdata);
    }
}

void _xxtea_encrypt_file(const tchar_t* source_path, const tchar_t* save_path) {
    _cc_buf_t* fdata;
    byte_t* output;
    size_t output_length;

    _cc_file_t* w;
    w = _cc_open_file(save_path, _T("w"));
    if (w == nullptr) {
        return;
    }

    fdata = _gzip_def(source_path, Z_DEFAULT_COMPRESSION, CHUNK_DEST);

    if (fdata) {
        size_t left;
        size_t bytes_write;
        output_length = 0;
        output = _cc_xxtea_encrypt(fdata->bytes, fdata->length, keys,
                                   &output_length);
        if (output && output_length > 0) {
            left = 0;
            do {
                bytes_write = _cc_file_write(w, output + left, sizeof(byte_t),
                                             output_length - left);
                if (bytes_write <= 0) {
                    break;
                }
                left += bytes_write;
            } while (output_length != left);

            _cc_file_close(w);
            _cc_free(output);
            if (output_length == left) {
                printf("successfully. %s\n", save_path);
            } else {
                printf("fwrite fail. %s\n", save_path);
            }
        } else {
            printf("_cc_xxtea_encrypt fail. %s\n", source_path);
        }
        _cc_destroy_buf(&fdata);
    } else {
        printf("_gzip_def fail. %s\n", source_path);
    }
}

void finder(const tchar_t* source_path, const tchar_t* save_path) {
    tchar_t fpath[_CC_MAX_PATH_];
    tchar_t spath[_CC_MAX_PATH_];
    tchar_t* ext;
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

        _sntprintf(fpath, _cc_countof(fpath), _T("%s/%s"), source_path,
                   d->d_name);
        _sntprintf(spath, _cc_countof(spath), _T("%s/%s"), save_path,
                   d->d_name);

        if (d->d_type == DT_DIR) {
            finder(fpath, spath);
        } else {
            ext = _tcsrchr(spath, '.');
            if (ext && _tcsicmp(".jsc", ext) == 0) {
                _mk(spath);
                *(ext + 3) = 0;
                _xxtea_decrypt_file(fpath, spath);
            }
        }
    }
    closedir(dir);
}

/*Print the usage message.*/
static int print_usage(void) {
    return fprintf(stdout,
                   _T("Usage:xxtea [-e,-d]... -f[File]...-o[File]\n"));
}


static void _injectJS(const tchar_t *source_path, const tchar_t* save_path) {
    _cc_buf_t *buf;
    _cc_buf_t *inject_file;
    _cc_buf_t modify;
    tchar_t *packageUrl;
    size_t pos = 0;
    FILE *wf;

    // 解压注入
    inject_file = _cc_buf_from_file(_T("./inject.js"));
    if (inject_file == nullptr) {
        return;
    }

    buf = _cc_buf_from_file(source_path);
    if (buf == nullptr) {
        return;
    }
    _cc_buf_alloc(&modify, buf->length + 2480 + inject_file->length);
    packageUrl = _tcsstr((tchar_t *)buf->bytes, "updateManifestUrls: function(");
    if (packageUrl) {
        _cc_AString_t updateManifestUrls = _cc_String(
            "updateManifestUrls: function(e, t, n){return this.old_updateManifestUrls(e,dyad.packageUrl,n);},old_");
        pos = (size_t)(packageUrl - (tchar_t *)buf->bytes);
        _cc_buf_append(&modify, buf->bytes, pos);
        _cc_buf_append(&modify, updateManifestUrls.data, updateManifestUrls.length);
        _cc_buf_append(&modify, buf->bytes + pos, buf->length - pos);
    }

    _cc_buf_append(&modify, inject_file->bytes, inject_file->length);

    wf = _tfopen(source_path, _T("wb"));
    if (wf) {
        fwrite(modify.bytes, sizeof(byte_t), modify.length, wf);
        fclose(wf);
    }

    _xxtea_encrypt_file(source_path, save_path);
    _cc_buf_free(&modify);
    _cc_destroy_buf(&buf);
    _cc_destroy_buf(&inject_file);
}

int main(int argc, char* const argv[]) {
    int i;
    const char* src = nullptr;
    const char* dest = nullptr;
    int m = 0,r = 0;

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
                case 'i':
                case 'I':
                    m = 3;
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
        tchar_t md[33];
        uint64_t fileSize;
        _xxtea_encrypt_file(src, dest);
        fileSize = fileCheck(dest, md);
        printf("MD5:%s\n", md);
        printf("fileSize:%lld\n", fileSize);
    } else if (m == 2) {
        if (r == 1) {
            finder(src, dest);
        } else {
            _xxtea_decrypt_file(src, dest);
        }
    } else if (m == 3) {
        _xxtea_decrypt_file(src, dest);
        _injectJS(dest, src);
    }

    /*finder("/Users/Desktop/Inject/Android-Inject/WePoker",
     "/Users/Desktop/Inject/Android-Inject/WePoker2");*/
    return 0;
}
