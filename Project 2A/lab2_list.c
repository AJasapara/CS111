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
int spinFlag = 0;
int m = 0, s = 0;
int iterations = 1;
int threadLen;
pthread_mutex_t pmt;
SortedListElement_t** arr;
SortedList_t *list;

void segFaultHandler(int signum){
	fprintf(stderr,"ERROR %d: %s\n", signum, strerror(errno));
	exit(2);
}

void* listRedirect(void* threadID) {
	int id = *((int*)threadID);
	for (int j = 0; j < iterations; j++){
		if (m) {
			pthread_mutex_lock(&pmt);
			SortedList_insert(list, &arr[id][j]);
			pthread_mutex_unlock(&pmt);
		}
		else if(s) {
			while(__sync_lock_test_and_set(&spinFlag, 1));
			SortedList_insert(list, &arr[id][j]);
			__sync_lock_release(&spinFlag);
		}
		else
			SortedList_insert(list, &arr[id][j]);
	}

	if (m) {
		pthread_mutex_lock(&pmt);
		threadLen = SortedList_length(list);
		pthread_mutex_unlock(&pmt);
	}
	else if(s) {
		while(__sync_lock_test_and_set(&spinFlag, 1));
		threadLen = SortedList_length(list);
		__sync_lock_release(&spinFlag);
	}
	else
		threadLen = SortedList_length(list);

	SortedListElement_t* temp;
	for (int j = 0; j < iterations; j++){
		if (m) {
			pthread_mutex_lock(&pmt);
			temp = SortedList_lookup(list, arr[id][j].key);
			SortedList_delete(temp);
			pthread_mutex_unlock(&pmt);
		}
		else if(s) {
			while(__sync_lock_test_and_set(&spinFlag, 1));
			temp = SortedList_lookup(list, arr[id][j].key);
			SortedList_delete(temp);
			__sync_lock_release(&spinFlag);
		}
		else {
			temp = SortedList_lookup(list, arr[id][j].key);
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
		{"yield", required_argument, 0, 'y'},
		{"sync", required_argument, 0, 's'},
		{0,0,0,0}
	};
	int opt = getopt_long(argc,argv,"t:i:y:s:",longOpts,NULL);
	while(opt != -1) {
		switch(opt){
			case 't': 
				numThreads = atoi(optarg); 
				threads = (pthread_t*) malloc(numThreads * sizeof(pthread_t));
				threadIDs = (int*) malloc(numThreads * sizeof(int));
				break;
			case 'i': iterations=atoi(optarg); break;
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
					fprintf(stderr, "ERROR: Unrecognized argument.\nCorrect Usage: ./lab2_add --iterations=# --threads=#\n");
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
					pthread_mutex_init(&pmt, NULL);
				}
				else{
					fprintf(stderr, "ERROR: Unrecognized argument.\nCorrect Usage: ./lab2_add --iterations=# --threads=#\n");
					exit(1);
				}
				break;
			default: 
				fprintf(stderr, "ERROR: Unrecognized argument.\nCorrect Usage: ./lab2_add --iterations=# --threads=#\n");
				exit(1);
		}
		opt = getopt_long(argc,argv,"t:i:y:s:",longOpts,NULL);
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
	list = (SortedList_t*) malloc(sizeof(SortedList_t));
	list->key = NULL;
	list->next = list;
	list->prev = list;
	signal(SIGSEGV, segFaultHandler);
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
	int time = (1000000000 * (end.tv_sec - begin.tv_sec)) + (end.tv_nsec - begin.tv_nsec);
	printf("%s,%d,%d,1,%d,%d,%d\n", msg1, numThreads, iterations, (numThreads*iterations*3), time, time/(numThreads*iterations*3));
	free(threads);
	free(threadIDs);
	for(int i = 0; i < numThreads; i++)
		free(arr[i]);
	free(arr);
	pthread_mutex_destroy(&pmt);
}