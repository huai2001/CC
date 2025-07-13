ifdef build
	LOCAL_SRC_FILES = $(SRCROOT)/examples/$(build).o
endif

TARGET_NAME = tests#$(build)

LIBS	+= cc cc.widgets

ifdef dll
	LIBS += $(dll)
endif