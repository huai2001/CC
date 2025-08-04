##################################################
SRCROOT	:= $(abspath .)

THIRD_PARTY_PATH = $(abspath $(SRCROOT)/..)/third-party
INCLUDE_PATH += $(THIRD_PARTY_PATH)


#生成的文件名
TARGET_NAME = cc

ifdef shared
	MACROS += _CC_ENABLE_SHARED_LIBRARY_=1
endif

#MACROS += _CC_SYSLOG_RFC5424_

include $(SRCROOT)/build/Makefile.mak

LOCAL_SRC_FILES = \
	src/crypto/aes.o \
	src/crypto/base16.o \
	src/crypto/base58.o \
	src/crypto/base64.o \
	src/crypto/md2.o \
	src/crypto/md4.o \
	src/crypto/md5.o \
	src/crypto/sha1.o \
	src/crypto/sha256.o \
	src/crypto/sha512.o \
	src/crypto/sha3.o \
	src/crypto/hmac.o \
	src/crypto/des.o \
	src/crypto/xxtea.o \
	src/crc.o \
	src/UTF.o \
	src/GBK.o \
	src/list.o \
	src/queue.o \
	src/hmap.o \
	src/rbtree.o \
	src/array.o \
	src/string.o \
	src/buf.o \
	src/uuid.o \
	src/url.o \
	src/rand.o \
	src/ring.o \
	src/logger.o \
	src/syslog.o \
	src/alloc.o \
	src/socket/inet.o \
	src/socket/socket.o \
	src/thread/thread.o \
	src/time/strptime.o \
	src/time/time.o \
	src/atomic/atomic.o \
	src/atomic/rwlock.o \
	src/core/cpu_info.o \
	src/core/generic.o \
	src/core/file.o \
	src/power/power.o

ifneq ($(filter $(PLATFORM), freebsd unix),)
	LOCAL_SRC_FILES += src/time/unix/sys_time.o \
		src/core/file.o \
		src/core/unix/sys_unix.o \
		src/core/unix/sys_dirent.o \
		src/core/unix/sys_locale.o \
		src/socket/unix/sys_socket.o \
		src/thread/pthread/sys_thread.o \
		src/thread/pthread/sys_cond.o \
		src/thread/pthread/sys_mutex.o \
		src/thread/pthread/sys_sem.o \
		src/loadso/dlopen/sys_loadso.o
endif

ifneq ($(filter $(PLATFORM), osx),)
		LOCAL_SRC_FILES += src/time/unix/sys_time.o \
		src/core/OSX/sys_file.o \
		src/core/OSX/sys_dirent.o \
		src/core/OSX/sys_locale.o \
		src/core/OSX/sys_ios.o \
		src/core/unix/sys_unix.o \
		src/power/macosx/sys_power.o \
		src/socket/unix/sys_socket.o \
		src/thread/pthread/sys_thread.o \
		src/thread/pthread/sys_cond.o \
		src/thread/apple/sys_mutex.o \
		src/thread/apple/sys_sem.o  \
		src/loadso/dlopen/sys_loadso.o
endif

ifneq ($(filter $(PLATFORM), ios),)
		LOCAL_SRC_FILES += src/time/unix/sys_time.o \
		src/core/IOS/sys_file.o \
		src/core/IOS/sys_dirent.o \
		src/core/IOS/sys_ios.o \
		src/core/unix/sys_unix.o \
		src/socket/unix/sys_socket.o \
		src/thread/pthread/sys_thread.o \
		src/thread/pthread/sys_mutex.o \
		src/thread/pthread/sys_cond.o \
		src/thread/apple/sys_sem.o 
endif


ifeq ($(PLATFORM), linux)
		LOCAL_SRC_FILES += src/time/linux/sys_time.o \
		src/core/file.o \
		src/core/unix/sys_unix.o \
		src/core/unix/sys_dirent.o \
		src/core/unix/sys_locale.o \
		src/power/linux/sys_power.o \
		src/socket/linux/sys_socket.o \
		src/thread/pthread/sys_thread.o \
		src/thread/pthread/sys_cond.o \
		src/thread/pthread/sys_mutex.o \
		src/thread/pthread/sys_sem.o \
		src/loadso/dlopen/sys_loadso.o
endif

ifeq ($(PLATFORM), windows)
		LOCAL_SRC_FILES += src/time/windows/sys_time.o \
		src/core/file.o \
		src/core/windows/sys_windows.o \
		src/core/windows/sys_mmap.o \
		src/core/windows/sys_dirent.o \
		src/core/windows/sys_file.o \
		src/core/windows/sys_pipe.o \
		src/core/windows/sys_locale.o \
		src/power/windows/sys_power.o \
		src/socket/windows/sys_socket.o \
		src/thread/windows/sys_thread.o \
		src/thread/windows/sys_cond.o \
		src/thread/windows/sys_mutex.o \
		src/thread/windows/sys_sem.o \
		src/loadso/windows/sys_loadso.o
endif