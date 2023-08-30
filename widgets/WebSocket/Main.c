#include "DataList.h"

#ifndef __CC_WINDOWS__

#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <pthread.h>

#define MAXFILE 65535

tchar_t already_running_file[_CC_MAX_PATH_];

#define write_lock(fd, offset, whence, len) lock_reg((fd), F_SETLK, F_WRLCK, (offset), (whence), (len))
int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len) {
    struct flock lock;
    lock.l_type = type; /* F_RDLCK, F_WRLCK, F_UNLCK */
    lock.l_start = offset; /* byte offset, relative to l_whence */
    lock.l_whence = whence; /* SEEK_SET, SEEK_CUR, SEEK_END */
    lock.l_len = len; /* #bytes (0 means to EOF) */
    return (fcntl(fd, cmd, &lock));
}

int already_running() {
    int fd = -1;
    int len = 0;
    char buf[32];

    fd = open(already_running_file, O_RDWR | O_CREAT, 0664);
    if (fd < 0) {
        return -1;
    }
    /* 加建议性写锁 */
    if (write_lock(fd,  0, SEEK_SET, 0) < 0) {
        close(fd);
        return -1;
    }

    ftruncate(fd,0);
    len = snprintf(buf, 32, "%ld", (long)getpid());
    write(fd, buf, len);
    return 0;
}

void pr_exit(int status) {
    _cc_logger_error( "child process exited with status %d \n", status );
    if(WIFEXITED(status))
        _cc_logger_error("normal termination,exitstatus=%d\n",WEXITSTATUS(status));
    else if(WIFSIGNALED(status))
        _cc_logger_error("abnormal termination,signalstatus=%d\n",WTERMSIG(status));                                                                                           
#ifdef WCOREDUMP
    else if WCOREDUMP(status)
        _cc_logger_error("(core file generated)", WCOREDUMP(status));
#endif
    else if(WIFSTOPPED(status))
        _cc_logger_error("child stopped ,signal number=%d\n", WSTOPSIG(status));
}

int daemon_runing(int (*daemon)(int argc,  tchar_t * const argv[]), int argc,  tchar_t * const argv[]) {
    int pid = 0;
    //int i = 0;
    char_t currentPath[_CC_MAX_PATH_];

    _cc_get_module_directory(NULL, currentPath, _cc_countof(currentPath));

    _sntprintf(already_running_file, _cc_countof(already_running_file), _T("%salready_running"),currentPath);

    if (already_running() != 0) {
        return 0;
    }

    pid = fork();
    //int fd, fdtablesize;
    if (pid < 0) {
        _cc_logger_error(_T("fork fail (%d)."), pid);
        exit( EXIT_FAILURE );
    }

    if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        pr_exit(status);
        
        _cc_sleep(5000);
        return daemon_runing(daemon, argc, argv);
    }

    _cc_set_current_directory(currentPath);
    setsid();
    /*
    fdtablesize = getdtablesize();
    for (i = 0; i < fdtablesize; ++i) {
        close(i);
    }
    fd = open("/dev/null", 0);
    if (fd == -1) {
        perror("can't open /dev/null");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < 3; ++i) {
        dup2(fd, i);
    }

    if (fd > 2)
        close(fd);

    */
    //_cc_set_current_directory(currentPath);
    
    umask(0);

    if (daemon)
        return daemon(argc, argv);

    return 0;
}
#endif
void _Mir2WebSocketListener(uint16_t port);
int WebSocket(int argc, const char* argv[]) {
    char ch;
    _Mir2DataBufferInit();

    _Mir2NetworkStartup(6);
    _Mir2WebSocketListener(9090);

    /*等待用户输入q退出*/
    while ((ch = getchar()) != 'q') {
        _cc_sleep(100);
    }

    _Mir2NetworkStop();
    _Mir2DataBufferQuit();
    return 0;
}

int main(int argc, const char * argv[]) {
    // insert code here...
    return WebSocket(argc, argv);
    //return daemon_runing(startSyncFils, argc, argv);
}
