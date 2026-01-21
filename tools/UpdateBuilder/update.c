#include <stdio.h>
#include "UpdateBuilder.h"
#include <zlib.h>

#define CHUNK_SOURCE (1024 * 6)
#define CHUNK_DEST (1024 * 8)

/* Compress from file source to file dest until EOF on source.
 def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
 allocated for processing, Z_STREAM_ERROR if an invalid compression
 level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
 version of the library linked do not match, or Z_ERRNO if there is
 an error reading or writing the files. */
static int def(FILE *source, FILE *dest, int level, uint64_t *resultSize) {
    int res, flush;
    unsigned have;
    z_stream strm;
    byte_t in_buffer[CHUNK_SOURCE];
    byte_t out_buffer[CHUNK_DEST];
    uint64_t r;
    
    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    res = deflateInit(&strm, level);
    //res = deflateInit2(&strm, level, Z_DEFLATED, MAX_WBITS + 16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (res != Z_OK) {
        return res;
    }
    
    /* compress until end of file */
    r = 0;
    do {
        strm.avail_in = (uint32_t)fread(in_buffer, 1, CHUNK_SOURCE, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }

        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = (Bytef*)in_buffer;
        
        /* run deflate() on input until output buffer not full, finish compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK_DEST;
            strm.next_out = (Bytef*)out_buffer;
            res = deflate(&strm, flush);        /* no bad return value */
            _cc_assert(res != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK_DEST - strm.avail_out;
            if (fwrite(out_buffer, sizeof(byte_t), have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
            r += have;
        } while (strm.avail_out == 0);
        /* all input will be used */
        _cc_assert(strm.avail_in == 0);
        
        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    /* stream will be complete */
    _cc_assert(res == Z_STREAM_END);

    if (resultSize) {
        *resultSize = r;
    }
    
    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}

/* report a zlib or i/o error */
static void zerr(int res) {
    switch (res) {
        case Z_ERRNO:
            if (ferror(stdin))
                _cc_logger(_CC_LOG_LEVEL_ERROR_, "zpipe: error reading stdin");
            if (ferror(stdout))
                _cc_logger(_CC_LOG_LEVEL_ERROR_, "zpipe: error reading stdout");
            break;
        case Z_STREAM_ERROR:
            _cc_logger(_CC_LOG_LEVEL_ERROR_, "zpipe: invalid compression level");
            break;
        case Z_DATA_ERROR:
            _cc_logger(_CC_LOG_LEVEL_ERROR_, "zpipe: invalid or incomplete deflate data");
            break;
        case Z_MEM_ERROR:
            _cc_logger(_CC_LOG_LEVEL_ERROR_, "zpipe: out of memory");
            break;
        case Z_VERSION_ERROR:
            _cc_logger(_CC_LOG_LEVEL_ERROR_, "zpipe: zlib version mismatch");
    }
}

static int compressZipFile(const char *source, const char *dest, uint64_t *resultSize) {
    int res;
    FILE *filein, *fileout;
    
    if ((filein = fopen(source, "rb")) == NULL) {
        _cc_logger_error("Can\'t open %s!", source);
        return -1;
    }
    
    if ((fileout = fopen(dest, "wb")) == NULL) {
        _cc_logger_error("Can\'t open %s!\n", dest);
        fclose(filein);
        return -1;
    }
    
    /* do compression if no arguments */
    res = def(filein, fileout, Z_DEFAULT_COMPRESSION, resultSize);
    if (res != Z_OK) {
        zerr(res);
    }
    
    fclose(filein);
    fclose(fileout);
    
    return res;
}

static bool_t isCompressFile(tchar_t *name, int32_t namlen) {
    char a = _tolower(*(name + namlen - 3));
    char b = _tolower(*(name + namlen - 2));
    char c = _tolower(*(name + namlen - 1));

    if (a == 'z' &&
        b == 'i'&&
        c == 'p') {
        return true;
    }

    if (a == 'r' &&
        b == 'a'&&
        c == 'r') {
        return true;
    }

    if (a == '.' &&
        b == '7'&&
        c == 'z') {
        return true;
    }
    
    return false;
}

static uint64_t file_check(const tchar_t *fileName, tchar_t *output) {
    byte_t t = 0;
    byte_t md[_CC_MD5_DIGEST_LENGTH_];
    byte_t buf[CHUNK_SOURCE];
    int32_t i;
    _cc_md5_t c;
    FILE *fp = _tfopen(fileName, _T("rb"));
    uint64_t fileSize = 0;

    if (fp == NULL)
        return 0;

    _cc_md5_init(&c);

    fseek(fp, 0, SEEK_SET);

    while ((i = (int32_t)fread (buf, sizeof(byte_t), _cc_countof(buf), fp))) {
        _cc_md5_update(&c, buf, (unsigned long)i);
        fileSize += i;
    }

    _cc_md5_final(&c, &(md[0]));

    if (output) {
        int index = 0;
        for (i = 0; i < _CC_MD5_DIGEST_LENGTH_; i++) {
            t = md[i];
            output[index++] = _lower_xdigits[t / 16];
            output[index++] = _lower_xdigits[t % 16];
        }
        output[index] = _T('\0');
    }

    return fileSize;

}
#ifndef __CC_WINDOWS__
static void CopyFile(const tchar_t *source, const tchar_t *dest) {
    bool_t err = false;
    FILE *fw = NULL;
    FILE* fr = _tfopen(source, _T("rb"));
    if (fr) {
        fw = _tfopen(dest, _T("wb"));
        if (fw) {
            byte_t out[CHUNK_SOURCE];
            while (!feof(fr)) {
                size_t writeSize = 0;
                size_t left = 0;
                size_t readSize = fread(out, sizeof(byte_t), CHUNK_SOURCE, fr);
                
                while ((writeSize = fwrite(out + left, sizeof(byte_t), readSize - left, fw)) > 0) {
                    left += writeSize;
                    if (left == readSize) {
                        break;
                    }
                }
                
                if (readSize != left) {
                    err = true;
                    break;
                }
            }
            fclose(fw);
        }
        fclose(fr);
    }
    
    if (err) {
        _tunlink(dest);
    } 
}
#else
#undef CopyFile
#define CopyFile(src, dst) CopyFileA(src, dst, false);
#endif

int createUpdateFile(const tchar_t *,_cc_sql_t *);

int builder_update(void) {
    _cc_sql_result_t *result = NULL;
    _cc_sql_result_t *resultUpdated = NULL;
    tchar_t name[64];
    tchar_t path[256];
    tchar_t token[33];
    tchar_t check[33];
    int32_t id = 0;
    uint64_t fileSize = 0;
    uint64_t resultSize = 0;

    _cc_sql_t *sql = open_sqlite3();

    if (sql == NULL) {
        return 1;
    }

    _CC_STRING_SET("abc");
    sql_delegator.execute(sql, _CC_SQL("UPDATE `FileList` SET `CheckMD5`=?, `Compress`=?, `CompressSize`=?, `Size`=? WHERE `ID`=?;"), &resultUpdated);
    sql_delegator.execute(sql, _CC_SQL("select `ID`, `Name`, `CheckMD5`, `Path` from `FileList`;"), &result);
    while (sql_delegator.fetch(result)) {
        int32_t is_compress = 0;
        tchar_t file[_CC_MAX_PATH_ * 4];

        id = sql_delegator.get_int(result, 0);
        sql_delegator.get_string(result, 1, name, 64);
        sql_delegator.get_string(result, 3, path, 256);
        
        _sntprintf(file, _cc_countof(file), _T("%s%s"), source_directory, path);

        fileSize = file_check(file, token);
        if (fileSize == 0) {
            _cc_sql_result_t *deleter;
            if(sql_delegator.execute(sql, _CC_SQL("DELETE FROM `FileList` WHERE `ID`=?;"), deleter)) {
                sql_delegator.bind(deleter, 0, &id, sizeof(int32_t), _CC_SQL_TYPE_INT32_);
                sql_delegator.step(sql, deleter);
                sql_delegator.free_result(sql, deleter);
            }
            continue;
        }

        sql_delegator.get_string(result, 2, check, 33);
        if (_tcsnicmp(check, token, 33) == 0) {
            continue;
        }
        
        _tcscat(updateDirectory + updateDirectoryLen - 1, path);
        _cc_mkdir(updateDirectory);

        if (!isCompressFile(name, strlen(name))) {
            resultSize = 0;
            if (compressZipFile(sourceDirectory, updateDirectory, &resultSize) == 0) {
                isCompress = 1;
            } else {
                resultSize = fileSize;
            }
        }

        if (!isCompress) {
            CopyFile(sourceDirectory, updateDirectory);
        }
        sql_delegator.bind(resultUpdated, 0, &token,32,_CC_SQL_TYPE_STRING_);
        sql_delegator.bind(resultUpdated, 1, &isCompress, sizeof(int32_t), _CC_SQL_TYPE_INT32_);
        sql_delegator.bind(resultUpdated, 2, &resultSize, sizeof(int64_t), _CC_SQL_TYPE_INT64_);
        sql_delegator.bind(resultUpdated, 3, &fileSize, sizeof(int64_t), _CC_SQL_TYPE_INT64_);
        sql_delegator.bind(resultUpdated, 4, &id, sizeof(int32_t), _CC_SQL_TYPE_INT32_);
        sql_delegator.step(sql, resultUpdated);
        sql_delegator.reset(sql, resultUpdated);
        printf("%s\t(%s)\n",&sourceDirectory[sourceDirectoryLen],token);
    }
    sql_delegator.free_result(sql, resultUpdated);
    puts("更新完成\n");
    if (result) {
        sql_delegator.free_result(sql, result);
    }

    updateDirectory[updateDirectoryLen] = 0;
    _tcscat(updateDirectory + updateDirectoryLen - 1, _T("/project.manifest"));

    createUpdateFile(updateDirectory, sql);

    closeSQLite3(sql);

    return 0;
}

int createUpdateFile(const tchar_t *saveFile, _cc_sql_t *sql) {
    char_t str[256];
    _cc_sql_result_t *result = NULL;
    _cc_buf_t buf;
    _cc_sql_t *sql = openSQLite3();
    _cc_json_t *rootJSON = _cc_json_alloc_object(_CC_JSON_ARRAY_, NULL);
    sql_delegator.execute(sql, _CC_SQL("select `ID`, `Name`, `CheckMD5`, `Compress`, `CompressSize`, `Size`, `Path` from `main`.`FileList`;"), &result);
    while (sql_delegator.fetch(result)) {
        _cc_json_t *json = _cc_json_alloc_object(_CC_JSON_OBJECT_, NULL);
        if (json) {
            _cc_json_add_number(json, "ID",  sql_delegator.get_int(result, 0));
            sql_delegator.get_string(result, 1, str, 256);
            _cc_json_add_string(json, "Name", str, true);
            sql_delegator.get_string(result, 2, str, 256);
            _cc_json_add_string(json, "MD5", str, true);
            sql_delegator.get_string(result, 6, str, 256);
            _cc_json_add_string(json, "Path", str, true);

            _cc_json_add_number(json, "Compress", sql_delegator.get_int(result, 3));
            _cc_json_add_number(json, "CompressSize", sql_delegator.get_int64(result, 4));
            _cc_json_add_number(json, "Size", sql_delegator.get_int64(result, 5));
            _cc_json_object_push(rootJSON, json, true);
        }
    }
    
    if (result) {
        sql_delegator.free_result(sql, result);
    }

    _cc_json_dump(rootJSON, buf);
    _cc_file_t *fp = _cc_open_file(saveFile, _T("wb"));
    if (fp) {
        _cc_file_write(fp, buf.bytes, 1, buf.length);
        _cc_file_close(fp);
    }
    _cc_free_buf(&buf);
    _cc_free_json(rootJSON);
    return 0;
}