///////////////////////////////////////////
// The University Command Line Interface //
///////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "ucli.h"

int getdir(char *cwd) {
    // Getting current working dir
    if (getcwd(cwd, PATH_MAX + 1) == NULL) {
        strcpy(cwd, "NULL");
        return -1;
    }
    return 1;
}

int readinput(char **bufferptr, int newlinestop) {
    char *buffer = *bufferptr;
    // We tried using getline(), but it wasn't in MingW, and stumbled upon this implementation that
    // could not really be altered - so credit where credit is due.
    // thanks to https://gist.github.com/btmills/4201660
    unsigned int buffer_size = BUFFERSIZE;
    int ch = EOF;
    int pos = 0;

    while (!((ch = fgetc(stdin)) == '\n' && newlinestop == 1) && ch != EOF && !feof(stdin)) {
        buffer[pos++] = ch;
        if (pos == buffer_size) {
            buffer_size = buffer_size + BUFFERSIZE;
            buffer = realloc(buffer, buffer_size * sizeof(char));
            if (buffer == NULL) {
                perror("memory allocation");
                exit(EXIT_FAILURE);
            }
        }
    }
    buffer_size = ++pos;
    buffer = realloc(buffer, buffer_size+1 * sizeof(char));
    if (buffer == NULL) {
        perror("memory allocation");
        exit(EXIT_FAILURE);
    }
    buffer[pos] = '\0';

//            printf("DEBUG: %s\n", buffer);
    bufferptr[0] = buffer;
    return 1;
}

int parseinput(char *buffer, char ****commandsptr) {
    char ***commands = *commandsptr;

    // initialize with first command
    char **command = malloc(2 * sizeof(char*));
    if (command == NULL) {
        perror("out of memory");
        exit(EXIT_FAILURE);
    }
    command[0] = NULL;
    command[1] = NULL;
    commands[0] = command;

    int i = 0;
    int j = 0;
    // if no arguments are passed
    if (strstr(buffer, " ") == NULL) {
        command[i] = malloc((strlen(buffer)+1) * sizeof(char));
        if (command[i] == NULL) {
            perror("out of memory");
            exit(EXIT_FAILURE);
        }
        strcpy(command[i], buffer);
    } // if arguments are passed
    else {
        char *arg = strtok(buffer, " ");
        if (arg == NULL) {
//            perror("failed to parse");
//            exit(EXIT_FAILURE);
            return -1;
        }

        while (arg != NULL) {
            commands[j] = command;
            // if arg is a pipe
            if (arg[0] == '|' || strcmp(arg, "|&") == 0) {
                i = -1;
                j++;
                // expand commands array
                commands = realloc(commands, j+2 * sizeof(char*)); // array of commands, ptr's to command
                if (commands == NULL) {
                    perror("out of memory");
                    exit(EXIT_FAILURE);
                }
                commands[j+1] = NULL;

                // allocate for new command (old command is kept in commands arr)
                command = malloc(2 * sizeof(char*));
                if (command == NULL) {
                    perror("out of memory");
                    exit(EXIT_FAILURE);
                }
                command[0] = NULL;
                command[1] = NULL;
            }
            else {
                if (i != 0) {
                    command = realloc(command, (i + 2) * sizeof(char *));
                    if (command == NULL) {
                        perror("out of memory");
                        exit(EXIT_FAILURE);
                    }
                    command[i + 1] = NULL;
                }

                // if arg is string
                if (arg[0] == '"' || arg[0] == '\'') {
                    // add arg to command
                    command[i] = malloc(strlen(arg) * sizeof(char));
                    if (command[i] == NULL) {
                        perror("out of memory");
                        exit(EXIT_FAILURE);
                    }
                    strcpy(command[i], arg);

                    arg = strtok(NULL, " ");
                    while (arg != NULL && strstr("\"", arg) == NULL) {
                        command[i] = realloc(command[i], (strlen(arg)+strlen(command[i])) * sizeof(char));
                        if (command[i] == NULL) {
                            perror("out of memory");
                            exit(EXIT_FAILURE);
                        }
                        strcat(command[i], " ");
                        strcat(command[i], arg);
                        arg = strtok(NULL, " ");
                    }

                    if (arg != NULL) {
                        command[i] = realloc(command[i], (strlen(arg)+strlen(command[i])) * sizeof(char));
                        if (command[i] == NULL) {
                            perror("out of memory");
                            exit(EXIT_FAILURE);
                        }
                        strcat(command[i], " ");
                        strcat(command[i], arg);
                        arg = strtok(NULL, " ");
                    }
                }
                else {
                    // add arg to command
                    command[i] = malloc((strlen(arg)+1) * sizeof(char));
                    if (command[i] == NULL) {
                        perror("out of memory");
                        exit(EXIT_FAILURE);
                    }
                    strcpy(command[i], arg);
                }
            }


            if (arg != NULL) {
                arg = strtok(NULL, " ");
            }
            i++;
            commands[j] = command;
        }
    }
    commands[j] = command;
    commandsptr[0] = commands;

    // Check if commands are in /bin, and change path
    i = 0;
    char **cur_command = commands[i];
    while (cur_command != NULL) {
        if (bincommand(cur_command)) {
            if (DEBUG) {
                printf("[DEBUG]: %s found in /bin!\n", cur_command[0]);
            }
        }
        cur_command = commands[++i];
    }

    // debug printing
    if (DEBUG) {
        printf("======[ DEBUG ]======\n"
               "PARSED COMMANDS: \n");
        i = 0;
        cur_command = commands[i];
        while (cur_command != NULL) {
            printf("#%d:\n", i+1);
            j = 0;
            char *cur_arg = cur_command[j];
            while (cur_arg != NULL) {
                printf("- %s\n", cur_arg);
                cur_arg = cur_command[++j];
            }
            cur_command = commands[++i];
        }
        printf("=====================\n");
    }

    return 1;
}

