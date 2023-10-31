#SHELL = /bin/sh #/bin/bash
#指定第三方库安装路径
INCLUDE_PATH += $(abspath $(SRCROOT)/..)/third-party
#编译器路径(不一定要指定，如果用20060119版本的，需要指定)
#GCC_PATH	=
CC		= gcc
CPP		= g++
MAKE	= make
#set 1 Debug or 0 Release
#debug = 1
MIN_VERSION = 10.15

# CUP
ARCH := $(shell uname -m 2>/dev/null || echo Unknown)

#获取当前操作名称
ifeq ($(platform),)
    # Platform detections and overrides
    PLATFORM := $(shell uname 2>/dev/null || echo Unknown)
    PLATFORM := $(shell echo $(PLATFORM) | tr A-Z a-z )
    #ifneq ($(filter $(PLATFORM), msys mingw))
    #    PLATFORM := $(patsubst msys%, msys, $(PLATFORM))
    #    PLATFORM := $(patsubst mingw%, mingw, $(PLATFORM))
    #endif
else
    PLATFORM := $(platform)
endif

ifneq ($(filter $(PLATFORM), osx darwin),)
    PLATFORM = mac64
endif


##################################################
##				操作系统参数设置				  ##
##################################################
ifeq ($(PLATFORM), ios_x86_64)
    CC		:= xcrun -sdk iphonesimulator clang
    CPP		:= xcrun -sdk iphonesimulator clang
    CFLAGS	+= -arch x86_64 -mios-simulator-version-min=$(MIN_VERSION) -DIOS 
    LDFLAGS += -arch x86_64 -mios-simulator-version-min=$(MIN_VERSION) -Wl, -Bsymbolic-functions 
endif

ifeq ($(PLATFORM), ios_i386)
    CC		:= xcrun -sdk iphonesimulator clang
    CPP		:= xcrun -sdk iphonesimulator clang
    CFLAGS	+= -arch i386 -mios-simulator-version-min=$(MIN_VERSION) -DIOS 
    LDFLAGS += -arch i386 -mios-simulator-version-min=$(MIN_VERSION) -Wl, -Bsymbolic-functions 
endif

ifeq ($(PLATFORM), ios64)
    CC		:= xcrun -sdk iphoneos clang
    CPP		:= xcrun -sdk iphoneos clang
    CFLAGS	+= -arch arm64 -mios-version-min=$(MIN_VERSION) 
    LDFLAGS += -arch arm64 -mios-version-min=$(MIN_VERSION) -Wl, -Bsymbolic-functions 
endif

ifeq ($(PLATFORM), ios32)
    CC		:= xcrun -sdk iphoneos clang
    CPP		:= xcrun -sdk iphoneos clang
    CFLAGS  += -arch armv7 -mios-version-min=$(MIN_VERSION) -march=armv7-a -fmessage-length=0
    LDFLAGS += -arch armv7 -mios-version-min=$(MIN_VERSION) -march=armv7-a -Wl, -Bsymbolic-functions -read_only_relocs suppress 
endif

ifeq ($(PLATFORM), mac64)
    CC		:= clang
    LIBS    += m dl pthread
    CFLAGS  += -arch x86_64 -m64 -mmacosx-version-min=$(MIN_VERSION)
    LDFLAGS += -Wl,-rpath,./ -mmacosx-version-min=$(MIN_VERSION) -Bsymbolic-functions -framework Foundation -framework CoreLocation -framework Cocoa 
    INSTALL_NAME = -install_name @loader_path/lib$(TARGET_NAME).dylib
endif

ifeq ($(PLATFORM), mac32)
    CC		:= clang
    LIBS    += m dl pthread z
    CFLAGS  += -arch i386 -m32 -mmacosx-version-min=$(MIN_VERSION)
    LDFLAGS += -Wl,-rpath,./ -mmacosx-version-min=$(MIN_VERSION) -Bsymbolic-functions -framework Foundation -framework CoreLocation -framework Cocoa 
   	INSTALL_NAME = -install_name @loader_path/lib$(TARGET_NAME).dylib
endif

ifeq ($(PLATFORM), linux)
    LIBS    += m dl pthread z
    CFLAGS  += -D_DEFAULT_SOURCE 
    LDFLAGS += -Wl,--rpath=./ 

	# Use backwards compatible hash table style. This is because Fedora Core 6
	# defaults to using "gnu" style hash tables which produces incompatible
	# binaries with FC5 and before.
	#
	# By setting it to "both", we can have advantages of the faster hash style
	# on FC6 systems and backwards compatibilities with older systems, at a
	# small size penalty which is < 0.1% of file size.
	#ifneq ($(shell gcc -dumpspecs|grep "hash-style"),)
	#	LDFLAGS += --hash-style=both
	#endif
