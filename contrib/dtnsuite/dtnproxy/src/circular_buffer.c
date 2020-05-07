#include "proxy_thread.h"
#include "debugger.h"
#include "definitions.h"


void circular_buffer_init(circular_buffer_t * cb, int size){
   cb->buffer = malloc(sizeof(circular_buffer_item) * size);
   cb->buffer_end = cb->buffer + size;
   cb->size = size;
   cb->data_start = cb->buffer;
   cb->data_end = cb->buffer;
   cb->count = 0;
}

void circular_buffer_free(circular_buffer_t* cb){
    free(cb->buffer);
}

int circular_buffer_isFull(circular_buffer_t* cb){
    if(cb->count == cb->size) return 0;
    return 1;
}


int circular_buffer_push(circular_buffer_t* cb, circular_buffer_item data){
	if (cb == NULL || cb->buffer == NULL)
        return -1;

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
    return 0;
}

circular_buffer_item circular_buffer_pop(circular_buffer_t* cb){
    circular_buffer_item data = *cb->data_start;
    cb->data_start++;
    if (cb->data_start == cb->buffer_end)
        cb->data_start = cb->buffer;
    cb->count--;

    return data;
}

int circular_buffer_isEmpty(circular_buffer_t* cb){
	if(cb->count==0) return 0;
	return 1;
}

