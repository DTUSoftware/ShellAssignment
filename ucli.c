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

int readinput(char *buffer) {
    // We tried using getline(), but it wasn't in MingW, and stumbled upon this implementation that
    // could not really be altered - so credit where credit is due.
    // thanks to https://gist.github.com/btmills/4201660
    unsigned int buffer_size = BUFFERSIZE;
    int ch = EOF;
    int pos = 0;

    while ((ch = fgetc(stdin)) != '\n' && ch != EOF && !feof(stdin)) {
        buffer[pos++] = ch;
        if (pos == buffer_size) {
            buffer_size = buffer_size + BUFFERSIZE;
            buffer = (char *) realloc(buffer, buffer_size);
            if (buffer == NULL) {
                perror("memory allocation");
                exit(EXIT_FAILURE);
            }
        }
    }
    buffer_size = ++pos;
    buffer = (char *) realloc(buffer, buffer_size);
    if (buffer == NULL) {
        perror("memory allocation");
        exit(EXIT_FAILURE);
    }
    buffer[pos] = '\0';

//            printf("DEBUG: %s\n", buffer);
    return 1;
}

int parseinput(char *buffer, char ***command) {
    char **commands = *command;
    // if no arguments are passed
    if (strstr(buffer, " ") == NULL) {
        commands[0] = malloc(strlen(buffer) * sizeof(char));
        if (commands[0] == NULL) {
            perror("out of memory");
            exit(EXIT_FAILURE);
        }
        strcpy(commands[0], buffer);
    }
        // if arguments are passed
    else {
        char *arg = strtok(buffer, " ");
        if (arg == NULL) {
//            perror("failed to parse");
//            exit(EXIT_FAILURE);
            return -1;
        }

        int i = 0;
        while (arg != NULL) {
            if (i != 0) {
                commands = realloc(commands, (i + 2) * sizeof(char *));
                if (commands == NULL) {
                    perror("out of memory");
                    exit(EXIT_FAILURE);
                }
                commands[i + 1] = NULL;
            }

            commands[i] = malloc(strlen(arg) * sizeof(char));
            if (commands[i] == NULL) {
                perror("out of memory");
                exit(EXIT_FAILURE);
            }
            strcpy(commands[i], arg);

            // if arg is string
            if (arg[0] == '"' || arg[0] == '\'') {
                arg = strtok(NULL, " ");
                while (arg != NULL && strstr("\"", arg) == NULL) {
                    commands[i] = realloc(commands[i], (strlen(arg)+strlen(commands[i])) * sizeof(char));
                    if (commands[i] == NULL) {
                        perror("out of memory");
                        exit(EXIT_FAILURE);
                    }
                    strcat(commands[i], " ");
                    strcat(commands[i], arg);
                    arg = strtok(NULL, " ");
                }

                if (arg != NULL) {
                    commands[i] = realloc(commands[i], (strlen(arg)+strlen(commands[i])) * sizeof(char));
                    if (commands[i] == NULL) {
                        perror("out of memory");
                        exit(EXIT_FAILURE);
                    }
                    strcat(commands[i], " ");
                    strcat(commands[i], arg);
                    arg = strtok(NULL, " ");
                }
            }

            if (arg != NULL) {
                arg = strtok(NULL, " ");
            }
            i++;
        }
    }
    command[0] = commands;

    // debug printing
    if (DEBUG) {
        printf("======[ DEBUG ]======\n"
               "Parsed command: \n");
        int i = 0;
        char *cur_command = commands[i];
        while (cur_command != NULL) {
            printf("%s\n", cur_command);
            cur_command = commands[++i];
        }
        printf("=====================\n");
    }

    return 1;
}