// yes, good, kill that child :)
int executechild(char **command, int pipefd[2]) {
    // set stdout to pipe output (credit to https://stackoverflow.com/a/7292659)
    if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
        perror("pipeoutput");
        exit(EXIT_FAILURE);
    }
    if (dup2(pipefd[1], STDERR_FILENO) == -1) {
        perror("pipeoutput");
        exit(EXIT_FAILURE);
    }
    close(pipefd[0]); // close input pipe
    close(pipefd[1]); // close output pipe

    // call correct binary
    execvp(command[0], command);
    perror("Child Error");
    exit(EXIT_FAILURE); // not reached
}

int executecommands(char ***commands, int i) {
    char **command = commands[i];
    if (command != NULL) {
        // if we want to change working directory
        if (strcmp("cd", command[0]) == 0) {
            // TODO: path support path with spaces (only arg1 is passed rn)
            if (chdir(command[1]) != 0) {
                printf("Error changing working directory using %s\n", command[1]);
            }
        } // if we want to exit the shell
        else if (strcmp("exit", command[0]) == 0) {
            return 2;
        } else {
            // piping <insert manpage>
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }

            // forking: https://man7.org/linux/man-pages/man2/wait.2.html
            pid_t cpid;

            cpid = fork();
            if (cpid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } // child code
            else if (cpid == 0) {
                return executechild(command, pipefd); // doesn't return
            } // parent code
            else {
                // credit to https://stackoverflow.com/questions/7369286/c-passing-a-pipe-thru-execve for understanding of pipes
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]); // close input pipe
                close(pipefd[1]); // close output pipe
                return executecommands(commands, i+1);
            }
        }
    } // get input from last child after waiting command
    else {
        pid_t wpid;
        int wstatus;

        // https://man7.org/linux/man-pages/man2/wait.2.html
        // wait for all children
        while ((wpid = wait(&wstatus)) > 0) {
            if (wpid == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
            else {
                if (DEBUG) {
                    printf("[DEBUG]: Child returned: %d\n", wpid);
                }
            }
        }

//        while (!feof(stdin)) {
        char **bufferptr = calloc(1, sizeof(char*));
        if (bufferptr == NULL) {
            perror("out of memory");
            exit(EXIT_FAILURE);
        }
        char *buffer = calloc(BUFFERSIZE, sizeof(char));
        if (buffer == NULL) {
            perror("out of memory");
            exit(EXIT_FAILURE);
        }
        bufferptr[0] = buffer;

        // Read input from console
        if (!readinput(bufferptr, 0)) {
            // if we could not read, and not caused by memory error, try again
//                while (getchar() != EOF);
            free(buffer);
            buffer = NULL;
            return 2;
        }
//            while (getchar() != EOF); // flush stdin

        buffer = *bufferptr;
        free(bufferptr);
        bufferptr = NULL;

        printf("%s\n", buffer);

        free(buffer);
        buffer = NULL;
    }

    return 1;
}

