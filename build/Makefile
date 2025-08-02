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

ifdef shared
	LIBS		+= cc
endif

ifeq ($(PLATFORM), windows)
	INSTALL_DIR	= c:\\libcc
	INSTALL_SUF = a
else
	INSTALL_DIR	= /usr/local
	INSTALL_SUF = a
endif

install:
	@$(MKDIR) -p -m 755 $(INSTALL_DIR)/lib
	@$(MKDIR) -p -m 755 $(INSTALL_DIR)/bin
	@$(MKDIR) -p -m 755 $(INSTALL_DIR)/include
	@cp -f -r $(SRCROOT)/include/* $(INSTALL_DIR)/include
	@cp -f -r $(EXT_LIB_PATH)/libcc.$(INSTALL_SUF) $(INSTALL_DIR)/lib/libcc-$(ARCH)-$(CONFIGURATION).$(INSTALL_SUF)
	#@cp -f -r $(EXT_BIN_PATH)/libcc.widgets.$(INSTALL_SUF) $(INSTALL_DIR)/lib/libcc-widgets-$(ARCH)-$(CONFIGURATION).$(INSTALL_SUF)

uninstall:
	$(RM) $(INSTALL_DIR)/include/libcc.h
	@$(RMDIR) $(INSTALL_DIR)/include/libcc/
	@$(RM)  $(INSTALL_DIR)/lib/libcc*