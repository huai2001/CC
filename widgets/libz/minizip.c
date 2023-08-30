#include <libcc.h>
#include "zip.h"
#include "unzip.h"
#include "zlib.h"

#define CHUNK 16384

static bool_t saveFile(unzFile zFile, FILE *wfp) {
    while (1) {
        byte_t buff[CHUNK];
        int readSize = unzReadCurrentFile(zFile, buff, _cc_countof(buff));
        if (readSize < 0) {
            _cc_logger_error(_T("read Size %d with zipfile in unzReadCurrentFile"), readSize);
            return false;
        } else if (readSize == 0) {
            break;
        }
        
        fwrite(buff, sizeof(byte_t), readSize, wfp);
    }
    return true;
}

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
    byte_t in[CHUNK];
    byte_t out[CHUNK];
    
    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    res = deflateInit(&strm, level);
    if (res != Z_OK)
        return res;
    
    /* compress until end of file */
    do {
        strm.avail_in = (uint32_t)fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = (Bytef*)in;
        
        /* run deflate() on input until output buffer not full, finish
         compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = (Bytef*)out;
            res = deflate(&strm, flush);    /* no bad return value */
            _cc_assert(res != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, sizeof(byte_t), have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
            if (resultSize) {
                *resultSize += have;
            }
        } while (strm.avail_out == 0);
        _cc_assert(strm.avail_in == 0);     /* all input will be used */
        
        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    _cc_assert(res == Z_STREAM_END);        /* stream will be complete */
    
    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}

/* Decompress from file source to file dest until stream ends or EOF.
 inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
 allocated for processing, Z_DATA_ERROR if the deflate data is
 invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
 the version of the library linked do not match, or Z_ERRNO if there
 is an error reading or writing the files. */
static int inf(FILE *source, FILE *dest, uint64_t *resultSize) {
    int res;
    unsigned have;
    z_stream strm;
    byte_t in[CHUNK];
    byte_t out[CHUNK];
    
    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    res = inflateInit(&strm);
    if (res != Z_OK)
        return res;
    
    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = (uint32_t)fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = (Bytef*)in;
        
        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = (Bytef*)out;
            res = inflate(&strm, Z_NO_FLUSH);
            _cc_assert(res != Z_STREAM_ERROR);  /* state not clobbered */
            switch (res) {
                case Z_NEED_DICT:
                    res = Z_DATA_ERROR;     /* and fall through */
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    (void)inflateEnd(&strm);
                    return res;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, sizeof(byte_t), have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
            if (resultSize) {
                *resultSize += have;
            }
        } while (strm.avail_out == 0);
        
        /* done when inflate() says it's done */
    } while (res != Z_STREAM_END);
    
    /* clean up and return */
    (void)inflateEnd(&strm);
    return res == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/* report a zlib or i/o error */
static void zerr(int res) {
    fputs("zpipe: ", stderr);
    switch (res) {
        case Z_ERRNO:
            if (ferror(stdin))
                fputs("error reading stdin\n", stderr);
            if (ferror(stdout))
                fputs("error writing stdout\n", stderr);
            break;
        case Z_STREAM_ERROR:
            fputs("invalid compression level\n", stderr);
            break;
        case Z_DATA_ERROR:
            fputs("invalid or incomplete deflate data\n", stderr);
            break;
        case Z_MEM_ERROR:
            fputs("out of memory\n", stderr);
            break;
        case Z_VERSION_ERROR:
            fputs("zlib version mismatch!\n", stderr);
    }
}

int CompressZipFile(const char *source, const char *dest, uint64_t *resultSize) {
    int res;
    FILE *filein, *fileout;
    
    if((filein = fopen(source, "rb")) == NULL) {
        _cc_logger_error(_T("Can\'t open %s!"), source);
        return -1;
    }
    
    if((fileout = fopen(dest, "wb")) == NULL) {
        _cc_logger_error(_T("Can\'t open %s!\n"), dest);
        fclose(filein);
        return -1;
    }
    
    /* do compression if no arguments */
    res = def(filein, fileout, Z_DEFAULT_COMPRESSION, resultSize);
    if (res != Z_OK)
        zerr(res);
    
    fclose(filein);
    fclose(fileout);
    
    return res;
}

