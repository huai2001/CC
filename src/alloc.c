/*
 * Copyright libcc.cn@gmail.com. and other libcc contributors.
 * All rights reserved.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/
#include <time.h>
#include <libcc/dirent.h>
#include <libcc/generic.h>
#include <libcc/alloc.h>
#include <libcc/string.h>
#include <libcc/atomic.h>

#ifndef __CC_WINDOWS__
#include <dlfcn.h>
#endif

static tchar_t *mem_types[4] = {"free","_cc_malloc","_cc_calloc","_cc_realloc"};

/* Explicitly override malloc/free etc when using tcmalloc. */
#if defined(__CC_USE_TCMALLOC__)
    #define malloc(size) tc_malloc(size)
    #define calloc(count, size) tc_calloc(count, size)
    #define realloc(ptr, size) tc_realloc(ptr, size)
    #define free(ptr) tc_free(ptr)
#elif defined(__CC_USE_JEMALLOC__)
    #define malloc(size) je_malloc(size)
    #define calloc(count, size) je_calloc(count, size)
    #define realloc(ptr, size) je_realloc(ptr, size)
    #define free(ptr) je_free(ptr)
#endif

#ifdef _CC_ENABLE_MEMORY_TRACKED_
#if _CC_USE_SYSTEM_SQLITE3_LIB_
#include <sqlite3.h>
#else
#include <sqlite3/sqlite3.h>
#endif
static sqlite3 *g_memory_db = nullptr;

_CC_API_PRIVATE(void) open_sqlite() {
    bool_t is_create_table = false;
    tchar_t path[_CC_MAX_PATH_];
    tchar_t sqlite3_file[_CC_MAX_PATH_];

    const tchar_t *createMemoryTable = "CREATE TABLE IF NOT EXISTS `MEMORYS` (" \
        "`ID`       INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
        "`PTR`      BIGINT          NOT NULL," \
        "`SIZE`     INTEGER         DEFAULT (0)," \
        "`METHOD`   VARCHAR(64)     DEFAULT ('-')," \
        "`Name`     VARCHAR(64)     DEFAULT ('-')," \
        "`MSG`      TEXT            DEFAULT ('-')," \
        "`ALLOC_DATE` timestamp NOT NULL DEFAULT(DATETIME('now','localtime'))" \
    ");"\
    "CREATE INDEX INDEX_PTR ON MEMORYS (`PTR`);";

    _cc_get_executable_path(path,_CC_MAX_PATH_);
    _cc_fpath(sqlite3_file,_cc_countof(sqlite3_file), _T("%s.memory.db"), path);

    if (_taccess(sqlite3_file, _CC_ACCESS_F_) == -1) {
        is_create_table = true;
    }

    if (sqlite3_open(sqlite3_file, &g_memory_db) != SQLITE_OK) {
        return ;
    }

    if (is_create_table) {
        if (sqlite3_exec(g_memory_db, createMemoryTable, NULL, NULL, nullptr) != SQLITE_OK) {
            sqlite3_close(g_memory_db);
            g_memory_db = nullptr;
        }
    }
}

_CC_API_PRIVATE(void) close_sqlite() {
    sqlite3_close(g_memory_db);
    g_memory_db = nullptr;
}

/*
 * Remove a pointer to the rbtree with some key
 */
