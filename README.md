# ShellAssignment

62588 Operating Systems - Mandatory Assignment Shell

## How To Run

To compile the programmen open a terminal and navigate to the project folder and then run

### Compile
```
gcc -o ucli ucli.c
```
### Run
If the program is compiled type the following in a terminal that should be in the project folder to run the program

```
./ucli
```

or find the program called ucli and run it.

## Ho To Use

Usage of the program is like using other shells, firstly run the program, and then type the command to run. The program is able to run the commands in the systems `$PATH`. If unsure about the systems `$PATH` run 
```
echo $PATH
```
and eventually checkout what is shown there.
## Explainable topics

### System Calls
The shell should be able to make system calls, which is basically the way a program ask the kernel to do a task. A task could be hardware related, like choosing a directory on the harddrive with `cd` or list files in a directory with `ls`. It can also be used for starting and running a new proccess.

### I/O Redirection
I/O redirection is also supported in the shell, I/O redirection is taking the output from one command and used it as a input in the next command.  `ip addr | grep inet` where the output from ip addr 

### The Program Envirement
The program environment is the required enviroment needed for the program to be able to run. For the ucli program to run, a linux envirement or if ran on windos a cygwin enviroment. We cannot guarentee that the program will work on mac-os since a device running macos was not avaivable for testing. The program is able to execute commands that are in the path, so as an example on one computer the path consists of
```
/usr/local/sbin:
/usr/local/bin:
/usr/bin:
/usr/lib/jvm/default/bin:
/usr/bin/site_perl:
/usr/bin/vendor_perl:
/usr/bin/core_perl
```
btw `execvp` takes directly from path so the tokenization of path in our program is unnessesary, but done as proof of how it can be done.w

### Background Process
A background process is a process that runs in the background, without the user handling it, it could for example be system monitoring. When a process is running as a background process 