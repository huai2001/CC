# 入门：构建并运行第一个示例
***
本教程演示如何在本地构建 libcc 并运行一个最小示例：创建一个异步事件管理器，注册一个 1 秒后触发的计时器，触发后打印日志并退出。

## 先决条件
-   **macOS**（或 Linux/FreeBSD），命令行工具（Xcode Command Line Tools 或 GCC/Clang）。查看 [Build Tools](https://libcc.cn/docs/libcc/build-tools.md) 获取详细安装说明。
-   已安装 `git`、`make` 和常见编译工具链。
-   仓库已 clone 到本地（见下方“构建并安装”）。

## 构建并运行
***步骤概述：***
1. 先构建 libcc（使用仓库已有脚本）。
2. 构建示例程序。
3. 运行并观察输出。

### 构建 libcc 库
```bash
git clone https://github.com/libcc/libcc.git
cd libcc
# 编译静态库（适用于需要静态链接的场景）
make .a platform=linux debug=1
# 或使用构建脚本（推荐）
cd build
./build.sh debug
```

构建完成后，库文件将输出到 `bin/` 和 `lib/` 目录下。

### 构建示例程序
你有两种方式构建示例：
1. 将示例加入到仓库的 `tests/` 并使用仓库构建系统。
2. 手动编写 Makefile 或直接编译。

#### 方式1：使用仓库构建系统
将示例放到 `tests/test_getting_started.c`，然后使用仓库的 Makefile 构建：

```bash
make .bin target=tests build=test_getting_started.c
```

#### 方式2：手动构建
创建一个新的 C 文件 `getting_started.c`，内容见下方示例代码。

然后使用以下 Makefile 编译（需根据你的 `bin/` 输出目录和架构调整）：

```makefile
# 当前项目目录, 例子为:./getting_started.c
SRCROOT := $(abspath .)
# libcc 路径, 注意：修改为你自己的路径！
LIBCC_SRC_ROOT := /opt/libcc

# 生成的文件名，你可以随意起名
TARGET_NAME ?= getting_started

# Makefile 基础定义
include $(LIBCC_SRC_ROOT)/build/local-init.mk

# macOS 系统 包含 homebrew 路径，根据你自己需求选择
ifeq ($(PLATFORM), osx)
    INCLUDE_PATH    += /opt/homebrew/include
    LIBRARY_PATH    += /opt/homebrew/lib
endif

# 包含 libcc.so 动态库
LIBS += cc
# 包含 libcc 路径
INCLUDE_PATH    += $(LIBCC_SRC_ROOT)/include
LIBRARY_PATH    += $(LIBCC_SRC_ROOT)/bin/$(ARCH)/$(CONFIGURATION) $(LIBCC_SRC_ROOT)/lib/$(ARCH)/$(CONFIGURATION)

# 你项目的源文件
LOCAL_SRC_FILES += $(SRCROOT)/getting_started.c

# Makefile 构建脚本
include $(LIBCC_SRC_ROOT)/build/Makefile.mk
```

运行 `make` 构建。

***注意：***
-   在 macOS Apple Silicon 机器上，请使用对应的架构目录（例如 `bin/arm64/debug`）。
-   若找不到 libcc 动态库，请查看 `build/` 输出或使用 Xcode 打开 `proj.OSX/cc.xcodeproj` 来编译并调试。
-   确保 `LIBCC_SRC_ROOT` 路径正确指向 libcc 源码根目录。

## 示例代码
创建 `getting_started.c` 文件，内容如下：

```c
#include <stdio.h>
#include <libcc.h>

static bool_t timer_cb(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    if (which & _CC_EVENT_TIMEOUT_) {
        _cc_logger_info(_T("[event-loop] timeout ident=%d data=%ld"), e->ident, e->data);
        /* 停止事件循环 */
        async->running = false;
        return false;
    }
    return true;
}

int main(void) {
    _cc_async_event_t async;
    _cc_event_t *ev;

    /* 在平台上注册 poller（macOS 使用 kqueue，Linux 使用 epoll，Windows 使用 IOCP） */
    if (!_cc_register_poller(&async)) {
        fprintf(stderr, "register poller failed\n");
        return EXIT_FAILURE;
    }

    /* 添加 2 秒定时器 */
    ev = _cc_add_event_timeout(&async, 2000, timer_cb, 42);
    if (!ev) {
        fprintf(stderr, "failed to add timer\n");
        async.free(&async);
        return EXIT_FAILURE;
    }

    /* 事件循环 */
    while (async.running) {
        async.wait(&async, 500);
    }

    async.free(&async);
    return EXIT_SUCCESS;
}
```

## 运行示例
编译完成后，运行生成的可执行文件：

```bash
./getting_started
```

预期输出：
```
[event-loop] timeout ident=1 data=42
```

## 详细说明
-   `_cc_register_poller(&async)`：根据平台自动选择最优的 I/O 多路复用机制（epoll/kqueue/IOCP/select）。
-   `_cc_add_event_timeout()`：添加软定时器，基于时间轮算法实现高效定时。
-   事件循环 `async.wait()`：等待事件发生，超时时间为 500ms。
-   回调函数返回 `false` 时，定时器会被自动移除并释放。

## 注意事项
-   **内存管理**：libcc 使用手动内存管理，确保在程序退出前调用 `async.free()` 释放资源。
-   **线程安全**：单线程事件循环是线程安全的，多线程使用需额外同步。
-   **平台差异**：macOS 使用 kqueue，Linux 使用 epoll，Windows 使用 IOCP，确保编译时包含对应平台支持。
-   **调试**：使用 `_cc_logger_*` 函数记录日志，便于调试。
-   **错误处理**：始终检查 API 返回值，失败时使用 `_cc_last_error()` 获取错误信息。

完成并运行示例后，可继续查看：[事件循环](event-loop.md)、[TCP 服务端/客户端示例](event-tcp.md)、[HTTP 服务端](event-http.md) 等教程。
