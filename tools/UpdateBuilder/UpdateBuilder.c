#include <stdio.h>
#include "UpdateBuilder.h"

_cc_sds_t dest_directory = NULL;
_cc_sds_t source_directory = NULL;


int32_t sourceDirectoryLen = 0;
tchar_t sourceDirectory[_CC_MAX_PATH_] = {0};

#ifdef __CC_WINDOWS__

#else
static __attribute__((constructor)) void _dynamic_attach(void) {
    _cc_register_sqlite(&sqldelegate);
}

static __attribute__((destructor)) void _dynamic_detach(void) {

}
#endif
/*
void DeleteDeepDirectory(const tchar_t *directory) {
    tchar_t sourceFile[_CC_MAX_PATH_] = {0};
    DIR *dpath = NULL;
    struct dirent *d;
    struct stat stat_buf;
    
    if( (dpath = opendir(directory)) == NULL) {
        return;
    }
    
    //读取目录
    while ((d = readdir(dpath)) != NULL) {
        //
        if ((d->d_name[0]=='.' && d->d_name[1] == 0) ||
            (d->d_name[0]=='.' && d->d_name[1] == '.' && d->d_name[2] == 0)) {
            continue;
        }
        
        sourceFile[0] = 0;
        _tcscat(sourceFile,directory);
        _tcscat(sourceFile,_T("/"));
        _tcscat(sourceFile,d->d_name);
        _tstat( sourceFile, &stat_buf);
        
        if (S_ISDIR(stat_buf.st_mode) == 0) {
            //_tprintf("delete %s\n", sourceFile);
            _tunlink(sourceFile);
        } else {
            DeleteDeepDirectory(sourceFile);
            _trmdir(sourceFile);
        }
    }
    closedir(dpath);
}*/

int main(int argc, char const *argv[]) {
    _cc_register_sqlite(&sqldelegate);

    if (argc < 3) {
        return 0;
    }

    if (!_cc_isdir(argv[1])) {
        _cc_logger_warin("%s directory does not exist", argv[1]);
        return 0;
    }
    
    if (!_cc_isdir(argv[2])) {
        _cc_logger_warin("%s directory does not exist", argv[2]);
        return 0;
    }

    source_directory = _cc_sds_alloc(argv[1], _cc_strlen(argv[1]));
    dest_directory = _cc_sds_alloc(argv[2], _cc_strlen(argv[2]));

    builder_reload();
    builder_updated();
    system("pause");
    return 0;
}