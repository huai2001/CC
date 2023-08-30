##################################################
SRCROOT	:= $(abspath .)
#生成的文件名
ifdef debug
	TARGET_NAME = ccd.$(PLATFORM)
else
	TARGET_NAME = cc.$(PLATFORM)
endif

ifdef shared
	LIBS	+= sqlite3
	MACROS += _CC_ENABLE_SHARED_LIBRARY_=1
endif
include $(SRCROOT)/build/Makefile.mak

LOCAL_SRC_FILES = src/aes.o \
	src/base16.o \
	src/base64.o \
	src/crc.o \
	src/md2.o \
	src/md4.o \
	src/md5.o \
	src/sha1.o \
	src/sha256.o \
	src/sha512.o \
	src/sha3.o \
	src/hmac.o \
	src/des.o \
	src/xxtea.o \
	src/UTF.o \
	src/GBK.o \
	src/list.o \
	src/queue.o \
	src/hashtable.o \
	src/rbtree.o \
	src/array.o \
	src/string.o \
	src/buf.o \
	src/uuid.o \
	src/url.o \
	src/rand.o \
	src/ring.o \
	src/logger.o \
	src/alloc.o \
	src/socket/inet.o \
	src/socket/socket.o \
	src/thread/thread.o \
	src/time/strptime.o \
	src/time/time.o \
	src/atomic/atomic.o \
	src/atomic/spinlock.o \
	src/atomic/rwlock.o \
	src/core/cpu_info.o \
	src/core/generic.o \
	src/core/file.o \
	src/power/power.o

ifneq ($(filter $(PLATFORM), freebsd unix),)
	LOCAL_SRC_FILES += src/time/unix/sys_time.o \
		src/core/file.o \
		src/core/unix.o \
		src/core/unix/sys_core.o \
		src/core/unix/sys_locale.o \
		src/socket/unix/sys_socket.o \
		src/thread/pthread/sys_thread.o \
		src/thread/pthread/sys_cond.o \
		src/thread/pthread/sys_mutex.o \
		src/thread/pthread/sys_sem.o \
		src/loadso/dlopen/sys_loadso.o
endif

ifneq ($(filter $(PLATFORM), mac64 mac32),)
		LOCAL_SRC_FILES += src/time/unix/sys_time.o \
		src/core/OSX/sys_file.o \
		src/core/OSX/sys_core.o \
		src/core/OSX/sys_locale.o \
		src/core/unix.o \
		src/power/macosx/sys_power.o \
		src/socket/unix/sys_socket.o \
		src/thread/pthread/sys_thread.o \
		src/thread/pthread/sys_cond.o \
		src/thread/apple/sys_mutex.o \
		src/thread/apple/sys_sem.o  \
		src/loadso/dlopen/sys_loadso.o
endif

ifneq ($(filter $(PLATFORM), ios64 ios32 ios_x86_64 ios_i386),)
		LOCAL_SRC_FILES += src/time/unix/sys_time.o \
		src/core/IOS/sys_file.o \
		src/core/IOS/sys_core.o \
		src/core/unix.o \
		src/socket/unix/sys_socket.o \
		src/thread/pthread/sys_thread.o \
		src/thread/pthread/sys_mutex.o \
		src/thread/pthread/sys_cond.o \
		src/thread/apple/sys_sem.o 
endif


ifeq ($(PLATFORM), linux)
		LOCAL_SRC_FILES += src/time/linux/sys_time.o \
		src/core/file.o \
		src/core/unix.o \
		src/core/unix/sys_core.o \
		src/core/unix/sys_locale.o \
		src/power/linux/sys_power.o \
		src/socket/linux/sys_socket.o \
		src/thread/pthread/sys_thread.o \
		src/thread/pthread/sys_cond.o \
		src/thread/pthread/sys_mutex.o \
		src/thread/pthread/sys_sem.o \
		src/loadso/dlopen/sys_loadso.o
endif
