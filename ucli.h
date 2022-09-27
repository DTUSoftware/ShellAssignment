///////////////////////////////////////////
// The University Command Line Interface //
///////////////////////////////////////////

#ifndef SHELL_UCLI_H
#define SHELL_UCLI_H

#define BUFFERSIZE 128
#define DEBUG 1

int getdir(char *cwd);
int readinput(char **bufferptr, int newlinestop);
int parseinput(char *buffer, char ****commandsptr);
int executecommands(char ***commands, int i);
int executechild(char **command, int pipefd[2]);
int checkpath(char **command);

#endif //SHELL_UCLI_H
