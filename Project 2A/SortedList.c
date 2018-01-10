// NAME: Arpit Jasapara
// EMAIL: ajasapara@ucla.edu
// ID: XXXXXXXXX

#include "SortedList.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void SortedList_insert(SortedList_t *list, SortedListElement_t *element){
	if (!list || !element)
		return;
	SortedListElement_t* s = list;
	while(s->next != list){
		if (strcmp(element->key, s->next->key) <= 0)
			break;
		s = s->next;
	}
	if(opt_yield & INSERT_YIELD)
		sched_yield();
	element->prev = s;
	element->next = s->next;
	s->next = element;
	element->next->prev = element;
}

int SortedList_delete( SortedListElement_t *element){
	if (!element)
		return 1;
	if (element->next->prev != element || element->prev->next != element)
		return 1;
	if(opt_yield & DELETE_YIELD)
		sched_yield();
	element->next->prev = element->prev;
	element->prev->next = element->next;
	return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
	if (!list || !key)
		return NULL;
	SortedListElement_t* s = list->next;
	while(s != list) {
		if (strcmp(key, s->key) == 0)
			return s;
		if(opt_yield & LOOKUP_YIELD)
			sched_yield();
		s = s->next;
	}
	return NULL;
}

int SortedList_length(SortedList_t *list) {
	if(!list) 
		return -1;
	int c = 0;
	SortedListElement_t* s = list;
	while (s->next != list){
		if (s->prev->next != s || s->next->prev != s)
            return -1;
		c += 1;
		if(opt_yield & LOOKUP_YIELD)
			sched_yield();
		s = s->next;
	}
	return c;
}