#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libcc.h>
#include <cc/db/sql.h>
#include <sqlext.h>
#include <odbcinst.h>
#include <cc/db/sql.h>
#include <locale.h>


static _cc_sql_driver_t sql_driver;


/*
** Returns the name  for a SQL type.
*/
static const char *_sqlsvr_types (const SQLSMALLINT type) {
	switch (type) {
	case SQL_UNKNOWN_TYPE:
	case SQL_CHAR:
	case SQL_VARCHAR: 
	case SQL_TYPE_DATE:
	case SQL_TYPE_TIME:
	case SQL_TYPE_TIMESTAMP: 
	case SQL_DATE:
	case SQL_INTERVAL:
	case SQL_TIMESTAMP: 
	case SQL_LONGVARCHAR:
	case SQL_WCHAR:
	case SQL_WVARCHAR:
	case SQL_WLONGVARCHAR:
		return "string";
	case SQL_BIGINT:
		return "big int";
	case SQL_TINYINT: 
	case SQL_INTEGER:
	case SQL_SMALLINT: 
		return "integer";
	case SQL_NUMERIC:
	case SQL_DECIMAL: 
	case SQL_FLOAT:
		return "float";
	case SQL_REAL:
	case SQL_DOUBLE:
		return "double";
	case SQL_BINARY:
	case SQL_VARBINARY:
	case SQL_LONGVARBINARY:
		return "binary";	/* !!!!!! nao seria string? */
	case SQL_BIT:
		return "boolean";
	default:
		assert(0);
		return NULL;
	}
}
static bool_t _sqlsvr_get_field_names(_cc_sql_result_t *res) {
	SQLTCHAR names[256];
	SQLSMALLINT namelen, datatype, i;
	SQLRETURN res;

	int32_t numcols = sql_driver.get_num_fields(res);
	for (i = 1; i <= numcols; i++) {
		res = SQLDescribeCol(sql_driver.get_stmt(res), i, names, sizeof(names),
			&namelen, &datatype, NULL, NULL, NULL);
		if (res == SQL_ERROR) {
			return false;
		}
		printf("%s(%s), ", names, _sqlsvr_types(datatype));
	}
	printf("\n");
	return true;
}

int main(int argc, char *const arvg[]) {
    _cc_sql_t *conn_ptr = NULL;
    _cc_sql_result_t *sql_result = NULL;
	tchar_t strConnect[1024];
    
    tchar_t drivers[4096];
    WORD res = 0;

    setlocale(LC_ALL, "chs");

    bzero(drivers, sizeof(drivers));
    if (SQLGetInstalledDrivers(drivers, _cc_countof(drivers), &res)) {
        tchar_t *p = drivers;
        int i = 0;
        while(*p) {
            _tprintf(_T("%d. %s\n"), i, p);
            p += _tcslen(p) + 1;
            i++;
        }
    }
    
    _sntprintf(strConnect,_cc_countof(strConnect),_T("DRIVER={SQL Server};SERVER=%s,%d;UID=%s;PWD=%s;DATABASE=%s"), _T("127.0.0.1"), 1433, _T("sa"), _T("123456"), _T("test"));
    
     //_sntprintf(strConnect,_cc_countof(strConnect),_T("DRIVER={%s};DSN='';FIRSTROWHASNAMES=1;READONLY=false;CREATE_DB=\"%s\";DBQ=%s"), _T("MICROSOFT EXCEL DRIVER (*.XLS)"),_T("c:\\table.xls"),_T("c:\\table.xls"));
    _cc_init_sqlsvr(&sql_driver);

    conn_ptr = sql_driver.connect(strConnect);
    
    if(conn_ptr) {
        _cc_logger_error("connection succed\n");
    } else {
        _cc_logger_error("connection failed\n");
        return 0;
    }
    
    sql_driver.execute(conn_ptr, _T("CREATE TABLE [test] ([mid] VARCHAR,[text] VARCHAR)"), false);
    
    //sql_driver.execute(conn_ptr, _T("TRUNCATE TABLE [test]"), false);
    sql_driver.execute(conn_ptr, _T("insert into [test] ( [mid],[text]) values ( '1','test1')"), false);
    sql_driver.execute(conn_ptr, _T("insert into [test] ( [mid],[text]) values ( '2','test2')"), false);
    sql_driver.execute(conn_ptr, _T("insert into [test] ( [mid],[text]) values ( '3','test3')"), false);
    /*
    sql_driver.begin_transaction(conn_ptr);
    sql_driver.execute(conn_ptr, _T("update [test] set mid=100 where id=1"), false);
    sql_driver.execute(conn_ptr, _T("update [test] set mid=200 where id=2"), false);
    sql_driver.execute(conn_ptr, _T("update [test] set mid=300 where id=3"), false);
    sql_driver.commit(conn_ptr);

    sql_driver.begin_transaction(conn_ptr);
    sql_driver.execute(conn_ptr, _T("update [test] set mid=101 where id=1"), false);
    sql_driver.execute(conn_ptr, _T("update [test] set mid=202 where id=2"), false);
    sql_driver.rollback(conn_ptr);
    sql_driver.execute(conn_ptr, _T("update [test] set mid=4012345678 where id=3"), false);
    sql_driver.commit(conn_ptr);
    */
    sql_result = sql_driver.execute(conn_ptr, _T("select * from [test]"), true);

    if (sql_result) {
        int num_fields = sql_driver.get_num_fields(sql_result);
        int i = 0;
        while(sql_driver.fetch(sql_result)) {
            for (i = 0; i < num_fields; ++i) {
                tchar_t buff[256];
                sql_driver.get_string(sql_result, i, buff, 256);
                _tprintf(_T("%s, "),buff);
                //printf("%lld, ", sql_driver.get_int64(sql_result, i));
            }
            printf("\n");
        }
        _sqlsvr_get_field_names(sql_result);
        sql_driver.free_result(conn_ptr, sql_result);
    }
	

    sql_driver.disconnect(conn_ptr);

	system("pause");
    return 0;
}
