ifdef build
	LOCAL_SRC_FILES = $(SRCROOT)/examples/$(build).o
	TARGET_NAME = $(build)
endif

ifneq (,$(filter $(PLATFORM),mac64 mac32))
	LIBRARY_PATH	+= /usr/local/mysql/lib/
	MACROS	+= _CC_ENABLE_UNIXODBC_=1
	INCLUDE_PATH	+= /usr/local/Cellar/unixodbc/2.3.9_1/include/
	LIBRARY_PATH	+= /usr/local/Cellar/unixodbc/2.3.9_1/lib/

	LIBS += ssl.3 crypto.3 mysqlclient.21 odbc
else
	LIBRARY_PATH	+= /usr/lib64/mysql/
	INCLUDE_PATH	+= /usr/include/
	LIBS += ssl crypto mysqlclient
endif

ifdef debug
	LIBS	+= cc.eventd.$(PLATFORM) cc.widgetsd.$(PLATFORM) cc.xmld.$(PLATFORM) cc.jsond.$(PLATFORM) cc.dbd.$(PLATFORM) z
else
	LIBS	+= cc.event.$(PLATFORM) cc.widgets.$(PLATFORM) cc.xml.$(PLATFORM) cc.json.$(PLATFORM) cc.db.$(PLATFORM) z
endif

ifdef dll
	LIBS += $(dll)
endif