SRCROOT	:= $(abspath ../..)
include $(SRCROOT)/build/Makefile.mak

ifeq ($(debug),1)
	LIBS	+= ccd.$(PLATFORM) 
	TARGET_NAME = Packetsd.$(PLATFORM)
else
	LIBS	+= cc.$(PLATFORM)
	TARGET_NAME = Packets.$(PLATFORM)
endif


LOCAL_SRC_FILES = mapped.o packet.o quicklz.o
