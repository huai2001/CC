#include <stdio.h>
#include <libcc.h>

#define CHUNK 16384

int32_t removeDirectoryLen = 0;

uint32_t filesCount = 0;
/**/
const tchar_t* filterFileList[] = {
#if defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    _T(".DS_Store"),
#endif
    _T("Builder"),
    _T(".bash_profile"),
    _T(".bashrc"),
    _T(".bash_logout"),
    _T(".svn"),
};

static bool_t isFillerList(tchar_t *name, int32_t namlen) {
    int32_t i = 0;
    if ((name[0]=='.' && name[1] == 0) ||
        (name[0]=='.' && name[1] == '.' && name[2] == 0)) {
        return true;
    }

    for (i = 0; i < _cc_countof(filterFileList); i++) {
        if (_tcsnicmp(filterFileList[i], name, namlen) == 0) {
            return true;
        }
    }
    return false;
}

tchar_t* toDecimal(double f, const tchar_t *unit, tchar_t *buf, int32_t buf_len) {
    int64_t v = (int64_t)_cc_round(f, 100);
    _sntprintf(buf, buf_len, _T("%lld%s"), v, unit);
    return buf;

}

tchar_t* getDiskSizeUnit(int64_t diskSize, tchar_t *buf, int32_t buf_len) {
    if (diskSize < 1024) {
        return toDecimal((double)diskSize, "B", buf, buf_len);
    }
    
    if (diskSize < 1048576) {
        return toDecimal(diskSize / 1024.0f,"KB", buf, buf_len);
    }
    
    if (diskSize < 1073741824) {
        return toDecimal(diskSize / 1048576.0f,"MB", buf, buf_len);
    }
    
    if (diskSize < 1099511627776) {
        return toDecimal(diskSize / 1073741824.0f,"GB", buf, buf_len);
    }
    
    //if (diskSize < 1125899906842624) {
        return toDecimal(diskSize / 1099511627776.0f,"TB", buf, buf_len);
    //}
    
    //_sntprintf(buf, buf_len, _T("%lld B"), diskSize);
    //return buf;
}

void OpenDeepDirectory(const tchar_t *directory) {
    tchar_t sourceFile[_CC_MAX_PATH_] = {0};
    tchar_t diskBuf[1024] = {0};
    DIR *dpath = nullptr;
    struct dirent *d;
    struct _stat stat_buf;
    
    if( (dpath = opendir(directory)) == nullptr) {
        return;
    }
    
    while ((d = readdir(dpath)) != nullptr) {
        //
        if (isFillerList(d->d_name, d->d_reclen)) continue;

        sourceFile[0] = 0;
        _tcscat(sourceFile,directory);
        _tcscat(sourceFile,_T("/"));
        _tcscat(sourceFile,d->d_name);
        
        _tstat( sourceFile, &stat_buf);

        if (S_ISDIR(stat_buf.st_mode) == 0) {
            //3M
            //if (stat_buf.st_size >= 3145728) {
                //s.st_size/1024.0/1024.0;
                getDiskSizeUnit(stat_buf.st_size, diskBuf, _cc_countof(diskBuf));
                printf("%s/%s %s\n",directory, d->d_name, diskBuf);
            //}
            //_putts(sourceFile);
            filesCount++;
        } else {
            OpenDeepDirectory(sourceFile);
        }
    }
    closedir(dpath);
}

int main (int argc, char * const argv[]) {
	_cc_install_memory_tracked();
    //tchar_t directory[_CC_MAX_PATH_];
    //_cc_get_base_path(directory, _CC_MAX_PATH_);
    OpenDeepDirectory("C:\\Legend of mir");
    _tprintf(_T("OK, Find %d files.\n"), filesCount);

    while(getchar() != 'q') _cc_sleep(100);

	_cc_uninstall_memory_tracked();
    return 0;
}
