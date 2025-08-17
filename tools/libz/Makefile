SRCROOT	:= $(abspath ../..)
include $(SRCROOT)/build/Makefile.mak

ifeq ($(debug),1)
	LIBS	+= pthread z m dl
else
	LIBS	+= pthread z m dl
endif

LOCAL_SRC_FILES = ./ioapi.o ./minizip.o ./mztools.o ./unzip.o ./zip.o

TARGET_NAME = minizip