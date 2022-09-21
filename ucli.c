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

int parseinput(char *buffer, char *command, char *args) {
    // if arguments are parsed
    if (strstr(buffer, " ") != NULL) {
        char *arg = strtok(buffer, " ");
        if (arg == NULL) {
//            perror("failed to parse");
//            exit(EXIT_FAILURE);
            return -1;
        }

        // load command
        sscanf(arg, "%s", command);
        arg = strtok(NULL, " ");
        command = (char *) realloc(command, sizeof(char) * strlen(command));
        if (command == NULL) {
            perror("out of memory");
            exit(EXIT_FAILURE);
        }

        // load args
        // where completely and utterly retarded at this point but if it works it works :) not overly
        // complicated at all might fix later :)
        args = (char *) realloc(args, sizeof(char) * (strlen(buffer) - strlen(command)));
        if (args == NULL) {
            perror("out of memory");
            exit(EXIT_FAILURE);
        }
        int i = 0;
        while (arg != NULL) {
            if (i == 0) {
                sscanf(arg, "%s", args);
            } else {
                // this is a very elegant solution
                // modern problems require modern solutions
                strcat(args, " ");
                strcat(args, arg);
            }
            i++;
            arg = strtok(NULL, " ");
        }
    } else {
        sscanf(buffer, "%s", command);
    }
    return 1;
}

int executecommand(char *command, char *args) {
    // if we want to change working directory
    if (strcmp("cd", command) == 0) {
        if (chdir(args) != 0) {
            printf("Error changing working directory using %s\n", args);
        }
    }
        // if we want to exit the shell
    else if (strcmp("exit", command) == 0) {
        return 2;
    } else {
        // forking: https://man7.org/linux/man-pages/man2/wait.2.html
        pid_t cpid, w;
        int wstatus;
        cpid = fork();
        // command the parent should run
        if (cpid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (cpid == 0) { // child code
            printf("[DEBUG]: Command: %s\n", command);

            // call correct binary
            if (args != NULL && args[0] != '\0') {
                printf("[DEBUG]: Args: %s\n", args);
                execl(command, command, args);
            } else {
                execl(command, command, NULL);
            }
            perror("Child Error");
            exit(EXIT_FAILURE); // not reached
        } else { // parent code
            // https://man7.org/linux/man-pages/man2/wait.2.html
            do {
                w = waitpid(cpid, &wstatus, WUNTRACED | WCONTINUED);
                if (w == -1) {
                    perror("waitpid");
                    exit(EXIT_FAILURE);
                }

//                        if (WIFEXITED(wstatus)) {
//                            printf("exited, status=%d\n", WEXITSTATUS(wstatus));
//                        } else if (WIFSIGNALED(wstatus)) {
//                            printf("killed by signal %d\n", WTERMSIG(wstatus));
//                        } else if (WIFSTOPPED(wstatus)) {
//                            printf("stopped by signal %d\n", WSTOPSIG(wstatus));
//                        } else if (WIFCONTINUED(wstatus)) {
//                            printf("continued\n");
//                        }
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
            char *command = calloc(strlen(buffer), sizeof(char));
            if (command == NULL) {
                perror("out of memory");
                exit(EXIT_FAILURE);
            }

            // String for keeping other arguments
            char *args = calloc(1, sizeof(char));
            if (args == NULL) {
                perror("out of memory");
                exit(EXIT_FAILURE);
            }

            if (!parseinput(buffer, command, args)) {
                // if parsing fails, try again
                free(buffer);
                buffer = NULL;
                free(command);
                command = NULL;
                free(args);
                args = NULL;
                continue;
            }

            // We no longer need to keep the buffer in memory :)
            free(buffer);
            buffer = NULL;

            // Check if command is in /bin, and change path
            if (bincommand(command)) {
                printf("[DEBUG]: Command found in /bin!\n");
            }

            // Command execution
            status = executecommand(command, args);
            // Exit command
            if (status == 2) {
                break;
            }

            // free command and args
            free(command);
            command = NULL;
            free(args);
            args = NULL;
        } else {
            free(buffer);
            buffer = NULL;
        }
    }
    exit(EXIT_SUCCESS);
}