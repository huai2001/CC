#include <stdio.h>
#include <libcc.h>

#define CHUNK 16384

static tchar_t appDirectory[_CC_MAX_PATH_];
int32_t removeDirectoryLen = 0;
/**/
bool_t _cc_copyfile(const tchar_t* existing_file_name, const tchar_t* new_file_name, bool_t fail_if_exists) {
    int input, output;
    long r = -1;
    unsigned char buf[5120];
 
    input = _topen(existing_file_name, O_RDONLY);
    if (input <= 0) {
        _cc_logger_error(_T("Could't open file:%s\n"), existing_file_name);
        return false;
    }

    if (fail_if_exists) {
        output = _topen(new_file_name, O_RDONLY);
        if (output) {
            close(input);
            close(output);
            return false;
        }
    }

    output = _topen(new_file_name, O_WRONLY | O_CREAT, S_IREAD | S_IWRITE);
    if(output <= 0) {
        _cc_logger_error(_T("Could't open or write file:%s\n"), new_file_name);
        close(input);
        return false;
    }
    
    while ((r = (long)read(input, buf, _cc_countof(buf))) > 0) {
        long w = 0;
        long left = 0;

        while ((w = (long)write(output, buf + left, r - left)) > 0) {
            left += w;
            if (left == r) {
                break;
            }
        }

        if (r != left) {
            r = -1;
            if (w < 0) {
                int r = _cc_last_errno();
                _cc_logger_error(_T("write error: %s (%d)"), _cc_last_error(r), r);
            }
            break;
        }
    }

    close(input);
    close(output);

    return (r > 0);
}

static bool_t isFillerList(tchar_t *name, int32_t namlen) {
    if ((name[0]=='.' && name[1] == 0) ||
        (name[0]=='.' && name[1] == '.' && name[2] == 0)) {
        return true;
    }
    return false;
}

size_t OpenDeepDirectory(const tchar_t *sourceDirectory, const tchar_t *targetDirectory) {
    tchar_t sourceFile[_CC_MAX_PATH_] = {0};
    tchar_t targetFile[_CC_MAX_PATH_] = {0};
    DIR *dpath = NULL;
    struct dirent *d;
    struct _stat stat_buf;
    size_t filesCount = 0;
    
    if( (dpath = opendir(sourceDirectory)) == NULL) {
        _cc_logger_error(_T("Could't open directory:%s\n"), sourceDirectory);
        return 0;
    }
    
    //¶ÁÈ¡Ä¿Â¼
    while ((d = readdir(dpath)) != NULL) {
        //
        if (isFillerList(d->d_name, d->d_reclen)) continue;

        sourceFile[0] = 0;
        targetFile[0] = 0;
        
        _tcscat(sourceFile,sourceDirectory);
        _tcscat(sourceFile,_T("/"));
        _tcscat(sourceFile,d->d_name);

        _tcscat(targetFile,targetDirectory);
        _tcscat(targetFile,_T("/"));
        _tcscat(targetFile,d->d_name);

        _tstat(sourceFile, &stat_buf);

        if (S_ISDIR(stat_buf.st_mode) == 0) {
            copyfile(sourceFile, targetFile, true);
            filesCount++;
            //_tprintf(_T("copy %s\n"), targetFile);
        } else {
            size_t fcount = 0;

            _tmkdir(targetFile);
            fcount = OpenDeepDirectory(sourceFile, targetFile);

            _tprintf(_T("copy directory: %s(%ld)\n"), targetFile, fcount);

            filesCount += fcount;
        }
    }

    closedir(dpath);

    return filesCount;
}

int _tmain (int argc, tchar_t * const argv[]) {
    size_t filesCount = 0;
    tchar_t *source = NULL;
    tchar_t *target = NULL;
    tchar_t *ptr = NULL;
    struct _stat stat_buf;
    
    if (!argv[1]) {
        printf("usage: cp source_file target_file\n");
        return;
    }

    if (!argv[2]) {
        printf("usage: cp source_file target_file\n");
        return;
    }

    _tstat(argv[1], &stat_buf);

    if (S_ISDIR(stat_buf.st_mode) == 0) {
        _cc_copyfile(argv[1], argv[2], true);
        filesCount++;
    } else {
        size_t fcount = 0;
        _cc_mkdir(argv[2]);

        fcount = OpenDeepDirectory(argv[1], argv[2]);

        _tprintf(_T("copy directory: %s(%ld)\n"), argv[2], fcount);

        filesCount += fcount;
    }

    _tprintf(_T("OK, Find %ld files.\n"), filesCount);

    return 0;
}
