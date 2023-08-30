##################################################
SRCROOT	:= $(abspath ..)
include $(SRCROOT)/build/Makefile.mak

ifdef debug
	LIBS	+= ccd.$(PLATFORM)
	TARGET_NAME = cc.$(target)d.$(PLATFORM)
else
	LIBS	+= cc.$(PLATFORM)
	TARGET_NAME = cc.$(target).$(PLATFORM)
endif

ifdef target
	include $(SRCROOT)/build/$(target).mak
endif