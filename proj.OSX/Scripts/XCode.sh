
#!/bin/bash

# Xcode Run Script集成脚本
# 在Xcode Build Phases -> Run Script中添加执行

echo "=== Xcode Framework打包集成脚本 ==="

# Xcode环境变量

# 设置路径
# 对于 Framework target，实际的二进制文件在 Framework 内部的 Versions/A 目录下
FRAMEWORK_NAME="${EXECUTABLE_NAME}"
SOURCE_DYLIB="${BUILT_PRODUCTS_DIR}/${FRAMEWORK_NAME}.framework/Versions/A/${FRAMEWORK_NAME}"
FRAMEWORK_BUILDER="${SRCROOT}/Scripts/FrameworkBuilder.sh"
OUTPUT_DIR="${BUILT_PRODUCTS_DIR}"

echo "配置信息:"
echo "  配置: ${CONFIGURATION}"
echo "  Framework名称: ${FRAMEWORK_NAME}"
echo "  源dylib路径: ${SOURCE_DYLIB}"
echo "  输出路径: ${OUTPUT_DIR}"

# 检查打包脚本是否存在
if [ ! -f "${FRAMEWORK_BUILDER}" ]; then
    echo "错误: Framework打包脚本不存在: ${FRAMEWORK_BUILDER}"
    exit 1
fi

# 赋予执行权限
chmod +x "${FRAMEWORK_BUILDER}"

# 执行打包
echo "执行Framework打包..."
"${FRAMEWORK_BUILDER}" "${SOURCE_DYLIB}" "${FRAMEWORK_NAME}" "${OUTPUT_DIR}"

# 检查打包结果
FRAMEWORK_PATH="${OUTPUT_DIR}/${FRAMEWORK_NAME}.framework"
if [ -d "${FRAMEWORK_PATH}" ]; then
    echo "Framework打包成功完成!"
    echo "Framework路径: ${FRAMEWORK_PATH}"

    # 显示Framework结构
    echo "Framework目录结构:"
    find "${FRAMEWORK_PATH}" -type f | sort

    # 显示依赖信息
    echo "Framework依赖信息:"
    otool -L "${FRAMEWORK_PATH}/Versions/A/${FRAMEWORK_NAME}"
else
    echo "错误: Framework打包失败!"
    exit 1
fi

echo "Xcode集成脚本执行完成!"
