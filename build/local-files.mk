ifdef shared
	MACROS += _CC_API_USE_DYNAMIC_
endif

INCLUDE_PATH	+= $(SRCROOT)/include
LIBRARY_PATH	+= $(SRCROOT)/lib

ifdef USE_LIB_OPENSSL
	ifeq ($(PLATFORM), osx)
		LIBS += ssl.3 crypto.3
	else
		LIBS += ssl crypto
	endif
	MACROS	+= _CC_USE_OPENSSL_=1
endif # --end USE_LIB_OPENSSL--

ifdef USE_LIB_SQLSERVER
	ifeq ($(PLATFORM), osx)
		MACROS			+= _CC_USE_UNIXODBC_=1
		LIBS			+= odbc
	endif
endif # --end USE_LIB_SQLSERVER--

ifdef USE_LIB_MYSQL
	ifeq ($(PLATFORM), osx)
		LIBS			+= mysqlclient
		INCLUDE_PATH	+= /opt/homebrew/opt/mysql-client/include
		LIBRARY_PATH	+= /opt/homebrew/opt/mysql-client/lib
	else ifeq ($(PLATFORM), windows)
		LIBS += mysql
	else
		LIBS += mysqlclient
		LIBRARY_PATH	+= /usr/local/lib /usr/local/lib/mysql
	endif
endif # --end USE_LIB_MYSQL--

ifdef USE_LIB_SQLITE3
	LIBS += sqlite3 
endif # --end USE_LIB_SQLITE3--

#MACROS += _CC_SYSLOG_RFC5424_

LOCAL_SRC_FILES += \
	$(SRCROOT)/src/crypto/aes.c \
	$(SRCROOT)/src/crypto/base16.c \
	$(SRCROOT)/src/crypto/base32.c \
	$(SRCROOT)/src/crypto/base58.c \
	$(SRCROOT)/src/crypto/base64.c \
	$(SRCROOT)/src/crypto/md2.c \
	$(SRCROOT)/src/crypto/md4.c \
	$(SRCROOT)/src/crypto/md5.c \
	$(SRCROOT)/src/crypto/sha1.c \
	$(SRCROOT)/src/crypto/sha256.c \
	$(SRCROOT)/src/crypto/sha512.c \
	$(SRCROOT)/src/crypto/hmac.c \
	$(SRCROOT)/src/crypto/des.c \
	$(SRCROOT)/src/crypto/xxtea.c \
	$(SRCROOT)/src/crypto/OpenSSL.c \
	$(SRCROOT)/src/crc.c \
	$(SRCROOT)/src/UTF.c \
	$(SRCROOT)/src/GBK.c \
	$(SRCROOT)/src/list.c \
	$(SRCROOT)/src/queue.c \
	$(SRCROOT)/src/hmap.c \
	$(SRCROOT)/src/rbtree.c \
	$(SRCROOT)/src/array.c \
	$(SRCROOT)/src/string.c \
	$(SRCROOT)/src/sds.c \
	$(SRCROOT)/src/buf.c \
	$(SRCROOT)/src/rwbuf.c \
	$(SRCROOT)/src/uuid.c \
	$(SRCROOT)/src/url.c \
	$(SRCROOT)/src/rand.c \
	$(SRCROOT)/src/ring.c \
	$(SRCROOT)/src/logger.c \
	$(SRCROOT)/src/syslog.c \
	$(SRCROOT)/src/cpu.c \
	$(SRCROOT)/src/file.c \
	$(SRCROOT)/src/snowflake.c \
	$(SRCROOT)/src/google.auth.c \
	$(SRCROOT)/src/malloc/alloc.c \
	$(SRCROOT)/src/thread/thread.c \
	$(SRCROOT)/src/atomic/atomic.c \
	$(SRCROOT)/src/atomic/rwlock.c \
	$(SRCROOT)/src/os/os.c \
	$(SRCROOT)/src/event/event.c \
	$(SRCROOT)/src/event/loop.c \
	$(SRCROOT)/src/event/select.c \
	$(SRCROOT)/src/event/timeout.c \
	$(SRCROOT)/src/event/buffer.c \
	$(SRCROOT)/src/event/tcp.c \
	$(SRCROOT)/src/json/json.c \
	$(SRCROOT)/src/json/json.array.c \
	$(SRCROOT)/src/json/json.object.c \
	$(SRCROOT)/src/json/json.parser.c \
	$(SRCROOT)/src/ini/ini.c \
	$(SRCROOT)/src/ini/ini.parser.c\
	$(SRCROOT)/src/xml/xml.c \
	$(SRCROOT)/src/xml/xml.parser.c \
	$(SRCROOT)/src/misc/tick.c \
	$(SRCROOT)/src/misc/time.c \
	$(SRCROOT)/src/misc/inet.c \
	$(SRCROOT)/src/misc/socket.c \
	$(SRCROOT)/src/misc/power.c  \
	$(SRCROOT)/src/misc/misc.c \
	$(SRCROOT)/src/misc/WS.c \
	$(SRCROOT)/src/main.c

