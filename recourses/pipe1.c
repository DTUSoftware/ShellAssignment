#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {

    int pipefd[2];
    int pid;
    char recv[32];

    pipe(pipefd);

    switch(pid= fork()) {
    case -1: perror("fork");
	     exit(1);
    
    case 0:    // in child process
	close(pipefd[0]);       //close reading pipefd
	FILE *out = fdopen(pipefd[1], "w"); // ope pipe as stream for writing
	fprintf (out, "Howyoudoing(childpid:%d)\n", (int) getpid()); // write to stream
	break;
   default:               // in parent process
	close(pipefd[1]);      	//close	writing	pipefd
        FILE *in = fdopen(pipefd[0], "r"); // ope pipe as stream for reading
        fscanf	(in, "%s", recv); // write to stream
        printf (" Hello parent (pid:%d) received %s\n",(int)getpid(), recv);
	break;
 	
     }
}
