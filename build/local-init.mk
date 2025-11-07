# SHELL配置（兼容Unix/Linux/Windows）
# SHELL := /bin/sh

#编译器路径(不一定要指定，如果用20060119版本的，需要指定)
#GCC_PATH	=

CC		= gcc
CPP		= g++
MAKE	= make
AR      = ar
RANLIB  = ranlib

#set 1 Debug or 0 Release
#debug = 1
VERSION 	:= 1.0.0
MIN_VERSION := 10.15

# CUP
# 改进后的架构检测逻辑（支持更多ARM变体）
ifeq ($(arch),)
	ARCH := $(shell uname -m 2>/dev/null || echo unknown)
	ARCH := $(patsubst armv7l,arm,$(ARCH))
	ARCH := $(patsubst armv6l,arm,$(ARCH))
	ifeq ($(ARCH), x86_64)
		CFLAGS += -m64 -msse4.2
		ARCH_x64 = 1
	else ifeq ($(ARCH), i686)
		CFLAGS += -m32 -msse2
	else ifeq ($(ARCH), arm64)
		#CFLAGS += -march=armv8-a+crc+crypto
		CFLAGS += -m64
		ARCH_x64 = 1
	else ifeq ($(ARCH), arm)
		#CFLAGS += -march=armv7-a+neon-vfpv4
		CFLAGS += -m32
	else
		#$(warning Unknown architecture $(ARCH), using generic flags)
		CFLAGS  += -m64
		ARCH_x64 = 1
	endif
else ifeq ($(arch), x64)
	CFLAGS  += -m64
	ARCH_x64 = 1
else ifeq ($(arch), x32)
	CFLAGS  += -m32
endif

##################################################
##				 操作系统参数设置				   ##
##################################################
ifeq ($(platform),)
	# Platform detections and overrides
	PLATFORM ?= $(shell uname 2>/dev/null | tr A-Z a-z)
	PLATFORM := $(patsubst msys%,windows,$(PLATFORM))
	PLATFORM := $(patsubst mingw%,windows,$(PLATFORM))
	PLATFORM := $(patsubst darwin,osx,$(PLATFORM))
else
	PLATFORM := $(platform)
endif

ifneq ($(filter $(PLATFORM), osx darwin),)
	PLATFORM = osx
endif

ifeq ($(PLATFORM), ios)
	CC		= xcrun -sdk iphoneos clang
	CPP		= xcrun -sdk iphoneos clang
	ifdef ARCH_x64
		CFLAGS  += -arch arm64 # x86_64
	else
		CFLAGS  += -arch armv7 # i386
	endif
	CFLAGS  += -mios-version-min=$(MIN_VERSION) -march=armv7-a -fmessage-length=0
	LDFLAGS += -mios-version-min=$(MIN_VERSION) -march=armv7-a -Wl, -Bsymbolic-functions -read_only_relocs suppress
else ifeq ($(PLATFORM), osx)
	CC		= clang
	CFLAGS  += -mmacosx-version-min=$(MIN_VERSION)
	LDFLAGS += -Wl,-rpath,./ -mmacosx-version-min=$(MIN_VERSION) -Bsymbolic-functions -framework Foundation -framework CoreLocation -framework Cocoa
	INSTALL_NAME = -install_name @loader_path/lib$(TARGET_NAME)$@
else ifeq ($(PLATFORM), linux)
	LDFLAGS += -Wl,--rpath=./
else ifeq ($(PLATFORM), freebsd)
	MAKE 	= gmake
	CC		= clang
	LDFLAGS += -Wl,--rpath=./
else ifeq ($(PLATFORM), windows)

endif

ifeq ($(CC), gcc)
	CFLAGS  += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -D_GNU_SOURCE
endif

# 如果 shared 未定义，则检查是否传递了 .so、.dylib .dll 或 .bin 参数
ifeq ($(shared),)
	ifneq ($(filter .so .dylib .dll .bin,$(MAKECMDGOALS)),)
		shared := 1
	endif
