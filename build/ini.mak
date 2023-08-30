ifdef shared
	MACROS += _CC_INI_EXPORT_SHARED_LIBRARY_=1
endif

LOCAL_SRC_FILES = $(SRCROOT)/src/ini/ini.o \
					$(SRCROOT)/src/ini/ini.parser.o


