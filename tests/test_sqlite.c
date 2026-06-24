#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libcc.h>
#include <libcc/sql.h>

#define SQLite_DB "SQLITE:///./sqlite3test.db"

_cc_sql_delegate_t sql_delegate;
_cc_sql_t *sql = NULL;

typedef struct {const tchar_t *data; size_t length;} sql_table_t;

const sql_table_t createTable1 = {_CC_STRING("create table if not exists FileList1 (" \
        "`check` VARCHAR(65) PRIMARY KEY NOT NULL," \
        "`size` INTEGER NOT NULL," \
        "`path` TEXT NOT NULL" \
    ")")};
const sql_table_t createTable2 = {_CC_STRING("create table if not exists FileList2 (" \
        "`check` VARCHAR(65) PRIMARY KEY NOT NULL," \
        "`size` INTEGER NOT NULL," \
        "`path` TEXT NOT NULL," \
        "`upload` INTEGER default(0)" \
    ")")};

const sql_table_t createTable3 = {_CC_STRING("create table if not exists FileList (" \
        "`check` VARCHAR(65) PRIMARY KEY NOT NULL," \
        "`size` INTEGER NOT NULL," \
        "`path` TEXT NOT NULL," \
        "`upload` INTEGER default(0)," \
        "`update_time` timestamp NOT NULL default(datetime('now','localtime'))" \
    ")")};
bool_t scanFile(const tchar_t *directory, _cc_sql_result_t *result) {
    tchar_t sourceFile[_CC_MAX_PATH_] = {0};
    tchar_t check[_CC_SHA1_DIGEST_LENGTH_ * 2 + 1];
    DIR *dpath;
    struct dirent *d;
    struct _stat stat_buf;
    size_t size = 0;
    
    if( (dpath = opendir(directory)) == NULL) {
        _tprintf(_T("Couldn't open directory:%s"), directory);
        return false;
    }
    
    while ((d = readdir(dpath)) != NULL) {
        if (d->d_name[0]=='.') {
            continue;
        }
        _cc_fpath(sourceFile, _cc_countof(sourceFile), "%s/%s", directory, d->d_name);
        _tstat( sourceFile, &stat_buf);

        if (S_ISDIR(stat_buf.st_mode) == 0) {
            //_cc_md5_from_file(sourceFile, check);
            _cc_sha1_from_file(sourceFile, check);
            size = stat_buf.st_size;
            sql_delegate.reset(result);
            sql_delegate.bind(result, 0, check, _CC_SHA1_DIGEST_LENGTH_ * 2, _CC_SQL_TYPE_STRING_);
            sql_delegate.bind(result, 1, &size, sizeof(int64_t), _CC_SQL_TYPE_INT64_);
            sql_delegate.bind(result, 2, sourceFile, -1, _CC_SQL_TYPE_STRING_);
            sql_delegate.step(result);
        } else {
            scanFile(sourceFile, result);
        }
    }
    closedir(dpath);
    return true;
}

int main(int argc, char *const arvg[]) {
    bool_t createTable = false;
    _cc_sql_result_t *sql_result = NULL;
    tchar_t currentPath[_CC_MAX_PATH_];
    _cc_get_cwd(currentPath,_CC_MAX_PATH_);

    _cc_register_sqlite(&sql_delegate);
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
        sql_delegate.execute(sql, createTable1.data,createTable1.length, NULL);
        sql_delegate.execute(sql, createTable2.data,createTable2.length, NULL);
        sql_delegate.execute(sql, createTable3.data,createTable3.length, NULL);
    }
    sql_delegate.execute(sql, _CC_SQL("delete from `FileList1`;"), NULL);
    sql_delegate.execute(sql, _CC_SQL("delete from `FileList2`;"), NULL);

    if (sql_delegate.execute(sql, _CC_SQL("INSERT INTO `FileList1` (`check`, `size`, `path`) VALUES (?,?,?);"), &sql_result)) {
        scanFile(currentPath, sql_result);
        sql_delegate.free_result(sql_result);
    }
    
    sql_delegate.execute(sql, _CC_SQL("REPLACE INTO `FileList2` (`check`, `size`, `path`,`upload`) SELECT f1.`check`, f1.`size`, "\
                   "f1.`path`,f2.`upload` FROM FileList1 as f1 left join FileList as f2 on f1.`check`=f2.`check`;"), NULL);
    sql_delegate.execute(sql, _CC_SQL("REPLACE INTO `FileList`(`check`, `size`, `path`,`upload`) SELECT `check`, `size`, `path`, `upload` FROM FileList2;"), NULL);
    sql_delegate.execute(sql, _CC_SQL("UPDATE `FileList` SET `upload`=0 WHERE `upload` is null;"), NULL);
    sql_delegate.execute(sql, _CC_SQL("delete from `FileList1`;"), NULL);
    sql_delegate.execute(sql, _CC_SQL("delete from `FileList2`;"), NULL);
    sql_delegate.commit(sql);
    
    uint16_t status = 10;
    #define CHECK_SIZE (_CC_SHA1_DIGEST_LENGTH_ * 2 + 1)
    tchar_t check[CHECK_SIZE] = {0};
    memcpy(check, _T("d80c14694c68ca064cff6ad99470029334a2bd38"), CHECK_SIZE-1);
    sql_delegate.begin_transaction(sql);
    if (sql_delegate.execute(sql, _CC_SQL("UPDATE `FileList` SET `upload`=? WHERE `check`=?;"), &sql_result)) {
        sql_delegate.reset(sql_result);
        sql_delegate.bind(sql_result, 0, &status, sizeof(uint16_t), _CC_SQL_TYPE_UINT16_);
        sql_delegate.bind(sql_result, 1, check, CHECK_SIZE - 1, _CC_SQL_TYPE_STRING_);
        sql_delegate.step(sql_result);
        sql_delegate.free_result(sql_result);
    }
    sql_delegate.commit(sql);

    if (sql_delegate.execute(sql, _CC_SQL("SELECT `check`, `size`, `path` FROM FileList;"), &sql_result)) {
        int num_fields = sql_delegate.get_num_fields(sql_result);
        int i = 0;
        //struct tm t;
        while(sql_delegate.fetch(sql_result)) {
            for (i = 0; i < num_fields; ++i) {
                char_t buff[10240];
                sql_delegate.get_string(sql_result, i, buff, 10240);
                printf("%s, ",buff);
            }
            putc('\n',stdout);
            //sql_delegate.get_datetime(sql_result, num_fields - 1, &t);
            //printf("%4d-%02d-%02d %02d:%02d:%02d\n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
        }
        sql_delegate.free_result(sql_result);
    }
    sql_delegate.disconnect(sql);
    system("pause");
    return 0;
}
