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
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <mcrypt.h>

struct termios terminalRef;
struct termios terminalCurr;

void restoreReference() {
	tcsetattr(STDIN_FILENO, TCSANOW, &terminalRef);
}

int main(int argc, char** argv)
{
	char buf[256];
	ssize_t bytesRead = 256;
	int count = 0;
	struct pollfd fds[2];
	int port;
	char* log = NULL;
	int logfd;
	int socketfd;
	struct hostent* server;
	struct sockaddr_in addr;
	int encryptFlag = 0;
	MCRYPT cfd, dfd;
	char* key;
	int keyLen;

	struct option longOpts[] = {
		{"port", required_argument, 0, 'p'},
		{"log", required_argument, 0, 'l'},
		{"encrypt", required_argument, 0, 'e'},
		{0,0,0,0}
	};
	int opt = getopt_long(argc,argv,"p:l:e:",longOpts,NULL);
	while(opt != -1) {
		switch(opt){
			case 'p': port=atoi(optarg); break;
			case 'l': 
				log=optarg; 
				logfd=creat(log, 0644);
				if (logfd < 0){
					fprintf(stderr, "ERROR: %s\nUnable to open log file %s. Check --output argument.\n", strerror(errno), log);
					exit(1);
				}
				break;
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
				fprintf(stderr, "ERROR: Unrecognized argument.\nCorrect Usage: ./lab1b-client --port=num [--log=filename] [--encrypt=filename]\n");
				exit(1);
		}
		opt = getopt_long(argc,argv,"p:l:e:",longOpts,NULL);
	}

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

	socketfd=socket(AF_INET, SOCK_STREAM, 0);
	if (socketfd <0) {
		fprintf(stderr,"ERROR: %s\n", strerror(errno));
		exit(1);
	}
	server = gethostbyname("localhost");
	if(server == NULL) { 
		fprintf(stderr, "ERROR: %s\n", strerror(errno)); 
		exit(1); 
	}
	memset((char*) &addr,0, sizeof(addr));
	addr.sin_family = AF_INET;
	memcpy((char *) &addr.sin_addr.s_addr, (char*) server->h_addr, server->h_length);
	addr.sin_port = port;
	int connection = connect(socketfd,(struct sockaddr *) &addr, sizeof(addr));
	if(connection < 0) { 
		fprintf(stderr, "ERROR: %s\n", strerror(errno));  
		exit(1); 
	}
	if (encryptFlag) {
		cfd = mcrypt_module_open("twofish", NULL, "cfb", NULL);
		mcrypt_generic_init(cfd, key, keyLen, NULL);
		dfd = mcrypt_module_open("twofish", NULL, "cfb", NULL);
		mcrypt_generic_init(dfd, key, keyLen, NULL);
	}
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN+POLLERR+POLLHUP;
	fds[1].fd = socketfd;
	fds[1].events = POLLIN+POLLERR+POLLHUP;
	while (1){
		poll(fds, 2, -1);
		if (fds[0].revents & POLLIN) {
			bytesRead = read(STDIN_FILENO, buf, 256);
			count = 0;
			while (count < bytesRead){
				if (buf[count] == '\r' || buf[count] == '\n'){
					char temp[2] = {'\r', '\n'};
					write(STDOUT_FILENO, temp, 2);
					buf[count] = '\n';
					if(encryptFlag){
						mcrypt_generic(cfd, &buf[count], 1);
					}
					write(socketfd, &buf[count], 1);
					count += 1;
				}
				else {
					write(STDOUT_FILENO, &buf[count], 1);
					if(encryptFlag){
						mcrypt_generic(cfd, &buf[count], 1);
					}
					write(socketfd,&buf[count],1);
					count += 1;
				}
			}
			if(log){
				dprintf(logfd, "SENT %ld bytes: ", bytesRead);
				write(logfd, buf, bytesRead);
				dprintf(logfd, "\n");
			}
		}
		if(fds[1].revents & POLLIN){
			bytesRead = read(socketfd, buf, 256);
			if(bytesRead == 0) {
				close(socketfd);
				if(encryptFlag){
					mcrypt_generic_deinit(cfd);
					mcrypt_module_close(cfd);
					mcrypt_generic_deinit(dfd);
					mcrypt_module_close(dfd);
				}
				exit(0);
			}
			if(log){
				dprintf(logfd, "RECEIVED %ld bytes: ", bytesRead);
				write(logfd, buf, bytesRead);
				dprintf(logfd, "\n");
			}
			count = 0;
			while (count < bytesRead){
				if (encryptFlag){
					mdecrypt_generic(dfd, &buf[count], 1);
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
		if (fds[1].revents & (POLLHUP|POLLERR)){
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