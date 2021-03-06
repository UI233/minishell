#include "../include/command.h"
#include "../include/builtin.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <stdlib.h>
#define MAXLEN 500
int cnt = 0;
int now = 0;
job *first_job = NULL;
// 外部全局变量，myshell进程相关
extern pid_t shell_pgid;
extern struct termios shell_tmodes;
extern bool shell_is_interactive;

// 释放线程数据结构占有的内存
void freeProcessGroup(process *p) {
    if(p ==NULL)
        return ;
    // 释放所有参数
    for (int i = 0; p->argv[i]; ++i)
        free(p->argv[i]);
    free(p->argv);
    // 递归地释放链表内存
    freeProcessGroup(p->next);
    free(p->next);
}

// 释放任务数据结构占有的内存
void freeJob(job *jobs) {
    freeProcessGroup(jobs->first);
    free(jobs->first);
}

// 更新所有任务的状态信息
void updateAll() {
    for (job *now = first_job; now; now = now ->next) {
        bool completed = true;
        for(process *p = now->first; p; p = p->next) {
            int status;
            // 更新线程状态
            waitpid(p->pid, &status, WNOHANG);
            if (WIFSTOPPED(status)) // 判断是否停止
                p->stopped = true;
            else if(kill(p->pid, 0) == -1) // 判断是否退出
                p->completed = true;
            completed &= p->completed;
        }

        // 后台运行的进程退出会打印任务信息
        if(completed && now->background && !now->notified) {
            printJobInfo(stderr, "Done", now);
            now->notified = true;
        }
    }
}

// 判断一个任务是否完成
bool isCompleted(job *jobs) {
    bool completed = false;
    if (jobs==NULL)
        return false;
    // 判断属于该任务的所有进程是否完成
    for (process *p = jobs->first; p; p = p->next)
        if (p->completed)
            completed = true;
        else if (kill(p->pid, 0) == -1) { // 如果kill函数返回值为-1则表示该线程不存在，已退出
            p->completed = true;
            completed = true;
        }
        else return false; // 如果有线程未退出，返回false
    return completed;
}

// 判断一个任务是否停止
bool isStopped(job *jobs) {
    bool stopped = false;
    if (jobs==NULL)
        return false;
    // 判断停止状态
    for (process *p = jobs->first; p && !stopped; p = p->next)
        if (p->stopped)
            stopped = true;
        else {
            int status;
            waitpid(p->pid, &status, WNOHANG);
            if (WIFSTOPPED(status)) {
                p -> stopped = true;
                stopped = true;
            }
        }
    return stopped;
}

// 将任务放到链表表尾
void addJob(job *jobs) {
    // 更新所有线程状态
    updateAll();
    job* before = NULL;
    job* next;
    // 将已退出的任务从链表删除
    for(job *p = first_job; p; p = next) {
        next = p->next;
        // 线程已退出，释放内存
        if (isCompleted(p)) {
            --cnt;
            if (before) {
                before->next = p->next;
                freeJob(p);
                free(p);
            }
            else {
                first_job = p->next;
                freeJob(p);
                free(p);
            }
        }
        else
            before = p;
    }

    if (jobs == NULL)
        return;
    if (cnt == 0)
        now = 0;
    // 更新id
    ++cnt;
    jobs->id = ++now;
    job **current;
    // 寻找表尾
    for (current = &first_job; *current; current = &(*current)->next);
    *current = jobs; 

    return ;    
}

// 打印线程信息
void printJobInfo(FILE* stream, const char* msg, job *j) {
    if (stream == NULL || j == NULL)
        return;
    
    fprintf(stream, "[%d]  %s\t%s", j->id, msg,  j->cmd);
}

