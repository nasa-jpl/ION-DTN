/********************************************************
    Authors:
    Lorenzo Tullini, lorenzo.tullini@studio.unibo.it
    Carlo Caini (DTNfog project supervisor), carlo.caini@unibo.it

    License:
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Copyright (c) 2018, Alma Mater Studiorum, University of Bologna

 ********************************************************/

/**
 * circular_buffer.c
 * implementation of circular buffer model.
 */
#include "proxy_thread.h"
#include "debugger.h"
#include "definitions.h"

/**
 * Function inits circular buffer
 */
void circular_buffer_init(circular_buffer_t * cb, int size){
   cb->buffer = malloc(sizeof(circular_buffer_item) * size);
   cb->buffer_end = cb->buffer + size;
   cb->size = size;
   cb->data_start = cb->buffer;
   cb->data_end = cb->buffer;
   cb->count = 0;
}

/**
 * Function frees circular buffer
 */
void circular_buffer_free(circular_buffer_t* cb){
	free(cb->buffer);
}

/**
 * Function return 0 if circular buffer is full else return 1
 */
int circular_buffer_isFull(circular_buffer_t* cb){
	if(cb->count == cb->size) return 0;
	return 1;
}

/**
 * Function return 0 if circular buffer is empty else return 1
 */
int circular_buffer_isEmpty(circular_buffer_t* cb){
	if(cb->count==0){
		return 0;
	}
	return 1;
}

/**
 * Function pushes a entry object in circular buffer
 */
int circular_buffer_push(circular_buffer_t* cb, circular_buffer_item data){
	while(circular_buffer_isFull(cb)==0) usleep(1000*POLLING_TIME);
	pthread_mutex_lock(&(cb->mutex));
    *cb->data_end = data;
    cb->data_end++;
    if (cb->data_end == cb->buffer_end)
        cb->data_end = cb->buffer;

    if (circular_buffer_isFull(cb) == 0) {
        if ((cb->data_start + 1) == cb->buffer_end)
            cb->data_start = cb->buffer;
        else
            cb->data_start++;
    } else {
        cb->count++;
    }
    pthread_mutex_unlock(&(cb->mutex));
    return 0;
}

/**
 * Function return a circular buffer's entry and remove it from buffer
 */
circular_buffer_item circular_buffer_pop(circular_buffer_t* cb){
	while(circular_buffer_isEmpty(cb)==0) usleep(1000*POLLING_TIME);
	pthread_mutex_lock(&(cb->mutex));
	circular_buffer_item data = *cb->data_start;
    cb->data_start++;
    if (cb->data_start == cb->buffer_end)
        cb->data_start = cb->buffer;
    cb->count--;
    pthread_mutex_unlock(&(cb->mutex));
    return data;
}

