SRCROOT	:= $(abspath ../..)

include $(SRCROOT)/build/Makefile.mak

ifeq ($(PLATFORM), windows)
	INCLUDE_PATH += D:\\240216\\third-party\\
	LIBRARY_PATH += D:\\240216\\third-party\\lib
else
	INCLUDE_PATH += /usr/local/Cellar/jpeg/9e/include
	LIBRARY_PATH += /usr/local/Cellar/jpeg/9e/lib
endif

LIBS += z png jpeg cc

LOCAL_SRC_FILES = loaderBMP.o \
				  loaderJPG.o \
				  loaderPCX.o \
				  loaderPNG.o \
				  loaderPSD.o \
				  loaderTGA.o \
				  writeBMP.o \
				  writeJPG.o \
				  writePCX.o \
				  writePNG.o \
				  writeTGA.o \
				  image.o \
				  color.o \
				  test.o

TARGET_NAME = bitmap.$(PLATFORM)