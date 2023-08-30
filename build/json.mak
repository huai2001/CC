ifdef shared
	MACROS += _CC_JSON_EXPORT_SHARED_LIBRARY_=1
endif

LOCAL_SRC_FILES = $(SRCROOT)/src/json/json.o \
					$(SRCROOT)/src/json/json.parser.o


