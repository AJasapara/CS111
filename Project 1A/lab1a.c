// NAME: Arpit Jasapara
// EMAIL: ajasapara@ucla.edu
// ID: XXXXXXXXX

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

struct termios terminalRef;
struct termios terminalCurr;
int childID;
int shellFlag = 0;

void restoreReference() {
	tcsetattr(STDIN_FILENO, TCSANOW, &terminalRef);
}

void sigPipeHandler(int signum){
	if (shellFlag){
		int temp;
		waitpid(childID, &temp, 0);
		fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WIFSIGNALED(temp), WEXITSTATUS(temp));
	}
	fprintf(stderr, "SIGPIPE ERROR %d: All read ends of pipe have been closed.\n", signum);
	exit(1);
}

int main(int argc, char** argv)
{
	// Variables
	int pipeIn[2];
	int pipeOut[2];
	char buf[256];
	ssize_t bytesRead = 256;
	int count = 0;
	struct pollfd fds[2];

	// Saves terminal state
	if (!isatty (STDIN_FILENO)) {
      fprintf (stderr, "ERROR: Not a terminal.\n");
	  exit(1);
	}
	tcgetattr(STDIN_FILENO, &terminalRef);
	atexit(restoreReference);
	tcgetattr(STDIN_FILENO, &terminalCurr);
	terminalCurr.c_iflag = ISTRIP;
	terminalCurr.c_oflag = 0;
	terminalCurr.c_lflag = 0;
	tcsetattr (STDIN_FILENO, TCSANOW, &terminalCurr);
	
	// Accepts shell option
	struct option longOpts[] = {
		{"shell", no_argument, 0, 's'},
		{0,0,0,0}
	};
	int opt = getopt_long(argc,argv,"s",longOpts,NULL);
	while(opt != -1) {
		switch(opt){
			case 's': shellFlag = 1; break;
			default: 
				fprintf(stderr, "ERROR: Unrecognized argument.\nCorrect Usage: ./lab1a [--shell]\n");
				exit(1);
		}
		opt = getopt_long(argc,argv,"s",longOpts,NULL);
	}

	// Shell Code
	if (shellFlag){
		if (pipe(pipeIn) == -1 || pipe(pipeOut) == -1) {
				fprintf(stderr, "ERROR: %s\n", strerror(errno));
				exit(1);
		}
		childID = fork();
		if (childID < -1){
			fprintf(stderr, "ERROR: %s\n", strerror(errno));
			exit(1);
		}
		else if (childID == 0){
			close(pipeIn[1]);
			close(pipeOut[0]);
			close(0);
			dup(pipeIn[0]);
			close(pipeIn[0]);
			close(1);
			dup(pipeOut[1]);
			close(2);
			dup(pipeOut[1]);
			close(pipeOut[1]);
			char** temp = NULL;
			if (execvp("/bin/bash",temp) == -1){
				fprintf(stderr, "ERROR: %s\n", strerror(errno));
				exit(1);
			}
		}
		else{
			signal(SIGPIPE,sigPipeHandler);	
			close(pipeIn[0]);
			close(pipeOut[1]);
			fds[0].fd = STDIN_FILENO;
			fds[0].events = POLLIN+POLLERR+POLLHUP;
			fds[1].fd = pipeOut[0];
			fds[1].events = POLLIN+POLLERR+POLLHUP;
			while (1){
				poll(fds, 2, -1);
				if (fds[0].revents & POLLIN) {
					bytesRead = read(STDIN_FILENO, buf, 256);
					count = 0;
					while (count < bytesRead){
						if (buf[count] == 0x03){
							kill(childID, SIGINT);
							close(pipeIn[1]);
							int temp;
							waitpid(childID, &temp, 0);
							fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WIFSIGNALED(temp), WEXITSTATUS(temp));
							exit(0);
						}
						if (buf[count] == 0x04){
							close(pipeIn[1]);
							int temp;
							waitpid(childID, &temp, 0);
							fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WIFSIGNALED(temp), WEXITSTATUS(temp));
							exit(0);
						}
						if (buf[count] == '\r' || buf[count] == '\n'){
							char temp[2] = {'\r', '\n'};
							write(STDOUT_FILENO, temp, 2);
							char c = '\n';
							write(pipeIn[1],&c,1);
							count += 1;
						}
						else {
							write(pipeIn[1],&buf[count],1);
							write(STDOUT_FILENO, &buf[count], 1);
							count += 1;
						}
					}
				}
				if(fds[1].revents & POLLIN){
					bytesRead = read(pipeOut[0], buf, 256);
					count = 0;
					while (count < bytesRead){
						if (buf[count] == 0x04){
							int temp;
							waitpid(childID, &temp, 0);
							fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WIFSIGNALED(temp), WEXITSTATUS(temp));
							exit(0);
						}
						if (buf[count] == '\r' || buf[count] == '\n'){
							char temp[2] = {'\r', '\n'};
							write(STDOUT_FILENO, temp, 2);
							count += 1;
						}
						else {
							write(STDOUT_FILENO, &buf[count], 1);
							count += 1;
						}
					}
				}
				if (fds[1].revents & (POLLHUP&POLLERR)){
					int temp;
					waitpid(childID, &temp, 0);
					fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WIFSIGNALED(temp), WEXITSTATUS(temp));
					exit(1);
				}	
			}
		}
	}
	// Non-Shell Code
	else {
		while(1){
			bytesRead = read(STDIN_FILENO, buf, 256);
			count = 0;
			while (count < bytesRead){
				if (buf[count] == 0x04){
					exit(0);
				}
				if (buf[count] == '\r' || buf[count] == '\n'){
					char temp[2] = {'\r', '\n'};
					write(STDOUT_FILENO, temp, 2);
					count += 1;
				}
				else {
					write(STDOUT_FILENO, &buf[count], 1);
					count += 1;
				}
			}
		}
	}
	
}