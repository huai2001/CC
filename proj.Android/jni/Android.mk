LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -D_CC_JNI_BUILD_SHARED_LIBRARY_

ifeq ($(NDK_UNICODE), 1)
    LOCAL_CFLAGS +=  -DUNICODE
    ifeq ($(NDK_DEBUG),1)
        LOCAL_CFLAGS +=  -DDEBUG
        MODULE_NAME_EXT := UD
    else
        MODULE_NAME_EXT := U
    endif
else
    ifeq ($(NDK_DEBUG),1)
        LOCAL_CFLAGS +=  -DDEBUG
        MODULE_NAME_EXT := D
    endif
endif

#LOCAL_CFLAGS +=  -D_CC_JNI_BUILD_SHARED_LIBRARY_

LOCAL_MODULE := libcc$(MODULE_NAME_EXT)
LOCAL_C_INCLUDES := ../../include ./
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_SRC_FILES := \
        ../../src/aes.c \
        ../../src/base64.c \
        ../../src/md2.c \
        ../../src/md4.c \
        ../../src/md5.c \
        ../../src/sha1.c \
        ../../src/sha256.c \
        ../../src/sha512.c \
        ../../src/sha3.c \
        ../../src/des.c \
        ../../src/crc.c \
        ../../src/list.c \
        ../../src/queue.c \
        ../../src/rbtree.c \
        ../../src/hashtable.c \
        ../../src/array.c \
        ../../src/string.c \
        ../../src/UTF.c \
        ../../src/buf.c \
        ../../src/url.c \
        ../../src/rand.c \
        ../../src/logger.c \
        ../../src/alloc.c \
        ../../src/power/power.c \
        ../../src/power/android/sys_power.c \
        ../../src/socket/inet.c \
        ../../src/socket/socket.c \
        ../../src/thread/thread.c \
        ../../src/time/time.c \
        ../../src/time/strptime.c \
        ../../src/time/linux/sys_time.c \
        ../../src/atomic/atomic.c \
        ../../src/atomic/spinlock.c \
        ../../src/atomic/rwlock.c \
        ../../src/loadso/dlopen/sys_loadso.c \
        ../../src/event/event.c \
        ../../src/event/timeout.c \
        ../../src/event/buffer.c \
        ../../src/event/loop.c \
        ../../src/event/tcp.c \
        ../../src/event/select.c \
        ../../src/event/linux/sys_epoll.c \
        ../../src/core/cpu_info.c \
        ../../src/core/generic.c \
        ../../src/core/file.c \
        ../../src/core/android.c \
        ../../src/core/android/sys_core.c \
        ../../src/core/android/sys_file.c \
        ../../src/core/android/sys_locale.c \
        ../../src/socket/linux/sys_socket.c \
        ../../src/thread/pthread/sys_thread.c \
        ../../src/thread/pthread/sys_cond.c \
        ../../src/thread/pthread/sys_mutex.c \
        ../../src/thread/pthread/sys_sem.c \
        ../../src/ini/ini.c \
        ../../src/ini/ini.parser.c \
        ../../src/json/json.c \
        ../../src/json/json.parser.c \
        ../../src/xml/xml.c \
        ../../src/xml/xml.parser.c \
        ./main.c

LOCAL_STATIC_LIBRARIES := cpufeatures

LOCAL_LDLIBS := -ldl -llog -landroid
LOCAL_EXPORT_LDLIBS := $(LOCAL_LDLIBS)
include $(BUILD_SHARED_LIBRARY)

#include $(BUILD_STATIC_LIBRARY)