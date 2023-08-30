SRCROOT	:= $(abspath ../..)

include $(SRCROOT)/build/Makefile.mak
MACROS +=  _CC_OPENSSL_HTTPS_

ifeq ($(debug),1)
	LIBS 	+= ccd.$(PLATFORM) cc.eventd.$(PLATFORM) cc.jsond.$(PLATFORM) cc.widgetsd.$(PLATFORM)
else
	LIBS	+= cc.$(PLATFORM) cc.event.$(PLATFORM) cc.json.$(PLATFORM) cc.widgets.$(PLATFORM)
endif

LIBS += z

LOCAL_SRC_FILES = SyncFiles.o Version.o Project.o Sync.o RequestURL.o Main.o xxtea.o
TARGET_NAME = SyncFiles.$(PLATFORM)