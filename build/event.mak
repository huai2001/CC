ifdef shared
	MACROS += _CC_EVENT_EXPORT_SHARED_LIBRARY_=1
endif


EVENT_FILES = $(SRCROOT)/src/event

LOCAL_SRC_FILES = \
		$(EVENT_FILES)/event.o \
		$(EVENT_FILES)/loop.o \
		$(EVENT_FILES)/select.o \
		$(EVENT_FILES)/timeout.o \
		$(EVENT_FILES)/tcp.o \
		$(EVENT_FILES)/buffer.o

ifneq ($(filter $(PLATFORM), ios64 ios32 ios_x86_64 ios_i386 mac64 mac32 freebsd unix),)
	LOCAL_SRC_FILES += $(EVENT_FILES)/unix/sys_kqueue.o
endif

ifeq ($(PLATFORM), linux)
	LOCAL_SRC_FILES += $(EVENT_FILES)/linux/sys_epoll.o
endif