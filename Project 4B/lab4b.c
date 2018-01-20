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

int period = 1;
char scale = 'F';
char* logFile = NULL;
int logfd;
mraa_aio_context adc_a0;
mraa_gpio_context gpio;
pthread_t threads[2];
int pauseFlag = 0;

void handler(int signum){
	fprintf(stderr,"ERROR %d: %s\n", signum, strerror(errno));
	exit(1);
}

void* tempInput(void* id) {
	if (id != NULL)
		return NULL;
	const int B = 4275;               
	const int R0 = 100000;
	float temperature = 0.0, r;
	struct tm* timeTm;
	time_t t;
	char tempTime[9];
	while(1){
		if (!pauseFlag){
			r = (1023.0/mraa_aio_read(adc_a0)-1.0)*R0;
			temperature = 1.0/(log(r/R0)/B+1/298.15)-273.15;
			if(scale == 'F')
				temperature = temperature*9/5 + 32;
			t = time(NULL);
			timeTm = localtime(&t);
			strftime(tempTime,9,"%H:%M:%S",timeTm);
			printf("%s %.1f\n", tempTime, temperature);
			if (logFile)
				dprintf(logfd,"%s %.1f\n", tempTime, temperature);
		}
		sleep(period);
	}
	return NULL;
}

void* stdInput (void* id) {
	struct timespec tempTime;
	tempTime.tv_nsec = 1000000;
	nanosleep(&tempTime, NULL);
	if (id != NULL)
		return NULL;
	char buf[1024];
	ssize_t bytesRead;
	int offset = 0;
	int count = 0;
	while (1) {
		bytesRead = read(STDIN_FILENO, buf+offset, 32);
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

				if(!strncmp(buf+offset, "OFF", 3*sizeof(char))) {
					pauseFlag = 1;
					struct tm* timeTm;
					time_t t;
					char tTime[9];
					t = time(NULL);
					timeTm = localtime(&t);
					strftime(tTime,9,"%H:%M:%S",timeTm);
					printf("%s SHUTDOWN\n", tTime);
					if (logFile)
						dprintf(logfd,"%s SHUTDOWN\n", tTime);
					mraa_aio_close(adc_a0);
					mraa_gpio_close(gpio);
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

void buttonInput (void* id) {
	if (id != NULL)
		return;
	pauseFlag = 1;
	struct tm* timeTm;
	time_t t;
	char tempTime[9];
	t = time(NULL);
	timeTm = localtime(&t);
	strftime(tempTime,9,"%H:%M:%S",timeTm);
	printf("%s SHUTDOWN\n", tempTime);
	if (logFile)
		dprintf(logfd,"%s SHUTDOWN\n", tempTime);
	mraa_aio_close(adc_a0);
	mraa_gpio_close(gpio);
	exit(0);
}

int main(int argc, char **argv) {	
	struct option longOpts[] = {
		{"period", required_argument, 0, 'p'},
		{"scale", required_argument, 0, 's'},
		{"log", required_argument, 0, 'l'},
		{0,0,0,0}
	};
	int opt = getopt_long(argc,argv,"p:s:l:",longOpts,NULL);
	while(opt != -1) {
		switch(opt){
			case 'p': period = atoi(optarg); break;
			case 's':
				if (strlen(optarg) == 1 && (toupper(optarg[0]) == 'C' || toupper(optarg[0]) == 'F'))
					scale=toupper(optarg[0]);
				else {
					fprintf(stderr, "ERROR: Unrecognized argument.\nCorrect Usage: ./lab4b [--period=#] [--scale=[CF]] [--log=filename]\n");
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
			default: 
				fprintf(stderr, "ERROR: Unrecognized argument.\nCorrect Usage: ./lab4b [--period=#] [--scale=[CF]] [--log=filename]\n");
				exit(1);
		}
		opt = getopt_long(argc,argv,"p:s:l:",longOpts,NULL);
	}

	adc_a0 = mraa_aio_init(1);
	if (adc_a0 == NULL) {
		fprintf(stderr,"ERROR: %s\n", strerror(errno));
		exit(1);
	}
	gpio = mraa_gpio_init(62);
	if (gpio == NULL) {
		fprintf(stderr,"ERROR: %s\n", strerror(errno));
		exit(1);
	}
	mraa_gpio_dir(gpio, MRAA_GPIO_IN);
	signal(SIGINT, handler);

	mraa_gpio_isr(gpio, MRAA_GPIO_EDGE_RISING, buttonInput, NULL);
	pthread_create(&threads[0], NULL, tempInput, NULL);
	pthread_create(&threads[1], NULL, stdInput, NULL);

	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
	return 0;
}