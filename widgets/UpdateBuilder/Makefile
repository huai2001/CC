SRCROOT	:= $(abspath ../..)

include $(SRCROOT)/build/Makefile.mak

LIBS += sqlite3
ifeq ($(debug),1)
	LIBS 	+= ccd.$(PLATFORM) cc.dbd.$(PLATFORM) cc.jsond.$(PLATFORM) 
else
	LIBS	+= cc.$(PLATFORM) cc.db.$(PLATFORM) cc.json.$(PLATFORM)
endif

LIBS += z

LOCAL_SRC_FILES = UpdateBuilder.o reload.o update.o SQLite3.o
TARGET_NAME = UpdateBuilder.$(PLATFORM)