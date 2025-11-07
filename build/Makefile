##################################################
SRCROOT	:= $(abspath ..)

TARGET_NAME ?= cc

include $(SRCROOT)/build/local-init.mk

ifdef target
	include $(SRCROOT)/build/$(target).mk
endif

include $(SRCROOT)/build/Makefile.mk