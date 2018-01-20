// NAME: Arpit Jasapara
// EMAIL: ajasapara@ucla.edu
// ID: XXXXXXXXX

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <mraa.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>

int period = 1;
char scale = 'F';
char* logFile = NULL;
int logfd, socketfd;
mraa_aio_context adc_a0;
pthread_t threads[2];
int pauseFlag = 0;
SSL_CTX* ssl_context;
SSL * ssl;

void handler(int signum){
	fprintf(stderr,"ERROR %d: %s\n", signum, strerror(errno));
	exit(2);
}

void* tempInput(void* pid) {
	if (pid != NULL)
		return NULL;
	const int B = 4275;               
	const int R0 = 100000;
	float temperature = 0.0, r;
	struct tm* timeTm;
	time_t t;
	char tempTime[9];
	char sslBuf[15];
	while(1){
		if (!pauseFlag){
			r = (1023.0/mraa_aio_read(adc_a0)-1.0)*R0;
			temperature = 1.0/(log(r/R0)/B+1/298.15)-273.15;
			if(scale == 'F')
				temperature = temperature*9/5 + 32;
			t = time(NULL);
			timeTm = localtime(&t);
			strftime(tempTime,9,"%H:%M:%S",timeTm);
			sprintf(sslBuf, "%s %.1f\n", tempTime, temperature);
			SSL_write(ssl, sslBuf, strlen(sslBuf));
			if (logFile)
				dprintf(logfd,"%s %.1f\n", tempTime, temperature);
		}
		sleep(period);
	}
	return NULL;
}

void* servInput (void* pid) {
	struct timespec tempTime;
	tempTime.tv_nsec = 1000000;
	nanosleep(&tempTime, NULL);
	if (pid != NULL)
		return NULL;
	char buf[1024];
	ssize_t bytesRead;
	int offset = 0;
	int count = 0;
	while (1) {
		bytesRead = SSL_read(ssl, buf+offset, 32);
		ssize_t tempSize = bytesRead+offset;
		while (count < tempSize){
			if (buf[count] == '\n'){
				if (logFile)
					dprintf(logfd,"%s\n", strtok(buf+offset,"\n"));
				if(!strncmp(buf+offset, "SCALE", 5*sizeof(char))){
					switch(buf[offset+6]){
						case 'F': case 'f': scale = 'F'; break;
						case 'C': case 'c': scale = 'C'; break;
					}
				}
				if(!strncmp(buf+offset, "PERIOD", 6*sizeof(char)))
					period = atoi(strtok(buf+offset+7,"\n"));

				if(!strncmp(buf+offset, "STOP", 4*sizeof(char)))
					pauseFlag = 1;

				if(!strncmp(buf+offset, "START", 5*sizeof(char)))
					pauseFlag = 0;
				if(!strncmp(buf+offset, "LOG", 3*sizeof(char)))
					dprintf(logfd,"%s\n", strtok(buf+offset+4,"\n"));

				if(!strncmp(buf+offset, "OFF", 3*sizeof(char))) {
					pauseFlag = 1;
					struct tm* timeTm;
					time_t t;
					char tTime[9];
					char sslBuf[15];
					t = time(NULL);
					timeTm = localtime(&t);
					strftime(tTime,9,"%H:%M:%S",timeTm);
					sprintf(sslBuf, "%s SHUTDOWN\n", tTime);
					dprintf(logfd,"%s SHUTDOWN\n", tTime);
					SSL_write(ssl, sslBuf, strlen(sslBuf));						
					mraa_aio_close(adc_a0);
					SSL_shutdown(ssl);
					exit(0);
				}
				offset = count+1;
			}
			count++;
		}
		offset = count;
		if (offset > 800){
			offset = 0;
			count = 0;
		}
	}	
	return NULL;
}

