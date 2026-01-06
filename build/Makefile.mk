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

define build-successfully
	@echo " "
	@echo "\033[36mCompile successfully\033[0m"
	@echo "\033[36mPlatform: \033[34m$(PLATFORM)\033[0m"
	@echo "\033[36mARCH: \033[34m$(ARCH)\033[0m"
	@echo "\033[36mBuild: $(1)\033[0m"
	@echo "\033[36mOutput file: $(2)\033[0m"
	@echo " "
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
	@echo "clean            - clean all target"
	@echo "clean-debug      - clean debug target"
	@echo "clean-release    - clean release target"
	@echo
	@echo  -e "To make a target, do '\033[31mmake .X1 platform=X2 arch=X3\033[0m'"
	@echo  -e "X1 = (\033[36m.dylib、.so、.dll、.a、.bin\033[0m)"
	@echo  -e "X2 = (\033[36mwindows、linux、freebsd、unix、osx、ios\033[0m)"
	@echo  -e "X3 = (\033[36mx64 = Compile 64-bit, x32 = Compile 32-bit\033[0m"
	@echo "See REANDME.Makefile for complete instructions."
	@echo
	@echo "System is : $(PLATFORM)"
	@echo "====================== Version2.6 ============================"
	@exit

LOCAL_OBJ_FILES := $(LOCAL_SRC_FILES:.c=.o)
LOCAL_OBJ_FILES := $(LOCAL_OBJ_FILES:.m=.o)
LOCAL_OBJ_FILES := $(subst $(SRCROOT),$(EXT_OBJ_PATH),$(LOCAL_OBJ_FILES))


# 包含依赖文件
-include $(LOCAL_OBJ_FILES:.o=.d)

##将.o文件编译成动态文件(.so,dll)##
$(SO_SUF) $(DLL_SUF): $(LOCAL_OBJ_FILES)
	@$(MKDIR) -p $(EXT_BIN_PATH)
	$(CC) -shared -o $(EXT_BIN_PATH)/lib$(TARGET_NAME)$@ $^ $(LDFLAGS) $(INSTALL_NAME)
	$(call build-successfully,$(EXT_BIN_PATH),lib$(TARGET_NAME)$@)

##将.o文件编译成动态文件(.dylib)##
$(DYLIB_SUF): $(LOCAL_OBJ_FILES)
	@$(MKDIR) -p $(EXT_BIN_PATH)
	$(CC) -dynamiclib -o $(EXT_BIN_PATH)/lib$(TARGET_NAME)$@ $^ $(LDFLAGS) $(INSTALL_NAME)
	$(call build-successfully,$(EXT_BIN_PATH),lib$(TARGET_NAME)$@)

##将.o文件编译成可执行文件##
$(BIN_SUF): $(LOCAL_OBJ_FILES)
	@$(MKDIR) -p $(EXT_BIN_PATH)
	$(CC) -o $(EXT_BIN_PATH)/$(TARGET_NAME) $^ $(LDFLAGS)
	$(call build-successfully,$(EXT_BIN_PATH),$(TARGET_NAME))

##将.o文件编译成lib文件(.a)##
$(LIB_SUF): $(LOCAL_OBJ_FILES)
	@$(MKDIR) -p $(EXT_LIB_PATH)
	$(AR) $(EXT_LIB_PATH)/lib$(TARGET_NAME).static$@ $^
	$(call build-successfully,$(EXT_LIB_PATH),lib$(TARGET_NAME).static$@)

##将.cpp文件编译成目标文件(.o)##
$(EXT_OBJ_PATH)/%$(OBJ_SUF): $(SRCROOT)/%$(CPP_SUF)
	@$(MKDIR) -p $(dir $@)
	$(CPP) $(CXXFLAGS) -MF $(@:.o=.d) -c $< -o $@

##将.c文件编译成目标文件(.o)##
$(EXT_OBJ_PATH)/%$(OBJ_SUF): $(SRCROOT)/%$(C_SUF)
	@$(MKDIR) -p $(dir $@)
	$(CC) $(CFLAGS) -MF $(@:.o=.d) -c $< -o $@

##将.m文件编译成目标文件(.o)##
$(EXT_OBJ_PATH)/%$(OBJ_SUF): $(SRCROOT)/%$(M_SUF)
	@$(MKDIR) -p $(dir $@)
	$(CC) $(CFLAGS) -MF $(@:.o=.d) -c $< -o $@

##将.c文件编译成汇编文件(.asm)##
$(EXT_OBJ_PATH)/%$(ASM_SUF) : $(SRCROOT)/%$(C_SUF)
	@$(MKDIR) -p $(dir $@)
	$(CC) $(CFLAGS) -S $< -o $@

##将.cpp文件编译成汇编文件(.asm)##
$(EXT_OBJ_PATH)/%$(ASM_SUF) : $(SRCROOT)/%$(CPP_SUF)
	@$(MKDIR) -p $(dir $@)
	$(CPP) $(CFLAGS) -S $< -o $@

.PHONY: clean
clean: 
	@if test -d $(SRCROOT)/obj/$(ARCH); then $(RMDIR) $(SRCROOT)/obj/$(ARCH); fi
