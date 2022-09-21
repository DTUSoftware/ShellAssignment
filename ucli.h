///////////////////////////////////////////
// The University Command Line Interface //
///////////////////////////////////////////

#ifndef SHELL_UCLI_H
#define SHELL_UCLI_H

#define BUFFERSIZE 128

int getdir(char *cwd);
int readinput(char *buffer);
int parseinput(char *buffer, char *command, char *args);
int executecommand(char *command, char *args);
int bincommand(char *command);

#endif //SHELL_UCLI_H
