# SHELL配置（兼容Unix/Linux/Windows）
# SHELL := /bin/sh
# ifneq ($(findstring windows,$(PLATFORM)),)
#     SHELL := cmd
# endif

#编译器路径(不一定要指定，如果用20060119版本的，需要指定)
#GCC_PATH	=

CC		?= gcc
CPP		?= g++
MAKE	?= make
AR      ?= ar
RANLIB  ?= ranlib

#set 1 Debug or 0 Release
#debug = 1
VERSION 	:= 1.0.0
MIN_VERSION := 10.15

INSTALL_DIR	?=	/usr/local

#获取当前操作名称
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

# CUP
# 改进后的架构检测逻辑（支持更多ARM变体）
ifeq ($(arch),)
	ARCH := $(shell uname -m 2>/dev/null || echo unknown)
	ARCH := $(patsubst arm64,aarch64,$(ARCH))
	ARCH := $(patsubst armv7l,arm,$(ARCH))
	ARCH := $(patsubst armv6l,arm,$(ARCH))
	ifeq ($(ARCH), x86_64)
		CFLAGS += -m64 -msse4.2
		ARCH_x64 = 1
	else ifeq ($(ARCH), i686)
		CFLAGS += -m32 -msse2
	else ifeq ($(ARCH), aarch64)
		CFLAGS += -march=armv8-a+crc+crypto
		ARCH_x64 = 1
	else ifeq ($(ARCH), arm)
		CFLAGS += -march=armv7-a+neon-vfpv4
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
##				操作系统参数设置				  ##
##################################################

ifeq ($(PLATFORM), ios)
	CC		?= xcrun -sdk iphoneos clang
	CPP		?= xcrun -sdk iphoneos clang
	ifdef ARCH_x64
		CFLAGS  += -arch arm64 # x86_64
	else
		CFLAGS  += -arch armv7 # i386
	endif
	CFLAGS  += -mios-version-min=$(MIN_VERSION) -march=armv7-a -fmessage-length=0
	LDFLAGS += -mios-version-min=$(MIN_VERSION) -march=armv7-a -Wl, -Bsymbolic-functions -read_only_relocs suppress
else ifeq ($(PLATFORM), osx)
	ifdef ARCH_x64
		CFLAGS  += -arch x86_64
	else
		CFLAGS  += -arch i386
	endif
	CC		?= clang
	CPP		?= clang
	CFLAGS  += -mmacosx-version-min=$(MIN_VERSION)
	LDFLAGS += -Wl,-rpath,./ -mmacosx-version-min=$(MIN_VERSION) -Bsymbolic-functions -framework Foundation -framework CoreLocation -framework Cocoa
	INSTALL_NAME = -install_name @loader_path/lib$(TARGET_NAME).dylib
else ifeq ($(PLATFORM), linux)
	LDFLAGS += -Wl,--rpath=./
else ifeq ($(PLATFORM), freebsd)
	MAKE 	?= gmake
	CC		?= clang
	LDFLAGS += -Wl,--rpath=./
else ifeq ($(PLATFORM), windows)

endif

ifeq ($(CC), gcc)
	CFLAGS  += -D_GNU_SOURCE=1 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2
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
ifdef debug
	CONFIGURATION = debug
else
	CONFIGURATION = release
endif

BUILD_PATH	  = $(SRCROOT)/build/intermediates/$(ARCH)/$(CONFIGURATION)
EXT_LIB_PATH  = $(SRCROOT)/lib/$(ARCH)/$(CONFIGURATION)
EXT_BIN_PATH  = $(SRCROOT)/bin/$(ARCH)/$(CONFIGURATION)

##编译过程中间文件输出目录
EXT_OBJ_PATH  = $(BUILD_PATH)/objs/$(TARGET_NAME)
EXT_LST_PATH  = $(BUILD_PATH)/lsts/$(TARGET_NAME)

#要包含的路径(本用例包含iclude,lib,bin 三个目录)
INCLUDE_PATH += $(SRCROOT)/include $(THIRD_PARTY_PATH)
LIBRARY_PATH += $(EXT_LIB_PATH) $(SRCROOT)/bin/$(ARCH) $(SRCROOT)/lib/$(ARCH) $(EXT_BIN_PATH)

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
#指明使用标准 ISO C99 再加上 GNU 的一些扩展作为标准来编译程序。c89, c99,gnu99
CFLAGS += -std=gnu99

#该选项能发现程序中一系列的常见错误警告
CFLAGS+=-Wall

#优化等级
ifdef debug
	#-O0禁止编译器进行优化
	CFLAGS  += -g -O0 -DDEBUG
else
	CFLAGS  += -O3 -DNDEBUG
endif

ifdef unicode
MACROS	+= _UNICODE UNICODE
endif

## get all macro
CFLAGS	+= $(addprefix -D,$(sort $(MACROS)))
## get all include path
CFLAGS  += $(addprefix -I,$(sort $(INCLUDE_PATH)))

ifdef shared
## get all library path
LDFLAGS += $(addprefix -L,$(sort $(LIBRARY_PATH)))
## get all librarys
LDFLAGS += $(addprefix -l,$(sort $(LIBS)))
endif

CXXFLAGS 	= $(CFLAGS) 
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

define build-successfully
	@echo " "
	@echo -e "\033[36mCompile successfully\033[0m"
	@echo -e "\033[36mPlatform: \033[34m$(PLATFORM)\033[0m"
	@echo -e "\033[36mARCH: \033[34m$(ARCH)\033[0m"
	@echo -e "\033[36mBuild file: $(2)\033[0m"
	@echo -e "\033[36mOutput file: $(1)\033[0m"
	@echo " "
endef

