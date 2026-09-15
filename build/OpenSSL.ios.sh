#!/bin/bash

set -e # 遇到错误立即退出

# ================= 配置区域 =================
OPENSSL_VERSION="3.2.5"
# 动态获取当前 Xcode 中最新的 iPhoneOS 和 iPhoneSimulator SDK 版本
# 注意：这假设你希望使用最新安装的 SDK。如果需要特定版本，请手动指定，但必须确保该版本已安装。
IOS_SDK_VERSION=$(xcrun --sdk iphoneos --show-sdk-version 2>/dev/null || echo "17.5")
SIMULATOR_SDK_VERSION=$(xcrun --sdk iphonesimulator --show-sdk-version 2>/dev/null || echo "17.5")

# 动态获取 SDK 版本和路径
IOS_SDK=$(xcrun --sdk iphoneos --show-sdk-path)
SIM_SDK=$(xcrun --sdk iphonesimulator --show-sdk-path)

echo "Using iOS SDK Version: ${IOS_SDK_VERSION}"
echo "Using Simulator SDK Version: ${SIMULATOR_SDK_VERSION}"

BUILD_ROOT=$(pwd)
OPENSSL_OUTPUT=${BUILD_ROOT}/openssl-${OPENSSL_VERSION}-iOS
INSTALL_OPENSSL_INCLUDE=${BUILD_ROOT}/../include/openssl
INSTALL_OPENSSL_LIB=${BUILD_ROOT}/../lib

OPENSSL_SSL_NAME="libssl.3.dylib"
OPENSSL_CRYPTO_NAME="libcrypto.3.dylib"

# ================= 准备阶段 =================

# 下载源码 (如果不存在)
if [ ! -f "openssl-${OPENSSL_VERSION}.tar.gz" ]; then
    echo "Downloading OpenSSL ${OPENSSL_VERSION}..."
    curl -L -O https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
fi

# 清理旧构建目录
rm -rf openssl-${OPENSSL_VERSION}
rm -rf ${OPENSSL_OUTPUT}
rm -rf ${INSTALL_OPENSSL_INCLUDE}
rm -rf ${INSTALL_OPENSSL_LIB}

# 解压源码
tar -xzf openssl-${OPENSSL_VERSION}.tar.gz
cd openssl-${OPENSSL_VERSION}

# 创建输出目录
mkdir -p ${OPENSSL_OUTPUT}
mkdir -p ${INSTALL_OPENSSL_INCLUDE}


echo "🚀 开始编译 OpenSSL ${OPENSSL_VERSION} for iOS..."

# ================= 编译函数 =================
build_arch() {
    local ARCH=$1
    local PLATFORM=$2
    local SDK_PATH=$3
    local TARGET=$4
    local SDK_VER=$5

    mkdir -p "${INSTALL_OPENSSL_LIB}/${ARCH}"

    echo "----------------------------------------"
    echo "🔨 Building for ${ARCH} (${PLATFORM})..."
    echo "----------------------------------------"

    # 清除可能冲突的 macOS 部署目标变量
    unset MACOSX_DEPLOYMENT_TARGET

    # 如果上述命令失败，尝试通用方式获取路径
    if [ -z "$SDK_PATH" ]; then
        export CROSS_TOP=/Applications/Xcode.app/Contents/Developer/Platforms/${PLATFORM}.platform/Developer
        export CROSS_SDK=${PLATFORM}${SDK_VER}.sdk
    else
        # 设置编译器环境变量
        export CROSS_SDK=$(basename "$SDK_PATH")
        export CROSS_TOP=$(dirname "$(dirname "$SDK_PATH")")
    fi

    # 设置编译器环境变量
    if [ "$ARCH" == "x86_64" ]; then
        # 关键：必须指定 -arch 和 -isysroot
        export CC="clang -arch ${ARCH} -isysroot ${SDK_PATH}"
        export CXX="clang++ -arch ${ARCH} -isysroot ${SDK_PATH}"
    else 
        export CC="clang"
        export CXX="clang++"
    fi


    # 清理之前的构建残留，确保干净编译
    make clean >/dev/null 2>&1 || true
    make distclean >/dev/null 2>&1 || true

    # 配置 OpenSSL
    ./Configure ${TARGET} \
        no-dso \
        no-hw \
        no-asm \
        --prefix=${OPENSSL_OUTPUT}/${ARCH}

    # 编译
    make -j$(sysctl -n hw.ncpu)

    # 安装到指定前缀
    make install_sw # 只安装软件部分，不安装文档等，加快速度

    # 拷贝库文件到最终目录
    cp -f ${OPENSSL_OUTPUT}/${ARCH}/lib/${OPENSSL_SSL_NAME} "${INSTALL_OPENSSL_LIB}/${ARCH}/${OPENSSL_SSL_NAME}"
    cp -f ${OPENSSL_OUTPUT}/${ARCH}/lib/${OPENSSL_CRYPTO_NAME} "${INSTALL_OPENSSL_LIB}/${ARCH}/${OPENSSL_CRYPTO_NAME}"

    # 设置 Install Name ID (@rpath 以便动态链接)
    install_name_tool -id @rpath/${OPENSSL_SSL_NAME} "${INSTALL_OPENSSL_LIB}/${ARCH}/${OPENSSL_SSL_NAME}"
    install_name_tool -id @rpath/${OPENSSL_CRYPTO_NAME} "${INSTALL_OPENSSL_LIB}/${ARCH}/${OPENSSL_CRYPTO_NAME}"

    # 验证架构
    echo "Verifying ${ARCH} library architecture:"
    lipo -info "${INSTALL_OPENSSL_LIB}/${ARCH}/${OPENSSL_CRYPTO_NAME}"

    unset CC
    unset CXX
}

