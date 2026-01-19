#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/statvfs.h>
#include <errno.h>
#include <time.h>
#include <libcc.h>

#define OVERWRITE_PASSES 7
#define BUFFER_SIZE (4 * 1024 * 1024) // 4MB缓冲区，平衡内存占用与I/O效率
#define ZERO_PATTERN 0x00
#define ONE_PATTERN 0xFF

// 定义覆写模式序列：3次0，3次1，1次随机
typedef enum {
    PASS_ZERO = 0,
    PASS_ONE,
    PASS_RANDOM
} overwrite_pattern_t;

// 覆写模式序列数组，对应DoD 5220.22-M C级标准
const overwrite_pattern_t overwrite_sequence[OVERWRITE_PASSES] = {
    PASS_ZERO, PASS_ZERO, PASS_ZERO,  // 前三次：全0
    PASS_ONE, PASS_ONE, PASS_ONE,     // 中三次：全1
    PASS_RANDOM                       // 最后一次：随机
};

// 进度条显示
void print_progress(long current, long total) {
    int progress = (int)((current * 100) / total);
    printf("\r擦除进度: [%-50.*s] %d%%", 
           progress/2, 
           "==================================================",
           progress);
    fflush(stdout);
}

// 执行单次覆写操作
int perform_single_pass(int fd, size_t file_size, overwrite_pattern_t pattern) {
    char *buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        perror("内存分配失败");
        return -1;
    }
    // 根据模式初始化缓冲区
    if (pattern == PASS_ZERO) {
        memset(buffer, ZERO_PATTERN, BUFFER_SIZE);
    } else if (pattern == PASS_ONE) {
        memset(buffer, ONE_PATTERN, BUFFER_SIZE);
    } else if (pattern == PASS_RANDOM) {
        _cc_random_bytes((byte_t*)buffer, BUFFER_SIZE);
    } else {
        fprintf(stderr, "未知的覆写模式\n");
        free(buffer);
        return -1;
    }

    // 从文件开头开始，逐块写入
    off_t offset = 0;
    while (offset < file_size) {
        size_t write_size = (file_size - offset) < BUFFER_SIZE ? (file_size - offset) : BUFFER_SIZE;
        ssize_t written = write(fd, buffer, write_size);
        if (written == -1) {
            perror("写入失败");
            free(buffer);
            return -1;
        }
        if (written < (ssize_t)write_size) {
            fprintf(stderr, "写入字节数不足，期望 %zu，实际 %zd\n", write_size, written);
            // 继续尝试写入剩余部分
            offset += written;
            continue;
        }
		print_progress(offset, file_size);
        offset += written;
    }

    // 刷新写入缓冲区，确保数据落盘
    if (fsync(fd) == -1) {
        perror("fsync失败，数据可能未落盘");
        free(buffer);
        return -1;
    }

    free(buffer);
    return 0;
}

// 执行完整的DoD 5220.22-M七次覆写
int dod_5220_22m_wipe(int fd, size_t file_size) {
    printf("开始执行DoD 5220.22-M七次覆写擦除...\n");
    for (int i = 0; i < OVERWRITE_PASSES; i++) {
        const char *pattern_names[] = {"全0", "全1", "随机"};
        const char *pass_name = (i < 3) ? pattern_names[0] : (i < 6) ? pattern_names[1] : pattern_names[2];
        printf("第 %d 次覆写: %s\n", i + 1, pass_name);
        if (perform_single_pass(fd, file_size, overwrite_sequence[i]) != 0) {
            fprintf(stderr, "第 %d 次覆写失败，擦除过程终止。\n", i + 1);
            return -1;
        }
        printf("第 %d 次覆写完成。\n", i + 1);
    }
    printf("DoD 5220.22-M七次覆写擦除成功完成。\n");
    return 0;
}

// 重命名并删除文件，增加一层逻辑删除
void rename_and_delete(const char *filename) {
    //生成一个单字符新文件名，避免文件名被恢复工具识别
    char new_name[2] = {filename[rand() % strlen(filename)], '\0'};
    if (rename(filename, new_name) == 0) {
        printf("文件已重命名为: %s\n", new_name);
        if (unlink(new_name) == 0) {
            printf("文件已从文件系统中删除。\n");
        } else {
            perror("删除重命名后的文件失败");
        }
    } else {
        perror("重命名文件失败");
    }
}

// 获取磁盘空闲空间并覆写
int free_space(const char* mount_point) {
    int fd;
    long total_size, free_size;
    char temp_file[_CC_MAX_PATH_];
    
    // 构造临时文件路径
    snprintf(temp_file, sizeof(temp_file), "%s/.wipe_temp_XXXXXX", mount_point);
    
    // 创建临时文件
    fd = mkstemp(temp_file);
    if (fd == -1) {
        perror("无法创建临时文件");
        return -1;
    }
    
    printf("创建临时文件: %s\n", temp_file);
    
    // 获取文件系统信息
    struct statvfs buf;
    if (statvfs(mount_point, &buf) == -1) {
        perror("无法获取文件系统信息");
        close(fd);
        unlink(temp_file);
        return -1;
    }
    
    free_size = (long)buf.f_bavail * buf.f_frsize;
    total_size = (long)buf.f_blocks * buf.f_frsize;
    
    printf("文件系统总大小: %.2f GB\n", (double)total_size / (1024*1024*1024));
    printf("可用空间大小: %.2f GB\n", (double)free_size / (1024*1024*1024));
	// 执行七次覆写
	if (dod_5220_22m_wipe(fd, free_size) != 0) {
		fprintf(stderr, "文件 %s 擦除失败。\n", temp_file);
		close(fd);
        unlink(temp_file);
		return -1;
	}

	// 关闭文件描述符
	close(fd);
    rename_and_delete(temp_file);
	return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "使用方法: %s <文件1> <文件2> ... <文件N>\n", argv[0]);
        fprintf(stderr, "说明: 本程序将对指定文件执行DoD 5220.22-M七次覆写擦除，并删除文件。\n");
        return 1;
    }

    // 初始化随机数种子
    srand(time(NULL));

	if (argc == 1) {
		free_space("/home");
		return 0;
	}

    for (int i = 1; i < argc; i++) {
        printf("\n正在处理文件: %s\n", argv[i]);

        // 打开文件，以读写模式
        int fd = open(argv[i], O_RDWR);
        if (fd == -1) {
            perror("open");
            continue;
        }

        // 获取文件大小
        struct stat st;
        if (fstat(fd, &st) == -1) {
            perror("fstat");
            close(fd);
            continue;
        }

        // 检查文件是否为空
        if (st.st_size == 0) {
            printf("文件为空，跳过覆写。\n");
            close(fd);
            rename_and_delete(argv[i]);
            continue;
        }

        // 执行七次覆写
        if (dod_5220_22m_wipe(fd, st.st_size) != 0) {
            fprintf(stderr, "文件 %s 擦除失败。\n", argv[i]);
            close(fd);
            continue;
        }

        // 关闭文件描述符
        close(fd);

        // 执行逻辑删除
        rename_and_delete(argv[i]);
    }

    printf("\n所有文件处理完毕。\n");
    return 0;
}
