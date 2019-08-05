#ifndef INTERPRETER_H
#define INTERPRETER_H
#include <stdbool.h>
#define MAXARGS 25 
#define MAXCMDS 50
// 支持指令的类型
// 内部指令的类型为指令的名字
// 外部指令的类型为OUTTER
enum command_t {
    CD,
    FG,
    BG,
    PWD,
    TIME,
    CLR,
    IDIR,
    ENVIRON,
    IECHO,
    HELP,
    QUIT,
    EXIT,
    SET,
    SHIFT,
    TEST,
    EXEC,
    UMASK,
    UNSET,
    JOBS,
    OUTTER // 外部指令
};

// 重定向输入输出的模式
enum IOMODE {
    READ,
    WRITE,
    APPEND
};
// 储存参数组
struct args {
    char *argv[MAXARGS];
    int args_num;
};

typedef struct args args;


// 单条指令的类型
struct command {
    enum command_t cmd;
    unsigned st, ed; // 参数开头与结尾在命令组的参数组的编号
    char* output; // 输出文件的文件名
    char* input; // 输入文件的文件名
    enum IOMODE omod; // 重定向输出的输出模式
};
typedef struct command command;

// 用管道符相连的命令组, 表示一个任务
struct commandGroup {
    command cmds[MAXCMDS];
    unsigned cmds_num; // 表示命令的数量
    args arg_group; // 储存每个命令的参数的数据结构
    bool background; // 是否应该在后台运行
};
typedef struct commandGroup commandGroup;

// 将指定命令解释并运行
bool interprete(const char* cmd);
#endif