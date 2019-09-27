#include "../include/command.h"
#include "../include/interpreter.h"
#include <sys/file.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#define eq(a,b) (strcmp(a, b) == 0)
static bool syntax_error = false;
static char* error_token;

bool isNum(char* str) {
    char *invalid = NULL;
    int mask = strtol(str, &invalid, 10);
    return str!=NULL && *invalid == '\0';
}
// 释放参数组内存
bool freeArgs(args* operand) {
    // 操作数为空
    if (operand == NULL)
        return false;
    // 将每个参数逐一释放
    for (int i = 0; i < operand -> args_num; ++i)
        free(operand->argv[i]);
    return true;
}

// 将一句命令按空格分割成很多词
args lex(const char* cmd) {
    args res;
    res.args_num = 0;
    // 将前缀的空格跳过
    while((*cmd == ' ' || *cmd == '\t') && *cmd && *cmd != '\n')
        ++cmd;
    // 上一个空格的下一个字符
    const char* last = cmd;
    while (*cmd && *cmd != '\n') {
        // 跳到一个词的末尾
        while(*cmd != ' ' && *cmd != '\t' && *cmd && *cmd != '\n')
            ++cmd;
        int len = cmd - last + 1;
        // 分配内存，将词复制进arg_group
        res.argv[res.args_num] = malloc(len);
        memcpy(res.argv[res.args_num], last, len - 1);
        res.argv[res.args_num][len - 1] = 0;

        // 解析变量，将变量替换成相应值的字符串
        if (res.argv[res.args_num][0] == '$') {
            // 如果为数字，则解析命令行参数
            if (isNum(&res.argv[res.args_num][1])) {
                extern int _argc;
                extern char** _argv;
                extern int shiftv;
                char* invalid;
                // 将数字装换为int
                int num = strtol(&res.argv[res.args_num][1], &invalid, 10);
                // 非$0的命令行参数
                if (num > 0 && num + shiftv <  _argc) {
                    free(res.argv[res.args_num]);
                    res.argv[res.args_num] = malloc(sizeof(char) * strlen(_argv[num + shiftv]));
                    strcpy(res.argv[res.args_num], _argv[num + shiftv]);
                }
                else if(num == 0) { // 当前运行的文件
                    free(res.argv[res.args_num]);
                    res.argv[res.args_num] = malloc(sizeof(char) * strlen(_argv[0]));
                    strcpy(res.argv[res.args_num], _argv[0]);
                }
                else { // 非法参数，传入空指针
                    free(res.argv[res.args_num]);
                    res.argv[res.args_num] = malloc(sizeof(char));
                    res.argv[res.args_num][0] = '\0';
                }
            }
            else { // 解析为环境变量
                char* envarg = getenv(&res.argv[res.args_num][1]);
                free(res.argv[res.args_num]);
                if (envarg == NULL)
                    res.argv[res.args_num] = malloc(sizeof(char));
                else res.argv[res.args_num] = malloc(strlen(envarg) + 1);
                char* arg = res.argv[res.args_num];
                // 环境变量是否存在
                if (envarg != NULL)
                    strcpy(arg, envarg); // 存在，复制进去
                else arg[0] = '\0'; // 不存在将其设置为空字符串
            }
        } 

        // 判断下一个空格的位置
        while((*cmd == ' ' || *cmd == '\t') && *cmd && *cmd != '\n') 
            ++cmd;
        last = cmd;
        // 参数数量累加
        ++res.args_num;
        // 超过最大参数数量则退出
        if (res.args_num >= MAXARGS)
            break;
    }

    return res;
}

// 解析一条指令，命令中保障无管道符
command parseSingle(args arg_group, int st, int *end) {
    command res;
    res.st = st;
    res.input = NULL;
    res.output = NULL;
    res.omod = WRITE;
    // 如果空指令由管道或者后台符相连
    if (eq(arg_group.argv[st], "|") || eq(arg_group.argv[st], "&")){
        syntax_error = true;
        error_token = arg_group.argv[st];
        return res;
    }
    // 将用空格分隔开的参数，转换为一个个由管道符分割的命令
    while (*end < arg_group.args_num && !eq(arg_group.argv[*end], "|")) {
        char* arg = arg_group.argv[*end];
        if (eq(arg, ">>")) {
            // 重定向添加
            // 未指定重定向目标
            if (*end == arg_group.args_num - 1) {
                syntax_error = true;
                error_token = "newline";
                return res;
            }
            res.omod = APPEND; // 模式为添加
            res.output = arg_group.argv[*end + 1];
            ++*end;
        } 

        // 重定向输出
        if (eq(arg, ">")) {
            // 如果未指定重定向目标
            if (*end == arg_group.args_num - 1) {
                syntax_error = true;
                error_token = "newline";
                return res;
            }
            res.omod = WRITE;
            res.output = arg_group.argv[*end + 1];
            ++*end;
        }

        // 重定向输入
        if (eq(arg, "<")) {
            // 如果未指定重定向目标
            if (*end == arg_group.args_num - 1) {
                syntax_error = true;
                error_token = "newline";
                return res;
            }
            res.input = arg_group.argv[*end + 1];
            ++*end;
        }

        ++*end;
    }

    char* cmd = arg_group.argv[st];
    res.cmd = OUTTER; // 先设置为外部指令，这样如果没有找到匹配项则可以识别为外部指令
    // 根据第一个参数, 确定指令类型，
    if (eq(cmd, "echo"))
        res.cmd = IECHO;
    if (eq(cmd, "help"))
        res.cmd = HELP;
    if (eq(cmd, "fg"))
        res.cmd = FG;
    if (eq(cmd, "bg"))
        res.cmd = BG;
    if (eq(cmd, "cd"))
        res.cmd = CD;
    if (eq(cmd, "clr"))
        res.cmd = CLR;
    if (eq(cmd, "dir"))
        res.cmd = IDIR;
    if (eq(cmd, "exec"))
        res.cmd = EXEC;
    if (eq(cmd, "environ"))
        res.cmd = ENVIRON;
    if (eq(cmd, "jobs"))
        res.cmd = JOBS;
    if (eq(cmd, "pwd"))
        res.cmd = PWD;
    if (eq(cmd, "set"))
        res.cmd = SET;
    if (eq(cmd, "shift"))
        res.cmd = SHIFT;
    if (eq(cmd, "test"))
        res.cmd = TEST;
    if (eq(cmd, "time"))
        res.cmd = TIME;
    if (eq(cmd, "unset"))
        res.cmd = UNSET;
    if (eq(cmd, "umask"))
        res.cmd = UMASK;
    if (eq(cmd, "quit"))
        res.cmd = QUIT;
    if (eq(cmd, "exit"))
        res.cmd = EXIT;

    res.ed = *end;
    ++*end;
    return res; 
}

