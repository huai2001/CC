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
DEBUG=debug=1
else
DEBUG=
fi

current_path=$(cd `dirname $0`; pwd)
cd $current_path

cd ..
$MAKE_TOOL .a platform=$PLATFORM $DEBUG

cd ./build

$MAKE_TOOL $TARGET_NAME platform=$PLATFORM target=widgets all=1 $DEBUG