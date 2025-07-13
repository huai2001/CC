##################################################
SRCROOT	:= $(abspath ..)

#指定第三方库安装路径
THIRD_PARTY_PATH = $(abspath $(SRCROOT)/..)/third-party
INCLUDE_PATH += $(THIRD_PARTY_PATH)
LIBRARY_PATH += $(THIRD_PARTY_PATH)/lib/$(ARCH)/

include $(SRCROOT)/build/Makefile.mak

ifdef target
	include $(SRCROOT)/build/$(target).mak
endif

install:
	@$(MKDIR) -p -m 755 $(INSTALL_DIR)/lib
	@$(MKDIR) -p -m 755 $(INSTALL_DIR)/bin
	@$(MKDIR) -p -m 755 $(INSTALL_DIR)/include
	@cp -f -r $(SRCROOT)/include/* $(INSTALL_DIR)/include
	@cp -f -r $(EXT_LIB_PATH)/libcc.a $(INSTALL_DIR)/lib/libcc.$(ARCH)-$(CONFIGURATION).a

uninstall:
	@$(RMDIR) $(INSTALL_DIR)/include/libcc.h
	@$(RMDIR) $(INSTALL_DIR)/include/libcc
	@$(RMDIR) $(INSTALL_DIR)/lib/libcc.*