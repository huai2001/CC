
#!/bin/bash

# macOS Framework打包脚本 - 将sqlite3和mysqlclient等库文件打包进Framework
# 使用方法: ./FrameworkBuilder.sh <源dylib路径> <Framework名称> <目标目录>

set -e  # 遇到错误立即退出
set -u  # 使用未定义的变量时报错

echo "=== macOS Framework打包工具 ==="
echo "开始构建包含依赖库的Framework..."

# 参数检查
if [ $# -ne 3 ]; then
    echo "错误: 参数数量不正确"
    echo "使用方法: $0 <源dylib路径> <Framework名称> <目标目录>"
    echo "示例: $0 ./libmylib.dylib MyFramework ./Build"
    exit 1
fi

SOURCE_DYLIB="$1"
FRAMEWORK_NAME="$2"
OUTPUT_DIR="$3"

# 检查源 dylib 是否存在
if [ ! -f "${SOURCE_DYLIB}" ]; then
    echo "错误: 源 dylib 不存在: ${SOURCE_DYLIB}"
    exit 1
fi

# 创建工作目录
WORK_DIR="${OUTPUT_DIR}/${FRAMEWORK_NAME}.framework"
VERSIONS_DIR="${WORK_DIR}/Versions"
CURRENT_DIR="${VERSIONS_DIR}/A"

# 创建Frameworks子目录用于存放依赖库
mkdir -p "${CURRENT_DIR}/Frameworks"

# 使用关联数组记录依赖关系（避免重复）
declare -A PROC_LIBS
declare -A LIB_DEPS

# 递归获取所有依赖库（包括依赖的依赖）
echo "分析依赖库..."

get_all_dependencies() {
    local DYLIB_PATH="$1"
    local DEPS=$(otool -L "$DYLIB_PATH" 2>/dev/null | tail -n +2 | awk '{print $1}')

    for DEP in $DEPS; do
        # 跳过系统库
        if [[ "$DEP" == /usr/lib/* ]] || [[ "$DEP" == /System/* ]]; then
            continue
        fi

        local LIB_NAME=$(basename "$DEP")

        # 跳过已处理的库
        if [[ -v "PROC_LIBS[$LIB_NAME]" ]]; then
            continue
        fi

        # 检查文件是否存在
        if [ ! -f "$DEP" ]; then
            echo "  警告: 依赖库不存在: $DEP"
            continue
        fi

        # 标记为已处理
        PROC_LIBS["$LIB_NAME"]=1
        LIB_DEPS["$LIB_NAME"]="$DEP"

        echo "  发现依赖: $LIB_NAME"
        # 递归获取该依赖的依赖
        get_all_dependencies "$DEP"
    done
}

# 从主 Framework 开始获取所有依赖
get_all_dependencies "${CURRENT_DIR}/${FRAMEWORK_NAME}"

# 第一步：拷贝所有依赖库
echo "拷贝依赖库 (${#PROC_LIBS[@]} 个)..."
for LIB_NAME in "${!LIB_DEPS[@]}"; do
    DEP="${LIB_DEPS[$LIB_NAME]}"
    DEST_PATH="${CURRENT_DIR}/Frameworks/$LIB_NAME"

    # 拷贝库文件
    echo "  拷贝: $LIB_NAME"
    cp -f "$DEP" "$DEST_PATH"

    # 设置依赖库自身的 install_name id
    install_name_tool -id "@loader_path/Frameworks/$LIB_NAME" "$DEST_PATH"
done

# 第二步：修复主 Framework 中对依赖库的引用
echo "修复主 Framework 的依赖引用..."
for LIB_NAME in "${!LIB_DEPS[@]}"; do
    DEP="${LIB_DEPS[$LIB_NAME]}"

    # 修改主 Framework 中对该依赖库的引用路径
    echo "  修复: $LIB_NAME"
    install_name_tool -change "$DEP" "@loader_path/Frameworks/$LIB_NAME" "${CURRENT_DIR}/${FRAMEWORK_NAME}"
done

# 第三步：修复依赖库之间的相互引用
echo "修复依赖库之间的相互引用..."
for LIB_NAME in "${!LIB_DEPS[@]}"; do
    DEST_PATH="${CURRENT_DIR}/Frameworks/$LIB_NAME"

    # 获取该依赖库的依赖
    SUB_DEPS=$(otool -L "$DEST_PATH" 2>/dev/null | tail -n +2 | awk '{print $1}')

    for SUB_DEP in $SUB_DEPS; do
        SUB_NAME=$(basename "$SUB_DEP")

        # 跳过自身和系统库
        if [[ "$LIB_NAME" == "$SUB_NAME" ]] || [[ "$SUB_DEP" == /usr/lib/* ]] || [[ "$SUB_DEP" == /System/* ]]; then
            continue
        fi

        # 跳过不在我们 Frameworks 目录中的子依赖
        if [[ ! -v "PROC_LIBS[$SUB_NAME]" ]]; then
            continue
        fi

        # 更新引用路径：使用 @loader_path/ (因为所有库都在 Frameworks/ 目录中)
        install_name_tool -change "$SUB_DEP" "@loader_path/$SUB_NAME" "$DEST_PATH"
        echo "  修复: $LIB_NAME -> $SUB_NAME"
    done
done

# 第四步：对整个 Framework 进行深度重新签名，确保所有内部库使用相同的签名
echo "重新签名整个 Framework..."
if [ -n "$CODE_SIGN_IDENTITY" ] && [ "$CODE_SIGN_IDENTITY" != "" ]; then
    echo "使用指定签名身份: $CODE_SIGN_IDENTITY"
    /usr/bin/codesign --force --deep --sign "$CODE_SIGN_IDENTITY" "${WORK_DIR}"
else
    # 如果没有签名身份，使用 ad-hoc 签名
    echo "使用 ad-hoc 签名"
    /usr/bin/codesign --force --deep --sign - "${WORK_DIR}"
fi

echo "打包完成! 共处理 ${#PROC_LIBS[@]} 个依赖库"