ifneq ($(filter $(PLATFORM), freebsd unix),)
	LOCAL_SRC_FILES += \
		$(SRCROOT)/src/os/unix/sys_time.c \
		$(SRCROOT)/src/os/unix/sys_unix.c \
		$(SRCROOT)/src/os/unix/sys_dirent.c \
		$(SRCROOT)/src/os/unix/sys_locale.c \
		$(SRCROOT)/src/os/unix/sys_socket.c \
		$(SRCROOT)/src/os/unix/sys_loadso.c \
		$(SRCROOT)/src/os/unix/sys_clipboard.c \
		$(SRCROOT)/src/os/unix/sys_kqueue.c \
		$(SRCROOT)/src/os/unix/sys_poll.c \
		$(SRCROOT)/src/thread/pthread/sys_thread.c \
		$(SRCROOT)/src/thread/pthread/sys_cond.c \
		$(SRCROOT)/src/thread/pthread/sys_mutex.c \
		$(SRCROOT)/src/thread/pthread/sys_sem.c
endif

ifneq ($(filter $(PLATFORM), osx),)
		LOCAL_SRC_FILES += \
		$(SRCROOT)/src/os/unix/sys_time.c \
		$(SRCROOT)/src/os/OSX/sys_file.m \
		$(SRCROOT)/src/os/OSX/sys_dirent.m \
		$(SRCROOT)/src/os/OSX/sys_locale.m \
		$(SRCROOT)/src/os/OSX/sys_osx.m \
		$(SRCROOT)/src/os/OSX/sys_clipboard.m \
		$(SRCROOT)/src/os/OSX/sys_power.m \
		$(SRCROOT)/src/os/unix/sys_unix.c \
		$(SRCROOT)/src/os/unix/sys_socket.c \
		$(SRCROOT)/src/os/unix/sys_loadso.c \
		$(SRCROOT)/src/os/unix/sys_kqueue.c \
		$(SRCROOT)/src/thread/pthread/sys_thread.c \
		$(SRCROOT)/src/thread/pthread/sys_cond.c \
		$(SRCROOT)/src/thread/apple/sys_mutex.c \
		$(SRCROOT)/src/thread/apple/sys_sem.c
endif

ifneq ($(filter $(PLATFORM), ios),)
	LOCAL_SRC_FILES += \
		$(SRCROOT)/src/os/unix/sys_time.c \
		$(SRCROOT)/src/os/IOS/sys_file.m \
		$(SRCROOT)/src/os/IOS/sys_dirent.m \
		$(SRCROOT)/src/os/IOS/sys_ios.m \
		$(SRCROOT)/src/os/IOS/sys_power.m \
		$(SRCROOT)/src/os/IOS/sys_clipboard.m \
		$(SRCROOT)/src/os/unix/sys_unix.c \
		$(SRCROOT)/src/os/unix/sys_socket.c \
		$(SRCROOT)/src/os/unix/sys_kqueue.c \
		$(SRCROOT)/src/os/unix/sys_poll.c \
		$(SRCROOT)/src/thread/pthread/sys_thread.c \
		$(SRCROOT)/src/thread/pthread/sys_mutex.c \
		$(SRCROOT)/src/thread/pthread/sys_cond.c \
		$(SRCROOT)/src/thread/apple/sys_sem.c
endif

ifeq ($(PLATFORM), linux)
	LOCAL_SRC_FILES += \
		$(SRCROOT)/src/os/unix/sys_unix.c \
		$(SRCROOT)/src/os/unix/sys_dirent.c \
		$(SRCROOT)/src/os/unix/sys_loadso.c \
		$(SRCROOT)/src/os/unix/sys_locale.c \
		$(SRCROOT)/src/os/linux/sys_linux.c \
		$(SRCROOT)/src/os/linux/sys_time.c \
		$(SRCROOT)/src/os/linux/sys_power.c \
		$(SRCROOT)/src/os/linux/sys_socket.c \
		$(SRCROOT)/src/os/linux/sys_epoll.c \
		$(SRCROOT)/src/thread/pthread/sys_thread.c \
		$(SRCROOT)/src/thread/pthread/sys_cond.c \
		$(SRCROOT)/src/thread/pthread/sys_mutex.c \
		$(SRCROOT)/src/thread/pthread/sys_sem.c
		#$(SRCROOT)/src/os/linux/sys_io_uring.c
