##################################################
SRCROOT	:= $(abspath ..)

PROJECT_NAME ?= cc
LIBCC_SRC_ROOT ?= $(SRCROOT)

include $(SRCROOT)/build/local-init.mk

ifdef target
	include $(SRCROOT)/build/$(target).mk
endif

include $(SRCROOT)/build/Makefile.mk