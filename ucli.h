///////////////////////////////////////////
// The University Command Line Interface //
///////////////////////////////////////////

#ifndef SHELL_UCLI_H
#define SHELL_UCLI_H

#define BUFFERSIZE 128
#define DEBUG 1

int getdir(char *cwd);
int readinput(char *buffer);
int parseinput(char *buffer, char ****commandsptr);
int executecommand(char **command);
int bincommand(char *command);

#endif //SHELL_UCLI_H
