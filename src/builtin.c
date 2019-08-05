#include "../include/builtin.h"
#include "../include/command.h"
#include <time.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

// 退出myshell
void myExit(char **argv) {
    extern job* first_job;
    // 将所有任务退出, 防止僵尸进程
    for (job* j = first_job; j; j = j ->next) {
        if (j->pgid)
            kill(-j->pgid, SIGTERM);
    }
    exit(0);
}

// 打印命令行参数到屏幕上
void myEcho(char **argv) {
    // 遍历参数，用空格分割
    for (int i = 1; argv[i]; ++i) {
        if (i != 1)
            printf(" ");
        dprintf(STDOUT_FILENO, "%s", argv[i]);
    }
    // 打印换行符
    printf("\n");
}

// 打印任务信息
void myJobs(char **argv) {
    extern job* first_job;
    pid_t mypgid = __getpgid(getpid());
    // 遍历所有存在的任务
    for (job* j = first_job; j; j = j->next) 
        if (j->pgid != mypgid) {
            //判断任务的状态，打印相关的信息
            if (isCompleted(j))
                printJobInfo(stderr, "Done", j); // 已退出
            else if (isStopped(j))
                printJobInfo(stderr, "Stopped", j); // 已停止
            else
                printJobInfo(stderr, "Running", j); // 正在运行
            j ->notified=true; // 表示已经通知给用户
        }    
}

// 打印指定目录下的所有文件
void myDir(char **argv) {
    char *path;
    // 如果第一个命令为空
    // 则打印当前目录
    if (!argv[1])
        path = getenv("PWD");
    else 
        path = argv[1]; // 否则打印指定目录

    // 打开指定的目录
    DIR* dir = opendir(path);
    // 如果返回值为空，则表示目录非法
    if (dir == NULL) {
        fprintf(stderr, "Invalid dir");
        return ;
    }
    // 读取目录
    struct dirent* handle;
    // 遍历目录，打印所有文件名
    while(handle = readdir(dir))
        printf("%s ", handle->d_name);
    printf("\n");
    closedir(dir);
}

// 更改工作目录
void myCd(char **argv) {
    // 包含两个或者以上参数，错误
    if (argv[2]) {
        fprintf(stderr, "Too many arguments\n");
        return ;
    }

    if (argv[1]) {
        char buff[100];
        int err, cherr;
        // 更改工作目录到指定位置
        cherr = chdir(argv[1]);
        // 设置环境变量
        if (cherr == 0)
            err = setenv("PWD", getcwd(buff, 100), 1);
        // 检查错误
        if (err == -1 || cherr == -1)
            fprintf(stderr, "Invalid Path\n");
    }

}

// 将停止的任务放在后台运行
void myBg(char **argv) {
    job *j;
    pid_t job_id;
    if (!argv[1])  {
        // 未指定id，找第一个停止的任务
        j = findStoppedJob(-1);
    } else {
        // 将命令行字符串转换为数字
        char *invalid = NULL;
        job_id = strtol(argv[1], &invalid, 8);

        // 命令非数字
        if (*invalid != '\0') {
            fprintf(stderr, "Invalid job specifier\n");
            return ;
        }

        // 指定id，找停止的任务
        j = findStoppedJob(job_id);
    }

    if (j == NULL) {
        fprintf(stderr, "No stopped job\n");
        return ;
    }

    // 更新任务信息
    j ->notified = false;
    for (process *p = j -> first; p; p = p -> next)
        p -> stopped = false;
    // 发送继续的信号
    kill(-j->pgid, SIGCONT);
    fprintf(stderr, "[%d]\tContinued\t%d\n", j->id, j->pgid);
    return ;
}

// 将停止的任务放在后台运行
void myFg(char **argv) {
    job *j;
    pid_t job_id;
    if (!argv[1])  {
        j = findStoppedJob(-1);
    } else {
        char *invalid = NULL;
        job_id = strtol(argv[1], &invalid, 8);

        if (*invalid != '\0') {
            fprintf(stderr, "Invalid job specifier\n");
            return ;
        }

        j = findStoppedJob(job_id);
    }

    if (j == NULL) {
        fprintf(stderr, "No stopped job\n");
        return ;
    }


    putJobFront(j, true);
    j -> notified = false;
    for (process *p = j -> first; p; p = p -> next)
        p -> stopped = true;
    return ;
}

