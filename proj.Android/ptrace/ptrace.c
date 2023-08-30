#include <android/log.h>
#include <asm/ptrace.h>
#include <asm/user.h>
#include <dirent.h>
#include <dlfcn.h>
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(__i386__)
#define pt_regs user_regs_struct
#endif
#define LOG_TAG "INJECT"
#define LOGD(fmt, args...) \
    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define DEBUG_PRINT(format, args...) LOGD(format, ##args)

#define CPSR_T_MASK (1u << 5)

//挂载到目标进程
int ptrace_attach(pid_t pid) {
    if (ptrace(PTRACE_ATTACH, pid, NULL, 0) < 0) {
        perror("ptrace_attach");
        return -1;
    }

    int status = 0;
    waitpid(pid, &status, WUNTRACED);

    return 0;
}

//从目标进程中卸载
int ptrace_detach(pid_t pid) {
    if (ptrace(PTRACE_DETACH, pid, NULL, 0) < 0) {
        perror("ptrace_detach");
        return -1;
    }

    return 0;
}

//读取进程寄存器数据
int ptrace_getregs(pid_t pid, struct pt_regs* regs) {
    if (ptrace(PTRACE_GETREGS, pid, NULL, regs) < 0) {
        perror("ptrace_getregs: Can not get register values");
        return -1;
    }

    return 0;
}

//设置进程寄存器
int ptrace_setregs(pid_t pid, struct pt_regs* regs) {
    if (ptrace(PTRACE_SETREGS, pid, NULL, regs) < 0) {
        perror("ptrace_setregs: Can not set register values");
        return -1;
    }

    return 0;
}

//进程继续执行函数
int ptrace_continue(pid_t pid) {
    if (ptrace(PTRACE_CONT, pid, NULL, 0) < 0) {
        perror("ptrace_cont");
        return -1;
    }

    return 0;
}

//获得函数返回值
long ptrace_retval(struct pt_regs* regs) {
#ifdef __arm__
    return regs->ARM_r0;
#elif defined(__i386__)
    return regs->eax;
#else
#error "Not supported"
#endif
}

//获得PC执行地址
long ptrace_ip(struct pt_regs* regs) {
#ifdef __arm__
    return regs->ARM_pc;
#elif defined(__i386__)
    return regs->eip;
#else
#error "Not supported"
#endif
}

/**
 * 读取目标进程数据函数
 *
 * 参数一：目标进程id
 * 参数二：目标进程读取的地址
 * 参数二：buf用于保存读出的结果
 * 参数四：读取的大小
 *
 * 这里是每次读取4个字节，最后在if判断中读取剩下的字节。
 * 这里之所以使用union 是因为ptrace这时候返回的是long，使用union就省去了转化。
 */
//
int ptrace_readdata(pid_t pid, uint8_t* src, uint8_t* buf, size_t size) {
    uint32_t i, j, remain;
    uint8_t* laddr;

    union u {
        long val;
        char chars[sizeof(long)];
    } d;

    j = size / 4;
    remain = size % 4;

    laddr = buf;

    for (i = 0; i < j; i++) {
        //拷贝src指向的数据
        d.val = ptrace(PTRACE_PEEKTEXT, pid, src, 0);
        memcpy(laddr, d.chars, 4);
        src += 4;
        laddr += 4;
    }

    if (remain > 0) {
        d.val = ptrace(PTRACE_PEEKTEXT, pid, src, 0);
        memcpy(laddr, d.chars, remain);
    }

    return 0;
}
/**
 * 写入目标进程数据函数
 *
 * 参数一：目标进程id
 * 参数二：目标进程写入的地址
 * 参数二：data写入数据
 * 参数四：写入的大小
 *
 * 这个和上面读取的操作是类似的*/

int ptrace_writedata(pid_t pid, uint8_t* dest, uint8_t* data, size_t size) {
    uint32_t i, j, remain;
    uint8_t* laddr;

    union u {
        long val;
        char chars[sizeof(long)];
    } d;

    j = size / 4;
    remain = size % 4;

    laddr = data;

    for (i = 0; i < j; i++) {
        memcpy(d.chars, laddr, 4);
        ptrace(PTRACE_POKETEXT, pid, dest, d.val);

        dest += 4;
        laddr += 4;
    }

    if (remain > 0) {
        for (i = 0; i < remain; i++) {
            d.chars[i] = *laddr++;
        }

        ptrace(PTRACE_POKETEXT, pid, dest, d.val);
    }

    return 0;
}

/**
 * 调用目标进程指定地址函数
 *
 * 参数一：目标进程id
 * 参数二：函数地址
 * 参数三：参数数组
 * 参数四：参数数量
 * 参数五：寄存器结构体
 *
 * 这里有一点需要注意，在ARM架构下有ARM和Thumb两种指令，因此在调用函数前需要判断函数被解析成哪种指令，
 * 上面代码就是通过地址的最低位是否为1来判断调用地址处指令为ARM或Thumb，若为Thumb指令，则需要将最低位重新设置为0，
 * 并且将CPSR寄存器的T标志位置位，若为ARM指令，则将CPSR寄存器的T标志位复位。
 */
#ifdef __arm__
int ptrace_call(pid_t pid,
                uint32_t addr,
                long* params,
                uint32_t num_params,
                struct pt_regs* regs) {
    uint32_t i;
    int stat = 0;
    //前4个参数放入寄存器
    for (i = 0; i < num_params && i < 4; i++) {
        regs->uregs[i] = params[i];
    }

    //后面的参数从右往左依次入栈
    if (i < num_params) {
        //栈空间大小
        regs->ARM_sp -= (num_params - i) * sizeof(long);
        //写入栈中
        ptrace_writedata(pid, (void*)regs->ARM_sp, (uint8_t*)&params[i],
                         (num_params - i) * sizeof(long));
    }

    regs->ARM_pc = addr;
    if (regs->ARM_pc & 1) {
        /* thumb */
        regs->ARM_pc &= (~1u);
        regs->ARM_cpsr |= CPSR_T_MASK;
    } else {
        /* arm */
        regs->ARM_cpsr &= ~CPSR_T_MASK;
    }

    //那么如何notify进程我们mmp执行完了。就是通过下面这句话。
    //原因是当函数调用时候，当我们使用bl或者bx，链接寄存器指向的是下一条返回地址，
    //如果把下条返回地址赋值成0，返回时候pc=0，就会产生异常。相当于一个notify，
    //然后用下面那个waitpid得到异常模式，确定mmp执行完。所以其实下面不一定是0，只要是无效即可。
    regs->ARM_lr = 0;

    if (ptrace_setregs(pid, regs) == -1 || ptrace_continue(pid) == -1) {
        DEBUG_PRINT("error\n");
        return -1;
    }

    waitpid(pid, &stat, WUNTRACED);
    while (stat != 0xb7f) {
        if (ptrace_continue(pid) == -1) {
            DEBUG_PRINT("error\n");
            return -1;
        }
        waitpid(pid, &stat, WUNTRACED);
    }

    return 0;
}
#elif defined(__i386__)
long ptrace_call(pid_t pid,
                 uint32_t addr,
                 long* params,
                 uint32_t num_params,
                 struct user_regs_struct* regs) {
    int stat = 0;
    long tmp_addr = 0x00;
    regs->esp -= (num_params) * sizeof(long);
    ptrace_writedata(pid, (void*)regs->esp, (uint8_t*)params,
                     (num_params) * sizeof(long));

    regs->esp -= sizeof(long);
    ptrace_writedata(pid, regs->esp, (char*)&tmp_addr, sizeof(tmp_addr));
    regs->eip = addr;
    if (ptrace_setregs(pid, regs) == -1 || ptrace_continue(pid) == -1) {
        DEBUG_PRINT("error\n");
        return -1;
    }
    waitpid(pid, &stat, WUNTRACED);
    while (stat != 0xb7f) {
        if (ptrace_continue(pid) == -1) {
            DEBUG_PRINT("error\n");
            return -1;
        }
        waitpid(pid, &stat, WUNTRACED);
    }
    return 0;
}
#else
#error "Not supported"
#endif
/**
 *
 * 再次封装得到返回值代码如下
 * */
int ptrace_call_wrapper(pid_t target_pid,
                        const char* func_name,
                        void* func_addr,
                        long* parameters,
                        int param_num,
                        struct pt_regs* regs) {
    DEBUG_PRINT("[+] Calling %s in target process.\n", func_name);
    if (ptrace_call(target_pid, (uint32_t)func_addr, parameters, param_num,
                    regs) == -1)
        return -1;

    if (ptrace_getregs(target_pid, regs) == -1)
        return -1;
    DEBUG_PRINT(
        "[+] Target process returned from %s, return value=%x, pc=%x \n",
        func_name, ptrace_retval(regs), ptrace_ip(regs));
    return 0;
}

int find_pid_of(const char* process_name) {
    int id;
    pid_t pid;
    FILE* fp;
    DIR* dir;
    char filename[32];
    char cmdline[256];

    struct dirent* entry;
    if (process_name == NULL) {
        return -1;
    }

    dir = opendir("/proc");
    if (dir == NULL) {
        return -1;
    }

    pid = -1;

    while ((entry = readdir(dir)) != NULL) {
        id = atoi(entry->d_name);
        if (id != 0) {
            snprintf(filename, "/proc/%d/cmdline", id);
            fp = fopen(filename, "r");
            if (fp) {
                fgets(cmdline, sizeof(cmdline), fp);
                fclose(fp);
                if (strcmp(process_name, cmdline) == 0) {
                    pid = id;
                    break;
                }
            }
        }
    }
    closedir(dir);
    return pid;
}

/**
 * 获取目标进程模块基址
 *
 * */
void* get_module_base(pid_t pid, const char* module_name) {
    FILE* fp;
    long addr = 0;
    char* pch;
    char filename[32];
    char line[1024];

    if (pid < 0) {
        /* self process */
        _tcsncpy(filename, "/proc/self/maps", 32);
        filename[31] = 0;
    } else {
        snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    }

    fp = fopen(filename, "r");

    if (fp != NULL) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, module_name)) {
                pch = strtok(line, "-");
                //转成16进制
                addr = strtoul(pch, NULL, 16);

                if (addr == 0x8000)
                    addr = 0;

                break;
            }
        }

        fclose(fp);
    }

    return (void*)addr;
}

