# Thread

> 多线程

多线程结构体
---------------
```c
/**/
struct __cc_thread {
    int32_t status;              //线程结束返回状态
    uint32_t thread_id;          //线程ID
    _cc_atomic32_t state;        //线程运行状态值
    _cc_thread_handle_t handle;  //线程句柄
};

/*线程回调函数定义*/
typedef int32_t (*_cc_thread_callback_t)(_cc_thread_t*, pvoid_t);
```

宏
---------------

| 宏 | 描述  |
| :--------------- |:----------------------- |
| [_CC_THREAD_PRIORITY_LOW_](#THREAD) | 优先权 低 |
| [_CC_THREAD_PRIORITY_NORMAL_](#THREAD) | 优先权 正常 |
| [_CC_THREAD_PRIORITY_HIGH_](#THREAD) | 优先权 高 |

链表函数
---------------

| 函数 | 描述  |
| :--------------- |:----------------------- |
| [_cc_create_thread](#THREAD) | 创建一个新线程返回 _cc_thread |
| [_cc_thread_priority](#THREAD) | 设置线程优先权 |
| [_cc_wait_thread](#THREAD) | 等待一个线程结束并释放资源 |
| [_cc_detach_thread](#THREAD) | 分离线程，线程结束之后自动释放资源 |
| [_cc_get_thread_id](#THREAD) | 获取当前线程ID |
| [_cc_thread_start](#THREAD) | 启动一个线程并分离线程 |

## THREAD
```c

int32_t fn_thread1(_cc_thread_t *t, pvoid_t args)
{
    printf("thread %s args:%d\n", t->name, (int32_t)*args);

    //等待1秒
    _cc_sleep(1000);

    return 100;
}

int32_t fn_thread2(_cc_thread_t *t, pvoid_t args)
{
    printf("thread %s args:%d\n", t->name, (int32_t)*args);

    //等待10秒
    _cc_sleep(10000);

    return 100;
}

int main (int argc, char * const argv[])
{
    int32_t arg1 = 1;
    int32_t arg2 = 2;
    int32_t st = 0;
    _cc_thread_t *t = NULL;

    _cc_thread_start(fn_thread1, "test 1" &arg1)

    t = _cc_create_thread(fn_thread2, "test 2", &arg2);
    _cc_wait_thread(t, &st);
    printf("thread 2 end, return:%d\n", st);


    printf("输入 `q` 退出:\n");
    while(getchar()!='q') {
        _cc_sleep(1);
    }
}
```