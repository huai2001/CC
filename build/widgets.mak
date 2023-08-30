ifdef shared
	MACROS += _CC_WIDGETS_EXPORT_SHARED_LIBRARY_=1
	ifeq ($(PLATFORM), mac64)
		LIBS += ssl.3 crypto.3
	else
		LIBS += ssl crypto
	endif
	LIBS += z
	ifdef debug
		LIBS	+= cc.eventd.$(PLATFORM)
	else
		LIBS	+= cc.event.$(PLATFORM)
	endif
endif

MACROS += _CC_OPENSSL_HTTPS_=1 

WIDGET_FILES = $(SRCROOT)/src/widgets

#IPLOCATOR_SRC_FILES = $(WIDGET_FILES)/ip_locator/ip_locator.o

PROXY_SRC_FILES = \
					$(WIDGET_FILES)/proxy/http.o

URL_SRC_FILES = \
					$(WIDGET_FILES)/url_request/http/gzip.o \
					$(WIDGET_FILES)/url_request/http/url_request.o \
					$(WIDGET_FILES)/url_request/http/url_response.o

#SMTP_SRC_FILES = \
					$(WIDGET_FILES)/smtp/libsmtp.o \
					$(WIDGET_FILES)/smtp/connected.o \
					$(WIDGET_FILES)/smtp/login.o \
					$(WIDGET_FILES)/smtp/from_rcpt.o

#FTP_SRC_FILES = \
					$(WIDGET_FILES)/ftp/libftp.o \
					$(WIDGET_FILES)/ftp/connected.o \
					$(WIDGET_FILES)/ftp/login.o \
					$(WIDGET_FILES)/ftp/cdup.o \
					$(WIDGET_FILES)/ftp/cwd.o \
					$(WIDGET_FILES)/ftp/delete_file.o \
					$(WIDGET_FILES)/ftp/delete_folder.o \
					$(WIDGET_FILES)/ftp/list.o \
					$(WIDGET_FILES)/ftp/mkdir.o \
					$(WIDGET_FILES)/ftp/opts.o \
					$(WIDGET_FILES)/ftp/rename_file.o

#DNS_SRC_FILES = \
					$(WIDGET_FILES)/dns/dns.o \
					$(WIDGET_FILES)/dns/dnss.o

LOCAL_SRC_FILES = \
					$(PROXY_SRC_FILES) \
					$(URL_SRC_FILES) \
					$(SMTP_SRC_FILES) \
					$(FTP_SRC_FILES) \
					$(DNS_SRC_FILES) \
					$(IPLOCATOR_SRC_FILES) \
					$(WIDGET_FILES)/dict.o \
					$(WIDGET_FILES)/SSL.o \
					$(WIDGET_FILES)/widgets.o