const char* libc_path = "/system/lib/libc.so";
const char* linker_path = "/system/bin/linker";

int inject_remote_process(pid_t target_pid,
                          const char* library_path,
                          const char* function_name,
                          const char* param,
                          size_t param_size) {
    int ret = -1;
    void *mmap_addr, *dlopen_addr, *dlsym_addr, *dlclose_addr, *dlerror_addr;
    void *local_handle, *remote_handle, *dlhandle;
    uint8_t* map_base = 0;
    uint8_t *dlopen_param1_ptr, *dlsym_param2_ptr, *saved_r0_pc_ptr,
        *inject_param_ptr, *remote_code_ptr, *local_code_ptr;

    struct pt_regs regs, original_regs;
    extern uint32_t _dlopen_addr_s, _dlopen_param1_s, _dlopen_param2_s,
        _dlsym_addr_s, \  
        _dlsym_param2_s,
        _dlclose_addr_s, _inject_start_s, _inject_end_s,
        _inject_function_param_s, \  
        _saved_cpsr_s,
        _saved_r0_pc_s;

    uint32_t code_length;
    long parameters[10];

    DEBUG_PRINT("[+] Injecting process: %d\n", target_pid);

    if (ptrace_attach(target_pid) == -1)
        goto exit;

    if (ptrace_getregs(target_pid, &regs) == -1)
        goto exit;

    /* save original registers */
    memcpy(&original_regs, &regs, sizeof(regs));

    mmap_addr = get_remote_addr(target_pid, libc_path, (void*)mmap);
    DEBUG_PRINT("[+] Remote mmap address: %x\n", mmap_addr);

    /* call mmap */
    parameters[0] = 0;                                   // addr
    parameters[1] = 0x4000;                              // size
    parameters[2] = PROT_READ | PROT_WRITE | PROT_EXEC;  // prot
    parameters[3] = MAP_ANONYMOUS | MAP_PRIVATE;         // flags
    parameters[4] = 0;                                   // fd
    parameters[5] = 0;                                   // offset

    if (ptrace_call_wrapper(target_pid, "mmap", mmap_addr, parameters, 6,
                            &regs) == -1)
        goto exit;

    map_base = ptrace_retval(&regs);

    dlopen_addr = get_remote_addr(target_pid, linker_path, (void*)dlopen);
    dlsym_addr = get_remote_addr(target_pid, linker_path, (void*)dlsym);
    dlclose_addr = get_remote_addr(target_pid, linker_path, (void*)dlclose);
    dlerror_addr = get_remote_addr(target_pid, linker_path, (void*)dlerror);

    DEBUG_PRINT(
        "[+] Get imports: dlopen: %x, dlsym: %x, dlclose: %x, dlerror: %x\n",
        dlopen_addr, dlsym_addr, dlclose_addr, dlerror_addr);

    DEBUG_PRINT("library path = %s\n", library_path);
    ptrace_writedata(target_pid, map_base, library_path,
                     strlen(library_path) + 1);

    parameters[0] = map_base;
    parameters[1] = RTLD_NOW | RTLD_GLOBAL;

    if (ptrace_call_wrapper(target_pid, "dlopen", dlopen_addr, parameters, 2,
                            &regs) == -1)
        goto exit;

    void* sohandle = ptrace_retval(&regs);

#define FUNCTION_NAME_ADDR_OFFSET 0x100
    ptrace_writedata(target_pid, map_base + FUNCTION_NAME_ADDR_OFFSET,
                     function_name, strlen(function_name) + 1);
    parameters[0] = sohandle;
    parameters[1] = map_base + FUNCTION_NAME_ADDR_OFFSET;

    if (ptrace_call_wrapper(target_pid, "dlsym", dlsym_addr, parameters, 2,
                            &regs) == -1)
        goto exit;

    void* hook_entry_addr = ptrace_retval(&regs);
    DEBUG_PRINT("hook_entry_addr = %p\n", hook_entry_addr);

#define FUNCTION_PARAM_ADDR_OFFSET 0x200
    ptrace_writedata(target_pid, map_base + FUNCTION_PARAM_ADDR_OFFSET, param,
                     strlen(param) + 1);
    parameters[0] = map_base + FUNCTION_PARAM_ADDR_OFFSET;

    if (ptrace_call_wrapper(target_pid, "hook_entry", hook_entry_addr,
                            parameters, 1, &regs) == -1)
        goto exit;

    DEBUG_PRINT("Press enter to dlclose and detach\n");
    getchar();
    parameters[0] = sohandle;

    if (ptrace_call_wrapper(target_pid, "dlclose", dlclose, parameters, 1,
                            &regs) == -1)
        goto exit;

    /* restore */
    ptrace_setregs(target_pid, &original_regs);
    ptrace_detach(target_pid);
    ret = 0;

exit:
    return ret;
}

int main(int argc, char** argv) {
    pid_t target_pid;
    target_pid = find_pid_of("com.tencent.mm");
    if (-1 == target_pid) {
        printf("Can't find the process\n");
        return -1;
    }
    // target_pid = find_pid_of("/data/test");
    inject_remote_process(target_pid, "/data/libhello.so", "hook_entry",
                          "I'm parameter!", strlen("I'm parameter!"));
    return 0;
}