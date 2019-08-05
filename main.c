#define _GNU_SOURCE
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <termios.h>
#include "include/interpreter.h"
#include "include/command.h"
#define MAXLEN 500
#define DIRLEN 100
pid_t shell_pgid;
struct termios shell_tmodes;
bool shell_is_interactive = true;
char **_argv;
int _argc;
int shiftv = 0;

void sigCHLDHanlder(pid_t pid);
void init();

int main(int argc, char* argv[]) {
    // 储存当前目录的字符串
    char cwd[DIRLEN];
    bool should_run = true;
    shell_pgid = getpid();
    _argc = argc - 1;
    _argv = argv + 1;
    // myshell有参数，从文件输入命令
    FILE* cmd_in = stdin;
    if (argc != 1) {
      cmd_in = fopen(argv[1], "r");
      shell_is_interactive = false;
      if (cmd_in == NULL) {
        fprintf(stderr, "No such file\n");
        return -1;
      }
    }

    init();

    while(should_run) {
        // 如果前台运行，打印命令提示符，包括myshell和目录名
        if (argc == 1) {
          printf("\e[1;32mmyshell:\033[0m");
          getcwd(cwd, DIRLEN);
          printf("\e[1;34m%s\033[0m$ ", cwd);
        }
        int num;
        char *cmd;
        int err = getline(&cmd, &num, cmd_in);
        fflush(cmd_in);
        if (err == -1) {
          should_run = false;
        }
        else should_run = interprete(cmd);

        if (cmd)
          free(cmd);
    }
    return 0;
}

void init() {
  // 是否以键盘输入交互
  int shell_terminal = STDIN_FILENO;

  if (shell_is_interactive)
    {
      // 等待myshell已经放置到前台
      while (tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp ()))
        kill (- shell_pgid, SIGTTIN);

      //  初始化shell，将shell信号全部设置为忽略
      signal (SIGINT, SIG_IGN);
      signal (SIGQUIT, SIG_IGN);
      signal (SIGTSTP, SIG_IGN);
      signal (SIGTTIN, SIG_IGN);
      signal (SIGTTOU, SIG_IGN);
      signal (SIGCHLD, SIG_IGN);

      // 将myshell放进自己的线程组中
      shell_pgid = getpid ();
      if (setpgid (shell_pgid, shell_pgid) < 0) {
          exit (1);
      }

      // 把myshell放置到前台
      tcsetpgrp (shell_terminal, shell_pgid);

      // 保存myshell1的属性
      tcgetattr (shell_terminal, &shell_tmodes);
    }

    // 得到myshell的绝对路径
    char link[]="/proc/self/exe";
    char buf[4096];
    readlink(link, buf, 4096);
    setenv("myshell", buf, 1);
}