_CC_API_PUBLIC(void) __cc_tracked_memory_unlink(uintptr_t ptr) {
    if (g_memory_db == nullptr) {
        open_sqlite();
    }
    if (g_memory_db) {
        sqlite3_stmt *stmt;
        const _cc_String_t delete_sql = _cc_String("DELETE FROM MEMORYS WHERE `PTR` = ?;");
        if (sqlite3_prepare_v2(g_memory_db, delete_sql.data, (int)delete_sql.length, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, ptr);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
}

/*
 * Add a pointer to the rbtree with some key
 */
_CC_API_PUBLIC(void) __cc_tracked_memory(uintptr_t ptr, size_t size, const int _type) {
    tchar_t buffer[_CC_8K_BUFFER_SIZE_];
    size_t buffer_length;
    if (g_memory_db == nullptr) {
        open_sqlite();
    }

    buffer_length = _cc_get_resolve_symbol(buffer, _CC_8K_BUFFER_SIZE_);
 
    if (g_memory_db) {
        sqlite3_stmt *stmt;
        const _cc_String_t *module_name = _cc_get_module_file_name();
        const _cc_String_t insert_sql = _cc_String("INSERT INTO MEMORYS(`PTR`,`SIZE`,`METHOD`,`NAME`,`MSG`) VALUES (?, ?, ?, ?, ?);");
        if (sqlite3_prepare_v2(g_memory_db, insert_sql.data, (int)insert_sql.length, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, ptr);
            sqlite3_bind_int64(stmt, 2, size);
            sqlite3_bind_text(stmt, 3, mem_types[_type], -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, module_name->data, (int)module_name->length, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 5, buffer, (int)buffer_length, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
}
#endif
/**/
_CC_API_PRIVATE(pvoid_t) __cc_check_memory(pvoid_t ptr, size_t size, const int _type) {
    if (_cc_unlikely(nullptr == ptr)) {
        _cc_logger_error(_T("%s: Out of memory trying to allocate %zu bytes"), mem_types[_type], size);
        _cc_abort();
    }
#ifdef _CC_ENABLE_MEMORY_TRACKED_
    __cc_tracked_memory((uintptr_t)ptr, size, _type);
#endif
    return ptr;
}

/**/
_CC_API_PUBLIC(pvoid_t) _cc_malloc(size_t n) {
    return __cc_check_memory(malloc(n), n, _CC_MEM_MALLOC_);
}

/**/
_CC_API_PUBLIC(pvoid_t) _cc_calloc(size_t c, size_t n) {
    return __cc_check_memory(calloc(c, n), c * n, _CC_MEM_CALLOC_);
}

/**/
_CC_API_PUBLIC(pvoid_t) _cc_realloc(pvoid_t d, size_t n) {
    if (_cc_unlikely(n <= 0)) {
        _cc_free(d);
        return nullptr;
    }

    if (_cc_unlikely(d == nullptr)) {
        return _cc_malloc(n);
    }
    
#ifdef _CC_ENABLE_MEMORY_TRACKED_
    __cc_tracked_memory_unlink((uintptr_t)d);
#endif

    return __cc_check_memory(realloc(d, n), n, _CC_MEM_REALLOC_);
}

/**/
_CC_API_PUBLIC(void) _cc_free(pvoid_t p) {
    _cc_assert(p != nullptr);
#ifdef _CC_ENABLE_MEMORY_TRACKED_
    __cc_tracked_memory_unlink((uintptr_t)p);
#endif
    free(p);
}

/**/
_CC_API_PUBLIC(wchar_t*) _cc_strdupW(const wchar_t *str) {
    return _cc_strndupW(str, wcslen(str));
}

/**/
_CC_API_PUBLIC(char_t*) _cc_strdupA(const char_t *str) {
    return _cc_strndupA(str, strlen(str));
}

/**/
_CC_API_PUBLIC(wchar_t*) _cc_strndupW(const wchar_t *str, size_t str_len) {
    wchar_t *req_str;

    if (_cc_unlikely(str_len <= 0)) {
        return nullptr;
    }

    req_str = (wchar_t *)_cc_malloc(sizeof(wchar_t) * (str_len + 1));
    if (_cc_likely(req_str)) {
        memcpy(req_str, str, str_len * sizeof(wchar_t));
        req_str[str_len] = 0;
    }

    return req_str;
}

/**/
_CC_API_PUBLIC(char_t*) _cc_strndupA(const char_t *str, size_t str_len) {
    char_t *req_str;

    if (_cc_unlikely(str_len <= 0)) {
        return nullptr;
    }

    req_str = (char_t *)_cc_malloc(sizeof(char_t) * (str_len + 1));
    if (_cc_likely(req_str)) {
        memcpy(req_str, str, str_len * sizeof(char_t));
        req_str[str_len] = 0;
    }

    return req_str;
}