// 执行一个进程
bool lauchProcess(process p, pid_t pgid, int infile, int outfile, bool background) {
    pid_t pid = getpid();
    // 将信号接收设置为默认状态
    if (shell_is_interactive)
    {
        if (pgid == 0)
            pgid = pid;
        setpgid(pid, pgid);
        if (!background) // 如果为前台运行则接管权限
            tcsetpgrp(STDIN_FILENO, pgid);
        // 将信号设置为默认
        signal (SIGINT, SIG_DFL);
        signal (SIGQUIT, SIG_DFL);
        signal (SIGTSTP, SIG_DFL);
        signal (SIGTTIN, SIG_DFL);
        signal (SIGTTOU, SIG_DFL);
        signal (SIGCHLD, SIG_DFL);
    }
    
    // 处理输入输出文件
    if (p.infile != STDIN_FILENO) { // 该进程进行了输入重定向
        dup2(p.infile, STDIN_FILENO);
        if (infile != STDIN_FILENO)
            close(infile);
        close(p.infile);
    } else { // 管道输入
        if (infile != STDIN_FILENO) {
            dup2(infile, STDIN_FILENO);
            close(infile);
        }
    }

    if (p.outfile != STDOUT_FILENO) { // 该进程进行了输出重定向
        dup2(p.outfile, STDOUT_FILENO);
        if (outfile != STDOUT_FILENO)
            close(outfile);
        close(p.outfile);
    } else { // 管道输出
        if (outfile != STDOUT_FILENO) {
            dup2(outfile, STDOUT_FILENO);
            close(outfile);
        }
    }

    // 根据命令类型调用相关函数
    switch (p.cmdt)
    {
    // 执行外部指令
    case OUTTER:
        if (execvp(p.argv[0], p.argv) == -1) {
            fprintf(stderr, "No such Command\n");
        }
        break;
    // 执行内部指令
    case IDIR:
        myDir(p.argv);
        break;
    case IECHO:
        myEcho(p.argv);
        break;
    case CD:
        myCd(p.argv);
        break;
    case FG:
        myFg(p.argv);
        break;
    case BG: 
        myBg(p.argv);
        break;
    case PWD:
        myPwd(p.argv);
        break;
    case TIME:
        myTime(p.argv);
        break;
    case CLR:
        myClr(p.argv);
        break;
    case ENVIRON:
        myEnviron(p.argv);
        break;
    case HELP:
        myHelp(p.argv);
        break;
    case EXIT:
    case QUIT:
        myExit(p.argv);
        break;
    case SET:
        mySet(p.argv);
        break;
    case SHIFT:
        myShift(p.argv);
        break;
    case TEST:
        myTest(p.argv);
        break;
    case EXEC:
        myExec(p.argv);
        break;
    case UMASK:
        myUmask(p.argv);
        break;
    case UNSET:
        mySet(p.argv);
        break;
    case JOBS:
        myJobs(p.argv);
        break;
    default:
        break;
    }
    exit(1);
}

// 把指定的线程状态标记为status
bool markStatus(job *jobs, pid_t pid, int status) {
    if (pid > 0) {
        for (job* j = first_job; j; j = j->next) {
            for (process *p = j->first; p; p = p ->next) {
                if (p->pid == pid) {
                    p->status = status;
                    // 如果为停止
                    if (WIFSTOPPED(status)) {
                        p->stopped = true;
                        // 发现停止进程，进行标记
                        if (!j->notified) {
                            printJobInfo(stderr, "Stopped", j);
                            j->notified = true;
                        }
                    }
                    else {
                        p->completed = true;
                    }

                    return 0;
                }
            }
        }
    }
    else return true;

    return true;
}

void waitForJobs(job *jobs) {
    int status;
    pid_t pid;
    do {
        pid = waitpid (-(jobs->pgid), &status, WUNTRACED);
    }
    while(!markStatus(jobs, pid, status) && !isCompleted(jobs) && !isStopped(jobs) );
}

