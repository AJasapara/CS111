// NAME: Arpit Jasapara
// EMAIL: ajasapara@ucla.edu
// ID: XXXXXXXXX

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <errno.h>

long long counter = 0;
int opt_yield = 0;
int spinFlag = 0;
int m = 0, s = 0, c = 0;
pthread_mutex_t pmt;

void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    if (opt_yield)
    	sched_yield();
    *pointer = sum;
}

void addMutex(long long *pointer, long long value){
	pthread_mutex_lock(&pmt);
	long long sum = *pointer + value;
    if (opt_yield)
    	sched_yield();
    *pointer = sum;
	pthread_mutex_unlock(&pmt);
}

void addSpin(long long *pointer, long long value){
	while(__sync_lock_test_and_set(&spinFlag, 1));
	long long sum = *pointer + value;
    if (opt_yield)
    	sched_yield();
    *pointer = sum;
    __sync_lock_release(&spinFlag);
}

void addCompare(long long *pointer, long long value){
	long long temp, sum;
	do
	{
		temp = *pointer;
		sum = temp + value;
		if (opt_yield)
			sched_yield();
	} while(__sync_val_compare_and_swap(pointer, temp, sum) != temp);
}

void* addRedirect(void* iterations){
	int iter = *((int*)iterations);
	for(int i = 0; i < iter; i++){
		if (s) 
			addSpin(&counter, 1);
		else if(c)
			addCompare(&counter,1);
		else if (m)
			addMutex(&counter, 1);
		else
			add(&counter, 1);
	}
	for(int i = 0; i < iter; i++){
		if (s) 
			addSpin(&counter, -1);
		else if(c)
			addCompare(&counter,-1);
		else if (m)
			addMutex(&counter, -1);
		else
			add(&counter, -1);
	}
	return NULL;
}

int main(int argc, char **argv) {
	int numThreads = 1;
	int iterations = 1;
	pthread_t* threads; 
	struct timespec begin, end;
	char* msg = "add-none";
	struct option longOpts[] = {
		{"threads", required_argument, 0, 't'},
		{"iterations", required_argument, 0, 'i'},
		{"yield", no_argument, 0, 'y'},
		{"sync", required_argument, 0, 's'},
		{0,0,0,0}
	};
	int opt = getopt_long(argc,argv,"t:i:ys:",longOpts,NULL);
	while(opt != -1) {
		switch(opt){
			case 't': 
				numThreads = atoi(optarg); 
				threads = (pthread_t*) malloc(numThreads * sizeof(pthread_t));
				break;
			case 'i': iterations=atoi(optarg); break;
			case 'y': opt_yield = 1; break;
			case 's':
				if(!strcmp(optarg,"s"))
					s = 1;
				else if(!strcmp(optarg,"c"))
					c = 1;
				else if (!strcmp(optarg,"m")) {
					m = 1;
					pthread_mutex_init(&pmt, NULL);
				}
				else{
					fprintf(stderr, "ERROR: Unrecognized argument.\nCorrect Usage: ./lab2_add --iterations=# --threads=# [--yield] [--sync=[scm]]\n");
					exit(1);
				}
				break;
			default: 
				fprintf(stderr, "ERROR: Unrecognized argument.\nCorrect Usage: ./lab2_add --iterations=# --threads=# [--yield] [--sync=[scm]]\n");
				exit(1);
		}
		opt = getopt_long(argc,argv,"t:i:ys:",longOpts,NULL);
	}
	if (opt_yield){
		msg = "add-yield-none";
		if (s) msg = "add-yield-s";
		if (c) msg = "add-yield-c";
		if (m) msg = "add-yield-m";
	}
	else{
		if (s) msg = "add-s";
		if (c) msg = "add-c";
		if (m) msg = "add-m";
	}

	if(clock_gettime(CLOCK_MONOTONIC, &begin) == -1){
		fprintf(stderr,"ERROR: %s\n", strerror(errno));
		exit(1);
	}
	for (int i = 0; i < numThreads; i++){
		if (pthread_create(&threads[i], NULL, addRedirect, &iterations)){
			fprintf(stderr,"ERROR: %s\n", strerror(errno));
			exit(1);
		}
	}
	for (int i = 0; i < numThreads; i++){
		if (pthread_join(threads[i], NULL)){
			fprintf(stderr,"ERROR: %s\n", strerror(errno));
			exit(1);
		}
	}
	if(clock_gettime(CLOCK_MONOTONIC, &end) == -1){
		fprintf(stderr,"ERROR: %s\n", strerror(errno));
		exit(1);
	}
	int time = (1000000000 * (end.tv_sec - begin.tv_sec)) + (end.tv_nsec - begin.tv_nsec);
	printf("%s,%d,%d,%d,%d,%d,%lld\n", msg, numThreads, iterations, (numThreads*iterations*2), time, time/(numThreads*iterations*2), counter);
	free(threads);
	pthread_mutex_destroy(&pmt);
}