endif

ifeq ($(PLATFORM), windows)
	LIBS += bcrypt
	LOCAL_SRC_FILES += \
		$(SRCROOT)/src/os/windows/sys_time.c \
		$(SRCROOT)/src/os/windows/sys_strptime.c \
		$(SRCROOT)/src/os/windows/sys_windows.c \
		$(SRCROOT)/src/os/windows/sys_mmap.c \
		$(SRCROOT)/src/os/windows/sys_dirent.c \
		$(SRCROOT)/src/os/windows/sys_file.c \
		$(SRCROOT)/src/os/windows/sys_pipe.c \
		$(SRCROOT)/src/os/windows/sys_locale.c \
		$(SRCROOT)/src/os/windows/sys_power.c \
		$(SRCROOT)/src/os/windows/sys_socket.c \
		$(SRCROOT)/src/os/windows/sys_loadso.c \
		$(SRCROOT)/src/os/windows/sys_WSA.c \
		$(SRCROOT)/src/os/windows/sys_iocp.c \
		$(SRCROOT)/src/os/windows/sys_io_context.c \
		$(SRCROOT)/src/thread/windows/sys_thread.c \
		$(SRCROOT)/src/thread/windows/sys_cond.c \
		$(SRCROOT)/src/thread/windows/sys_mutex.c \
		$(SRCROOT)/src/thread/windows/sys_sem.c
endif

ifdef USE_LIB_SQLSERVER
	LOCAL_SRC_FILES += \
		$(SRCROOT)/src/db/sqlsvr.c
endif # --end USE_LIB_SQLSERVER--

ifdef USE_LIB_SQLITE3
	LOCAL_SRC_FILES += \
		$(SRCROOT)/src/db/sqlite.c
endif # --end USE_LIB_SQLITE3--

ifdef USE_LIB_MYSQL
	LOCAL_SRC_FILES += \
		$(SRCROOT)/src/db/mysql.c
endif # --end USE_LIB_MYSQL--

ifdef USE_LIB_URL_REQUEST
	LOCAL_SRC_FILES += \
		$(SRCROOT)/src/misc/gzip.c \
		$(SRCROOT)/src/http/http-v2.x/http2.c \
		$(SRCROOT)/src/http/http-v1.x/header.c \
		$(SRCROOT)/src/http/http-v1.x/request.parser.c \
		$(SRCROOT)/src/http/http-v1.x/response.parser.c \
		$(SRCROOT)/src/http/http-v1.x/request.c \
		$(SRCROOT)/src/http/http-v1.x/request.response.c
endif # --end USE_LIB_URL_REQUEST --

ifdef USE_LIB_SMTP
	LOCAL_SRC_FILES += \
		$(SRCROOT)/src/smtp/libsmtp.c \
		$(SRCROOT)/src/smtp/connected.c \
		$(SRCROOT)/src/smtp/login.c \
		$(SRCROOT)/src/smtp/from_rcpt.c
endif # --end USE_LIB_SMTP --

ifdef USE_LIB_FTP
#LOCAL_SRC_FILES += \
		$(SRCROOT)/src/ftp/libftp.c \
		$(SRCROOT)/src/ftp/connected.c \
		$(SRCROOT)/src/ftp/login.c \
		$(SRCROOT)/src/ftp/cdup.c \
		$(SRCROOT)/src/ftp/cwd.c \
		$(SRCROOT)/src/ftp/delete_file.c \
		$(SRCROOT)/src/ftp/delete_folder.c \
		$(SRCROOT)/src/ftp/list.c \
		$(SRCROOT)/src/ftp/mkdir.c \
		$(SRCROOT)/src/ftp/opts.c \
		$(SRCROOT)/src/ftp/rename_file.c
endif # --end USE_LIB_FTP --

ifdef USE_LIB_DNS
	#LOCAL_SRC_FILES += \
		$(SRCROOT)/src/dns/dns.c \
		$(SRCROOT)/src/dns/dnss.c
endif # --end USE_LIB_DNS --