##################################################

SRCROOT	:= $(abspath .)
#生成的文件名
TARGET_NAME ?= cc

USE_LIB_OPENSSL=1
USE_LIB_SMTP=1
# USE_LIB_MYSQL=1
# USE_LIB_SQLITE3=1
USE_LIB_URL_REQUEST=1

include $(SRCROOT)/build/local-init.mk
include $(SRCROOT)/build/local-files.mk
include $(SRCROOT)/build/Makefile.mk