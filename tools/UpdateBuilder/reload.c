#include <stdio.h>
#include "UpdateBuilder.h"

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
    DIR *dpath = nullptr;
    struct dirent *d;
    struct _stat stat_buf;

    if( (dpath = opendir(directory)) == nullptr) {
        _cc_logger_error("opendir:fail(%s).\n",directory);
        return;
    }
    
    while ((d = readdir(dpath)) != nullptr) {
        //
        if (isFillerList(d->d_name, -1)) continue;

        sourceFile[0] = 0;
        _tcscat(sourceFile,directory);
        _tcscat(sourceFile, _CC_SLASH_S_);
        _tcscat(sourceFile,d->d_name);
        
        _tstat( sourceFile, &stat_buf);
        _sntprintf(updateFile, _cc_countof(updateFile),_T("%s\\%s"), updateDirectory, sourceFile + updateDirectoryLen);

        if (S_ISDIR(stat_buf.st_mode) == 0) {
            int i = 0;
            sqldelegate.reset(sql, result);
            sqldelegate.bind(result, i++, &d->d_name, -1, _CC_SQL_TYPE_STRING_);
            sqldelegate.bind(result, i++, &stat_buf.st_size, sizeof(int64_t), _CC_SQL_TYPE_INT64_);
            sqldelegate.bind(result, i++, &stat_buf.st_size, sizeof(int64_t), _CC_SQL_TYPE_INT64_);
            sqldelegate.bind(result, i++, (sourceFile + sourceDirectoryLen), -1, _CC_SQL_TYPE_STRING_);
            sqldelegate.step(sql, result);
        } else {
            _cc_mkdir(updateFile);
            OpenDeepDirectory(sourceFile, sql, result);
        }
    }
    closedir(dpath);
}

int builder_ReloadList(void) {
    _cc_sql_result_t *result = nullptr;

    _cc_sql_t *sql = openSQLite3();
    if (sql == nullptr) {
        return 1;
    }

    sqldelegate.execute(sql, _T("DELETE FROM `FileList`;"), nullptr);
    sqldelegate.execute(sql, _T("UPDATE sqlite_sequence SET seq = 0 WHERE name = 'FileList';"), nullptr);
    if (sqldelegate.prepare(sql, _T("INSERT INTO `FileList` (`Name`, `CheckMD5`, `Compress`, `CompressSize`, `Size`, `Path`) VALUES ( ?,'',0,?,?,?);"), &result)) {
        OpenDeepDirectory(sourceDirectory, sql, result);
        sqldelegate.free_result(sql, result);
    }
    
    closeSQLit3(sql);
    return 0;
}