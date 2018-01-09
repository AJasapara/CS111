// NAME: Arpit Jasapara
// EMAIL: ajasapara@ucla.edu
// ID: XXXXXXXXX

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>


void handler(int signum){

	fprintf(stderr,"SIGSEGV ERROR %d: Segmentation Fault! Fault caught by --catch option.\n", signum);
	exit(4);
}

int main(int argc, char** argv){
	struct option longOpts[] = {
		{"input", required_argument, 0, 1},
		{"output", required_argument, 0, 2},
		{"segfault", no_argument, 0, 3},
		{"catch", no_argument, 0, 4},
		{0,0,0,0}
	};
	char* inFile = NULL;
	char* outFile = NULL;
	int segFault = 0;
	int opt = getopt_long(argc,argv,"",longOpts,NULL);
	while(opt != -1) {
		switch (opt){
			case 1:
				inFile = optarg; break;
			case 2:
				outFile = optarg; break;
			case 3:
				segFault = 1; break;
			case 4:
				signal(SIGSEGV, handler); break;
			default:
				fprintf(stderr, "ERROR: Unrecognized argument.\nCorrect Usage: ./lab0 [--input=filename] [--output=filename] [--segfault] [--catch]\n");
				exit(1);
		}
		opt = getopt_long(argc,argv,"",longOpts,NULL);
	}

	if (inFile != NULL){
		int ifd = open(inFile, O_RDONLY);
		if (ifd >= 0) {
			close(0);
			dup(ifd);
			close(ifd);
		}
		else {
			fprintf(stderr, "ERROR: %s\nUnable to open input file %s. Check --input argument.\n", strerror(errno), inFile);
			exit(2);
		}
	}

	if (outFile != NULL){
		int ofd = creat(outFile, 0644);
		if (ofd >= 0) {
			close(1);
			dup(ofd);
			close(ofd);
		}
		else {
			fprintf(stderr, "ERROR: %s\nUnable to open output file %s. Check --output argument.\n", strerror(errno), outFile);
			exit(3);
		}
	}

	if(segFault){
		char* temp = NULL;
		*temp = 'X';
	}

	char* buf = (char*) malloc(1024);
	ssize_t bytesRead = 1024;
	while(bytesRead > 0){
		bytesRead = read(STDIN_FILENO, buf, 1024);
		write(STDOUT_FILENO, buf, bytesRead);
	}
	exit(0);
}