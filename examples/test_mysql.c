#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libcc.h>
#include <libcc/widgets/sql.h>

#define MYSQL_DB "mysql://root:123654asd@127.0.0.1:3306/test"

_cc_sql_delegate_t sql_delegate;

int main(int argc, char *const arvg[]) {
    _cc_String_t sql_str;
    _cc_sql_t *conn_ptr1 = nullptr;
    _cc_sql_t *conn_ptr2 = nullptr;
    _cc_sql_result_t *sql_result = nullptr;

    _cc_init_mysql(&sql_delegate);

    conn_ptr1 = sql_delegate.connect(MYSQL_DB);
    if (conn_ptr1) {
        printf("connection succed1\n");
    } else {
        printf("connection failed1\n");
        return 1;
    }

    conn_ptr2 = sql_delegate.connect(MYSQL_DB);
    if (conn_ptr2) {
        printf("connection succed2\n");
    } else {
        printf("connection failed2\n");
        return 1;
    }
    
    _cc_String_Set(sql_str,"TRUNCATE TABLE `test`.`test`");
    sql_delegate.execute(conn_ptr1, &sql_str, nullptr);
    _cc_String_Set(sql_str,"insert into `test`.`test` ( `mid`,`text`,`update_time`) values ( '1',now(),now())");
    sql_delegate.execute(conn_ptr1, &sql_str, nullptr);
    _cc_String_Set(sql_str,"insert into `test`.`test` ( `mid`,`text`,`update_time`) values ( '2',now(),now())");
    sql_delegate.execute(conn_ptr1, &sql_str, nullptr);
    _cc_String_Set(sql_str,"insert into `test`.`test` ( `mid`,`text`,`update_time`) values ( '3',now(),now())");
    sql_delegate.execute(conn_ptr1, &sql_str, nullptr);
    
    sql_delegate.begin_transaction(conn_ptr2);
    _cc_String_Set(sql_str,"update test set mid=100 where id=1");
    sql_delegate.execute(conn_ptr2, &sql_str, nullptr);
    _cc_String_Set(sql_str,"update test set mid=200 where id=2");
    sql_delegate.execute(conn_ptr2, &sql_str, nullptr);
    _cc_String_Set(sql_str,"update test set mid=300 where id=3");
    sql_delegate.execute(conn_ptr2, &sql_str, nullptr);
    sql_delegate.commit(conn_ptr2);
    
    sql_delegate.begin_transaction(conn_ptr1);
    _cc_String_Set(sql_str,"update test set mid=101 where id=1");
    sql_delegate.execute(conn_ptr1, &sql_str, nullptr);
    _cc_String_Set(sql_str,"update test set mid=201 where id=1");
    sql_delegate.execute(conn_ptr1, &sql_str, nullptr);
    sql_delegate.rollback(conn_ptr1);
    _cc_String_Set(sql_str,"update test set mid=301 where id=3");
    sql_delegate.execute(conn_ptr1, &sql_str, nullptr);
    sql_delegate.commit(conn_ptr1);
    
    
    _cc_String_Set(sql_str,_T("call `UpdateDevice`(?,?,?,?);"));
    if (sql_delegate.execute(conn_ptr1, &sql_str, &sql_result)) {
        uint32_t update_time = 100000;
        int a = 0;
        sql_delegate.bind(sql_result, 0, "aaaaa", -1, _CC_SQL_TYPE_STRING_);
        sql_delegate.bind(sql_result, 1, "127.0.0.1", -1, _CC_SQL_TYPE_STRING_);
        sql_delegate.bind(sql_result, 2, &a, sizeof(uint32_t), _CC_SQL_TYPE_UINT32_);
        sql_delegate.bind(sql_result, 3, &update_time, sizeof(uint32_t), _CC_SQL_TYPE_UINT32_);
        sql_delegate.step(conn_ptr1, sql_result);
        sql_delegate.free_result(conn_ptr1, sql_result);
    }

    _cc_String_Set(sql_str,_T("select `id`,`mid`,`update_time`,`text`,`desc` from test where text like ?;"));
    if (sql_delegate.execute(conn_ptr1, &sql_str, &sql_result)) {
        //int v = 1;
        char *date = "2023%%";
        //sql_delegate.bind(sql_result, 0, &v, sizeof(int), _CC_SQL_TYPE_INT32_);
        sql_delegate.bind(sql_result, 0, date, -1, _CC_SQL_TYPE_STRING_);
        sql_delegate.step(conn_ptr1, sql_result);
        //int num_fields = sql_delegate.get_num_fields(sql_result);
        while(sql_delegate.fetch(sql_result)) {
            static struct tm t;
            char text[128];
            char desc[128];
            
            int id = sql_delegate.get_int(sql_result, 0);
            int mid = sql_delegate.get_int(sql_result, 1);
            sql_delegate.get_datetime(sql_result, 2, &t);
            sql_delegate.get_string(sql_result, 3, text, 128);
            sql_delegate.get_string(sql_result, 4, desc, 128);
            printf("%d | %d | %4d-%02d-%02d %02d:%02d:%02d | %s\n", id, mid, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour,
                   t.tm_min, t.tm_sec, text);
        }
        sql_delegate.free_result(conn_ptr1, sql_result);
    }
    
    puts("-----------\n");
    
    _cc_String_Set(sql_str,_T("select `id`,`mid`,`update_time`,`text`,`desc` from test;"));
    if (sql_delegate.execute(conn_ptr1, &sql_str, &sql_result)) {
        //int num_fields = sql_delegate.get_num_fields(sql_result);
        while(sql_delegate.fetch(sql_result)) {
            static struct tm t;
            char text[128];
            char desc[128];
            
            int id = sql_delegate.get_int(sql_result, 0);
            int mid = sql_delegate.get_int(sql_result, 1);
            sql_delegate.get_datetime(sql_result, 2,&t);
            sql_delegate.get_string(sql_result, 3, text, 128);
            sql_delegate.get_string(sql_result, 4, desc, 128);
            printf("%d | %d | %4d-%02d-%02d %02d:%02d:%02d | %s | %s\n", id, mid, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour,
                   t.tm_min, t.tm_sec, text, desc);
        }
        sql_delegate.free_result(conn_ptr1, sql_result);
    }
    sql_delegate.disconnect(conn_ptr1);
    sql_delegate.disconnect(conn_ptr2);
    return 0;
}