int executecommand(char **command) {
    // if we want to change working directory
    if (strcmp("cd", command[0]) == 0) {
        // TODO: path support path with spaces (only arg1 is passed rn)
        if (chdir(command[1]) != 0) {
            printf("Error changing working directory using %s\n", command[1]);
        }
    }
        // if we want to exit the shell
    else if (strcmp("exit", command[0]) == 0) {
        return 2;
    } else {
        // piping <insert manpage>
        int pipefd[2];
        char recv[4096];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        // forking: https://man7.org/linux/man-pages/man2/wait.2.html
        pid_t cpid, w;
        int wstatus;

        cpid = fork();
        if (cpid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } // child code
        else if (cpid == 0) {
            // set stdout to pipe output (credit to https://stackoverflow.com/a/7292659)
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[0]); // close reading pipe
            close(pipefd[0]); // close writing pipe

            // call correct binary
            execvp(command[0], command);
            perror("Child Error");
            exit(EXIT_FAILURE); // not reached
        } // parent code
        else {
            close(pipefd[1]); // close writing pipe
            // https://man7.org/linux/man-pages/man2/wait.2.html
            do {
                w = waitpid(cpid, &wstatus, WUNTRACED | WCONTINUED);
                if (w == -1) {
                    perror("waitpid");
                    exit(EXIT_FAILURE);
                }

                int nbytes = read(pipefd[0], recv, sizeof(recv));
                printf("%.*s\n", nbytes, recv);

//                if (WIFEXITED(wstatus)) {
//                    printf("exited, status=%d\n", WEXITSTATUS(wstatus));
//                } else if (WIFSIGNALED(wstatus)) {
//                    printf("killed by signal %d\n", WTERMSIG(wstatus));
//                } else if (WIFSTOPPED(wstatus)) {
//                    printf("stopped by signal %d\n", WSTOPSIG(wstatus));
//                } else if (WIFCONTINUED(wstatus)) {
//                    printf("continued\n");
//                }
            } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
        }
    }
    return 1;
}

// Checks whether a command is in /bin or not - and if in bin, change command to fit to path
int bincommand(char *command) {
    // read list of binaries to find command
    // https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
    DIR *d;
    struct dirent *dir;
    d = opendir("/bin");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            // TODO: replace with startswith
//            printf("%d\n", strcmp(dir->d_name, command));
            if (strstr(dir->d_name, command) != NULL) {
                char *old_command = (char *) malloc(sizeof(char) * strlen(command));
                if (old_command == NULL) {
                    perror("out of memory");
                    exit(EXIT_FAILURE);
                }

                strcpy(old_command, command);
                command = (char *) realloc(command, sizeof(char) * (strlen(command) + strlen("/bin/")));
                if (command == NULL) {
                    perror("out of memory");
                    exit(EXIT_FAILURE);
                }

                strcpy(command, "/bin/");
                strncat(command, old_command, strlen(old_command));
                free(old_command);
                old_command = NULL;
                return 1;
            }
//            printf("[DEBUG]: %s\n", dir->d_name);
        }
        closedir(d);
    }
    return 0;
}

int main() {
    int status;
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

        char *buffer = calloc(BUFFERSIZE, sizeof(char));
        if (buffer == NULL) {
            perror("out of memory");
            exit(EXIT_FAILURE);
        }

        // Read input from console
        if (!readinput(buffer)) {
            // if we could not read, and not caused by memory error, try again
            free(buffer);
            buffer = NULL;
            continue;
        }

        // Check that input exists and is above 1 (end of string char takes up 1 char)
        if (strlen(buffer) > 1) {
            // String for keeping the command for the child to execute
            // TODO: free me :)
            char ***commands = malloc(sizeof(char*));
            if (commands == NULL) {
                perror("out of memory");
                exit(EXIT_FAILURE);
            }
            char **command = malloc(2 * sizeof(char*));
            if (command == NULL) {
                perror("out of memory");
                exit(EXIT_FAILURE);
            }
            command[0] = NULL;
            command[1] = NULL;
            commands[0] = command;

            if (!parseinput(buffer, commands)) {
                // if parsing fails, try again
                free(buffer);
                buffer = NULL;
                free(command);
                command = NULL;
                continue;
            }
            command = *commands;

            // We no longer need to keep the buffer in memory :)
            free(buffer);
            buffer = NULL;

            // Check if command is in /bin, and change path
            if (bincommand(command[0])) {
                if (DEBUG) {
                    printf("[DEBUG]: Command found in /bin!\n");
                }
            }

            // Command execution
            status = executecommand(command);
            // Exit command
            if (status == 2) {
                break;
            }

            // free command and args
            int i = 0;
            char *cur_command = command[i];
            while (cur_command != NULL) {
                free(cur_command);
                command[i] = NULL;
                cur_command = command[++i];
            }
            free(command);
            command = NULL;
            free(commands);
            commands = NULL;
        } else {
            free(buffer);
            buffer = NULL;
        }
    }
    exit(EXIT_SUCCESS);
}