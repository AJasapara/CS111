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
#include <signal.h>
#include <fcntl.h>
#include "SortedList.h"

int opt_yield = 0;
int* spinFlag;
int m = 0, s = 0;
int iterations = 1;
pthread_mutex_t* pmt;
SortedListElement_t** arr;
SortedList_t** list;
int* threadTimes;
int numLists = 1;

void segFaultHandler(int signum){
	fprintf(stderr,"ERROR %d: %s\n", signum, strerror(errno));
	exit(2);
}

void* listRedirect(void* threadID) {
	struct timespec beginThread, endThread;
	int id = *((int*)threadID);
	for (int j = 0; j < iterations; j++){
		int hash = abs((int) *arr[id][j].key) % numLists;
		if (m) {		
			clock_gettime(CLOCK_MONOTONIC, &beginThread);
			pthread_mutex_lock(&pmt[hash]);
			clock_gettime(CLOCK_MONOTONIC, &endThread);
			threadTimes[id] += (1000000000 * (endThread.tv_sec - beginThread.tv_sec)) + (endThread.tv_nsec - beginThread.tv_nsec);
			SortedList_insert(list[hash], &arr[id][j]);
			pthread_mutex_unlock(&pmt[hash]);
		}
		else if(s) {
			while(__sync_lock_test_and_set(&spinFlag[hash], 1));
			SortedList_insert(list[hash], &arr[id][j]);
			__sync_lock_release(&spinFlag[hash]);
		}
		else
			SortedList_insert(list[hash], &arr[id][j]);
	}

	int threadLen = 0;
	if (m)	
		for (int i = 0; i < numLists; i++){
			clock_gettime(CLOCK_MONOTONIC, &beginThread);
			pthread_mutex_lock(&pmt[i]);
			clock_gettime(CLOCK_MONOTONIC, &endThread);
			threadTimes[id] += (1000000000 * (endThread.tv_sec - beginThread.tv_sec)) + (endThread.tv_nsec - beginThread.tv_nsec);
			threadLen += SortedList_length(list[i]);
			pthread_mutex_unlock(&pmt[i]);	
		}
	else if(s) 
		for (int i = 0; i < numLists; i++){
			while(__sync_lock_test_and_set(&spinFlag[i], 1));
			threadLen += SortedList_length(list[i]);
			__sync_lock_release(&spinFlag[i]);
		}
	else
		for (int i = 0; i < numLists; i++)
			threadLen += SortedList_length(list[i]);

	if (threadLen < 0) {
		fprintf(stderr,"ERROR: %s\n", strerror(errno));
		exit(2);
	}

	SortedListElement_t* temp;
	for (int j = 0; j < iterations; j++){
		int hash = abs((int) *arr[id][j].key) % numLists;
		if (m) {
			clock_gettime(CLOCK_MONOTONIC, &beginThread);
			pthread_mutex_lock(&pmt[hash]);
			clock_gettime(CLOCK_MONOTONIC, &endThread);
			threadTimes[id] += (1000000000 * (endThread.tv_sec - beginThread.tv_sec)) + (endThread.tv_nsec - beginThread.tv_nsec);
			temp = SortedList_lookup(list[hash], arr[id][j].key);
			SortedList_delete(temp);
			pthread_mutex_unlock(&pmt[hash]);
		}
		else if(s) {
			while(__sync_lock_test_and_set(&spinFlag[hash], 1));
			temp = SortedList_lookup(list[hash], arr[id][j].key);
			SortedList_delete(temp);
			__sync_lock_release(&spinFlag[hash]);
		}
		else {
			temp = SortedList_lookup(list[hash], arr[id][j].key);
			SortedList_delete(temp);
		}

	}

	return NULL;
}

