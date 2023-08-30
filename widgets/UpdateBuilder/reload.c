#include <stdio.h>
#include "UpdateBuilder.h"

static int32_t removeDirectoryLen = 0;
static tchar_t updateDirectory[_CC_MAX_PATH_];

struct{
    const tchar_t* name;
    long len;
} filterFileList[] = {
#if defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    {_T(".DS_Store"),sizeof(_T(".DS_Store"))},
#endif
    {_T("project.manifest"),sizeof(_T("project.manifest"))},
    {_T("UpdateBuilder"),sizeof(_T("UpdateBuilder"))}
};

static bool_t isFillerList(tchar_t *name, int32_t namlen) {
    int32_t i = 0;
    if ((name[0]=='.' && name[1] == 0) ||
        (name[0]=='.' && name[1] == '.' && name[2] == 0)) {
        return true;
    }

    for (i = 0; i < _cc_countof(filterFileList); i++) {
        if (_tcsnicmp(filterFileList[i].name, name, filterFileList[i].len) == 0) {
            return true;
        }
    }
    return false;
}

static void OpenDeepDirectory(const tchar_t *directory, _cc_sql_t *sql, _cc_sql_result_t *result) {
    tchar_t sourceFile[_CC_MAX_PATH_] = {0};
    tchar_t updateFile[_CC_MAX_PATH_] = {0};
    DIR *dpath = NULL;
    struct dirent *d;
    struct _stat stat_buf;

    if( (dpath = opendir(directory)) == NULL) {
        _cc_logger_error("opendir:fail(%s).\n",directory);
        return;
    }
    
    while ((d = readdir(dpath)) != NULL) {
        //
        if (isFillerList(d->d_name, d->d_namlen)) continue;

        sourceFile[0] = 0;
        _tcscat(sourceFile,directory);
        _tcscat(sourceFile, _CC_PATH_SEP_S_);
        _tcscat(sourceFile,d->d_name);
        
        _tstat( sourceFile, &stat_buf);
        _sntprintf(updateFile, _cc_countof(updateFile),_T("%s\\%s"), updateDirectory, sourceFile + removeDirectoryLen);

        if (S_ISDIR(stat_buf.st_mode) == 0) {
            int i = 0;
            sqlDriver.reset(sql, result);
            sqlDriver.bind(result, i++, &d->d_name, d->d_namlen, _CC_SQL_TYPE_STRING_);
            sqlDriver.bind(result, i++, &stat_buf.st_size, sizeof(int64_t), _CC_SQL_TYPE_INT64_);
            sqlDriver.bind(result, i++, &stat_buf.st_size, sizeof(int64_t), _CC_SQL_TYPE_INT64_);
            sqlDriver.bind(result, i++, (sourceFile + removeDirectoryLen), -1, _CC_SQL_TYPE_STRING_);
            sqlDriver.step(sql, result);
        } else {
            _cc_mkdir(updateFile);
            OpenDeepDirectory(sourceFile, sql, result);
        }
    }
    closedir(dpath);
}

int builder_ReloadList(void) {
    tchar_t directory[_CC_MAX_PATH_];
    tchar_t sourceDirectory[_CC_MAX_PATH_];
    _cc_sql_result_t *result = NULL;

    _cc_sql_t *sql = openSQLite3();
    if (sql == NULL) {
        return 1;
    }

    _cc_get_module_directory(NULL, directory, _cc_countof(directory));

    removeDirectoryLen = _sntprintf(sourceDirectory, _cc_countof(sourceDirectory),_T("%s/Source"), directory);
    _sntprintf(updateDirectory, _cc_countof(updateDirectory),_T("%s/Update"), directory);
    
    _cc_mkdir(sourceDirectory);
    _cc_mkdir(updateDirectory);

    sqlDriver.execute(sql, _T("delete from `FileList`;"), NULL);
    sqlDriver.execute(sql, _T("UPDATE sqlite_sequence SET seq = 0 WHERE name = 'FileList';"), NULL);
    if (sqlDriver.prepare(sql, _T("INSERT INTO `FileList` (`Name`, `CheckMD5`, `Compress`, `CompressSize`, `Size`, `Path`) VALUES ( ?,'',0,?,?,?);"), &result)) {
        OpenDeepDirectory(sourceDirectory, sql, result);
        sqlDriver.free_result(sql, result);
    }
    
    closeSQLit3(sql);
    return 0;
}