#make .dll target=widgets event=1 json=1 db=1 debug=1
WIDGET_FILES = $(SRCROOT)/src/widgets
TARGET_NAME = cc.$(target)

ifdef all
	event = 1
	json = 1
	db = 1
	url_request = 1
endif

LIBS	+= cc

ifdef shared
	MACROS += _CC_WIDGETS_EXPORT_SHARED_LIBRARY_=1
	ifdef url_request
		MACROS += _CC_ENABLE_OPENSSL_=1 
	endif

	ifeq ($(PLATFORM), osx)
		LIBS += ssl.3 crypto.3
	else
		LIBS += ssl-3-x64 crypto-3-x64
	endif

	ifdef db
		LIBS += sqlite3 mysql
	endif

endif

#SQLite Download Page
#https://www.sqlite.org/download.html
# download：sqlite-amalgamation-3500100.zip sqlit3 header
# MSYS2 build sqlite3
# gcc -shared -o sqlite3.dll sqlite3.c -Wl,--out-implib,libsqlite3.a
# gcc -DSQLITE_ENABLE_COLUMN_METADATA sqlite3.c -shared -o sqlite3.dll -Wl,--out-implib,libsqlite3.a
# Linux Ubuntu/Debian
# bash sudo apt install libsqlite3-dev

#macOS Homebrew
# bash brew install sqlite

# 安装 64 位版本的 MySQL 客户端库
# pacman -S mingw-w64-x86_64-mysql
ifdef db
ifeq ($(PLATFORM), osx)
	LIBRARY_PATH	+= /usr/local/mysql/lib
	MACROS	+= _CC_ENABLE_UNIXODBC_=1
	INCLUDE_PATH	+= /usr/local/Cellar/unixodbc/2.3.9_1/include
	LIBRARY_PATH	+= /usr/local/Cellar/unixodbc/2.3.9_1/lib

	LOCAL_SRC_FILES += $(SRCROOT)/src/db/sqlsvr.o

	LIBS += odbc

	LIBRARY_PATH	+= /usr/lib64/mysql
	INCLUDE_PATH	+= /usr/include
endif

LOCAL_SRC_FILES += \
					$(WIDGET_FILES)/db/sqlite.o \
					$(WIDGET_FILES)/db/mysql.o
endif # --end db--

ifdef event
LOCAL_SRC_FILES += \
					$(WIDGET_FILES)/event/event.o \
					$(WIDGET_FILES)/event/loop.o \
					$(WIDGET_FILES)/event/select.o \
					$(WIDGET_FILES)/event/timeout.o \
					$(WIDGET_FILES)/event/tcp.o \
					$(WIDGET_FILES)/event/buffer.o

ifneq ($(filter $(PLATFORM), ios osx freebsd unix),)
LOCAL_SRC_FILES += $(WIDGET_FILES)/event/unix/sys_kqueue.o
endif

ifeq ($(PLATFORM), linux)
LOCAL_SRC_FILES += $(WIDGET_FILES)/event/linux/sys_epoll.o
endif

ifeq ($(PLATFORM), windows)
LOCAL_SRC_FILES += \
					$(WIDGET_FILES)/event/windows/sys_WSA.o \
					$(WIDGET_FILES)/event/windows/sys_iocp.o \
	 				$(WIDGET_FILES)/event/windows/sys_iocp_overlapped.o
endif

endif # --end event--

ifdef json
LOCAL_SRC_FILES += \
					$(WIDGET_FILES)/json/json.o \
					$(WIDGET_FILES)/json/json.array.o \
					$(WIDGET_FILES)/json/json.object.o \
					$(WIDGET_FILES)/json/json.parser.o

# LOCAL_SRC_FILES += \
# 					$(WIDGET_FILES)/ini/ini.o \
# 					$(WIDGET_FILES)/ini/ini.parser.o

LOCAL_SRC_FILES += \
					$(WIDGET_FILES)/xml/xml.o \
					$(WIDGET_FILES)/xml/xml.parser.o
endif # --json db--

ifdef url_request
LOCAL_SRC_FILES += \
					$(WIDGET_FILES)/generic/gzip.o \
					$(WIDGET_FILES)/generic/map.o \
					$(WIDGET_FILES)/generic/WS.o \
					$(WIDGET_FILES)/generic/OpenSSL.o \
					$(WIDGET_FILES)/url_request/http/url_request.o \
					$(WIDGET_FILES)/url_request/http/url_response.o
endif # --end url_request --

#LOCAL_SRC_FILES += \
					$(WIDGET_FILES)/ip_locator/ip_locator.o

#LOCAL_SRC_FILES += \
					$(WIDGET_FILES)/smtp/libsmtp.o \
					$(WIDGET_FILES)/smtp/connected.o \
					$(WIDGET_FILES)/smtp/login.o \
					$(WIDGET_FILES)/smtp/from_rcpt.o

#LOCAL_SRC_FILES += \
					$(WIDGET_FILES)/ftp/libftp.o \
					$(WIDGET_FILES)/ftp/connected.o \
					$(WIDGET_FILES)/ftp/login.o \
					$(WIDGET_FILES)/ftp/cdup.o \
					$(WIDGET_FILES)/ftp/cwd.o \
					$(WIDGET_FILES)/ftp/delete_file.o \
					$(WIDGET_FILES)/ftp/delete_folder.o \
					$(WIDGET_FILES)/ftp/list.o \
					$(WIDGET_FILES)/ftp/mkdir.o \
					$(WIDGET_FILES)/ftp/opts.o \
					$(WIDGET_FILES)/ftp/rename_file.o

#LOCAL_SRC_FILES += \
					$(WIDGET_FILES)/dns/dns.o \
					$(WIDGET_FILES)/dns/dnss.o

LOCAL_SRC_FILES += \
					$(WIDGET_FILES)/widgets.o