endif

ifeq ($(PLATFORM), freebsd)
    MAKE 	:= gmake
    CC		:= clang
    LIBS    += m dl pthread z
    CFLAGS  += -D_DEFAULT_SOURCE 
    LDFLAGS += -Wl,--rpath=./ 
endif

##################################################
##				编译器参数设置					  ##
##################################################

##编译过程中间文件输出目录
ifndef BUILD_PATH
BUILD_PATH = $(SRCROOT)/build/intermediates
endif

#定义输出目录名
EXT_OBJ_PATH = $(BUILD_PATH)/$(TARGET_NAME)/objs/
EXT_LST_PATH = $(BUILD_PATH)/$(TARGET_NAME)/lsts/
EXT_LIB_PATH = $(SRCROOT)/lib/
EXT_BIN_PATH = $(SRCROOT)/bin/

#要包含的路径(本用例包含iclude,lib,bin 三个目录)
INCLUDE_PATH += $(SRCROOT)/include
INCLUDE_PATH += $(SRCROOT)/lib
INCLUDE_PATH += $(SRCROOT)/bin
INCLUDE_PATH += $(THIRD_PARTY_PATH)
LIBRARY_PATH += $(EXT_LIB_PATH)
LIBRARY_PATH += $(EXT_BIN_PATH)

#在编译过程的不同阶段之间使用管道而非临时文件进行通信，可以加快编译速度。建议使用。
CFLAGS += -pipe
#该选项能发现程序中一系列的常见错误警告
CFLAGS += -Wall
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
#优化等级
ifdef debug
	#-O0禁止编译器进行优化
    CFLAGS  += -g -O0 -DDEBUG
else
    CFLAGS  += -O3 -DNDEBUG
endif

## get all macro
CFLAGS	+= $(addprefix -D,$(sort $(MACROS)))
## get all include path
CFLAGS  += $(addprefix -I,$(sort $(INCLUDE_PATH)))

## get all library path
LDFLAGS += $(addprefix -L,$(sort $(LIBRARY_PATH)))

## get all librarys
LDFLAGS += $(addprefix -l,$(sort $(LIBS)))

CXXFLAGS 	= $(CFLAGS) 
OBJCOPY		= objcopy
OBJDUMP		= objdump
SIZE		= size
AR			= ar crv ##ar rcu

MKDIR		= mkdir
RMDIR		= rm -rf 
RM			= rm -f

#定义文件类型
C_SUFFIX		= *.c
M_SUFFIX		= *.m
CPP_SUFFIX		= *.cpp
ASM_SUFFIX		= *.s
OBJ_SUFFIX		= *.o
LIB_SUFFIX		= *.a
BIN_SUFFIX		= *.bin
SO_SUFFIX		= *.so
DLL_SUFFIX		= *.dll
DYLIB_SUFFIX	= *.dylib

C_SUF		= $(suffix $(C_SUFFIX))
M_SUF		= $(suffix $(M_SUFFIX))
CPP_SUF		= $(suffix $(CPP_SUFFIX))
ASM_SUF		= $(suffix $(ASM_SUFFIX))
OBJ_SUF		= $(suffix $(OBJ_SUFFIX))
LIB_SUF		= $(suffix $(LIB_SUFFIX))
BIN_SUF		= $(suffix $(BIN_SUFFIX))
SO_SUF		= $(suffix $(SO_SUFFIX))
DLL_SUF		= $(suffix $(DLL_SUFFIX))
DYLIB_SUF	= $(suffix $(DYLIB_SUFFIX))

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
	@if test -d $(EXT_OBJ_PATH); then $(RM) $(EXT_OBJ_PATH)*.*;else $(MKDIR) -p -m 777 $(EXT_OBJ_PATH);fi
	@$(MAKE) $(LOCAL_SRC_FILES) $(2)
	@$(MAKE) $(TARGET_NAME)$(1) $(2)
endef

none: help
	@exit

.PHONY: help
help:
	@echo
	@echo "=============== A common Makefile for c programs =============="
	@echo "Copyright (C) 2011 - 2021 huai2011@163.com"
	@echo "The following targets aresupport:" 
	@echo 
	@echo "clean            - clean target"
	@echo "path             - create target path"
	@echo
	@echo  -e "To make a target, do '\033[31mmake .X1 platform=X2\033[0m'"
	@echo  -e "X1 = (\033[36m$(DYLIB_SUF), $(SO_SUF), $(BIN_SUF), $(LIB_SUF)\033[0m)"
	@echo  -e "X2 = (\033[36mlinux, freebsd, unix, osx, mac64, mac32, ios_x86_64, ios_i386, ios64, ios32\033[0m)"
	@echo
	@echo "See REANDME.Makefile for complete instructions."
	@echo
	@echo "====================== Version2.6 ============================"
	@exit

