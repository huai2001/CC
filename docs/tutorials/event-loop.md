# 事件循环与定时器
***
本教程介绍 libcc 的事件模型与时间轮计时器，包含示例：创建事件管理器、添加定时器与处理回调。

## 核心概念
-   **`_cc_async_event_t`**：事件管理器，封装平台轮询（epoll/kqueue/iocp/select），管理事件列表与时间轮。
-   **`_cc_event_t`**：具体事件（socket、定时器、文件等），包含 flags、回调与超时时间。
-   **时间轮**：高效的软定时器实现，使用 `_cc_add_event_timeout`/`_cc_kill_event_timeout` 等 API，支持高并发场景下的定时任务调度。

## 时间轮机制详解
libcc 使用时间轮算法实现定时器，具有以下特点：
-   **高效性**：O(1) 时间复杂度添加和删除定时器。
-   **精度**：最小精度为 10ms，受系统调度影响。
-   **多级轮**：支持多级时间轮，处理不同时间范围的定时器。
-   **线程安全**：在单线程事件循环中安全，多线程需额外同步。

## 示例代码（`tests/test_event.c`）

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
    if (!_cc_register_kqueue(&async)) {
        fprintf(stderr, "register kqueue failed\n");
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

## 详细说明
-   **注册 Poller**：`_cc_register_*` 函数根据平台选择最优 I/O 多路复用机制。
-   **添加定时器**：`_cc_add_event_timeout` 返回事件指针，失败时返回 NULL。
-   **事件循环**：`async.wait()` 阻塞等待事件，超时返回继续循环。
-   **回调处理**：返回 `true` 保持定时器，返回 `false` 移除定时器。
-   **资源清理**：调用 `async.free()` 释放所有资源。

## 高级用法
### 周期性定时器
```c
static bool_t periodic_timer(_cc_async_event_t *async, _cc_event_t *e, const uint32_t which) {
    if (which & _CC_EVENT_TIMEOUT_) {
        _cc_logger_info(_T("Periodic timer triggered"));
        // 返回 true 继续定时
        return true;
    }
    return false;
}

// 添加每秒触发的周期性定时器
_cc_add_event_timeout(&async, 1000, periodic_timer, 0);
```

### 移除定时器
```c
_cc_event_t *timer = _cc_add_event_timeout(&async, 5000, callback, data);
// 手动移除
_cc_kill_event_timeout(&async, timer);
```

## 注意事项
-   **回调函数**：避免在回调中执行耗时操作，以免阻塞事件循环。
-   **定时精度**：软定时器精度受系统负载影响，不适用于高精度计时。
-   **内存管理**：定时器由事件管理器管理，无需手动释放（除非提前移除）。
-   **多线程**：事件循环应在单线程中运行，多线程共享事件管理器需加锁。
-   **平台支持**：确保编译时包含对应平台的轮询实现（epoll/kqueue/iocp）。
-   **调试**：使用 `_cc_logger_*` 记录定时器事件，便于排查问题。
-   **资源限制**：大量定时器可能消耗内存，监控系统资源使用。