# 数据库集成
***
本教程介绍如何在 libcc 项目中使用数据库，包括 SQLite、MySQL 和 SQL Server 的集成方法。

## 编译选项
在使用数据库功能前，确保在构建时启用相应选项：

-   **SQLite**：`USE_LIB_SQLITE3=1` - 启用 SQLite 数据库支持
-   **MySQL**：`USE_LIB_MYSQL=1` - 启用 MySQL 数据库支持
-   **SQL Server**：`USE_LIB_SQLSERVER=1` - 启用 SQL Server 数据库支持

在 Makefile 中添加这些选项，或修改 `build/local-init.mk`。

## SQLite 集成
SQLite 是一个轻量级的嵌入式数据库，无需单独的服务器进程。

### 示例代码
基于 [`tests/test_sqlite.c`](https://github.com/libcc/libcc/blob/2.0/tests/test_sqlite.c)。

```c
#include <libcc.h>
#include <libcc/sql.h>

int main(void) {
    _cc_sql_delegate_t sql_delegate;
    _cc_sql_result_t *result = NULL;
    _cc_sql_t *sql = NULL;
    int32_t ret = EXIT_FAILURE;

    /* 初始化 SQLite 委托 */
    if (!_cc_init_sqlite(&sql_delegate)) {
        _cc_logger_error(_T("Failed to initialize SQLite delegate"));
        return EXIT_FAILURE;
    }

    /* 连接数据库 */
    sql = sql_delegate.connect(_T("SQLITE:///./example.db"));
    if (!sql) {
        _cc_logger_error(_T("SQLite connect failed"));
        return EXIT_FAILURE;
    }

    /* 创建表 */
    if (!sql_delegate.execute(sql, _CC_SQL("CREATE TABLE IF NOT EXISTS demo (id INTEGER PRIMARY KEY, val TEXT);"), NULL)) {
        _cc_logger_error(_T("Failed to create table"));
        goto cleanup;
    }

    /* 插入数据 */
    if (!sql_delegate.execute(sql, _CC_SQL("INSERT INTO demo(val) VALUES('hello');"), NULL)) {
        _cc_logger_error(_T("Failed to insert data"));
        goto cleanup;
    }

    /* 查询数据 */
    if (sql_delegate.execute(sql, _CC_SQL("SELECT id, val FROM demo;"), &result)) {
        while (sql_delegate.fetch(result)) {
            char buff[256] = {0};
            if (sql_delegate.get_string(result, 1, buff, sizeof(buff))) {
                _cc_logger_info(_T("row: %s"), buff);
            }
        }
        sql_delegate.free_result(sql, result);
    } else {
        _cc_logger_error(_T("Query failed"));
        goto cleanup;
    }

    ret = EXIT_SUCCESS;

cleanup:
    /* 断开连接 */
    sql_delegate.disconnect(sql);
    return ret;
}
```

### 注意事项
-   SQLite 数据库文件路径支持相对和绝对路径。
-   连接字符串格式：`SQLITE:///[path/to/database.db]`
-   SQLite 是文件级锁定，多进程并发访问可能导致问题。
-   适合小型应用或嵌入式场景。

## MySQL 集成
MySQL 是一个流行的开源关系型数据库，需要单独的 MySQL 服务器。

### 示例代码
基于 [`tests/test_mysql.c`](https://github.com/libcc/libcc/blob/2.0/tests/test_mysql.c)。

```c
#include <libcc.h>
#include <libcc/sql.h>

int main(void) {
    _cc_sql_delegate_t sql_delegate;
    _cc_sql_result_t *result = NULL;
    _cc_sql_t *sql = NULL;
    int32_t ret = EXIT_FAILURE;

    /* 初始化 MySQL 委托 */
    if (!_cc_init_mysql(&sql_delegate)) {
        _cc_logger_error(_T("Failed to initialize MySQL delegate"));
        return EXIT_FAILURE;
    }

    /* 连接数据库 */
    /* 格式: MYSQL://username:password@host:port/database */
    sql = sql_delegate.connect(_T("MYSQL://user:pass@localhost:3306/testdb"));
    if (!sql) {
        _cc_logger_error(_T("MySQL connect failed"));
        return EXIT_FAILURE;
    }

    /* 创建表 */
    if (!sql_delegate.execute(sql, _CC_SQL("CREATE TABLE IF NOT EXISTS demo (id INT AUTO_INCREMENT PRIMARY KEY, val VARCHAR(255));"), NULL)) {
        _cc_logger_error(_T("Failed to create table"));
        goto cleanup;
    }

    /* 插入数据 */
    if (!sql_delegate.execute(sql, _CC_SQL("INSERT INTO demo(val) VALUES('hello');"), NULL)) {
        _cc_logger_error(_T("Failed to insert data"));
        goto cleanup;
    }

    /* 查询数据 */
    if (sql_delegate.execute(sql, _CC_SQL("SELECT id, val FROM demo;"), &result)) {
        while (sql_delegate.fetch(result)) {
            char buff[256] = {0};
            if (sql_delegate.get_string(result, 1, buff, sizeof(buff))) {
                _cc_logger_info(_T("row: %s"), buff);
            }
        }
        sql_delegate.free_result(sql, result);
    } else {
        _cc_logger_error(_T("Query failed"));
        goto cleanup;
    }

    ret = EXIT_SUCCESS;

cleanup:
    /* 断开连接 */
    sql_delegate.disconnect(sql);
    return ret;
}
```

### 注意事项
-   需要安装 MySQL 客户端库（libmysqlclient）。
-   连接字符串格式：`MYSQL://username:password@host:port/database`
-   支持连接池和事务。
-   适合中大型应用和高并发场景。

## SQL Server 集成
SQL Server 是微软的商业关系型数据库。

### 示例代码
基于 [`tests/test_sqlserver.c`](https://github.com/libcc/libcc/blob/2.0/tests/test_sqlserver.c)。

```c
#include <libcc.h>
#include <libcc/sql.h>

int main(void) {
    _cc_sql_delegate_t sql_delegate;
    _cc_sql_result_t *result = NULL;
    _cc_sql_t *sql = NULL;
    int32_t ret = EXIT_FAILURE;

    /* 初始化 SQL Server 委托 */
    if (!_cc_init_sqlsvr(&sql_delegate)) {
        _cc_logger_error(_T("Failed to initialize SQL Server delegate"));
        return EXIT_FAILURE;
    }

    /* 连接数据库 */
    /* 格式: MSSQL://username:password@host:port/database */
    sql = sql_delegate.connect(_T("MSSQL://sa:password@localhost:1433/testdb"));
    if (!sql) {
        _cc_logger_error(_T("SQL Server connect failed"));
        return EXIT_FAILURE;
    }

    /* 创建表 */
    if (!sql_delegate.execute(sql, _CC_SQL("CREATE TABLE demo (id INT IDENTITY(1,1) PRIMARY KEY, val NVARCHAR(255));"), NULL)) {
        _cc_logger_error(_T("Failed to create table"));
        goto cleanup;
    }

    /* 插入数据 */
    if (!sql_delegate.execute(sql, _CC_SQL("INSERT INTO demo(val) VALUES('hello');"), NULL)) {
        _cc_logger_error(_T("Failed to insert data"));
        goto cleanup;
    }

    /* 查询数据 */
    if (sql_delegate.execute(sql, _CC_SQL("SELECT id, val FROM demo;"), &result)) {
        while (sql_delegate.fetch(result)) {
            char buff[256] = {0};
            if (sql_delegate.get_string(result, 1, buff, sizeof(buff))) {
                _cc_logger_info(_T("row: %s"), buff);
            }
        }
        sql_delegate.free_result(sql, result);
    } else {
        _cc_logger_error(_T("Query failed"));
        goto cleanup;
    }

    ret = EXIT_SUCCESS;

cleanup:
    /* 断开连接 */
    sql_delegate.disconnect(sql);
    return ret;
}
```

### 注意事项
-   需要安装 SQL Server 客户端库。
-   连接字符串格式：`MSSQL://username:password@host:port/database`
-   支持 Windows 身份验证和 SQL Server 身份验证。
-   适合企业级应用。

## 通用 SQL 接口
libcc 提供统一的 SQL 抽象层 ` _cc_sql_delegate_t`，包含以下主要函数：

-   `connect()`：连接数据库
-   `disconnect()`：断开连接
-   `execute()`：执行 SQL 语句
-   `fetch()`：获取查询结果行
-   `get_string()`：获取字符串字段值
-   `free_result()`：释放结果集

## 高级用法
### 事务处理
```c
/* 开始事务 */
sql_delegate.execute(sql, _CC_SQL("BEGIN;"), NULL);

/* 执行多个操作 */
sql_delegate.execute(sql, _CC_SQL("INSERT INTO table1 ..."), NULL);
sql_delegate.execute(sql, _CC_SQL("UPDATE table2 ..."), NULL);

/* 提交或回滚 */
sql_delegate.execute(sql, _CC_SQL("COMMIT;"), NULL);
// 或 sql_delegate.execute(sql, _CC_SQL("ROLLBACK;"), NULL);
```

### 预编译语句
```c
/* 预编译插入语句 */
_cc_sql_stmt_t *stmt = sql_delegate.prepare(sql, _CC_SQL("INSERT INTO demo(val) VALUES(?);"));
if (stmt) {
    sql_delegate.bind_string(stmt, 1, "test value");
    sql_delegate.execute_stmt(stmt);
    sql_delegate.free_stmt(stmt);
}
```

## 注意事项
-   **编译依赖**：确保系统安装了相应数据库的开发库。
-   **连接管理**：及时断开连接，避免资源泄漏。
-   **错误处理**：检查所有操作的返回值。
-   **SQL 注入**：使用参数化查询或预编译语句防止 SQL 注入。
-   **字符编码**：注意数据库和应用的字符编码一致性。
-   **并发访问**：SQLite 不支持多进程并发，MySQL/SQL Server 支持。
-   **性能优化**：使用连接池、索引和查询优化。
-   **备份恢复**：定期备份重要数据。