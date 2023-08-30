#include "Header.h"

#ifdef __CC_WINDOWS__
#ifndef _DEBUG
	#ifndef _UNICODE
		#pragma comment (lib,"libcc_vs2010.lib")
	#else
		#pragma comment (lib,"libccu_vs2010.lib")
	#endif
#else
	#ifndef _UNICODE
		#pragma comment (lib,"libcc_vs2010d.lib")
	#else
		#pragma comment (lib,"libccud_vs2010.lib")
	#endif
#endif

#elif 0

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

int _tmain(int argc, const tchar_t * argv[]) {
    // insert code here...
    return startSyncFils(argc, argv);
    //return daemon_runing(startSyncFils, argc, argv);
}
