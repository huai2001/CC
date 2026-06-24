#include <libcc.h>
#include <sys/statvfs.h>

#define OVERWRITE_PASSES 7
// 4MB缓冲区，平衡内存占用与I/O效率
#define BUFFER_SIZE (4 * 1024 * 1024)
#define ZERO_PATTERN 0x00
#define ONE_PATTERN 0xFF

// 定义覆写模式序列：3次0，3次1，1次随机
enum {
    PASS_ZERO = 0,
    PASS_ONE,
    PASS_RANDOM
};
// 覆写模式序列数组，对应DoD 5220.22-M C级标准
const byte_t overwrite_sequence[OVERWRITE_PASSES] = {
    PASS_ZERO, PASS_ZERO, PASS_ZERO,  // 前三次：全0
    PASS_ONE, PASS_ONE, PASS_ONE,     // 中三次：全1
    PASS_RANDOM                       // 最后一次：随机
};

// 执行单次覆写操作
int perform_single_pass(int fd, size_t file_size, byte_t pattern) {
    char *buffer = (char*)_cc_malloc(BUFFER_SIZE);
    // 根据模式初始化缓冲区
    if (pattern == PASS_ZERO) {
        memset(buffer, ZERO_PATTERN, BUFFER_SIZE);
    } else if (pattern == PASS_ONE) {
        memset(buffer, ONE_PATTERN, BUFFER_SIZE);
    } else if (pattern == PASS_RANDOM) {
        _cc_random_bytes((byte_t*)buffer, BUFFER_SIZE);
    } else {
        _cc_logger(_CC_LOG_LEVEL_ERROR_, "Unknown overwrite mode.");
        _cc_free(buffer);
        return -1;
    }

    // 从文件开头开始，逐块写入
    off_t offset = 0;
    while (offset < file_size) {
        size_t write_size = (file_size - offset) < BUFFER_SIZE ? (file_size - offset) : BUFFER_SIZE;
        ssize_t written = write(fd, buffer, write_size);
        if (written == -1) {
            _cc_logger(_CC_LOG_LEVEL_ERROR_, "write file fail.");
            _cc_free(buffer);
            return -1;
        }
        if (written < (ssize_t)write_size) {
            _cc_logger_error("The number of written bytes is insufficient. Expected %zu, but actual %zd.", write_size, written);
            // 继续尝试写入剩余部分
            offset += written;
            continue;
        }
        //print_progress(offset, file_size);
        offset += written;
    }

    // 刷新写入缓冲区，确保数据落盘
    if (fsync(fd) == -1) {
        _cc_logger(_CC_LOG_LEVEL_ERROR_, "fsync fail.");
        _cc_free(buffer);
        return -1;
    }

    _cc_free(buffer);
    return 0;
}

// 执行完整的DoD 5220.22-M七次覆写
static int dod_5220_22m_wipe(int fd, size_t file_size) {
    int i;
    //printf("开始执行DoD 5220.22-M七次覆写擦除...\n");
    for (i = 0; i < OVERWRITE_PASSES; i++) {
        //const char *pattern_names[] = {"全0", "全1", "随机"};
        //const char *pass_name = (i < 3) ? pattern_names[0] : (i < 6) ? pattern_names[1] : pattern_names[2];
        //printf("第 %d 次覆写: %s\n", i + 1, pass_name);
        if (perform_single_pass(fd, file_size, overwrite_sequence[i]) != 0) {
            _cc_logger_error("The overwrite %d failed. The erasure process has been terminated.", i + 1);
            return -1;
        }
        //printf("第 %d 次覆写完成。\n", i + 1);
    }
    //printf("DoD 5220.22-M七次覆写擦除成功完成。\n");
    return 0;
}

static void rename_unlink(const tchar_t *path) {
    //Generate a single-character new file name to prevent the file name from being recognized by recovery tools.
    char new_name[2] = {path[rand() % _tcslen(path)], '\0'};
    if (rename(path, new_name) == 0) {
        _cc_unlink(new_name);
    } else {
        _cc_unlink(path);
    }
}