# ================= 执行编译 =================
# A. 编译 Arm64 (用于真机和 M1/M2/M3 模拟器)
# 注意：ios64-cross 是 OpenSSL 3.x 推荐的真机目标
build_arch "arm64" "iPhoneOS" "$IOS_SDK" "ios64-cross" "$IOS_SDK_VERSION"

# B. 编译 x86_64 (用于 Intel Mac 上的模拟器)
# 注意：iossimulator-xcrun 是推荐的模拟器目标
build_arch "x86_64" "iPhoneSimulator" "$SIM_SDK" "iossimulator-x86_64-xcrun" "$SIMULATOR_SDK_VERSION"

# C. (可选) 编译 Arm64 模拟器专用
# 如果你希望将 M1 模拟器和真机分开处理，或者构建 XCFramework，可以单独编译
# build_arch "arm64" "iPhoneSimulator" "$SIM_SDK" "iossimulator-arm64-xcrun" "$SIMULATOR_SDK_VERSION"
# 注意：上面的 arm64 真机编译通常也可以直接在 M1 模拟器上运行，
# 但为了严谨的 XCFramework，通常建议真机和模拟器即使架构相同也分开目录存放。
# ================= 合并头文件 =================
# 通常 arm64 和 x86_64 的头文件是一样的，复制一份即可
cp -rf ${OPENSSL_OUTPUT}/arm64/include/openssl/* ${INSTALL_OPENSSL_INCLUDE}/

# ================= 可选：合并为 Fat Binary (如果需要) =================
echo "----------------------------------------"
echo "Creating Fat Binaries (Optional)..."
echo "----------------------------------------"

mkdir -p ${INSTALL_OPENSSL_LIB}/iOS

lipo -create \
    "${INSTALL_OPENSSL_LIB}/arm64/${OPENSSL_SSL_NAME}" \
    "${INSTALL_OPENSSL_LIB}/x86_64/${OPENSSL_SSL_NAME}" \
    -output "${INSTALL_OPENSSL_LIB}/iOS/${OPENSSL_SSL_NAME}"

lipo -create \
    "${INSTALL_OPENSSL_LIB}/arm64/${OPENSSL_CRYPTO_NAME}" \
    "${INSTALL_OPENSSL_LIB}/x86_64/${OPENSSL_CRYPTO_NAME}" \
    -output "${INSTALL_OPENSSL_LIB}/iOS/${OPENSSL_CRYPTO_NAME}"

echo "Fat Binary Info:"
lipo -info "${INSTALL_OPENSSL_LIB}/iOS/${OPENSSL_CRYPTO_NAME}"

echo "========================================"
echo "Build Complete!"
echo "Headers: ${INSTALL_OPENSSL_INCLUDE}"
echo "Arch-specific Libs: ${INSTALL_OPENSSL_LIB}/arm64 & x86_64"
echo "Fat Libs: ${INSTALL_OPENSSL_LIB}/iOS/"
echo "========================================"
