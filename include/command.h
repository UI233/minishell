#ifndef COMMAND_H
#define COMMAND_H
#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <termio.h>
#include "interpreter.h"

// 用于储存一个进程数据的数据结构
// 所有进程储存在一个链表中，该数据结构仅包含一个节点
struct process {
    struct process *next;
    char **argv; // 该进程的命令行参数
    pid_t pid; // 该进程的pid
    bool completed; // 该进程是否完成
    bool stopped; // 该进程是否停止
    int status; // 线程的状态
    int infile, outfile; //  输入输出文件的文件表述符
    enum command_t cmdt; // 该线程对应的指令的类型
};
typedef struct process process;

// 用于储存一个任务所需信息的数据结构
// 所有任务储存在一个链表中，该数据结构仅包含一个节点
// 任务由待执行的process数组组成，保证使用了管道符'|'连接
struct job{
    struct job *next; // 链表中的下一个任务
    process *first; // 该任务所有线程储存在一个链表中，该成员为这个链表的头指针
    pid_t pgid; // 该任务的进程组id
    bool notified; // 表示是否向用户通知该任务已完成
    bool background; // 该任务是否应该在后台运行
    int stdin, stdout; // 该任务标准输入输出的文件表述符
    int id; // 该任务的id
    char cmd[500]; // 该任务对应的命令行命令
    struct termios tmode; // 储存信号处理的模式的信息
};
typedef struct job job;

// 根据解析得到的命令组来执行指令
bool runJobs(job *jobs);
// 将解析出来的新任务添加到任务链表的表尾
void addJob(job *jobs);
// 将指定的任务放置到前台运行
// continued表示是否需要向该任务发送SIGCONT信号
void putJobFront(job *jobs, int continued);
// 向stream打印运行任务的信息，信息包括任务的id，任务的线程组id，还有指定的msg
void printJobInfo(FILE* stream, const char* msg, job *j);
// 如果pgid为-1则寻找链表中第一个停止了的线程的指针，如果不存在停止的线程，返回空指针
// 否则，若指定pgid对应的任务状态为停止，返回指向该任务的任务指针，若不是停止，返回空指针 
job* findStoppedJob(pid_t pgid);
// 判断指定的任务是否停止
bool isStopped(job *jobs);
// 判断指定的任务是否退出
bool isCompleted(job *jobs);
#endif