endif

ifdef shared
	ifeq ($(PLATFORM), windows)
		LIBS    += ws2_32 winmm dbghelp m z ucrt ole32
	else
		LIBS    += m dl pthread z
	endif

	ifeq ($(PLATFORM), freebsd)
		LIBS	+= execinfo
	endif
endif

##################################################
##				编译器参数设置					  ##
##################################################
#定义输出目录名
#优化等级
ifdef debug
	CONFIGURATION = debug
	#-O0禁止编译器进行优化
	CFLAGS  += -g -O0 -DDEBUG
else
	CONFIGURATION = release
	CFLAGS  += -O3 -DNDEBUG
endif

BUILD_PATH	  = $(SRCROOT)/build/intermediates/$(ARCH)/$(CONFIGURATION)
EXT_LIB_PATH  = $(SRCROOT)/lib/$(ARCH)/$(CONFIGURATION)
EXT_BIN_PATH  = $(SRCROOT)/bin/$(ARCH)/$(CONFIGURATION)

##编译过程中间文件输出目录
EXT_OBJ_PATH  = $(BUILD_PATH)/objs/$(TARGET_NAME)
EXT_LST_PATH  = $(BUILD_PATH)/lsts/$(TARGET_NAME)

ifeq ($(PLATFORM), osx)
	INCLUDE_PATH	+= /opt/homebrew/include
	LIBRARY_PATH	+= /opt/homebrew/lib
else ifneq ($(PLATFORM), windows)
	INCLUDE_PATH	+= /usr/local/include /usr/include
	LIBRARY_PATH	+= /usr/local/lib /usr/lib
endif

#要包含的路径(本用例包含include,lib,bin 三个目录)
INCLUDE_PATH += $(SRCROOT)/include $(THIRD_PARTY_PATH)
LIBRARY_PATH += $(EXT_LIB_PATH) $(EXT_BIN_PATH) $(SRCROOT)/bin/$(ARCH) $(SRCROOT)/lib/$(ARCH)

#在编译过程的不同阶段之间使用管道而非临时文件进行通信，可以加快编译速度。建议使用。
CFLAGS += -pipe
#为防止程序栈溢出而进行必要的检测，仅在多线程环境中运行时才可能需要它。
#CFLAGS += -fstack-check
#-fPIC 作用于编译阶段，告诉编译器产生与位置无关代码(Position-Independent Code)，
#则产生的代码中，没有绝对地址，全部使用相对地址，故而代码可以被加载器加载到内存的任意位置，
#都可以正确的执行。这正是共享库所要求的，共享库被加载时，在内存的位置不是固定的。
CFLAGS += -fPIC
#gnu工具链编译过程中,输出信息会根据控制台的宽度自动换行
CFLAGS += -fmessage-length=0

#该选项能发现程序中一系列的常见错误警告
CFLAGS += -Wall

ifdef unicode
MACROS	+= _UNICODE UNICODE
endif

CXXFLAGS 	:= $(CFLAGS)

#指明使用标准 ISO C99 再加上 GNU 的一些扩展作为标准来编译程序。c89, c99, gnu99 gnu11
CFLAGS 		+= -std=gnu99
CXXFLAGS 	+= -std=c++11

# 依赖生成选项
DEPFLAGS 	= -MMD -MP
CFLAGS 		+= $(DEPFLAGS)
CXXFLAGS 	+= $(DEPFLAGS)

OBJCOPY		= objcopy
OBJDUMP		= objdump
SIZE		= size
AR			= ar crv ##ar rcu

MKDIR		= mkdir
RMDIR		= rm -rf 
RM			= rm -f

C_SUF		:= .c
M_SUF		:= .m
CPP_SUF		:= .cpp
ASM_SUF		:= .s
OBJ_SUF		:= .o
LIB_SUF		:= .a
BIN_SUF		:= .bin
SO_SUF		:= .so
DLL_SUF		:= .dll
DYLIB_SUF	:= .dylib