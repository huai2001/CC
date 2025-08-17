#!/bin/bash
_CC_NDK=$NDK9_ROOT
sourcePath="$0"
# resolve $sourcePath until the file is no longer a symlink
while [ -h "$sourcePath"  ]; do
basePath="$( cd -P "$( dirname "$sourcePath"  )" && pwd  )"
sourcePath="$(readlink "$sourcePath")"
# if $sourcePath was a relative symlink,
# we need to resolve it relative to the path where the symlink file was located
[[ $sourcePath != /*  ]] && sourcePath="$basePath/$sourcePath"

done
basePath="$( cd -P "$( dirname "$sourcePath"  )" && pwd  )"

echo "NDK_ROOT = $CC_NDK"
echo "APP_ROOT = $basePath"

cd $basePath
bash $CC_NDK/ndk-build