int main(int argc, char **argv) {
	int numThreads = 1;
	pthread_t* threads; 
	int* threadIDs;
	struct timespec begin, end;
	char* msg1 = malloc(14*sizeof(char));
	memset(msg1, '\0', 14*sizeof(char));
	char* msg2 = "none";
	char* msg3 = "-none";
	int iflag = 0, dflag = 0, lflag = 0;
	struct option longOpts[] = {
		{"threads", required_argument, 0, 't'},
		{"iterations", required_argument, 0, 'i'},
		{"lists", required_argument, 0, 'l'},
		{"yield", required_argument, 0, 'y'},
		{"sync", required_argument, 0, 's'},
		{0,0,0,0}
	};
	int opt = getopt_long(argc,argv,"t:i:l:y:s:",longOpts,NULL);
	while(opt != -1) {
		switch(opt){
			case 't': 
				numThreads = atoi(optarg); 
				threads = (pthread_t*) malloc(numThreads * sizeof(pthread_t));
				threadIDs = (int*) malloc(numThreads * sizeof(int));
				break;
			case 'i': iterations=atoi(optarg); break;
			case 'l': numLists=atoi(optarg); break;
			case 'y': 
				if(strchr(optarg, 'i')){
					opt_yield |= INSERT_YIELD;
					msg2 = "i";
					iflag = 1;
				}
				if(strchr(optarg, 'd')){
					opt_yield |= DELETE_YIELD;
					dflag = 1;
					if(iflag)
						msg2 = "id";
					else
						msg2 = "d";
				}
				if(strchr(optarg, 'l')){
					opt_yield |= LOOKUP_YIELD;
					lflag = 1;
					if (iflag && dflag)
						msg2 = "idl";
					else if (iflag)
						msg2 = "il";
					else if (dflag)
						msg2 = "dl";
					else
						msg2 = "l";
				}
				if(!iflag && !dflag && !lflag){
					fprintf(stderr, "ERROR: Unrecognized argument.\nCorrect Usage: ./lab2_list --iterations=# --threads=# --lists=# [--yield=[idl]] [--sync=[sm]]\n");
					exit(1);
				}
				break;
			case 's':
				if(!strcmp(optarg,"s")){
					s = 1;
					msg3 = "-s";
				}
				else if (!strcmp(optarg,"m")) {
					m = 1;
					msg3 = "-m";
				}
				else{
					fprintf(stderr, "ERROR: Unrecognized argument.\nCorrect Usage: ./lab2_add --iterations=# --threads=# --lists=# [--yield=[idl]] [--sync=[sm]]\n");
					exit(1);
				}
				break;
			default: 
				fprintf(stderr, "ERROR: Unrecognized argument.\nCorrect Usage: ./lab2_add --iterations=# --threads=# --lists=# [--yield=[idl]] [--sync=[sm]]\n");
				exit(1);
		}
		opt = getopt_long(argc,argv,"t:i:l:y:s:",longOpts,NULL);
	}
	srand(time(NULL));
	arr = (SortedListElement_t**) malloc(numThreads*sizeof(SortedListElement_t*));
	for (int i = 0; i < numThreads; i++){
		arr[i] = (SortedListElement_t*) malloc(iterations*sizeof(SortedListElement_t));
		for(int j = 0; j < iterations; j++){
			int len = rand()%10 + 1; // lengths larger than 10 may severely impact performance
			char* key = malloc((len+1)*sizeof(char));
			for (int k = 0; k < len; k++)
				key[k] = (char)(rand()%95 + ' ');
			key[len] = '\0';
			arr[i][j].key = key;
		}
	}
	list = (SortedList_t**) malloc(numLists * sizeof(SortedList_t*));
	for(int i = 0; i < numLists; i++){
		list[i] = (SortedList_t*) malloc(sizeof(SortedList_t));
		list[i]->key = NULL;
		list[i]->next = list[i];
		list[i]->prev = list[i];
	}
	if(m){
		pmt = (pthread_mutex_t*) malloc (numLists*sizeof(pthread_mutex_t));
		for(int i = 0; i < numLists; i++)
			pthread_mutex_init(&pmt[i], NULL);
	}
	if (s){
		spinFlag = (int*) malloc (numLists*sizeof(int));
		for (int i = 0; i < numLists; i++)
			spinFlag[i] = 0;
	}
	
	signal(SIGSEGV, segFaultHandler);
	threadTimes = (int*) malloc(numThreads * sizeof(int));
	if(clock_gettime(CLOCK_MONOTONIC, &begin) == -1){
		fprintf(stderr,"ERROR: %s\n", strerror(errno));
		exit(1);
	}
	for (int i = 0; i < numThreads; i++){
		threadIDs[i] = i;
		if (pthread_create(&threads[i], NULL, listRedirect, &threadIDs[i])){
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

	msg1 = strcat(msg1, "list-");
	msg1 = strcat(msg1, msg2);
	msg1 = strcat(msg1,msg3);
	long long time = (1000000000 * (end.tv_sec - begin.tv_sec)) + (end.tv_nsec - begin.tv_nsec);
	long mutexRunTimes = 0;
	if (m){
		for (int i = 0; i < numThreads; i++)
			mutexRunTimes += threadTimes[i];
		mutexRunTimes /= (iterations * 2 * numThreads);
	}
	printf("%s,%d,%d,%d,%d,%lld,%lld,%ld\n", msg1, numThreads, iterations, numLists, (numThreads*iterations*3), time, time/(numThreads*iterations*3), mutexRunTimes);
	if (m){
		for (int i = 0; i < numLists; i++)
			pthread_mutex_destroy(&pmt[i]);
		free(pmt);
	}
	if (s)
		free(spinFlag);
	free(threads);
	free(threadIDs);
	free(threadTimes);
	for(int i = 0; i < numThreads; i++)
		free(arr[i]);
	free(arr);
	for (int i = 0; i < numLists; i++)
		free(list[i]);
	free(list);
}