// 将工作组放置到前台
void putJobFront(job *jobs, int continued) {
    tcsetpgrp(STDIN_FILENO, jobs->pgid);
    if (continued)
    {
        tcsetattr (STDIN_FILENO, TCSADRAIN, &jobs->tmode);
        kill(-jobs->pgid, SIGCONT);
    }
    // 等待前台进程完成
    waitForJobs(jobs);
    // 恢复状态，给前台控制
    tcsetpgrp(STDIN_FILENO, shell_pgid);
    tcgetattr (STDIN_FILENO, &jobs->tmode);
    tcsetattr (STDIN_FILENO, TCSADRAIN, &shell_tmodes);
}

// 执行一行命令
bool runJobs(job *jobs) {
    // 将解析出来的工作链表当中，并将已经结束的指令从链表中删除
    addJob(jobs);
    int infile, outfile;
    int mypipe[2];

    infile = jobs->stdin;
    for(process *p = jobs->first; p; p = p->next) {
        if (p -> next) {
            // 需要管道操作
            pipe(mypipe);
            outfile = mypipe[1];
        }
        else {
            outfile = jobs->stdout;
            if (p == jobs->first) {
                switch (p->cmdt)
                {
                    // 执行需要在父线程执行的内部指令
                    // 这些命令都需要改变当前父进程的状态
                    case CD:
                        myCd(p->argv);
                        jobs->first->completed = true;
                        return true;
                    case FG:
                        myFg(p->argv);
                        jobs->first->completed = true;
                        return true;
                    case BG: 
                        myBg(p->argv);
                        jobs->first->completed = true;
                        return true;
                    case EXIT:
                    case QUIT:
                        myExit(p->argv);
                        jobs->first->completed = true;
                        return true;
                    case EXEC:
                        myExec(p->argv);
                        jobs->first->completed = true;
                        return true;
                    case UMASK:
                        myUmask(p->argv);
                        jobs->first->completed = true;
                        return true;
                    case SET:
                        mySet(p->argv);
                        jobs->first->completed = true;
                        return true;
                    case SHIFT:
                        myShift(p->argv);
                        jobs->first->completed = true;
                        return true;
                    case UNSET:
                        myUnset(p->argv);
                        jobs->first->completed = true;
                        return true;
                    default:
                        break;
                }
            }
        }

        // 执行进程
        pid_t pid;
        pid = fork();
        if (pid == 0) {
            // 子线程，设置parent环境变量并执行相关线程
            setenv("parent", getenv("myshell"), 1);
            unsetenv("myshell");
            if (jobs->pgid == 0) {
                jobs->pgid = getpid();
            }
            p->pid = getpid();
            lauchProcess(*p, jobs->pgid, infile, outfile, jobs->background);
        }
        else if(pid > 0){
            if (shell_is_interactive) {
                // 父线程
                if (jobs->pgid == 0) {
                    jobs->pgid = pid;
                    if (jobs->background)
                        fprintf(stderr, "[%d]  %d\n", jobs->id, jobs->pgid);
                }
                setpgid(pid, jobs->pgid);
            }
            p->pid = pid;
        } else {
            // 打印错误信息
        }

        // 清理管道
        if (infile != jobs->stdin)
            close(infile);
        if (outfile != jobs->stdout)
            close(outfile);
        infile = mypipe[0];
    }

    // 如果不在后台执行，需要将前台的管理权交给当前任务的进程组
    if (!shell_is_interactive)
        waitForJobs(jobs);
    else if (!jobs->background)
        putJobFront(jobs, 0);
    fflush(stdout);
    return true;
}

// 寻找第一个停止的任务
job* findStoppedJob(pid_t pgid) {
    if (pgid == -1) {
        // 找到第一个停止的工作
        for (job* p = first_job; p; p = p->next)
            if (isStopped(p))
                return p;
        return NULL;
    }
    else {
        // 找到指定的pid
        for (job* p = first_job; p; p = p->next)
            if (isStopped(p) && p->pgid == pgid)
                return p;
        return NULL;
    }
}

