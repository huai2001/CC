INCLUDE_PATH	+= /usr/local/include
ifneq (,$(filter $(PLATFORM),mac64 mac32))
	LIBRARY_PATH	+= /usr/local/mysql/lib/
	MACROS	+= _CC_ENABLE_UNIXODBC_=1
	INCLUDE_PATH	+= /usr/local/Cellar/unixodbc/2.3.9_1/include/
	LIBRARY_PATH	+= /usr/local/Cellar/unixodbc/2.3.9_1/lib/
	LOCAL_SRC_FILES = $(SRCROOT)/src/db/sqlsvr.o

	LIBS += odbc
else
	LIBRARY_PATH	+= /usr/lib64/mysql/
	INCLUDE_PATH	+= /usr/include/
endif


ifdef shared
	LIBS	+= sqlite3 mysqlclient
	MACROS	+= _CC_DB_EXPORT_SHARED_LIBRARY_=1
endif

LOCAL_SRC_FILES += $(SRCROOT)/src/db/sqlite.o $(SRCROOT)/src/db/mysql.o  