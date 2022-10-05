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

// Getting current working dir
int getdir(char *cwd) {
    if (getcwd(cwd, PATH_MAX + 1) == NULL) {
        strcpy(cwd, "NULL");
        return -1;
    }
    return 1;
}

// Reads input from STDIN, while keeping a buffer, in order to
// read "unlimited" input
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
    buffer = realloc(buffer, buffer_size + 1 * sizeof(char));
    if (buffer == NULL) {
        perror("memory allocation");
        exit(EXIT_FAILURE);
    }
    buffer[pos] = '\0';

//            printf("DEBUG: %s\n", buffer);
    bufferptr[0] = buffer;
    return 1;
}

// Parse the input from the buffer into commands
// This means parsing arguments and pipes, and checking if a command is in /bin
int parseinput(char *buffer, char ****commandsptr) {
    char ***commands = *commandsptr;

    // initialize with first command
    char **command = malloc(2 * sizeof(char *));
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
        command[i] = malloc((strlen(buffer) + 1) * sizeof(char));
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
                commands = realloc(commands, (j + 2) * sizeof(char *)); // array of commands, ptr's to command
                if (commands == NULL) {
                    perror("out of memory");
                    exit(EXIT_FAILURE);
                }
                commands[j + 1] = NULL;

                // allocate for new command (old command is kept in commands arr)
                command = malloc(2 * sizeof(char *));
                if (command == NULL) {
                    perror("out of memory");
                    exit(EXIT_FAILURE);
                }
                command[0] = NULL;
                command[1] = NULL;
            } else {
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
                        command[i] = realloc(command[i], (strlen(arg) + strlen(command[i])) * sizeof(char));
                        if (command[i] == NULL) {
                            perror("out of memory");
                            exit(EXIT_FAILURE);
                        }
                        strcat(command[i], " ");
                        strcat(command[i], arg);
                        arg = strtok(NULL, " ");
                    }

                    if (arg != NULL) {
                        command[i] = realloc(command[i], (strlen(arg) + strlen(command[i])) * sizeof(char));
                        if (command[i] == NULL) {
                            perror("out of memory");
                            exit(EXIT_FAILURE);
                        }
                        strcat(command[i], " ");
                        strcat(command[i], arg);
                        arg = strtok(NULL, " ");
                    }
                } else {
                    // add arg to command
                    command[i] = malloc((strlen(arg) + 1) * sizeof(char));
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
    // TODO: find out why getenv("PATH") causes execvp to not be able to use path
    i = 0;
    char **cur_command = commands[i];
    // Commented out due to execvp not working when getenv() is used.
    // Check checkpath() for code showing path tokenization and directory crawling
//    while (cur_command != NULL) {
//        if (!checkpath(cur_command)) {
//            if (DEBUG) {
//                printf("[DEBUG]: Did not find %s in PATH!\n", cur_command[0]);
//            }
//        }
//        cur_command = commands[++i];
//    }

    // debug printing
    if (DEBUG) {
        printf("======[ DEBUG ]======\n"
               "PARSED COMMANDS: \n");
        i = 0;
        cur_command = commands[i];
        while (cur_command != NULL) {
            printf("#%d:\n", i + 1);
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

// Function to run the command on the child, and to change the output to the STDOUT
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

    // call correct binary - https://linux.die.net/man/3/execv & https://stackoverflow.com/a/5769803
    execvp(command[0], command);
    perror("Child Error");
    exit(EXIT_FAILURE); // should not be reached
}

// Execute all the commands, recursively.
int executecommands(char ***commands, int i) {
    // if there is a command, execute it (is false at end of list)
    if (commands[i] != NULL) {
        // we take out the i'th command from the list
        char **command = commands[i];
        // if we want to change working directory
        // note: these commands were moved out of this function too, due to a "feature" we experienced on Linux
        // where we needed to put this whole function in a fork, otherwise we couldn't recover the STDIN
        if (strcmp("cd", command[0]) == 0) {
            // TODO: path support path with spaces (only arg1 is passed rn)
            if (chdir(command[1]) != 0) {
                printf("Error changing working directory using %s\n", command[1]);
            }
        } // if we want to exit the shell
        else if (strcmp("exit", command[0]) == 0) {
            return 2;
        } else {
            // piping: https://man7.org/linux/man-pages/man7/pipe.7.html
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
//                while (getchar() != EOF); // flush stdin
                // credit to https://stackoverflow.com/questions/7369286/c-passing-a-pipe-thru-execve for understanding of pipes
                dup2(pipefd[0], STDIN_FILENO); // get output from child to stdin
                close(pipefd[0]); // close input pipe
                close(pipefd[1]); // close output pipe
                return executecommands(commands, i + 1); // execute the next command
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
            } else {
                if (DEBUG) {
                    printf("[DEBUG]: Child returned: %d\n", wpid);
                }
            }
        }

//        while (!feof(stdin)) {
        char **bufferptr = calloc(1, sizeof(char *));
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

        // Read output from last child
        if (!readinput(bufferptr, 0)) {
            // if we could not read, and not caused by memory error, try again
//                while (getchar() != EOF);
            free(buffer);
            buffer = NULL;
            return 2;
        }
        while (getchar() != EOF); // flush stdin

        buffer = *bufferptr;
        free(bufferptr);
        bufferptr = NULL;

        printf("%s\n", buffer); // print output

        free(buffer);
        buffer = NULL;
    }

    return 1;
}

// Checks whether a command is in /bin or not - and if in bin, change command to fit to path
int checkpath(char **command) {
    char *executable = command[0]; // the executable/binary is the first varible

    char* path = getenv("PATH"); // load path
    if (path == NULL) {
        if (DEBUG) {
            printf("[DEBUG]: Could not read path...\n");
        }
        return -1;
    }
    else if (DEBUG) {
        printf("[DEBUG]: Path: %s\n", path);
    }

    // read list of binaries to find command
    // https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
    DIR *d;
    struct dirent *dir;

    char *arg = strtok(path, ":");
    while (arg != NULL) {
        if (DEBUG) {
            printf("[DEBUG]: Checking %s\n", arg);
        }
        d = opendir(arg);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                // TODO: replace with startswith
                //            printf("%d\n", strcmp(dir->d_name, command));
                if (strstr(dir->d_name, executable) != NULL) {
                    if (DEBUG) {
                        printf("[DEBUG]: %s found in %s!\n", executable, arg);
                    }

                    // Not needed with execvp(), since it gets the path anyway
                    char *old_command = malloc(sizeof(char) * (strlen(executable) + 1));
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

                    strcpy(executable, arg);
                    strcat(executable, "/");
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
        arg = strtok(NULL, ":");
    }

    return 0;
}

// Main function
int main() {
    // Keep going until exit command is entered, or an exception occurs
    while (true) {
        fflush(stdout); // Flush if something is stuck :)

        // Variable to keep current working directory
        char *cwd = calloc(PATH_MAX + 1, sizeof(char));
        if (cwd == NULL) {
            perror("out of memory");
            exit(EXIT_FAILURE);
        }
        getdir(cwd);  // get current working directory

        // ================ Reading command-line input ================ //

        printf("%s > ", cwd);
        fflush(stdout);
        free(cwd);
        cwd = NULL;

        char **bufferptr = malloc(sizeof(char *));
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

        // ================ Parsing input ================ //

        // Check that input exists and is above 1 (end of string char takes up 1 char)
        if (strlen(buffer) > 1) {
            // String for keeping the commands for the child to execute
            char ****commandsptr = malloc(sizeof(char *)); // prt to array of commands
            if (commandsptr == NULL) {
                perror("out of memory");
                exit(EXIT_FAILURE);
            }
            char ***commands = malloc(2 * sizeof(char *)); // array of commands, ptr's to command
            if (commands == NULL) {
                perror("out of memory");
                exit(EXIT_FAILURE);
            }
            commands[0] = NULL;
            commands[1] = NULL;
            commandsptr[0] = commands;

            if (!parseinput(buffer, commandsptr)) {
                // if parsing fails, try again
                freeeverything(commands, commandsptr, buffer, bufferptr);
                buffer = NULL;
                commands = NULL;
                commandsptr = NULL;
                continue;
            }
            commands = *commandsptr;

            // We no longer need to keep the buffer in memory :)
            free(buffer);
            buffer = NULL;

            // ================ Command execution ================//

            // if we want to change working directory
            if (strcmp("cd", commands[0][0]) == 0 || strcmp("/bin/cd", commands[0][0]) == 0) {
                // TODO: path support path with spaces (only arg1 is passed rn)
                if (chdir(commands[0][1]) != 0) {
                    printf("Error changing working directory using %s\n", commands[0][1]);
                }
            } // if we want to exit the shell
            else if (strcmp("exit", commands[0][0]) == 0 || strcmp("/bin/exit", commands[0][0]) == 0) {
                freeeverything(commands, commandsptr, buffer, bufferptr);
                exit(EXIT_SUCCESS);
            } else {
                // Forking https://man7.org/linux/man-pages/man2/wait.2.html
                // we fork because on Linux it didn't want to open STDIN after we changed it, so let's leave it unchanged...
                pid_t cpid, w;
                int wstatus;

                cpid = fork();
                if (cpid == -1) {
                    perror("fork");
                    freeeverything(commands, commandsptr, buffer, bufferptr);
                    exit(EXIT_FAILURE);
                }
                if (cpid == 0) { // child
                    executecommands(commands, 0);
                    freeeverything(commands, commandsptr, buffer, bufferptr);
                    exit(EXIT_SUCCESS);
                } else { // child
                    do {
                        w = waitpid(cpid, &wstatus, WUNTRACED | WCONTINUED);
                        if (w == -1) {
                            perror("waitpid");
                            freeeverything(commands, commandsptr, buffer, bufferptr);
                            exit(EXIT_FAILURE);
                        }
                    } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
                }
            }

            // free commands and args
            freeeverything(commands, commandsptr, buffer, bufferptr);
            commands = NULL;
            commandsptr = NULL;
        } else {
            freeeverything(NULL, NULL, buffer, bufferptr);
            buffer = NULL;
            bufferptr = NULL;
        }
    }
    exit(EXIT_SUCCESS);
}

int freeeverything(char ***commands, char ****commandsptr, char *buffer, char **bufferptr) {
    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }
    if (bufferptr != NULL) {
        free(bufferptr);
        bufferptr = NULL;
    }
    // free parsed commands
    if (commands != NULL) {
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
    }
    if (commandsptr != NULL) {
        free(commandsptr);
        commandsptr = NULL;
    }
    return 1;
}