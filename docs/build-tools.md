# 构建工具与环境设置
***
本指南介绍如何设置用于构建和运行 libcc 的开发环境，以及开发新应用程序的详细步骤。libcc 支持多种平台和构建方式。

## 开发环境要求

### 基本工具
-   **Git**：[https://git-scm.com](https://git-scm.com) - 版本控制和源码获取
-   **GCC/Clang**：C/C++ 编译器
-   **GNU Make**：[http://www.gnu.org/software/make/](http://www.gnu.org/software/make/) - 构建系统
-   **CMake**（可选）：跨平台构建工具

### 平台特定要求
-   **macOS**：Xcode Command Line Tools 或 Homebrew
-   **Linux**：GCC、Make、开发库
-   **Windows**：MSYS2、MinGW-w64 或 Visual Studio
-   **FreeBSD**：Clang、Make、开发库

## 平台安装指南

### macOS 安装
1. **安装 Homebrew**（如果没有）：
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

2. **安装基础工具**：
```bash
brew install git gcc make cmake
```

3. **安装 libcc 依赖库**：
```bash
brew install sqlite mysql-client openssl
```

4. **验证安装**：
```bash
gcc --version
make --version
git --version
```

### Linux 安装（Ubuntu/Debian）
```bash
# 更新包管理器
sudo apt update

# 安装基础工具
sudo apt install -y git gcc make cmake

# 安装开发库
sudo apt install -y libsqlite3-dev libmysqlclient-dev libssl-dev

# 验证安装
gcc --version
make --version
```

### Linux 安装（CentOS/RHEL）
```bash
# 安装基础工具
sudo yum install -y git gcc make cmake

# 安装开发库
sudo yum install -y sqlite-devel mysql-devel openssl-devel

# 验证安装
gcc --version
make --version
```

### FreeBSD 安装
```bash
# 更新包管理器
sudo pkg update

# 安装基础工具
sudo pkg install -y git gcc make cmake

# 安装开发库
sudo pkg install -y sqlite3 mysql80-client openssl

# 验证安装
gcc --version
make --version
```

### Windows MSYS2 安装
1. **下载并安装 MSYS2**：[https://www.msys2.org/](https://www.msys2.org/)

2. **更新包数据库**：
```bash
pacman -Syu
```

3. **安装工具链**：
```bash
# 64位版本
pacman -S --needed git base-devel mingw-w64-x86_64-toolchain

# 或 32位版本
pacman -S --needed git base-devel mingw-w64-i686-toolchain
```

4. **安装依赖库**：
```bash
pacman -S --needed mingw-w64-x86_64-sqlite3 mingw-w64-x86_64-mysql mingw-w64-x86_64-openssl
```

5. **启动 MSYS2 环境**：
   - 使用 "MSYS2 MinGW 64-bit" 或 "MSYS2 MinGW 32-bit" 快捷方式

### Visual Studio Code 设置
1. **下载安装**：[https://code.visualstudio.com/Download](https://code.visualstudio.com/Download)

2. **安装中文语言包**：
   - 搜索 "Chinese" 并安装简体中文包

3. **推荐扩展**：
   - C/C++ (Microsoft)
   - CMake Tools
   - GitLens
   - Code Runner

## libcc 构建指南

### 获取源码
```bash
git clone https://github.com/libcc/libcc.git
cd libcc
```

### 构建选项
libcc 支持多种构建方式，根据需求选择：

#### 1. Makefile 构建（推荐）
```bash
# 构建静态库
make .a platform=linux debug=1

# 构建动态库
make .so platform=linux debug=1

# 构建测试程序
make .bin target=tests build=test_event.c

# 自定义构建
make .a platform=linux debug=1 USE_LIB_MYSQL=1 USE_LIB_OPENSSL=1
```

**平台参数**：
- `platform=linux` - Linux 系统
- `platform=freebsd` - FreeBSD 系统
- `platform=osx` - macOS 系统
- `platform=windows` - Windows 系统

**编译选项**：
- `debug=1` - 调试版本（默认）
- `release=1` - 发布版本

**功能模块**：
- `USE_LIB_MYSQL=1` - 启用 MySQL 支持
- `USE_LIB_SQLITE3=1` - 启用 SQLite 支持
- `USE_LIB_OPENSSL=1` - 启用 OpenSSL 支持
- `USE_LIB_SQLSERVER=1` - 启用 SQL Server 支持

#### 2. 脚本构建
```bash
# 调试版本
cd build && ./build.sh debug

# 发布版本
cd build && ./build.sh release
```

#### 3. Visual Studio 构建（Windows）
1. 打开 `proj.Win/libcc.vcxproj`
2. 选择配置（Debug/Release）
3. 构建解决方案

#### 4. Xcode 构建（macOS/iOS）
1. 打开 `proj.OSX/cc.xcodeproj` 或 `proj.IOS/cc.xcodeproj`
2. 选择目标和配置
3. 构建

#### 5. Android NDK 构建
```bash
cd proj.Android/JNI
ndk-build NDK_DEBUG=1
```

### 构建输出
构建完成后，库文件位于：
- **静态库**：`lib/[arch]/[config]/libcc.a`
- **动态库**：`bin/[arch]/[config]/libcc.so` (Linux/macOS) 或 `libcc.dll` (Windows)
- **头文件**：`include/libcc.h` 及其子头文件

## 开发新应用程序

### 项目结构
```
my_project/
├── src/
│   ├── main.c
│   └── other.c
├── include/
│   └── my_app.h
├── build/
│   └── Makefile
└── README.md
```

### 示例 Makefile
```makefile
# 项目根目录
SRCROOT := $(abspath .)
# libcc 源码路径（修改为实际路径）
LIBCC_SRC_ROOT := /path/to/libcc

# 目标名称
TARGET_NAME := my_app

# 包含 libcc 构建配置
include $(LIBCC_SRC_ROOT)/build/local-init.mk

# 平台特定设置
ifeq ($(PLATFORM), osx)
    INCLUDE_PATH += /opt/homebrew/include
    LIBRARY_PATH += /opt/homebrew/lib
endif

# libcc 库
LIBS += cc
INCLUDE_PATH += $(LIBCC_SRC_ROOT)/include
LIBRARY_PATH += $(LIBCC_SRC_ROOT)/bin/$(ARCH)/$(CONFIGURATION)
LIBRARY_PATH += $(LIBCC_SRC_ROOT)/lib/$(ARCH)/$(CONFIGURATION)

# 项目源文件
LOCAL_SRC_FILES += $(SRCROOT)/src/main.c
LOCAL_SRC_FILES += $(SRCROOT)/src/other.c

# 包含 libcc 构建规则
include $(LIBCC_SRC_ROOT)/build/Makefile.mk
```

### 编译运行
```bash
# 构建
make

# 运行
./my_app
```

## 故障排除

### 常见问题
1. **编译错误**：检查 GCC/Clang 版本，确保 >= 4.8
2. **库依赖**：确认已安装所有必需的开发库
3. **路径问题**：检查 `LIBCC_SRC_ROOT` 路径是否正确
4. **权限问题**：确保对构建目录有写权限

### 调试技巧
- 使用 `make clean` 清理构建文件
- 检查 `build/build.log` 获取详细错误信息
- 使用 `gcc -v` 验证编译器配置
- 在代码中添加 `_cc_logger_debug()` 进行调试

### 获取帮助
- 查看 `README.md` 获取项目概述
- 检查 `docs/` 目录的文档
- 提交 Issue 到 GitHub 项目页面

## 高级配置

### 自定义构建
修改 `build/local-files.mk` 添加新源文件：
```
# Linux 平台源文件
linux_src_files += src/my_module.c
```

### 交叉编译
```bash
# 交叉编译到 ARM
make .a platform=linux ARCH=arm64
```

### 集成 CI/CD
libcc 支持多种 CI 系统，参考项目根目录的 CI 配置文件。

## 许可证
libcc 遵循开源许可证，构建的应用程序需遵守相应许可证条款。