int main(int argc, char **argv) {
	char* host = NULL;
	int id = 0;
	struct option longOpts[] = {
		{"period", required_argument, 0, 'p'},
		{"scale", required_argument, 0, 's'},
		{"log", required_argument, 0, 'l'},
		{"id", required_argument, 0, 'i'},
		{"host", required_argument, 0, 'h'},
		{0,0,0,0}
	};
	int opt = getopt_long(argc,argv,"p:s:l:i:h:",longOpts,NULL);
	while(opt != -1) {
		switch(opt){
			case 'p': period = atoi(optarg); break;
			case 's':
				if (strlen(optarg) == 1 && (toupper(optarg[0]) == 'C' || toupper(optarg[0]) == 'F'))
					scale=toupper(optarg[0]);
				else {
					fprintf(stderr, "ERROR: Unrecognized argument.\nCorrect Usage: ./lab4c_tls portnum --id=# --host=name --log=filename [--period=#] [--scale=[CF]]\n");
					exit(1);
				}
				break;
			case 'l': 
				logFile=optarg; 
				logfd=creat(logFile, 0644);
				if (logfd < 0){
					fprintf(stderr, "ERROR: %s\nUnable to open log file %s. Check --log argument.\n", strerror(errno), logFile);
					exit(1);
				}
				break;
			case 'i': id = atoi(optarg); break;
			case 'h': host = optarg; break;
			default: 
				fprintf(stderr, "ERROR: Unrecognized argument.\nCorrect Usage: ./lab4c_tls portnum --id=# --host=name --log=filename [--period=#] [--scale=[CF]]\n");
				exit(1);
		}
		opt = getopt_long(argc,argv,"p:s:l:i:h:",longOpts,NULL);
	}
	if (!host || !id || !logFile){
		fprintf(stderr, "ERROR: Missing argument.\nCorrect Usage: ./lab4c_tls portnum --id=# --host=name --log=filename [--period=#] [--scale=[CF]]\n");
		exit(1);
	}
	if (argv[optind] == NULL) {
		fprintf(stderr, "ERROR: Missing argument.\nCorrect Usage: ./lab4c_tls portnum --id=# --host=name --log=filename [--period=#] [--scale=[CF]]\n");
		exit(1);
	}
	int port = atoi(argv[optind]);
	struct hostent* server;
	struct sockaddr_in addr;
	char sslBuf[14];

	adc_a0 = mraa_aio_init(1);
	if (adc_a0 == NULL) {
		fprintf(stderr,"ERROR: %s\n", strerror(errno));
		exit(2);
	}
	signal(SIGINT, handler);

	socketfd=socket(AF_INET, SOCK_STREAM, 0);
	if (socketfd <0) {
		fprintf(stderr,"ERROR: %s\n", strerror(errno));
		exit(2);
	}
	server = gethostbyname(host);
	if(server == NULL) { 
		fprintf(stderr, "ERROR: %s\n", strerror(errno)); 
		exit(2); 
	}
	memset((char*) &addr,0, sizeof(addr));
	addr.sin_family = AF_INET;
	memcpy((char *) &addr.sin_addr.s_addr, (char*) server->h_addr, server->h_length);
	addr.sin_port = htons(port);
	if(connect(socketfd,(struct sockaddr *) &addr, sizeof(addr)) < 0) { 
		fprintf(stderr, "ERROR: %s\n", strerror(errno));  
		exit(2); 
	}

	if (SSL_library_init() < 0) {
	 	fprintf(stderr, "ERROR: %s\n", strerror(errno)); 
	 	exit(2);
	 }
	 OpenSSL_add_all_algorithms();
	 ssl_context = SSL_CTX_new(TLSv1_client_method());
	 if(!ssl_context) {
	 	fprintf(stderr, "ERROR: %s\n", strerror(errno)); 
	 	exit(2);
	 }
	 ssl = SSL_new(ssl_context);
	 if(!SSL_set_fd(ssl, socketfd) || SSL_connect(ssl) != 1) {
	 	fprintf(stderr, "ERROR: %s\n", strerror(errno)); 
	 	exit(2);
	 }

	sprintf(sslBuf, "ID=%d\n", id);
	SSL_write(ssl, sslBuf, strlen(sslBuf));
	dprintf(logfd, "ID=%d\n", id);

	pthread_create(&threads[0], NULL, tempInput, NULL);
	pthread_create(&threads[1], NULL, servInput, NULL);

	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
	return 0;
}