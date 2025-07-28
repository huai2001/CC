#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libcc.h>
#include <libcc/widgets/sql.h>

#if _CC_USE_SYSTEM_SQLITE3_LIB_
#include <sqlite3.h>
#else 
#include <sqlite3/sqlite3.h>
#endif
#define SQLite_DB "SQLITE://127.0.0.1/./sqlite3test.db"

_cc_sql_delegate_t sql_delegate;
_cc_sql_t *sql = nullptr;

const _cc_String_t createTable1 = _cc_String("create table if not exists FileList1 (" \
        "`check` VARCHAR(65) PRIMARY KEY NOT NULL," \
        "`size` INTEGER NOT NULL," \
        "`path` TEXT NOT NULL" \
    ")");
const _cc_String_t createTable2 = _cc_String("create table if not exists FileList2 (" \
        "`check` VARCHAR(65) PRIMARY KEY NOT NULL," \
        "`size` INTEGER NOT NULL," \
        "`path` TEXT NOT NULL," \
        "`upload` INTEGER default(0)" \
    ")");

const _cc_String_t createTable3 = _cc_String("create table if not exists FileList (" \
        "`check` VARCHAR(65) PRIMARY KEY NOT NULL," \
        "`size` INTEGER NOT NULL," \
        "`path` TEXT NOT NULL," \
        "`upload` INTEGER default(0)," \
        "`update_time` timestamp NOT NULL default(datetime('now','localtime'))" \
    ")");
bool_t scanFile(const tchar_t *directory, _cc_sql_result_t *result) {
    tchar_t sourceFile[_CC_MAX_PATH_] = {0};
    tchar_t check[_CC_SHA1_DIGEST_LENGTH_ * 2 + 1];
    DIR *dpath;
    struct dirent *d;
    struct _stat stat_buf;
    size_t size = 0;
    
    if( (dpath = opendir(directory)) == nullptr) {
        _cc_logger_error(_T("Couldn't open directory:%s"), directory);
        return false;
    }
    
    while ((d = readdir(dpath)) != nullptr) {
        if (d->d_name[0]=='.') {
            continue;
        }
        _cc_fpath(sourceFile, _cc_countof(sourceFile), "%s/%s", directory, d->d_name);
        _tstat( sourceFile, &stat_buf);

        if (S_ISDIR(stat_buf.st_mode) == 0) {
            //_cc_md5file(sourceFile, check);
            _cc_sha1file(sourceFile, check);
            size = stat_buf.st_size;
            sql_delegate.reset(sql, result);
            sql_delegate.bind(result, 0, check, _CC_SHA1_DIGEST_LENGTH_ * 2, _CC_SQL_TYPE_STRING_);
            sql_delegate.bind(result, 1, &size, sizeof(int64_t), _CC_SQL_TYPE_INT64_);
            sql_delegate.bind(result, 2, sourceFile, -1, _CC_SQL_TYPE_STRING_);
            sql_delegate.step(sql, result);
        } else {
            scanFile(sourceFile, result);
        }
    }
    closedir(dpath);
    return true;
}

int main(int argc, char *const arvg[]) {
    bool_t createTable = false;
    _cc_sql_result_t *sql_result = nullptr;
    _cc_String_t sql_str;
    tchar_t currentPath[_CC_MAX_PATH_];
    _cc_get_cwd(currentPath,_CC_MAX_PATH_);

    _cc_init_sqlite(&sql_delegate);
    if (_taccess("./sqlite3test.db", _CC_ACCESS_F_) == -1) {
        createTable = true;
    }
    
    sql = sql_delegate.connect(SQLite_DB);
    if (sql) {
        printf("connection succed1\n");
    } else {
        printf("connection failed1\n");
        return 1;
    }
    
    sql_delegate.begin_transaction(sql);
    if (createTable) {
        sql_delegate.execute(sql, &createTable1, nullptr);
        sql_delegate.execute(sql, &createTable2, nullptr);
        sql_delegate.execute(sql, &createTable3, nullptr);
    }
    _cc_String_Set(sql_str, _T("delete from `FileList1`;"));
    sql_delegate.execute(sql, &sql_str, nullptr);
    _cc_String_Set(sql_str, _T("delete from `FileList2`;"));
    sql_delegate.execute(sql, &sql_str, nullptr);

    _cc_String_Set(sql_str, _T("INSERT INTO `FileList1` (`check`, `size`, `path`) VALUES (?,?,?);"));
    if (sql_delegate.execute(sql, &sql_str, &sql_result)) {
        scanFile(currentPath, sql_result);
        sql_delegate.free_result(sql, sql_result);
    }
    
    _cc_String_Set(sql_str,
                   _T("REPLACE INTO `FileList2` (`check`, `size`, `path`,`upload`) SELECT f1.`check`, f1.`size`, ")
                   _T("f1.`path`,f2.`upload` FROM FileList1 as f1 left join FileList as f2 on f1.`check`=f2.`check`;"));
    sql_delegate.execute(sql, &sql_str, nullptr);
    _cc_String_Set(sql_str, _T("REPLACE INTO `FileList`(`check`, `size`, `path`,`upload`) SELECT `check`, `size`, `path`, `upload` FROM FileList2;"));
    sql_delegate.execute(sql, &sql_str, nullptr);
    _cc_String_Set(sql_str, _T("UPDATE `FileList` SET `upload`=0 WHERE `upload` is null;"));
    sql_delegate.execute(sql, &sql_str, nullptr);
    _cc_String_Set(sql_str, _T("delete from `FileList1`;"));
    sql_delegate.execute(sql, &sql_str, nullptr);
    _cc_String_Set(sql_str, _T("delete from `FileList2`;"));
    sql_delegate.execute(sql, &sql_str, nullptr);
    sql_delegate.commit(sql);
    
    uint16_t status = 10;
    #define CHECK_SIZE (_CC_SHA1_DIGEST_LENGTH_ * 2 + 1)
    tchar_t check[CHECK_SIZE] = {0};
    memcpy(check, _T("d80c14694c68ca064cff6ad99470029334a2bd38"), CHECK_SIZE-1);
    sql_delegate.begin_transaction(sql);
    _cc_String_Set(sql_str, _T("UPDATE `FileList` SET `upload`=? WHERE `check`=?;"));
    if (sql_delegate.execute(sql, &sql_str, &sql_result)) {
        sql_delegate.reset(sql, sql_result);
        sql_delegate.bind(sql_result, 0, &status, sizeof(uint16_t), _CC_SQL_TYPE_UINT16_);
        sql_delegate.bind(sql_result, 1, check, CHECK_SIZE - 1, _CC_SQL_TYPE_STRING_);
        sql_delegate.step(sql, sql_result);
        sql_delegate.free_result(sql, sql_result);
    }
    sql_delegate.commit(sql);

    _cc_String_Set(sql_str, _T("SELECT `check`, `size`, `path` FROM FileList;"));
    if (sql_delegate.execute(sql, &sql_str, &sql_result)) {
        int num_fields = sql_delegate.get_num_fields(sql_result);
        int i = 0;
        //struct tm *t;
        while(sql_delegate.fetch(sql_result)) {
            for (i = 0; i < num_fields; ++i) {
                char_t buff[10240];
                sql_delegate.get_string(sql_result, i, buff, 10240);
                printf("%s, ",buff);
            }
            putc('\n',stdout);
            //t = sql_delegate.get_datetime(sql_result, num_fields - 1);
            //printf("%4d-%02d-%02d %02d:%02d:%02d\n", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
        }
        sql_delegate.free_result(sql, sql_result);
    }
    sql_delegate.disconnect(sql);
    system("pause");
    return 0;
}