define build-local-src-files
	@if [ ! -d "$(EXT_BIN_PATH)" ]; then $(MKDIR) -p -m 755 $(EXT_BIN_PATH);fi
	@if [ ! -d "$(EXT_LIB_PATH)" ]; then $(MKDIR) -p -m 755 $(EXT_LIB_PATH);fi
	@if test -d $(EXT_OBJ_PATH); then $(RM) $(EXT_OBJ_PATH)/*; else $(MKDIR) -p -m 755 $(EXT_OBJ_PATH); fi
	@$(MAKE) $(LOCAL_SRC_FILES) $(2)
	@$(MAKE) $(TARGET_NAME)$(1) $(2)
endef

none: help
	@exit

.PHONY: help
help:
	@echo
	@echo "=============== A common Makefile for c programs =============="
	@echo "Copyright (C) 2011 - 2025 libcc.cn@gmail.com"
	@echo "The following targets aresupport:" 
	@echo 
	@echo "clean            - clean target"
	@echo "path             - create target path"
	@echo
	@echo  -e "To make a target, do '\033[31mmake .X1 platform=X2 arch=X3\033[0m'"
	@echo  -e "X1 = (\033[36m$(DYLIB_SUF), $(SO_SUF), $(BIN_SUF), $(LIB_SUF)\033[0m)"
	@echo  -e "X2 = (\033[36mwindows, linux, freebsd, unix, osx, osx, ios\033[0m)"
	@echo  -e "X3 = (\033[36mx64 = Compile 64-bit, x32 = Compile 32-bit\033[0m"
	@echo "See REANDME.Makefile for complete instructions."
	@echo
	@echo "====================== Version2.6 ============================"
	@exit

OUTPUT_OBJ	= $(EXT_OBJ_PATH)/$(subst /,_,$(subst $(SRCROOT),obj,$@))

-include $(wildcard $(EXT_DEPS_PATH)/*.d)
##将.cpp文件编译成目标文件(.o)##
%$(OBJ_SUF) : %$(CPP_SUF)
	$(CPP) $(CXXFLAGS) -c $< -o $(OUTPUT_OBJ)

##将.c文件编译成目标文件(.o)##
%$(OBJ_SUF) : %$(C_SUF) 
	$(CC) $(CFLAGS) -c $< -o $(OUTPUT_OBJ)

##将.m文件编译成目标文件(.o)##
%$(OBJ_SUF) : %$(M_SUF)
	$(CC) $(CFLAGS) -c $< -o $(OUTPUT_OBJ)

##将.c文件编译成汇编文件(.asm)##
%$(ASM_SUF) : %$(C_SUF)
	$(CC) $(CFLAGS) -S $< -o $(OUTPUT_OBJ)

##将.cpp文件编译成汇编文件(.asm)##
%$(ASM_SUF) : %$(CPP_SUF)
	$(CPP) $(CFLAGS) -S $< -o $(OUTPUT_OBJ)

##将.o文件编译成lib文件(.a)##
%$(LIB_SUF) :
	#$(RM) $(EXT_LIB_PATH)/lib$@
	$(AR) $(EXT_LIB_PATH)/lib$@ $(wildcard $(EXT_OBJ_PATH)/*.o)
	$(call build-successfully,$(EXT_LIB_PATH),lib$@)

##将.o文件编译成动态文件(.so,dll)##
%$(SO_SUF) %$(DLL_SUF):
	#$(RM) $(EXT_BIN_PATH)/lib$@
	$(CC) -shared -o $(EXT_BIN_PATH)/lib$@ $(EXT_OBJ_PATH)/* $(LDFLAGS) $(INSTALL_NAME)
	$(call build-successfully,$(EXT_BIN_PATH),lib$@)

##将.o文件编译成动态文件(.dylib)##
%$(DYLIB_SUF) :
	#$(RM) $(EXT_BIN_PATH)/lib$@
	$(CC) -dynamiclib -o $(EXT_BIN_PATH)/lib$@ $(EXT_OBJ_PATH)/* $(LDFLAGS) $(INSTALL_NAME)
	$(call build-successfully,$(EXT_BIN_PATH),lib$@)

##将.o文件编译成可执行文件##
%$(BIN_SUF) :
	#$(RM) $(EXT_BIN_PATH)/$(basename $@)
	$(CC) -o $(EXT_BIN_PATH)/$(basename $@) $(EXT_OBJ_PATH)/* $(LDFLAGS)
	$(call build-successfully,$(EXT_BIN_PATH),$(basename $@))

#编译静态库文件
 $(LIB_SUF):
	$(call build-local-src-files,$@,)
	@exit

#编译动态库/可执行文件
$(BIN_SUF) $(DYLIB_SUF) $(SO_SUF) $(DLL_SUF) :
	$(call build-local-src-files,$@, shared=1)
	@exit

.PHONY: clean
clean:
	@if test -d $(BUILD_PATH); then $(RMDIR) $(BUILD_PATH); fi
	@if test -d $(EXT_LIB_PATH); then $(RMDIR) $(EXT_LIB_PATH); fi
	#@if test -d $(EXT_BIN_PATH); then $(RMDIR) $(EXT_BIN_PATH); fi

.PHONY: path
path:
	@if test -d $(EXT_LIB_PATH); then echo $(EXT_LIB_PATH) is exist; else $(MKDIR) -p -m 755 $(EXT_LIB_PATH); fi
	@if test -d $(EXT_BIN_PATH); then echo $(EXT_BIN_PATH) is exist; else $(MKDIR) -p -m 755 $(EXT_BIN_PATH); fi
	@if test -d $(BUILD_PATH); then echo $(BUILD_PATH) is exist; else $(MKDIR) -p -m 755 $(BUILD_PATH); fi