// 解析一行命令，命令可能由多条指令用管道符相连
commandGroup parse(args arg_group) {
    commandGroup cmdg;
    cmdg.cmds_num = 0;
    cmdg.arg_group = arg_group;
    cmdg.background = false;
    int st = 0;
    // 判断是否为背景执行
    if (eq(arg_group.argv[arg_group.args_num - 1], "&")) {
        --arg_group.args_num;
        cmdg.background = true;
    }

    // 逐个解析每个单独的命令
    while (st < arg_group.args_num && !syntax_error)
        cmdg.cmds[cmdg.cmds_num++] = parseSingle(arg_group, st, &st);
    

    return cmdg;
}

// 从命令行组构造成job
job* constructJob(commandGroup cmdg) {
    job* res = NULL;
    if (cmdg.cmds_num == 0)
        return res;
    res = malloc(sizeof(job));
    // 对工作组的初始化
    res->pgid = 0;
    res->notified = false;
    res->stdin = STDIN_FILENO;
    res->stdout = STDOUT_FILENO;
    res->notified = false;
    res->next = NULL;
    process** now = &res->first;
    // 构造工作组的进程
    for (int i = 0; i < cmdg.cmds_num; ++i) {
        // 对一个process进行状态的初始化
        *now = (process*)malloc(sizeof(process));
        (*now)->stopped = (*now)->completed = false;
        (*now)->next = NULL;
        (*now)->cmdt = cmdg.cmds[i].cmd;
        // 装载命令行参数
        int argnum = cmdg.cmds[i].ed - cmdg.cmds[i].st + 1; // 决定参数的数量
        (*now)->argv = malloc(sizeof(char*) * argnum);
        int vi = 0;
        // 将参数进行复制
        for (int j = 0; j < argnum - 1; ++j) {
            if (eq(">>", cmdg.arg_group.argv[j + cmdg.cmds[i].st]) || eq("<", cmdg.arg_group.argv[j + cmdg.cmds[i].st]) || eq(">" ,cmdg.arg_group.argv[j + cmdg.cmds[i].st])){
                ++j;
                continue;
            }
            int len = strlen(cmdg.arg_group.argv[j + cmdg.cmds[i].st]);
            (*now)->argv[vi] = malloc(len + 1);
            strcpy((*now)->argv[vi], cmdg.arg_group.argv[j + cmdg.cmds[i].st]);
            ++vi;
        }
        (*now)->argv[vi] = NULL;
        // 将重定向的文件打开
        if (cmdg.cmds[i].input == NULL) // 标准输入
            (*now)->infile = STDIN_FILENO;
        else 
            (*now)->infile = open(cmdg.cmds[i].input, O_RDONLY); // 打开重定向输入文件

        if (cmdg.cmds[i].output == NULL) // 标准输出
            (*now)->outfile = STDOUT_FILENO;
        else  {
            // 重定向输出, 打开文件
            int oflag = O_WRONLY | O_CREAT | (cmdg.cmds[i].omod == APPEND ? O_APPEND : O_TRUNC);
            (*now)->outfile = open(cmdg.cmds[i].output, oflag, 0777); // 打开重定向输出文件
        }
        now = &((*now)->next);
    }

    return res;
}

// 解释一行命令并运行
bool interprete(const char* cmd) {
    if (!*cmd || cmd[0] == '\n'){ // 空指令，不进行操作
        addJob(NULL);
        return true;
    }
    syntax_error = false;
    // 将一行命令分割成词
    args arg_group = lex(cmd);
    // 将分割成词的命令解析成指令组
    commandGroup cmdg = parse(arg_group);
    // 将指令组构造成Jobs数据结构，储存需要运行的任务信息
    job *jobs = constructJob(cmdg);
    // 如果发现了语法错误
    if (syntax_error || jobs == NULL) {
        printf("syntax error near token \'%s\'\n", error_token);
        freeArgs(&arg_group);
        return true;
    }
    bool should_run = true;
    jobs->background = cmdg.background;
    strcpy(jobs->cmd, cmd);
    // 解析完毕后运行命令
    should_run = runJobs(jobs);
    // 完成后释放内存
    freeArgs(&arg_group);
    return should_run;
}
