#!/usr/bin/env bash
TARGET_NAME=.so
PLATFORM=linux
MAKE_TOOL=make

KERNEL_NAME="$(uname -s)"
KERNEL_VERSION="$(uname -r)"
ARCHITECTURE="$(uname -m)"
#MINGW64
if [[ $KERNEL_NAME == MINGW64* ]];then
    KERNEL_NAME="Windows"
fi

case $KERNEL_NAME in
"FreeBSD")
    PLATFORM="freebsd"
    MAKE_TOOL=gmake
    ;;
"Linux")
    PLATFORM="linux"
    ;;
"Darwin")
    PLATFORM="osx"
    TARGET_NAME=.dylib
    ;;
"SunOS")
    PLATFORM="solaris"
    ;;
"Windows")
    PLATFORM="Windows"
    TARGET_NAME=.dll
    ;;
*)
    echo "Unknown platform" >&2
    exit 1
esac

if [ "$1" == "debug" ]; then
debug="debug=1"
fi

path=$(cd `dirname $0`; pwd)
cd $path

cd ..
$MAKE_TOOL .a platform=$PLATFORM
$MAKE_TOOL .a platform=$PLATFORM debug=1

cd ./build

$MAKE_TOOL $TARGET_NAME platform=$PLATFORM target='event'
$MAKE_TOOL $TARGET_NAME platform=$PLATFORM target='db'
$MAKE_TOOL $TARGET_NAME platform=$PLATFORM target='xml'
$MAKE_TOOL $TARGET_NAME platform=$PLATFORM target='json'
$MAKE_TOOL $TARGET_NAME platform=$PLATFORM target='ini'
$MAKE_TOOL $TARGET_NAME platform=$PLATFORM target='widgets'

$MAKE_TOOL $TARGET_NAME platform=$PLATFORM target='event' debug=1
$MAKE_TOOL $TARGET_NAME platform=$PLATFORM target='db' debug=1
$MAKE_TOOL $TARGET_NAME platform=$PLATFORM target='xml' debug=1
$MAKE_TOOL $TARGET_NAME platform=$PLATFORM target='json' debug=1
$MAKE_TOOL $TARGET_NAME platform=$PLATFORM target='ini' debug=1
$MAKE_TOOL $TARGET_NAME platform=$PLATFORM target='widgets' debug=1