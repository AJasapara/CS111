// NAME: Arpit Jasapara
// EMAIL: ajasapara@ucla.edu
// ID: XXXXXXXXX

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <mcrypt.h>

int childID;

void sigPipeHandler(int signum){
	int temp;
	waitpid(childID, &temp, 0);
	fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WIFSIGNALED(temp), WEXITSTATUS(temp));
	fprintf(stderr, "SIGPIPE ERROR %d: All read ends of pipe have been closed.\n", signum);
	exit(1);
}

int main(int argc, char** argv)
{
	int pipeIn[2];
	int pipeOut[2];
	char buf[256];
	ssize_t bytesRead = 256;
	int count = 0;
	struct pollfd fds[2];
	int port;
	int socketfd, commfd;
	struct sockaddr_in saddr;
	struct sockaddr_in caddr;
	socklen_t clen;
	int encryptFlag = 0;
	MCRYPT cfd, dfd;
	char* key;
	int keyLen;

	struct option longOpts[] = {
		{"port", required_argument, 0, 'p'},
		{"encrypt", required_argument, 0, 'e'},
		{0,0,0,0}
	};
	int opt = getopt_long(argc,argv,"p:e:",longOpts,NULL);
	while(opt != -1) {
		switch(opt){
			case 'p': port=atoi(optarg); break;
			case 'e':
				encryptFlag = 1;
				struct stat keyFileStat;
				int kfd = open(optarg, O_RDONLY);
				fstat(kfd, &keyFileStat);
				keyLen = keyFileStat.st_size;
				key = (char*) malloc(keyLen * sizeof(char));
				read(kfd, key, keyLen);
				break;
			default: 
				fprintf(stderr, "ERROR: Unrecognized argument.\nCorrect Usage: ./lab1b-server --port=num [--encrypt=filename]\n");
				exit(1);
		}
		opt = getopt_long(argc,argv,"p:e:",longOpts,NULL);
	}

	socketfd=socket(AF_INET, SOCK_STREAM, 0);
	if (socketfd <0) {
		fprintf(stderr,"ERROR: %s\n", strerror(errno));
		exit(1);
	}
	memset((char*) &saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port = port;
	bind(socketfd, (struct sockaddr *) &saddr, sizeof(saddr));
	listen(socketfd, 3);
	clen = sizeof(caddr);
	commfd = accept(socketfd, (struct sockaddr *) &caddr, &clen);
	if (commfd < 0) {
		fprintf(stderr,"ERROR: %s\n", strerror(errno));
		exit(1);
	}

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
		if (encryptFlag) {
			cfd = mcrypt_module_open("twofish", NULL, "cfb", NULL);
			mcrypt_generic_init(cfd, key, keyLen, NULL);
			dfd = mcrypt_module_open("twofish", NULL, "cfb", NULL);
			mcrypt_generic_init(dfd, key, keyLen, NULL);
		}
		fds[0].fd = commfd;
		fds[0].events = POLLIN+POLLERR+POLLHUP;
		fds[1].fd = pipeOut[0];
		fds[1].events = POLLIN+POLLERR+POLLHUP;
		while (1){
			poll(fds, 2, -1);
			if (fds[0].revents & POLLIN) {
				bytesRead = read(commfd, buf, 256);
				count = 0;
				while (count < bytesRead){
					if (encryptFlag){
						mdecrypt_generic(dfd, &buf[count], 1);
					}
					if (buf[count] == 0x03){
						kill(childID, SIGINT);
						close(pipeIn[1]);
					}
					if (buf[count] == 0x04){
						close(pipeIn[1]);
					}
					write(pipeIn[1],&buf[count],1);
					count += 1;
				}
			}
			if(fds[1].revents & POLLIN){
				bytesRead = read(pipeOut[0], buf, 256);
				count = 0;
				while (count < bytesRead){
					if(encryptFlag){
						mcrypt_generic(cfd, &buf[count], 1);
					}
					write(commfd, &buf[count], 1);
					count += 1;
				}
			}
			if (fds[1].revents & (POLLHUP|POLLERR)){
				int temp;
				close(pipeOut[0]);
				waitpid(childID, &temp, 0);
				fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(temp), WEXITSTATUS(temp));
				close(commfd);
				close(socketfd);
				if(encryptFlag){
					mcrypt_generic_deinit(cfd);
					mcrypt_module_close(cfd);
					mcrypt_generic_deinit(dfd);
					mcrypt_module_close(dfd);
				}
				free(key);
				exit(0);
			}	
		}
	}
}