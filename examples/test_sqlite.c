#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libcc.h>
#include <cc/db/sql.h>

#if _CC_USE_SYSTEM_SQLITE3_LIB_
#include <sqlite3.h>
#else 
#include <sqlite3/sqlite3.h>
#endif
#define SQLite_DB "SQLITE://127.0.0.1/./sqlite3test.db"

_cc_sql_driver_t sql_driver;
_cc_sql_t *sql = NULL;

const tchar_t *createTable1 = "create table if not exists FileList1 (" \
        "`check` VARCHAR(65) PRIMARY KEY NOT NULL," \
        "`size` INTEGER NOT NULL," \
        "`path` TEXT NOT NULL" \
    ")";
const tchar_t *createTable2 = "create table if not exists FileList2 (" \
        "`check` VARCHAR(65) PRIMARY KEY NOT NULL," \
        "`size` INTEGER NOT NULL," \
        "`path` TEXT NOT NULL," \
        "`upload` INTEGER default(0)" \
    ")";

const tchar_t *createTable3 = "create table if not exists FileList (" \
        "`check` VARCHAR(65) PRIMARY KEY NOT NULL," \
        "`size` INTEGER NOT NULL," \
        "`path` TEXT NOT NULL," \
        "`upload` INTEGER default(0)," \
        "`update_time` timestamp NOT NULL default(datetime('now','localtime'))" \
    ")";
bool_t scanFile(const tchar_t *directory, _cc_sql_result_t *result) {
    tchar_t sourceFile[_CC_MAX_PATH_] = {0};
    tchar_t check[_CC_SHA1_DIGEST_LENGTH_ * 2 + 1];
    DIR *dpath;
    struct dirent *d;
    struct _stat stat_buf;
    
    if( (dpath = opendir(directory)) == NULL) {
        _cc_logger_error(_T("Couldn't open directory:%s"), directory);
        return false;
    }
    
    while ((d = readdir(dpath)) != NULL) {
        if (d->d_name[0]=='.') {
            continue;
        }
        _sntprintf(sourceFile, _cc_countof(sourceFile), "%s%s", directory, d->d_name);
        _tstat( sourceFile, &stat_buf);

        if (S_ISDIR(stat_buf.st_mode) == 0) {
            //_cc_md5file(sourceFile, check);
            _cc_sha1file(sourceFile, check);
            
            sql_driver.reset(sql, result);
            sql_driver.bind(result, 0, check, _CC_SHA1_DIGEST_LENGTH_ * 2, _CC_SQL_TYPE_STRING_);
            sql_driver.bind(result, 1, &stat_buf.st_size, sizeof(int64_t), _CC_SQL_TYPE_INT64_);
            sql_driver.bind(result, 2, sourceFile, -1, _CC_SQL_TYPE_STRING_);
            sql_driver.step(sql, result);
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

    _cc_init_sqlite(&sql_driver);
    if (_taccess("./sqlite3test.db", _CC_ACCESS_F_) == -1) {
        createTable = true;
    }
    
    sql = sql_driver.connect(SQLite_DB);
    if (sql) {
        printf("connection succed1\n");
    } else {
        printf("connection failed1\n");
        return 1;
    }
    
    sql_driver.begin_transaction(sql);
    if (createTable) {
        sql_driver.execute(sql, createTable1, NULL);
        sql_driver.execute(sql, createTable2, NULL);
        sql_driver.execute(sql, createTable3, NULL);
    }
    sql_driver.execute(sql, _T("delete from `FileList1`;"), NULL);
    sql_driver.execute(sql, _T("delete from `FileList2`;"), NULL);

    if (sql_driver.prepare(sql, _T("INSERT INTO `FileList1` (`check`, `size`, `path`) VALUES (?,?,?);"), &sql_result)) {
        scanFile("./", sql_result);
        sql_driver.free_result(sql, sql_result);
    }
    
    sql_driver.execute(sql, "REPLACE INTO `FileList2` (`check`, `size`, `path`,`upload`) SELECT f1.`check`, f1.`size`, f1.`path`,f2.`upload` FROM FileList1 as f1 left join FileList as f2 on f1.`check`=f2.`check`;", NULL);
    sql_driver.execute(sql, "REPLACE INTO `FileList`(`check`, `size`, `path`,`upload`) SELECT `check`, `size`, `path`, `upload` FROM FileList2;", NULL);
    sql_driver.execute(sql, _T("UPDATE `FileList` SET `upload`=0 WHERE `upload` is null;"), NULL);
    sql_driver.execute(sql, _T("delete from `FileList1`;"), NULL);
    sql_driver.execute(sql, _T("delete from `FileList2`;"), NULL);
    sql_driver.commit(sql);
    
    uint16_t status = 10;
    #define CHECK_SIZE (_CC_SHA1_DIGEST_LENGTH_ * 2 + 1)
    tchar_t check[CHECK_SIZE] = {0};
    memcpy(check, _T("d80c14694c68ca064cff6ad99470029334a2bd38"), CHECK_SIZE-1);
    sql_driver.begin_transaction(sql);
    if (sql_driver.prepare(sql, _T("UPDATE `FileList` SET `upload`=? WHERE `check`=?;"), &sql_result)) {
        sql_driver.reset(sql, sql_result);
        sql_driver.bind(sql_result, 0, &status, sizeof(uint16_t), _CC_SQL_TYPE_UINT16_);
        sql_driver.bind(sql_result, 1, check, CHECK_SIZE - 1, _CC_SQL_TYPE_STRING_);
        sql_driver.step(sql, sql_result);
    }
    sql_driver.free_result(sql, sql_result);
    sql_driver.commit(sql);

    if (sql_driver.execute(sql, "SELECT * FROM FileList;", &sql_result)) {
        int num_fields = sql_driver.get_num_fields(sql_result);
        int i = 0;
        //struct tm *t;
        while(sql_driver.fetch(sql_result)) {
            for (i = 0; i < num_fields; ++i) {
                char_t buff[10240];
                sql_driver.get_string(sql_result, i, buff, 10240);
                printf("%s, ",buff);
            }
            putchar('\n');
            //t = sql_driver.get_datetime(sql_result, num_fields - 1);
            //printf("%4d-%02d-%02d %02d:%02d:%02d\n", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
        }
        sql_driver.free_result(sql, sql_result);
    }
    sql_driver.disconnect(sql);
    return 0;
}
