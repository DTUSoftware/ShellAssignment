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

int main() {
    char cwd[PATH_MAX+1];
    while (true) {
        fflush(stdout); // Flush if something is stuck :)

        // Getting current working dir
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            strcpy(cwd, "NULL");
        }

        // Reading command-line input
        // We tried using getline(), but it wasn't in MingW, and stumbled upon this implementation that
        // could not really be altered - so credit where credit is due.
        // thanks to https://gist.github.com/btmills/4201660
        printf("%s > ", cwd);
        fflush(stdout);

        unsigned int buffer_size = BUFFERSIZE;
        char *buffer = (char *) malloc(buffer_size);
        if (buffer != NULL) {
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
            buffer[pos] = '\0';
            buffer_size = ++pos;
            buffer = (char *) realloc(buffer, buffer_size);

//            printf("DEBUG: %s\n", buffer);
        } else {
            perror("memory allocation");
            exit(EXIT_FAILURE);
        }

        // Check that input exists and is above 1 (end of string char takes up 1 char)
        if (buffer != NULL && buffer_size > 1) {
            // String parsing
            char *command = (char *) malloc(buffer_size);
            char *args = NULL;

            // if arguments are parsed
            if (strstr(buffer, " ") != NULL) {
                char *arg = strtok(buffer, " ");
                if (arg == NULL) {
                    perror("failed to parse");
                    exit(EXIT_FAILURE);
                }

                // load command
                sscanf(arg, "%s", command);
                arg = strtok(NULL, " ");
                command = (char *) realloc(command, sizeof(char) * strlen(command));

                // load args
                // where completely and utterly retarded at this point but if it works it works :) not overly
                // complicated at all might fix later :) 
                args = (char *) malloc(buffer_size - strlen(command));
                int i = 0;
                while (arg != NULL) {
                    if (i == 0) {
                        sscanf(arg, "%s", args);
                    }
                    else {
                        // this is a very elegant solution
                        // modern problems require modern solutions
                        strcat(args, " ");
                        strcat(args, arg);
                    }
                    i++;
                    arg = strtok(NULL, " ");
                }
            }
            else {
                sscanf(buffer, "%s", command);
            }
            // We no longer need to keep the buffer in memory :)
            free(buffer);

            // if we want to change working directory
            if (strcmp("cd", command) == 0) {
                if (chdir(args) != 0) {
                    printf("Error changing working directory using %s\n", args);
                }
                // free command and args
                free(command);
                free(args);
            }
            else {
                // read list of binaries to find command
                // https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
                DIR *d;
                struct dirent *dir;
                d = opendir("/bin");
                if (d) {
                    while ((dir = readdir(d)) != NULL) {
                        // TODO: replace with startswith
//                    printf("%d\n", strcmp(dir->d_name, command));
                        if (strstr(dir->d_name, command) != NULL) {
                            printf("[DEBUG]: Command found in /bin!\n");
                            char *old_command = command;
                            command = (char *) malloc(sizeof(char*) * strlen(command) + sizeof("/bin/"));
                            strcpy(command, "/bin/");
                            strncat(command, old_command, strlen(old_command));
                            free(old_command);
                            break;
                        }
//                    printf("[DEBUG]: %s\n", dir->d_name);
                    }
                    closedir(d);
                }

                // Command execution
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
                    if (args != NULL) {
                        printf("[DEBUG]: Args: %s\n", args);
                    }
                    execl(command, command, args);
                    perror("Child Error");
                    exit(EXIT_FAILURE); // not reached
                } else { // parent code
                    // free command and args
                    free(command);
                    free(args);

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
        } else {
            free(buffer);
        }
    }
    exit(EXIT_SUCCESS);
}