static int dod_5220_22m_unlink(const tchar_t *path) {
    struct stat st;
    
    // 打开文件，以读写模式
    int fd = open(path, O_RDWR);
    if (fd == -1) {
        _cc_logger_error("open fail: %s", path);
        return -1;
    }

    // 获取文件大小
    if (fstat(fd, &st) == -1) {
        _cc_logger_error("fstat fail: %s", path);
        close(fd);
        return -1;
    }

    // 检查文件是否为空
    if (st.st_size == 0) {
        close(fd);
        rename_unlink(path);
        return -1;
    }

    // 执行七次覆写
    if (dod_5220_22m_wipe(fd, st.st_size) != 0) {
        _cc_logger_error("dod_5220_22m_wipe fail: %s", path);
        close(fd);
        return -1;
    }
    
    // 关闭文件描述符
    close(fd);
    return 0;
}

int rm(const tchar_t* root) {
    tchar_t path[_CC_MAX_PATH_] = { 0 };
    DIR* dir = opendir(root);
    struct dirent* d = NULL;
    int result = 0;
    size_t length = 0;

    if (dir == NULL) {
        return -1;
    }
    
    length = _tcslen(root);
    memcpy(path, root, length * sizeof(tchar_t));
    path[length] = 0;
    
    while ((d = readdir(dir)) != NULL) {
        if ((d->d_name[0] == '.' && d->d_name[1] == 0) ||
            (d->d_name[0] == '.' && d->d_name[1] == '.' && d->d_name[2] == 0)) {
            continue;
        }
        
        path[length] = _CC_SLASH_C_;
        path[length + 1] = 0;
        _tcscat(path + length, d->d_name);

        if (d->d_type == DT_DIR) {
            if (rm(path) != 0) {
                result = -1;
            }
        } else {
            if (dod_5220_22m_unlink(path) != 0) {
                _tprintf(_T("Failed to remove file: %s\n"), path);
                result = -1;
            } else {
                _tprintf(_T("Removed file: %s\n"), path);
            }
        }
    }
    
    closedir(dir);

    if (_rmdir(root) != 0) {
        _tprintf(_T("Failed to remove directory: %s\n"), root);
        return -1;
    } else {
        _tprintf(_T("Removed directory: %s\n"), root);
    }

    return result;
}

// 获取磁盘空闲空间并覆写
int free_disk(const char* mount) {
    int fd;
    struct statvfs buf;
    size_t total_size, free_size;
    char temp_file[_CC_MAX_PATH_];
    
    // 构造临时文件路径
    snprintf(temp_file, sizeof(temp_file), "%s/.wipe_temp_xx0216xx", mount);
    
    // 创建临时文件
    fd = mkstemp(temp_file);
    if (fd == -1) {
        _cc_logger_error("Create temp file fial. %s", temp_file);
        return -1;
    }
    
    //printf("创建临时文件: %s\n", temp_file);
    
    // 获取文件系统信息
    if (statvfs(mount, &buf) == -1) {
        _cc_logger_error("Unable to obtain file system information");
        close(fd);
        _cc_unlink(temp_file);
        return -1;
    }
    
    free_size = (long)buf.f_bavail * buf.f_frsize;
    total_size = (long)buf.f_blocks * buf.f_frsize;
    
    //printf("文件系统总大小: %.2f GB\n", (double)total_size / (1024*1024*1024));
    //printf("可用空间大小: %.2f GB\n", (double)free_size / (1024*1024*1024));
    // 执行七次覆写
    if (dod_5220_22m_wipe(fd, free_size) != 0) {
        //fprintf(stderr, "文件 %s 擦除失败。\n", temp_file);
        close(fd);
        _cc_unlink(temp_file);
        return -1;
    }

    // 关闭文件描述符
    close(fd);
    _cc_unlink(temp_file);
    
    return 0;
}

void free_unlink(const tchar_t *path) {
    struct _stat st;
    if (path == NULL) {
        return;
    }
    
    _tstat(path, &st);
    if (S_ISDIR(st.st_mode) == 0) {
        dod_5220_22m_unlink(path);
    } else {
        rm(path);
    }
}