// 获取当前目录
void myPwd(char **argv) {
    // 有参数，非法
    if (argv[1]) {
        fprintf(stderr, "Too many arguments\n");
        return ;
    }

    // 获取环境变量PWD并打印
    printf("%s\n", getenv("PWD"));
}

// 显示当前的时间
void myTime(char **argv) {
    // 获取当时时间戳
    time_t nowt = time(NULL);
    struct tm *tm_now;
    // 将其格式化为当前时间的年月日
    tm_now = localtime(&nowt);
 
    printf("now datetime: %d-%d-%d %d:%d:%d\n", tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec); 
}

// 清屏，并将光标移动至左上角
void myClr(char **argv) {
    // 先将光标置于左上角，然后清空
    printf("\033[0;0H");
    printf("\033[2J");
}

// 打印所有环境变量
void myEnviron(char **argv) {
    // 有参数，则错误
    if (argv[1]) {
        fprintf(stderr, "Too many arguments\n");
        return ;
    }

    // 遍历__environ全局变量，全部打印
    for (int i = 0; __environ[i]; i++)
        printf("%s\n", __environ[i]);
    return ;
}

// 显示帮助信息
void myHelp(char **argv) {
    if (argv[1]) {
        fprintf(stderr, "Too many arguments\n");
        return ;
    }

    execlp("more", "more", "readme.md", NULL);
}

// 将环境变量解除
void myUnset(char **argv) {
    // 检查命令
    if (! argv[1]) {
        fprintf(stderr, "Not enough arguments\n");
    } else if (argv[2]) {
        fprintf(stderr, "Too many arguments\n");
    }

    int result = unsetenv(argv[1]); // 系统调用，接触环境变量
    // 未成功，参数非法
    if (result == -1) {
        fprintf(stderr, "Invalid argument\n");
    }
        
}

// 设置环境变量
void mySet(char **argv) {
    // 检查参数个数
    if (!argv[1] || !argv[2] || argv[3]) {
        fprintf(stderr, "Invalid arguments\n");
        return ;
    }

    // 系统调用，设置环境变量
    int err = setenv(argv[1], argv[2], 1);
    // 设置失败，参数非法
    if (err == -1) {
        fprintf(stderr, "Invalid arguments\n");
        return ;
    }

    return ;
}

// 将myshell的命令行参数移动
void myShift(char **argv) {
    extern int shiftv;
    int shift_pos = -1;
    // 检查命令行参数个数
    if (! argv[1]) {
        // 一个参数，平移1位
        shift_pos = 1;
    }
    else if (argv[2]) {
        // 参数过多
        fprintf(stderr, "Too many arguments\n");
        return ;
    } else {
        // 将arg1转换为数字
        char *invalid = NULL;
        shift_pos = strtol(argv[1], &invalid, 8);

        if (*invalid != '\0' || shift_pos < 0) { // 参数非法
            fprintf(stderr, "Invalid symbolic mode operator\n");
            return ;
        }

    }

    shiftv += shift_pos; // 平移
}

// 检查表达式是否合法
void myTest(char **argv) {
    pid_t pid;
    // 还需要分裂一次 
    pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv);
        exit(-1);
    } else if (pid > 0) {
        int status = 0;
        int result = waitpid(pid, &status, 0);
        if (result == -1) {
            perror(argv[0]);
            return ;
        }
        // 根据进程返回状态判断真假
        if (status == 0)
            fprintf(stderr, "true\n");
        else 
            fprintf(stderr, "false\n");
    }

    return ;
}

// 执行参数给出的命令，并将该程序取代myshell
void myExec(char **argv) {
    // 检查参数个数
    if (!argv[1]) {
        fprintf(stderr, "Not enough argument");
        return ;
    }
    execvp(argv[1], argv + 1);
}

// 设置文件权限掩码
void myUmask(char **argv) {
    // 检查参数个数
    if (! argv[1])
        return;
    if (argv[2]) {
        fprintf(stderr, "Too many arguments\n");
        return ;
    }

    // 将arg1转换为数字(8进制)
    char *invalid = NULL;
    int mask = strtol(argv[1], &invalid, 8);

    // 参数不是数字
    if (*invalid != '\0') {
        fprintf(stderr, "Invalid symbolic mode operator\n");
        return ;
    }
    umask(mask); // 系统调用，设置掩码
}