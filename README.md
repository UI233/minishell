# minishell

This is a bash-like shell implemented using C for Linux.

## Prequistes

This project is built in `Ubuntu 18.04` since it makes use of system calls in Linux and this project has successfully compiled in gcc 7.3.0.

## Getting Started

1. Use `git` to  clone this project  `git clone https://github.com/UI233/minishell.git`


2. Direct to the root directory `cd ./minishell`

3. Build this project using Makefile `make`

4. Get Started! `./minishell`

## Supported Features

- Run program in background by adding **&** at the end of a command
- Add **|** between commands to use pipe eg. `ls | wc` 
- Internal commands implemented by myself using system calls of Linux
    - exec
    - echo
    - jobs
    - dir
    - cd
    - fg
    - bg
    - pwd
    - time
    - clr
    - environ
    - help
    - unset
    - set
    - shift
    - umask
- Run external commands in the shell
- Run shell script consisting of simple instructions
## Demo

![Dynamic Demo](https://github.com/UI233/imageTemp/blob/master/minishell_demo.gif)

## References

 The processes management system is inspired by
> [Implementing a Shell](https://www.gnu.org/software/libc/manual/html_node/Implementing-a-Shell.html#Implementing-a-Shell)