OUTPUT_OBJ_OPTION = $(EXT_OBJ_PATH)$(subst /,_,$(subst $(SRCROOT),obj,$@))

##将.cpp文件编译成目标文件(.o)##
%$(OBJ_SUF) : %$(CPP_SUF)
	$(CPP) $(CXXFLAGS) -c $< -o $(OUTPUT_OBJ_OPTION)

##将.c文件编译成目标文件(.o)##
%$(OBJ_SUF) : %$(C_SUF) 
	$(CC) $(CFLAGS) -c $< -o $(OUTPUT_OBJ_OPTION)

##将.m文件编译成目标文件(.o)##
%$(OBJ_SUF) : %$(M_SUF)
	$(CC) $(CFLAGS) -c $< -o $(OUTPUT_OBJ_OPTION)

##将.c文件编译成汇编文件(.asm)##
%$(ASM_SUF) : %$(C_SUF)
	$(CC) $(CFLAGS) -S $< -o $(OUTPUT_OBJ_OPTION)

##将.cpp文件编译成汇编文件(.asm)##
%$(ASM_SUF) : %$(CPP_SUF)
	$(CPP) $(CFLAGS) -S $< -o $(OUTPUT_OBJ_OPTION)

##将.o文件编译成lib文件(.a)##
%$(LIB_SUF) :
	$(RM) $(EXT_LIB_PATH)lib$@
	$(AR) $(EXT_LIB_PATH)lib$@ $(wildcard $(EXT_OBJ_PATH)$(OBJ_SUFFIX))
	$(call build-successfully,$(EXT_LIB_PATH),lib$@)


##将.o文件编译成动态文件(.dll)##
%$(DLL_SUF) :
	$(RM) $(EXT_BIN_PATH)lib$@
	$(CC) -shared -o $(EXT_BIN_PATH)lib$@ $(EXT_OBJ_PATH)* $(LDFLAGS) $(INSTALL_NAME)
	$(call build-successfully,$(EXT_LIB_PATH),lib$@)

##将.o文件编译成动态文件(.so)##
%$(SO_SUF) :
	$(RM) $(EXT_BIN_PATH)lib$@
	$(CC) -shared -o $(EXT_BIN_PATH)lib$@ $(EXT_OBJ_PATH)* $(LDFLAGS) $(INSTALL_NAME)
	$(call build-successfully,$(EXT_LIB_PATH),lib$@)

##将.o文件编译成动态文件(.dylib)##
%$(DYLIB_SUF) :
	$(RM) $(EXT_BIN_PATH)lib$@
	$(CC) -dynamiclib -o $(EXT_BIN_PATH)lib$@ $(EXT_OBJ_PATH)* $(LDFLAGS) $(INSTALL_NAME)
	$(call build-successfully,$(EXT_BIN_PATH),lib$@)

##将.o文件编译成可执行文件##
%$(BIN_SUF) :
	$(RM) $(EXT_BIN_PATH)$(basename $@)
	$(CC) -o $(EXT_BIN_PATH)$(basename $@) $(EXT_OBJ_PATH)* $(LDFLAGS)
	$(call build-successfully,$(EXT_BIN_PATH),$(basename $@))

#编译静态库文件
$(LIB_SUF) :
	$(call build-local-src-files,$@,)
	@exit
#编译可执行文件
$(BIN_SUF):
	$(call build-local-src-files,$@,)
	@exit
#编译动态库文件
$(DYLIB_SUF) $(SO_SUF) $(DLL_SUF) :
	$(call build-local-src-files,$@,shared=1)
	@exit



.PHONY: clean
clean:
	@if test -d $(BUILD_PATH); then $(RMDIR) $(BUILD_PATH); fi
	@if test -d $(EXT_LIB_PATH); then $(RMDIR) $(EXT_LIB_PATH); fi
	@if test -d $(EXT_BIN_PATH); then $(RMDIR) $(EXT_BIN_PATH); fi

.PHONY: path
path:
	@if test -d $(EXT_LIB_PATH); then echo $(EXT_LIB_PATH) is exist; else $(MKDIR) -p -m 777 $(EXT_LIB_PATH); fi
	@if test -d $(EXT_BIN_PATH); then echo $(EXT_BIN_PATH) is exist; else $(MKDIR) -p -m 777 $(EXT_BIN_PATH); fi
