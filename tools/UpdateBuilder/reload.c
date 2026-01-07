#include <stdio.h>
#include "UpdateBuilder.h"

struct {
    const tchar_t* name;
    long length;
} fillers[] = {
#if defined(__CC_MACOSX__) || defined(__CC_IPHONEOS__)
    {_T(".DS_Store"),sizeof(_T(".DS_Store"))},
#endif
    {_T("project.manifest"),sizeof(_T("project.manifest")) - 1},
    {_T("update_builder"),sizeof(_T("update_builder")) - 1}
};

static bool_t is_filler(tchar_t *name, int32_t namlen) {
    int32_t i = 0;
    if ((name[0]=='.' && name[1] == 0) ||
        (name[0]=='.' && name[1] == '.' && name[2] == 0)) {
        return true;
    }

    for (i = 0; i < _cc_countof(fillers); i++) {
        if (_tcsnicmp(fillers[i].name, name, fillers[i].length) == 0) {
            return true;
        }
    }
    return false;
}

static void open_deep_irectory(const _cc_sds_t source_directory, _cc_sql_t *sql, _cc_sql_result_t *result) {
    tchar_t source_file[_CC_MAX_PATH_] = {0};
    tchar_t update_file[_CC_MAX_PATH_] = {0};
    DIR *dpath = NULL;
    struct dirent *d;
    struct _stat stat_buf;

    if( (dpath = opendir(directory)) == NULL) {
        _cc_logger_error("opendir:fail(%s).\n",directory);
        return;
    }

    while ((d = readdir(dpath)) != NULL) {
        //
        if (is_filler(d->d_name, -1)) continue;

        source_file[0] = 0;
        _tcscat(source_file,directory);
        _tcscat(source_file, _CC_SLASH_S_);
        _tcscat(source_file,d->d_name);
        
        _tstat( source_file, &stat_buf);
        _sntprintf(update_file, _cc_countof(update_file),_T("%s\\%s"), dest_directory, source_file + _cc_sds_length(dest_directory));

        if (S_ISDIR(stat_buf.st_mode) == 0) {
            int i = 0;
            sql_delegator.reset(sql, result);
            sql_delegator.bind(result, i++, &d->d_name, -1, _CC_SQL_TYPE_STRING_);
            sql_delegator.bind(result, i++, &stat_buf.st_size, sizeof(int64_t), _CC_SQL_TYPE_INT64_);
            sql_delegator.bind(result, i++, &stat_buf.st_size, sizeof(int64_t), _CC_SQL_TYPE_INT64_);
            sql_delegator.bind(result, i++, (source_file + _cc_sds_length(source_directory)), -1, _CC_SQL_TYPE_STRING_);
            sql_delegator.step(sql, result);
        } else {
            _cc_mkdir(update_file);
            open_deep_irectory(source_file, sql, result);
        }
    }
    closedir(dpath);
}

int builder_reload(void) {
    _cc_sql_result_t *result = NULL;
    _cc_sql_t *sql = open_sqlite3();
    if (sql == NULL) {
        return 1;
    }
    sql_delegator.execute(sql, _CC_SQL("DELETE FROM `FileList`;"), NULL);
    sql_delegator.execute(sql, _CC_SQL("UPDATE sqlite_sequence SET seq = 0 WHERE name = 'FileList';"), NULL);
    if (sql_delegator.execute(sql, _CC_SQL("INSERT INTO `FileList` (`Name`, `CheckMD5`, `Compress`, `CompressSize`, `Size`, `Path`) VALUES ( ?,'',0,?,?,?);"), &result)) {
        open_deep_irectory(source_directory, sql, result);
        sql_delegator.free_result(sql, result);
    }
    
    close_sqlite3(sql);
    return 0;
}