int DecompressZipFile(const char *source, const char *dest, uint64_t *resultSize) {
    int res;
    FILE *filein, *fileout;
    
    if((filein = fopen(source, "rb")) == NULL) {
        _cc_logger_error(_T("Can\'t open %s!"), source);
        return -1;
    }
    
    if((fileout = fopen(dest, "wb")) == NULL) {
        _cc_logger_error(_T("Can\'t open %s!\n"), dest);
        fclose(filein);
        return -1;
    }
    
    /* do decompression */
    res = inf(filein, fileout, resultSize);
    if (res != Z_OK)
        zerr(res);
    
    fclose(filein);
    fclose(fileout);
    return res;
}

int MiniUnzip(const char *zipFile, const char *savePath, const char *pass) {
    unz_file_info64 fileInfo;
    char_t path[_CC_MAX_PATH_];
    char_t* p = NULL;
    char_t* fileWithoutPath = NULL;
    int result = 1;
    int err = 0;
    unzFile zFile = unzOpen64(zipFile);

    if (zFile == NULL) {
        _cc_logger_error(_T("Can\'t open zip file:%s"), zipFile);
        return 0;
    }
    
    err = unzGoToFirstFile(zFile);
    if (err != UNZ_OK) {
        _cc_logger_error(_T("error zip file:%s"), zipFile);
        return 0;
    }

    _cc_mkdir(savePath);
    _tcsncpy(path, savePath, _CC_MAX_PATH_);
    path[_CC_MAX_PATH_ - 1] = 0;
    
    do {
        char_t file[_CC_MAX_PATH_];
        /*
         char_t ext[_CC_MAX_PATH_];
         char_t com[1024];
         
         if ((err = unzGetCurrentFileInfo64(zFile, &fileInfo, file, sizeof(file), ext, _cc_countof(ext), com, _cc_countof(com))) != UNZ_OK) {
         _cc_logger_error(_T("unzGetCurrentFileInfo failed... error:%d\n"), err);
         return false;
         }
         */
        if ((err = unzGetCurrentFileInfo64(zFile, &fileInfo, file, sizeof(file), NULL, 0, NULL, 0)) != UNZ_OK) {
            _cc_logger_error(_T("unzGetCurrentFileInfo failed... error:%d\n"), err);
            result = 0;
            break;
        }
        
        p = fileWithoutPath = file;
        while ((*p) != '\0') {
            if (((*p)=='/') || ((*p)=='\\'))
                fileWithoutPath = p + 1;
            p++;
        }
        
        _sntprintf(path, _cc_countof(path),_T("%s/%s"), savePath, file);
        
        if ((*fileWithoutPath) == '\0') {
            _cc_mkdir(path);
        } else {
            FILE *wfp = _tfopen(path, _T("wb"));
            if (wfp == NULL) {
                _cc_logger_error(_T("don't create file:%s"), path);
                result = 0;
                break;
            }
            
            err = unzOpenCurrentFilePassword(zFile,pass);
            if (err != UNZ_OK) {
                _cc_logger_error(_T("error %d with zipfile in unzOpenCurrentFilePassword"),err);
                unzCloseCurrentFile(zFile);
                result = 0;
                break;
            }
            
            if (!saveFile(zFile, wfp)) {
                unzCloseCurrentFile(zFile);
                result = 0;
                break;
            }
            
            unzCloseCurrentFile(zFile);
        }
    } while (unzGoToNextFile(zFile) == UNZ_OK);

    unzClose(zFile);
    
    return result;
}
/*
int main (int argc, char * const argv[]) {
    //UnzipFile("/Users/QIU/Downloads/select.zip", "/Users/QIU/Downloads/select", NULL);
    //compressFile("/Users/QIU/Downloads/select/server.c","/Users/QIU/Downloads/select/server.zip");
    decompressFile("/Users/QIU/Downloads/select/server.zip","/Users/QIU/Downloads/select/server2.c");
}
*/
