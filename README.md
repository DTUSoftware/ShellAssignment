# ShellAssignment

62588 Operating Systems - Mandatory Assignment Shell

## How To Use

To compile the programmen open a terminal and navigate to the project folder and then run

```
gcc -o ucli ucli.c ucli.h
```

after compiling type the following to run the compiled program

```
./ucli
```

or find the program called ucli and run it.


The shell should be able to make system calls, which is basically the way a program ask the kernel to do a task. A task could be hardware related, like choosing a directory on the harddrive with `cd` or list files in a directory with `ls`. It can also be used for starting and running a new proccess.

I/O redirection is also supported in the shell, I/O redirection is taking the output from one command and used it as a input in the next command.  `ip addr | grep inet` 

The program environment

Linux ???

A background process is a process that runs in the background, without the user handling it, it could for example be system monitoring. When a process is running in the background 
