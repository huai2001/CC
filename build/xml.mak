ifdef shared
	MACROS += _CC_XML_EXPORT_SHARED_LIBRARY_=1
endif

LOCAL_SRC_FILES = $(SRCROOT)/src/xml/xml.o \
					$(SRCROOT)/src/xml/xml.parser.o


