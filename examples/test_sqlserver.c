#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libcc.h>
#include <libcc/widgets/sql.h>

#include <locale.h>


static _cc_sql_delegate_t sql_delegate;

#if 0
#include <sqlext.h>
#include <odbcinst.h>
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
		return nullptr;
	}
}
static bool_t _sqlsvr_get_field_names(_cc_sql_result_t *res) {
	SQLTCHAR names[256];
	SQLSMALLINT namelen, datatype, i;
	SQLRETURN res;

	int32_t numcols = sql_delegate.get_num_fields(res);
	for (i = 1; i <= numcols; i++) {
		res = SQLDescribeCol(sql_delegate.get_stmt(res), i, names, sizeof(names),
			&namelen, &datatype, nullptr, nullptr, nullptr);
		if (res == SQL_ERROR) {
			return false;
		}
		printf("%s(%s), ", names, _sqlsvr_types(datatype));
	}
	printf("\n");
	return true;
}
#endif

int main(int argc, char *const arvg[]) {
    _cc_sql_t *conn_ptr = nullptr;
    _cc_sql_result_t *sql_result = nullptr;
	tchar_t strConnect[1024];
    _cc_String_t sql_str;
    
    setlocale(LC_ALL, "chs");

#if 0
    WORD res = 0;
    tchar_t delegates[4096];
    bzero(delegates, sizeof(delegates));
    if (SQLGetInstalledDrivers(delegates, _cc_countof(delegates), &res)) {
        tchar_t *p = delegates;
        int i = 0;
        while(*p) {
            _tprintf(_T("%d. %s\n"), i, p);
            p += _tcslen(p) + 1;
            i++;
        }
    }

#endif
    _sntprintf(strConnect,_cc_countof(strConnect),
    	_T("Driver={ODBC Driver 18 for SQL Server};Server=tcp:%s,1433;Database=%s;Uid=libcc;Pwd=%s;Encrypt=yes;TrustServerCertificate=no;Connection Timeout=60;"),
    	_T("libcc.database.windows.net"),_T("username"),_T("password"));

    //_sntprintf(strConnect,_cc_countof(strConnect),_T("Driver={SQL Server};SERVER=%s,%d;UID=%s;PWD=%s;DATABASE=%s"), _T("127.0.0.1"), 1433, _T("sa"), _T("123456"), _T("test"));
    
     //_sntprintf(strConnect,_cc_countof(strConnect),_T("Driver={%s};DSN='';FIRSTROWHASNAMES=1;READONLY=false;CREATE_DB=\"%s\";DBQ=%s"), _T("MICROSOFT EXCEL delegator (*.XLS)"),_T("c:\\table.xls"),_T("c:\\table.xls"));
    _cc_init_sqlsvr(&sql_delegate);

    conn_ptr = sql_delegate.connect(strConnect);
    
    if(conn_ptr) {
        _cc_logger_error("connection succed\n");
    } else {
        _cc_logger_error("connection failed\n");
        return 0;
    }
    //_cc_String_Set(sql_str, _T("CREATE TABLE [test] ([mid] INT,[price] DECIMAL(15,2),[text] VARCHAR(200), [date] DATETIME)"));
    //sql_delegate.execute(conn_ptr, &sql_str, nullptr);
    _cc_String_Set(sql_str,_T("TRUNCATE TABLE test;"));
    sql_delegate.execute(conn_ptr, &sql_str, nullptr);
    _cc_String_Set(sql_str,_T("insert into test ( mid, price, text, date) values ( '1',100.99,'test1','2025-8-21 18:14:58')"));
    sql_delegate.execute(conn_ptr, &sql_str, nullptr);
    _cc_String_Set(sql_str,_T("insert into test ( mid, price, text, date) values ( '3',120.01,'test3', '2025-8-21 18:15:18')"));
    sql_delegate.execute(conn_ptr, &sql_str, nullptr);
    _cc_String_Set(sql_str,_T("insert into test ( mid, price, text, date) values ( '2',103.43,'test2', '2025-8-21 18:15:18')"));
    sql_delegate.execute(conn_ptr, &sql_str, nullptr);

    _cc_String_Set(sql_str,_T("insert into test ( mid, price, text, date) values ( ?, ?, ?, ?)"));
    if (sql_delegate.execute(conn_ptr, &sql_str, &sql_result)) {
    	struct tm tm_now;
    	uint16_t status = 10,i;
    	double price = 10.473;
    	time_t now = time(nullptr);
    	_cc_gmtime(&now, &tm_now);
    	_cc_String_t text = _cc_String("insert");
    	for (i = 0; i < 10; i++) {
    		status += i;
    		price += price * i;
	        sql_delegate.reset(conn_ptr, sql_result);
	        sql_delegate.bind(sql_result, 0, &status, sizeof(uint16_t), _CC_SQL_TYPE_UINT16_);
	        sql_delegate.bind(sql_result, 1, &price, sizeof(double), _CC_SQL_TYPE_DOUBLE_);
	        sql_delegate.bind(sql_result, 2, text.data, text.length, _CC_SQL_TYPE_STRING_);
	        sql_delegate.bind(sql_result, 3, &tm_now, sizeof(struct tm), _CC_SQL_TYPE_DATETIME_);
	        sql_delegate.step(conn_ptr, sql_result);
	    }
        sql_delegate.free_result(conn_ptr, sql_result);
    }
    
    sql_delegate.begin_transaction(conn_ptr);
    _cc_String_Set(sql_str,_T("update [test] set mid=100 where mid=1"));
    sql_delegate.execute(conn_ptr, &sql_str, nullptr);
    _cc_String_Set(sql_str,_T("update [test] set mid=200 where mid=2"));
    sql_delegate.execute(conn_ptr, &sql_str, nullptr);
    _cc_String_Set(sql_str,_T("update [test] set mid=300 where mid=3"));
    sql_delegate.execute(conn_ptr, &sql_str, nullptr);
    sql_delegate.commit(conn_ptr);
    _cc_String_Set(sql_str,_T("update [test] set mid=600 where mid=300"));
    sql_delegate.execute(conn_ptr, &sql_str, nullptr);
	sql_delegate.rollback(conn_ptr);

    _cc_String_Set(sql_str, _T("select * from [test]"));

    if (sql_delegate.execute(conn_ptr, &sql_str, &sql_result)) {
        int num_fields = sql_delegate.get_num_fields(sql_result);
        int i = 0;
        int n = 0;
        while(sql_delegate.fetch(sql_result)) {
            _tprintf(_T("%d, "),n++);
            for (i = 0; i < num_fields; ++i) {
                tchar_t buff[256];
                sql_delegate.get_string(sql_result, i, buff, 256);
                _tprintf(_T("%s, "),buff);
                //printf("%lld, ", sql_delegate.get_int64(sql_result, i));
            }
            printf("\n");
        }
        #if 0
        _sqlsvr_get_field_names(sql_result);
        #endif
        sql_delegate.free_result(conn_ptr, sql_result);
    }
	
    sql_delegate.disconnect(conn_ptr);

	system("pause");
    return 0;
}
