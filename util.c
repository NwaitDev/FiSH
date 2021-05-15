#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define INIT_CAP 10

#include "util.h"

void pid_list_create(struct pid_list *newlist){
	assert(newlist);
	newlist->data = calloc(INIT_CAP, sizeof(pid_t));
	newlist->size = 0;
	newlist->capacity = INIT_CAP;
}

void pid_list_destroy(struct pid_list *todestroy){
	assert(todestroy);
	free(todestroy->data);
	todestroy->size = 0;
	todestroy->capacity = -1;
}

/**
 * adds a pid inside the list of pid
 */
void pid_list_add(struct pid_list *list, pid_t toadd){
	assert(list);
	assert(list->size!=list->capacity);
	list->data[list->size++]=toadd;
	//if the dataset is full, double the capacity by reallocationg memory
	if(list->size == list->capacity){
		pid_t *newdata = calloc(2*list->capacity,sizeof(pid_t));
		for(size_t i = 0; i<list->size;++i){
			newdata[i] = list->data[i];
		}
		free(list->data);
		list->data = newdata;
		list->capacity*=2;
	}
}

/**
 * returns the index of the pid if the pid is present, else -1
 */
size_t pid_list_contain(struct pid_list *list, pid_t needle){
	size_t i = 0;
	while(i<list->size && list->data[i]!=needle){
		++i;
	}
	return i==list->size ? -1 : i;
}

/**
 * returns 0 if the pid has been succesfully removed, else -1
 */
int pid_list_remove(struct pid_list *list, pid_t toremove){
	size_t pos = pid_list_contain(list, toremove);
	if(pos!=-1){
		list->data[pos]=list->data[--list->size];
		return 0;
	}
	return -1;
}

void pid_list_print(struct pid_list *list){
	for(size_t i = 0; i<list->size;++i){
		printf("pid[%li]=%i\n",i,list->data[i]);
	}
}


