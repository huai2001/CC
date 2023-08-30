LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

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


LOCAL_MODULE := libpacket$(MODULE_NAME_EXT)
LOCAL_C_INCLUDES := /Users/QIU/Github/CC/include /Users/QIU/Github/3rd
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_SRC_FILES := \
                ../quicklz.c \
                ../packet.c \
                ../mapped.c
        

LOCAL_STATIC_LIBRARIES := cpufeatures

include $(BUILD_STATIC_LIBRARY)
