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

LOCAL_STATIC_LIBRARIES += cpufeatures
APP_ALLOW_MISSING_DEPS := true

#LOCAL_CFLAGS +=  -D_CC_JNI_BUILD_SHARED_LIBRARY_

LOCAL_MODULE := libcc$(MODULE_NAME_EXT)
LOCAL_C_INCLUDES := ../../include ./
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_SRC_FILES := \
        ../../src/aes.c \
        ../../src/base16.c \
        ../../src/base58.c \
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
        ../../src/hmap.c \
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
        ../../src/atomic/rwlock.c \
        ../../src/loadso/dlopen/sys_loadso.c \
        ../../src/core/cpu_info.c \
        ../../src/core/generic.c \
        ../../src/core/file.c \
        ../../src/core/android/sys_android.c \
        ../../src/core/android/sys_dirent.c \
        ../../src/core/android/sys_file.c \
        ../../src/core/android/sys_clipboard.c \
        ../../src/core/android/sys_locale.c \
        ../../src/socket/linux/sys_socket.c \
        ../../src/thread/pthread/sys_thread.c \
        ../../src/thread/pthread/sys_cond.c \
        ../../src/thread/pthread/sys_mutex.c \
        ../../src/thread/pthread/sys_sem.c \
        ../../src/widgets/event/event.c \
        ../../src/widgets/event/timeout.c \
        ../../src/widgets/event/buffer.c \
        ../../src/widgets/event/loop.c \
        ../../src/widgets/event/tcp.c \
        ../../src/widgets/event/select.c \
        ../../src/widgets/event/linux/sys_epoll.c \
        ../../src/widgets/generic/generic.c \
        ../../src/widgets/generic/WS.c \
        ../../src/widgets/ini/ini.c \
        ../../src/widgets/ini/ini.parser.c \
        ../../src/widgets/json/json.c \
        ../../src/widgets/json/json.object.c \
        ../../src/widgets/json/json.array.c \
        ../../src/widgets/json/json.parser.c \
        ../../src/widgets/xml/xml.c \
        ../../src/widgets/xml/xml.parser.c \
        ./main.c

LOCAL_LDLIBS := -ldl -llog -landroid
LOCAL_EXPORT_LDLIBS := $(LOCAL_LDLIBS)
include $(BUILD_SHARED_LIBRARY)

#include $(BUILD_STATIC_LIBRARY)