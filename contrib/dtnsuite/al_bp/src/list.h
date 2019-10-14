/********************************************************
 **  Authors: Andrea Bisacchi, andrea.bisacchi5@studio.unibo.it
 **           Carlo Caini (DTNperf_3 project supervisor), carlo.caini@unibo.it
 **
 **
 **  Copyright (c) 2013, Alma Mater Studiorum, University of Bologna
 **  All rights reserved.
 ********************************************************/

/*
 * list.h
 */

#ifndef DTNPERF_SRC_AL_BP_UTILITY_LIST_H_
#define DTNPERF_SRC_AL_BP_UTILITY_LIST_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct Node
{
	void* data;
	size_t data_size;

	struct Node* next;
} Node;

typedef Node* List;

#define empty_list NULL

/*
 * Returns a duplicate of list.
 */
List list_duplicate(List list);

/*
 * Destroy the List and dealloc all nodes
 */
void list_destroy(List* list);

/*
 * Inserts in the list a COPY of the element passed in the position index
 */
void list_insert_index(List* list, void* data, size_t data_size, int index);

/*
 * Inserts in the list a COPY of the element passed.
 */
void list_push_front(List* list, void* data, size_t data_size);

/*
 * Inserts in the end of the list a COPY of the element passed.
 */
void list_push_back(List* list, void* data, size_t data_size);

/*
 * Inserts in the end of the list a COPY of the element passed.
 */
void list_append(List* list, void* data, size_t data_size);

/*
 * Returns a COPY of the element in position index according to list_length value.
 * You should FREE the resource!
 */
void* list_get_value_index(List list, int index, size_t* data_size);

/*
 * Returns the pointer and the data_size of the element in position index according to list_length value.
 */
void* list_get_pointer_index(List list, int index, size_t* data_size);

/*
 * Returns the length of the list. Starts from 1.
 */
int list_length(List list);

/*
 * Removes and returns the pointer and the data_size of the element in position index according to list_length value.
 * You should FREE the resource!
 */
void* list_remove_index_get_pointer(List* list, int index, size_t* data_size);

/*
 * Removes and returns the pointer and the data_size of the first element.
 * You should FREE the resource!
 */
void* list_pop_front(List* list, size_t* data_size);

/*
 * Removes and returns the pointer and the data_size of the last element.
 * You should FREE the resource!
 */
void* list_pop_back(List* list, size_t* data_size);

/*
 * Removes from the list the element in position index according to list_length value.
 */
void list_remove_index(List* list, int index);

/*
 * Removes from the list the first element.
 */
void list_remove_first(List* list);

/*
 * Removes from the list the last element.
 */
void list_remove_last(List* list);

/*
 * Removes from the list the element equals to the passed element. The way it will compare the elements is obteined by compare param.
 * Compare is according to the standard compare functions. compare(a,b) --> Negative:a<b, Zero:a=b, Positive: a>b
 * If compare param not passed (NULL) it will use default_compare.
 * OUTPUT: TRUE if removed, FALSE if not.
 */
bool list_remove_data(List* list, void* data_to_search, size_t data_to_search_size, int (*compare)(void*,size_t,void*,size_t));

/*
 * Returns the index of the element passed. The way it will compare the elements is obteined by compare param.
 * Compare is according to the standard compare functions. compare(a,b) --> Negative:a<b, Zero:a=b, Positive: a>b
 * If compare param not passed (NULL) it will use default_compare.
 */
int list_find(List list, void* data_to_search, size_t data_to_search_size, int (*compare)(void*,size_t,void*,size_t));

/*
 * Returns the pointer to the first data which compare returns 0.
 */
void* list_get_pointer_data(List list, void* data_to_search, size_t data_to_search_size, int (*compare)(void*,size_t,void*,size_t));

/*
 * Returns a pointer to a copy to the first data which compare returns 0.
 * You should FREE the resource!
 */
void* list_get_value_data(List list, void* data_to_search, size_t data_to_search_size, int (*compare)(void*,size_t,void*,size_t));


/***************************************************
 *               DEFAULT FUNCTIONS                 *
 ***************************************************/

/*
 * Compare the two data.
 * If data_size are equals 	--> compare bit per bit according to memcmp
 * If not 					--> returns (data1_size - data2_size)
 */
int default_compare(void* data1, size_t data1_size, void* data2, size_t data2_size);

#endif /* DTNPERF_SRC_AL_BP_UTILITY_LIST_H_ */