// Checks whether a command is in /bin or not - and if in bin, change command to fit to path
int bincommand(char **command) {
    char *executable = command[0];
    // read list of binaries to find command
    // https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
    DIR *d;
    struct dirent *dir;
    d = opendir("/bin");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            // TODO: replace with startswith
//            printf("%d\n", strcmp(dir->d_name, command));
            if (strstr(dir->d_name, executable) != NULL) {
                char *old_command = malloc(sizeof(char) * (strlen(executable)+1));
                if (old_command == NULL) {
                    perror("out of memory");
                    exit(EXIT_FAILURE);
                }

                strcpy(old_command, executable);
                executable = realloc(executable, sizeof(char) * (strlen(executable) + strlen("/bin/") + 1));
                if (command == NULL) {
                    perror("out of memory");
                    exit(EXIT_FAILURE);
                }
                command[0] = executable;

                strcpy(executable, "/bin/");
                strcat(executable, old_command);
                free(old_command);
                old_command = NULL;
                closedir(d);
                return 1;
            }
//            printf("[DEBUG]: %s\n", dir->d_name);
        }
        closedir(d);
    }
    return 0;
}

int main() {
    int status = 1;
    while (true) {
        fflush(stdout); // Flush if something is stuck :)

        // Variable to keep current working directory
        char *cwd = calloc(PATH_MAX + 1, sizeof(char));
        if (cwd == NULL) {
            perror("out of memory");
            exit(EXIT_FAILURE);
        }
        getdir(cwd);

        // Reading command-line input
        printf("%s > ", cwd);
        fflush(stdout);
        free(cwd);
        cwd = NULL;

        char **bufferptr = malloc(sizeof(char*));
        if (bufferptr == NULL) {
            perror("out of memory");
            exit(EXIT_FAILURE);
        }
        char *buffer = calloc(BUFFERSIZE, sizeof(char));
        if (buffer == NULL) {
            perror("out of memory");
            exit(EXIT_FAILURE);
        }
        bufferptr[0] = buffer;

        // Read input from console
        if (!readinput(bufferptr, 1)) {
            // if we could not read, and not caused by memory error, try again
            free(buffer);
            buffer = NULL;
            continue;
        }
        buffer = *bufferptr;
        free(bufferptr);
        bufferptr = NULL;

        // Check that input exists and is above 1 (end of string char takes up 1 char)
        if (strlen(buffer) > 1) {
            // String for keeping the commands for the child to execute
            char ****commandsptr = malloc(sizeof(char*)); // prt to array of commands
            if (commandsptr == NULL) {
                perror("out of memory");
                exit(EXIT_FAILURE);
            }
            char ***commands = malloc(2 * sizeof(char*)); // array of commands, ptr's to command
            if (commands == NULL) {
                perror("out of memory");
                exit(EXIT_FAILURE);
            }
            commands[0] = NULL;
            commands[1] = NULL;
            commandsptr[0] = commands;

            if (!parseinput(buffer, commandsptr)) {
                // if parsing fails, try again
                free(buffer);
                buffer = NULL;
                // free parsed commands
                int i = 0;
                char **cur_command = commands[i];
                while (cur_command != NULL) {
                    int j = 0;
                    char *cur_arg = cur_command[j];
                    while (cur_arg != NULL) {
                        free(cur_arg);
                        cur_arg = cur_command[++j];
                    }
                    free(cur_command);
                    cur_command = commands[++i];
                }
                free(commands);
                commands = NULL;
                free(commandsptr);
                commandsptr = NULL;
                continue;
            }
            commands = *commandsptr;

            // We no longer need to keep the buffer in memory :)
            free(buffer);
            buffer = NULL;

            // Command execution
            int _stdin = dup(STDIN_FILENO); // we will replace stdin inside the call
            status = executecommands(commands, 0);
            // Exit command
            if (status == 2) {
                break;
            }
            dup2(_stdin, STDIN_FILENO); // recover stdin

            // free command and args
            int i = 0;
            char **cur_command = commands[i];
            while (cur_command != NULL) {
                int j = 0;
                char *cur_arg = cur_command[j];
                while (cur_arg != NULL) {
                    free(cur_arg);
                    cur_arg = cur_command[++j];
                }
                free(cur_command);
                commands[i] = NULL;
                cur_command = commands[++i];
            }
            // already freed, basically
            free(commands);
            commands = NULL;
            free(commandsptr);
            commandsptr = NULL;
        } else {
            free(buffer);
            buffer = NULL;
        }
    }
    exit(EXIT_SUCCESS);
}