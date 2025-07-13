#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libcc.h>

#include <sqlext.h>
#include <ODBCINST.H>
#pragma comment(lib,"odbccp32.lib")
#pragma comment(lib,"odbc32.lib")

tchar_t table_name[128];
void GetSQLdelegateList()
{
	WORD wRet = 0;
	TCHAR szdelegates[4096];
	memset(szdelegates, 0, sizeof(szdelegates));
	if(SQLGetInstalleddelegates(szdelegates, _countof(szdelegates), &wRet))
	{
		LPTSTR pszDrv = szdelegates;
		puts(_T("Installed delegator list:\n"));
		while(*pszDrv)
		{
			printf(_T("%s\n"), pszDrv);

			pszDrv += _tcslen(pszDrv) + 1;
		}
		puts(_T("\n"));      
	}
}
BOOL GetExcelAllTableNames( const tchar_t *sExcelFile )
{
	SQLHDBC m_hdbc;
	SQLHENV m_henv;
	UCHAR szConnectOutput[512];
	SWORD nResult;
	SQLHSTMT hstmt = nullptr;
	tchar_t strConnect[1024];
	SDWORD cb;
	char szTable[255];
	char szTableType[255];


	_sntprintf(strConnect,_cc_countof(strConnect),"DBQ=%s;delegator={Microsoft Excel delegator (*.xls)};",
		sExcelFile);
	//分配环境句柄
	if(SQLAllocEnv(&m_henv) != SQL_SUCCESS)
	{
		return false;
	}

	//分配连接句柄
	if(SQLAllocConnect(m_henv,&m_hdbc) != SQL_SUCCESS)
	{
		return false;
	}

	// 连接数据源
	if(SQLdelegateConnect( m_hdbc,nullptr,(UCHAR*)strConnect,SQL_NTS,szConnectOutput,sizeof(szConnectOutput),&nResult,SQL_delegate_COMPLETE ) != SQL_SUCCESS)
	{
		return false;
	}
	if(SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &hstmt) != SQL_SUCCESS)
	{
		return false;
	}

	/*也可对其进行限制，如 if(SQLTables( hstmt, nullptr, 0, nullptr, 0, nullptr, 0, (SQLCHAR*)"TABLE", strlen("TABLE")) != SQL_SUCCESS) 此名则只获取类型为TABLE的表*/
	//此是获取所有表名，包括名字后带有$符号的系统表
	if(SQLTables( hstmt, nullptr, 0, nullptr, 0, nullptr, 0, 0, 0) != SQL_SUCCESS )
	{
		return false;
	}
	SQLBindCol( hstmt, 3, SQL_C_CHAR, szTable, 255, &cb );
	SQLBindCol( hstmt, 4, SQL_C_CHAR, szTableType, 255, &cb );
	while(SQLFetch(hstmt) == SQL_SUCCESS)
	{
		printf("TalbeName:%s\n",szTable);
		strcpy(table_name, szTable);
		break;
	}

	SQLFreeHandle( SQL_HANDLE_STMT, hstmt );
	SQLDisconnect( m_hdbc ); 
	SQLFreeHandle( SQL_HANDLE_ENV, m_henv );

	return true;
}
int main(int argc, char *const arvg[])
{
   int c = 0;
    cc_sql_delegate_t sql_delegate;
    cc_sql_t *conn_ptr = nullptr;
    cc_sql_result_t *sql_result = nullptr;
	tchar_t strConnect[1024];
	tchar_t *file_name = "C:\\sample2.xls";

	/*
        Microsoft Excel 3.0 or 4.0  
        Examples: delegator={Microsoft Excel delegator (*.xls)}; DBQ=c:\temp; delegateID=278
        Microsoft Excel 5.0/7.0 
        Examples: delegator={Microsoft Excel delegator (*.xls)}; DBQ=c:\temp\sample.xls; delegateID=22
    */
	_sntprintf(strConnect,_cc_countof(strConnect),"delegator=Microsoft Excel delegator (*.xls);CREATE_DB=%s;DBQ=%s;READONLY=false;EXCLUSIVE=Yes;",
			file_name,file_name);

	GetSQLdelegateList();
	GetExcelAllTableNames(file_name);
    cc_init_sqlsvr(&sql_delegate);

    conn_ptr = sql_delegate.connect(strConnect);
    if(conn_ptr) {
        printf("connection succed\n");
    } else {
        printf("connection failed\n");
    }
	
	//sql_delegate.execute(conn_ptr, "CREATE TABLE sheet1 (Name TEXT,Age NUMBER)", false);
	
	sql_delegate.execute(conn_ptr, "INSERT INTO  [sheet1$] (Name,Age) VALUES ('徐景周',26)", false);
	sql_delegate.execute(conn_ptr, "INSERT INTO  [sheet1$] (Name,Age) VALUES ('徐志慧',22)", false);
	sql_delegate.execute(conn_ptr, "INSERT INTO  [sheet1$] (Name,Age) VALUES ('郭徽',27)", false);
	
	_sntprintf(strConnect,_cc_countof(strConnect),"SELECT Name,Age FROM [%s];","sheet1$");
	sql_result = sql_delegate.execute(conn_ptr, strConnect, true);


    if (sql_result) {
        int num_fields = 3;//sql_delegate.get_num_fields(sql_result);
        int i = 0;
		while(sql_delegate.fetch_row(sql_result)) {
			char_t buff[256];
			sql_delegate.get_string(sql_result, 1, buff, 256);
			printf("Name:%s Age:%d\n",buff,sql_delegate.get_int32(sql_result, 2));
        }
        sql_delegate.free_result(conn_ptr, sql_result);
    }

    sql_delegate.disconnect(conn_ptr);

	system("pause");
    return 0;
}