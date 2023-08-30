//
//  fork.c
//  
//
//  Created by CC on 2020/1/23.
//  Copyright  2020 CC. All rights reserved.
//


#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <libcc.h>
/*
 * @brief main For the understanding of the fork()
 *
 * @param argc
 * @param argv[]
 *
 * @return 在父进程中返回子进程的进程号;在子进程中返回0。
 */
pid_t pid;
int* shm = NULL;
int shmid = 0;

void exit_proc(int sig);

void exit_proc(int sig) {
    //kill(0,SIGTERM);
    switch (sig) {
        case 1:
            printf("exit_proc -- SIGHUP\n");
            break;
        case 2:
            printf("exit_proc -- SIGINT\n");
            break;
        case 3:
            printf("exit_proc -- SIGQUIT\n");
            break;
            
        default:
            printf("exit_proc error - %d\n", sig);
            break;
    }
    _cc_sleep(20000);
    exit(0);
}

void exit_clild_proc(int sig) {
    pid_t pid = 0;
    int status = 0;

    switch (sig) {
        case 1:
            printf("exit_clild_proc -- SIGHUP\n");
            break;
        case 2:
            printf("exit_clild_proc -- SIGINT\n");
            break;
        case 3:
            printf("exit_clild_proc -- SIGQUIT\n");
            break;
            
        default:
            printf("exit_clild_proc error - %d\n", sig);
            break;
    }

    if (signal(SIGCHLD,exit_clild_proc) == SIG_ERR) {
        perror("Signal error\n");
    }

    if ((pid = wait(&status)) < 0) {
        perror("wait (in signal) error\n");
    }

    printf("remove pid %d\n", pid);
}

int main(int argc, char *argv[])
{
    
    int cnt = 0;
    signal(SIGHUP,exit_proc);
    signal(SIGINT,exit_proc);
    signal(SIGQUIT,exit_proc);
    signal(SIGTERM,exit_proc);
    signal(SIGCHLD,exit_clild_proc);//捕捉子进程信号

    shmid = shmget(19850417, sizeof(int)*10, 0666|IPC_CREAT);
    if (shmid == -1) {
        printf("shmget failed\n");
        return 0;
    }
    
    pid = fork();
    
    shm = (int*)shmat(shmid, 0, 0);
    
     if (pid == -1) {
         perror("fork error");
         exit(1);
     } else {
         printf("The returned value is %d\nMy PID is %d\n", pid, getpid());
         cnt++;
     }
    
    while (1) {
        _cc_sleep(100);
        printf("pid:%d cnt = %d\n", pid, *shm);
        if (pid) {
            *shm=*shm + 1;
        }
        
        if (*shm > 10) {
            break;
        }
    }
    
    if (shmdt(shm) == -1) {
        printf("shmdt failed\n");
        return 0;
    }
    
    if (pid != 0) {
        shmctl(shmid, IPC_RMID, 0);
    }
    return 0;
}
