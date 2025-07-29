#include <stdio.h>
#include "UpdateBuilder.h"
#include <libcc/json/json.h>
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
                _cc_logger_error(_T("zpipe: error reading stdin"));
            if (ferror(stdout))
                _cc_logger_error(_T("zpipe: error reading stdout"));
            break;
        case Z_STREAM_ERROR:
            _cc_logger_error(_T("zpipe: invalid compression level"));
            break;
        case Z_DATA_ERROR:
            _cc_logger_error(_T("zpipe: invalid or incomplete deflate data"));
            break;
        case Z_MEM_ERROR:
            _cc_logger_error(_T("zpipe: out of memory"));
            break;
        case Z_VERSION_ERROR:
            _cc_logger_error(_T("zpipe: zlib version mismatch"));
    }
}

static int compressZipFile(const char *source, const char *dest, uint64_t *resultSize) {
    int res;
    FILE *filein, *fileout;
    
    if ((filein = fopen(source, "rb")) == nullptr) {
        _cc_logger_error(_T("Can\'t open %s!"), source);
        return -1;
    }
    
    if ((fileout = fopen(dest, "wb")) == nullptr) {
        _cc_logger_error(_T("Can\'t open %s!\n"), dest);
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

static uint64_t fileCheck(const tchar_t *fileName, tchar_t *output) {
    byte_t t = 0;
    byte_t md[_CC_MD5_DIGEST_LENGTH_];
    byte_t buf[CHUNK_SOURCE];
    int32_t i;
    _cc_md5_t c;
    FILE *fp = _tfopen(fileName, _T("rb"));
    uint64_t fileSize = 0;

    if (fp == nullptr)
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
    FILE *fw = nullptr;
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

int builder_UpdateList(void) {
    _cc_sql_result_t *resultSQL = nullptr;
    _cc_sql_result_t *resultUpdated = nullptr;
    tchar_t name[64];
    tchar_t path[256];
    tchar_t requestMD5[33];
    tchar_t oldCheck[33];
    int32_t sqlID = 0;
    uint64_t fileSize = 0;
    uint64_t resultSize = 0;
    tchar_t sqlString[1024] = {0};

    _cc_sql_t *sql = openSQLite3();

    if (sql == nullptr) {
        return 1;
    }

    sqldelegate.prepare(sql, _T("UPDATE `FileList` SET `CheckMD5`=?, `Compress`=?, `CompressSize`=?, `Size`=? WHERE `ID`=?;"),&resultUpdated);
    sqldelegate.execute(sql, _T("select `ID`, `Name`, `CheckMD5`, `Path` from `FileList`;"), &resultSQL);
    while (sqldelegate.fetch(resultSQL)) {
        int32_t isCompress = 0;
        sourceDirectory[sourceDirectoryLen] = 0;
        updateDirectory[updateDirectoryLen] = 0;

        sqlID = sqldelegate.get_int(resultSQL, 0);
        sqldelegate.get_string(resultSQL, 1, name, 64);
        sqldelegate.get_string(resultSQL, 3, path, 256);
        
        _tcscat(sourceDirectory + sourceDirectoryLen - 1, path);

        fileSize = fileCheck(sourceDirectory, requestMD5);
        if (fileSize == 0) {
            _sntprintf(sqlString, _cc_countof(sqlString), _T("DELETE FROM `FileList` WHERE `ID`=%d;"),sqlID);
            sqldelegate.execute(sql, sqlString, nullptr);
            continue;
        }

        sqldelegate.get_string(resultSQL, 2, oldCheck, 33);
        if (_tcsnicmp(oldCheck, requestMD5, 33) == 0) {
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
        sqldelegate.bind(resultUpdated, 0, &requestMD5,32,_CC_SQL_TYPE_STRING_);
        sqldelegate.bind(resultUpdated, 1, &isCompress, sizeof(int32_t), _CC_SQL_TYPE_INT32_);
        sqldelegate.bind(resultUpdated, 2, &resultSize, sizeof(int64_t), _CC_SQL_TYPE_INT64_);
        sqldelegate.bind(resultUpdated, 3, &fileSize, sizeof(int64_t), _CC_SQL_TYPE_INT64_);
        sqldelegate.bind(resultUpdated, 4, &sqlID, sizeof(int32_t), _CC_SQL_TYPE_INT32_);
        sqldelegate.step(sql, resultUpdated);
        sqldelegate.reset(sql, resultUpdated);
        printf("%s\t(%s)\n",&sourceDirectory[sourceDirectoryLen],requestMD5);
    }
    sqldelegate.free_result(sql, resultUpdated);
    puts("更新完成\n");
    if (resultSQL) {
        sqldelegate.free_result(sql, resultSQL);
    }

    updateDirectory[updateDirectoryLen] = 0;
    _tcscat(updateDirectory + updateDirectoryLen - 1, _T("/project.manifest"));

    createUpdateFile(updateDirectory, sql);

    closeSQLit3(sql);

    return 0;
}

int createUpdateFile(const tchar_t *saveFile, _cc_sql_t *sql) {
    char_t str[256];
    _cc_sql_result_t *resultSQL = nullptr;
    _cc_buf_t buf;
    _cc_json_t *rootJSON = _cc_json_alloc_object(_CC_JSON_ARRAY_, nullptr);
    sqldelegate.execute(sql, "select `ID`, `Name`, `CheckMD5`, `Compress`, `CompressSize`, `Size`, `Path` from `main`.`FileList`;", &resultSQL);
    while (sqldelegate.fetch(resultSQL)) {
        _cc_json_t *json = _cc_json_alloc_object(_CC_JSON_OBJECT_, nullptr);
        if (json) {
            _cc_json_add_number(json, "ID",  sqldelegate.get_int(resultSQL, 0), true);
            sqldelegate.get_string(resultSQL, 1, str, 256);
            _cc_json_add_string(json, "Name", str, true);
            sqldelegate.get_string(resultSQL, 2, str, 256);
            _cc_json_add_string(json, "MD5", str, true);
            sqldelegate.get_string(resultSQL, 6, str, 256);
            _cc_json_add_string(json, "Path", str, true);

            _cc_json_add_number(json, "Compress", sqldelegate.get_int(resultSQL, 3), true);
            _cc_json_add_number(json, "CompressSize", sqldelegate.get_int64(resultSQL, 4), true);
            _cc_json_add_number(json, "Size", sqldelegate.get_int64(resultSQL, 5), true);
            _cc_json_object_push(rootJSON, json, true);
        }
    }
    
    if (resultSQL) {
        sqldelegate.free_result(sql, resultSQL);
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