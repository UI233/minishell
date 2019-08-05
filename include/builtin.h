// shell内置命令的接口头文件
// 
#ifndef BUILTIN_H
#define BUILTIN_H
#include <stdbool.h>
#include "interpreter.h"
// 退出shell命令
void myExit(char **argv);
// echo命令，打印命令行参数的消息到屏幕
void myEcho(char **argv);
// 打印运行的命令的信息
void myJobs(char **argv);
// 显示指定文件夹的文件，若命令行参数为空，则打印当前文件夹
void myDir(char **argv);
// 改变当前工作目录
void myCd(char **argv);
// 将停止的工作放置到前台
void myFg(char **argv);
// 将停止的工作放置到后台
void myBg(char **argv);
// 显示当前工作目录
void myPwd(char **argv);
// 显示当前的时间
void myTime(char **argv);
// 清屏，并将光标移动至左上角
void myClr(char **argv);
// 打印所有环境变量
void myEnviron(char **argv);
// 打印帮助信息
void myHelp(char **argv);
// 将环境变量取消设置
void myUnset(char **argv);
// set arg1 arg2
// 将arg1环境变量的值设置为arg2
void mySet(char **argv);
// 将命令行参数向左移动arg1位
void myShift(char **argv);
// 测试表达式为真还是假，将结果打印到屏幕上
void myTest(char **argv);
// 执行参数给出的命令，并将该程序取代myshell
void myExec(char **argv);
// 设置文件权限mask为arg1
void myUmask(char **argv);
#endif
