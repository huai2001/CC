SRCROOT	:= $(abspath ../..)

INCLUDE_PATH += /usr/local/Cellar/jpeg/9e/include
LIBRARY_PATH += /usr/local/Cellar/jpeg/9e/lib

include $(SRCROOT)/build/Makefile.mak


ifeq ($(debug),1)
	LIBS 	+= ccd.$(PLATFORM)
else
	LIBS	+= cc.$(PLATFORM)
endif

LIBS += z png jpeg

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