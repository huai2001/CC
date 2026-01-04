#!/bin/bash

# 定义支持的架构
ARCHS=("arm64" "x86_64")
SDK_VERSION="18.4"
OPENSSL_VERSION="3.2.5"
OPENSSL_OUTPUT=$(pwd)/openssl-${OPENSSL_VERSION}-iOS
OPENSSL_SSL_NAME="libssl.3.dylib"
OPENSSL_CRYPTO_NAME="libcrypto.3.dylib"
#在编译的时候我碰到了一些编译问题，例如 fatal error: 'inttypes.h' file not found 使用sudo ./OpenSSL.ios.sh
## 编译 arm64 版本
#./Configure darwin64-x86_64-cc
# 编译 x86_64 版本
#./Configure darwin64-arm64-cc

#curl -L -O https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
#curl -L -O https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_VERSION}/openssl-${OPENSSL_VERSION}.tar.gz

# 清理旧文件
rm -rf openssl-${OPENSSL_VERSION}

# 解压源码
tar -xzf openssl-${OPENSSL_VERSION}.tar.gz
cd openssl-${OPENSSL_VERSION}

# 创建输出目录
mkdir -p $OPENSSL_OUTPUT

export CC=clang
export PATH="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin:$PATH"

# 编译每个架构
for ARCH in ${ARCHS[@]}; do
    case $ARCH in
        "arm64")
            PLATFORM="iPhoneOS"
            ;;
        "x86_64")
            PLATFORM="iPhoneSimulator"
            ;;
    esac

    export CROSS_TOP=/Applications/Xcode.app/Contents/Developer/Platforms/${PLATFORM}.platform/Developer
    export CROSS_SDK=${PLATFORM}${SDK_VERSION}.sdk

    ./Configure ios64-cross no-dso no-hw no-asm --prefix=${OPENSSL_OUTPUT}/${ARCH}

    make -j$(sysctl -n hw.ncpu)
    make install
    make clean

    # 拷贝
    mkdir -p ../../lib/${ARCH}/
    cp -f ${OPENSSL_OUTPUT}/${ARCH}/lib/${OPENSSL_SSL_NAME}  ../../lib/${ARCH}/${OPENSSL_SSL_NAME}
    cp -f ${OPENSSL_OUTPUT}/${ARCH}/lib/${OPENSSL_CRYPTO_NAME} ../../lib/${ARCH}/${OPENSSL_CRYPTO_NAME}

    install_name_tool -id @rpath/${OPENSSL_SSL_NAME} ../../lib/${ARCH}/${OPENSSL_SSL_NAME}
    install_name_tool -id @rpath/${OPENSSL_CRYPTO_NAME} ../../lib/${ARCH}/${OPENSSL_CRYPTO_NAME}

    otool -L ../../lib/${ARCH}/${OPENSSL_SSL_NAME}
    otool -L ../../lib/${ARCH}/${OPENSSL_CRYPTO_NAME}
done

mkdir -p ../../include/openssl/
cp -rf ${OPENSSL_OUTPUT}/arm64/include/openssl/* ../../include/openssl


# 合并静态库
#lipo -create ${OPENSSL_OUTPUT}/arm64/lib/${OPENSSL_SSL_NAME} ${OPENSSL_OUTPUT}/x86_64/lib/${OPENSSL_SSL_NAME} -output ../../lib/iOS/${OPENSSL_SSL_NAME}
#lipo -create ${OPENSSL_OUTPUT}/arm64/lib/${OPENSSL_CRYPTO_NAME} ${OPENSSL_OUTPUT}/x86_64/lib/${OPENSSL_CRYPTO_NAME} -output ../../lib/iOS/${OPENSSL_CRYPTO_NAME}

echo "编译完成！静态库已生成到 ${OPENSSL_OUTPUT} 目录。"
