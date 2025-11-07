# 定义源文件目录
SRC_TESTS_DIR := $(SRCROOT)/tests

LOCAL_SRC_FILES = $(SRC_TESTS_DIR)/test_$(build).o

TARGET_NAME = $(build)

MACROS	+= _CC_USE_OPENSSL_=1

LIBS	+= cc

ifeq ($(PLATFORM), osx)
	INCLUDE_PATH	+= /opt/homebrew/include
	LIBRARY_PATH	+= /opt/homebrew/lib
endif

ifdef dll
	LIBS += $(dll)
endif