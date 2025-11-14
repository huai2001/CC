(The file `/opt/libcc/.github/copilot-instructions.md` exists, but is empty)
## libcc — 给 AI 代码代理的速成说明

本文件帮助 AI 代理快速在本仓库开展修改与调试工作：重点讲架构要点、构建/测试命令、项目约定和可参考的示例位置。

1) 大局与关键模块
	- 核心在 `src/`：事件子系统（`src/event/`）、平台轮询实现（`src/platform/*`）、通用工具（`src/generic/`、`src/crypto/` 等）。
	- 公共对外头文件在 `include/`，入口聚合头是 `include/libcc.h`（包含 `include/libcc/event.h` 等）。
	- 事件模型：使用 `_cc_async_event_t` 与 `_cc_event_t`，事件标志为 `_CC_EVENT_*`。查看 `include/libcc/event.h`、`src/event/event.c` 和 `src/event/tcp.c` 获取典型用法。

2) 构建与测试（最常用）
	- 标准：在 macOS/Linux/FreeBSD 使用顶层 Makefile 或 `build/build.sh`：
	  ```sh
	  make .a platform=linux debug=1
	  # 或
	  cd build && ./build.sh debug
	  ```
	- Windows: 使用 `build/build.cmd` 或 `proj.Win/` 下的 Visual Studio 工程。Android: `proj.Android/JNI` 下使用 NDK (`ndk-build`)。
	- 注意：源文件由 `build/libcc-files.mak` 列表控制，新增源文件必须将路径添加到对应 platform 条目以被编译。

3) 项目约定（重要，避免常见破坏）
	- 命名：库公共接口与类型以 `_cc_` 前缀；宏常以 `_CC_` 大写前缀。
	- 事件/网络：高层调用 `_cc_tcp_listen/_cc_tcp_connect/_cc_alloc_event`，实际 io 注册/等待由 `src/os/*`（如 `sys_epoll.c`,`sys_kqueue.c`,`sys_iocp.c`,`sys_io_uring.c`）负责。
	- 错误与日志：使用库内 `_cc_logger_*` 系列函数记录；测试用例和 README 演示了典型日志用法。

4) 修改与回归测试流程（实操步骤）
	- 新增公共 API：在 `include/` 添加头声明并在 `include/libcc.h` 或子头中引用。
	- 新增源文件：编辑 `build/libcc-files.mak`（对应平台分支），然后执行 `make` 或 `build/build.sh` 验证编译通过。
	- 运行示例/测试：`tests/` 目录（例如 `tests/test_event.c`, `tests/test_tcp_client.c`）包含启动事件循环、listen/connect 的最小示例，用于快速回归验证。

5) 具体文件举例（便于定位）
	- API：`include/libcc/event.h`, `include/libcc/socket.h`。
	- 事件实现：`src/event/event.c`, `src/event/loop.c`, `src/event/buffer.c`, `src/event/tcp.c`。
	- 平台轮询：`src/os/linux/sys_epoll.c`, `src/os/unix/sys_kqueue.c`, `src/os/linux/sys_io_uring.c`。
	- 构建清单：`build/libcc-files.mak`。
	- 测试：`tests/test_event.c`、`tests/test_tcp_client.c`。

6) 限制与注意事项（AI 代理应优先考虑）
	- 不要随意改动 `build/libcc-files.mak` 以外的构建逻辑：若新增源未加入此清单，编译不会包含它们。
	- 事件对象生命周期复杂：优先参考 `src/event/event.c` 的分配/释放逻辑，保持 `ident` 与 async 对应关系不变。
	- 平台适配时同时更新对应的 `src/os/*` 实现（epoll/kqueue/iocp/io_uring），并在 `build/libcc-files.mak` 中确保该平台分支包含新文件。

7) 我能继续做的事
	- 将本说明进一步细化为 PR 模板或 CI 步骤；或把仓库中更具体的约定（比如格式化工具、CI 配置）合并进来。请指示需要的下一步。

若要我调整说明语气、添加更多示例代码片段或合并公司内部规范，请告诉我具体要点，我会迭代更新。

