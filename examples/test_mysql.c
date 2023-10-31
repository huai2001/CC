#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libcc.h>
#include <cc/db/sql.h>=

#define MYSQL_DB "mysql://root:123654asd@127.0.0.1:3306/test"

_cc_sql_driver_t sql_driver;

int main(int argc, char *const arvg[]) {
    _cc_sql_t *conn_ptr1 = NULL;
    _cc_sql_t *conn_ptr2 = NULL;
    _cc_sql_result_t *sql_result = NULL;

    _cc_init_mysql(&sql_driver);

    conn_ptr1 = sql_driver.connect(MYSQL_DB);
    if (conn_ptr1) {
        printf("connection succed1\n");
    } else {
        printf("connection failed1\n");
        return 1;
    }

    conn_ptr2 = sql_driver.connect(MYSQL_DB);
    if (conn_ptr2) {
        printf("connection succed2\n");
    } else {
        printf("connection failed2\n");
        return 1;
    }
    
    sql_driver.execute(conn_ptr1, "TRUNCATE TABLE `test`.`test`", NULL);
    sql_driver.execute(conn_ptr1, "insert into `test`.`test` ( `mid`,`text`,`update_time`) values ( '1',now(),now())", NULL);
    sql_driver.execute(conn_ptr2, "insert into `test`.`test` ( `mid`,`text`,`update_time`) values ( '2',now(),now())", NULL);
    sql_driver.execute(conn_ptr1, "insert into `test`.`test` ( `mid`,`text`,`update_time`) values ( '3',now(),now())", NULL);
    
    sql_driver.begin_transaction(conn_ptr2);
    sql_driver.execute(conn_ptr2, "update test set mid=100 where id=1", NULL);
    sql_driver.execute(conn_ptr2, "update test set mid=200 where id=2", NULL);
    sql_driver.execute(conn_ptr2, "update test set mid=300 where id=3", NULL);
    sql_driver.commit(conn_ptr2);
    
    sql_driver.begin_transaction(conn_ptr1);
    sql_driver.execute(conn_ptr1, "update test set mid=101 where id=1", NULL);
    sql_driver.execute(conn_ptr1, "update test set mid=202 where id=2", NULL);
    sql_driver.rollback(conn_ptr1);
    sql_driver.execute(conn_ptr1, "update test set mid=303 where id=3", NULL);
    sql_driver.commit(conn_ptr1);
    
    
    if (sql_driver.prepare(conn_ptr1, "select `id`,`mid`,`update_time`,`text`,`desc` from test where text like ?;", &sql_result)) {
        //int v = 1;
        char *date = "2023%%";
        //sql_driver.bind(sql_result, 0, &v, sizeof(int), _CC_SQL_TYPE_INT32_);
        sql_driver.bind(sql_result, 0, date, -1, _CC_SQL_TYPE_STRING_);
        sql_driver.step(conn_ptr1, sql_result);
        //int num_fields = sql_driver.get_num_fields(sql_result);
        while(sql_driver.fetch(sql_result)) {
            static struct tm t;
            char text[128];
            char desc[128];
            
            int id = sql_driver.get_int(sql_result, 0);
            int mid = sql_driver.get_int(sql_result, 1);
            sql_driver.get_datetime(sql_result, 2, &t);
            sql_driver.get_string(sql_result, 3, text, 128);
            sql_driver.get_string(sql_result, 4, desc, 128);
            printf("%d | %d | %4d-%02d-%02d %02d:%02d:%02d | %s\n", id, mid, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour,
                   t.tm_min, t.tm_sec, text);
        }
        sql_driver.free_result(conn_ptr1, sql_result);
    }
    
    puts("-----------\n");
    
    if (sql_driver.execute(conn_ptr1, "select `id`,`mid`,`update_time`,`text`,`desc` from test;", &sql_result)) {
        //int num_fields = sql_driver.get_num_fields(sql_result);
        while(sql_driver.fetch(sql_result)) {
            static struct tm t;
            char text[128];
            char desc[128];
            
            int id = sql_driver.get_int(sql_result, 0);
            int mid = sql_driver.get_int(sql_result, 1);
            sql_driver.get_datetime(sql_result, 2,&t);
            sql_driver.get_string(sql_result, 3, text, 128);
            sql_driver.get_string(sql_result, 4, desc, 128);
            printf("%d | %d | %4d-%02d-%02d %02d:%02d:%02d | %s | %s\n", id, mid, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour,
                   t.tm_min, t.tm_sec, text, desc);
        }
        sql_driver.free_result(conn_ptr1, sql_result);
    }
    sql_driver.disconnect(conn_ptr1);
    sql_driver.disconnect(conn_ptr2);
    return 0;
}
