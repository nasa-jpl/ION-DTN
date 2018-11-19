/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*
 * vector.h
 *
 *  Created on: Sep 13, 2018
 *      Author: ebirrane
 */

#ifndef NM_SHARED_PRIMITIVES_VECTOR_H_
#define NM_SHARED_PRIMITIVES_VECTOR_H_

#include "platform.h"
#include "../primitives/blob.h"

#define VEC_MAX_IDX 100
#define VEC_HALF_IDX 50
#define VEC_DEFAULT_NUM 4

#define VEC_FLAG_AS_STACK (0x1)



#define VEC_OK 1
#define VEC_FAIL 0
#define VEC_SYSERR -1
#define VEC_MISCONFIG -2


typedef uint16_t vec_idx_t;

typedef int (*vec_comp_fn)(void *key, void *cur_value); /* Semantic compare elements */
typedef void* (*vec_copy_fn)(void *src); /* Deep copy function. */
typedef void (*vec_del_fn)(void *item); /* Item Delete Function. */

typedef struct
{
	uint8_t occupied;
    void *value;
} vector_entry_t;


typedef struct
{
	uint8_t flags;

	vec_idx_t next_idx;     // Index of next free slot.
	vec_idx_t num_free;     // Number of free elements in the vector.
	vec_idx_t total_slots;  // Slots allocated

	ResourceLock lock;

	vec_comp_fn compare_fn;
	vec_del_fn  delete_fn;
	vec_copy_fn copy_fn;

	vector_entry_t *data;   // Values.
} vector_t;

typedef struct
{
	vec_idx_t idx;
	vector_t *vector;
} vecit_t;


int        vec_append(vector_t *dest, vector_t *src);
void*      vec_at(vector_t *vec, vec_idx_t idx);
void       vec_clear(vector_t *vec);
vector_t   vec_copy(vector_t *src, int *success);
vector_t   vec_create(uint8_t num, vec_del_fn delete_fn, vec_comp_fn compare_fn, vec_copy_fn copy, uint8_t flags, int *success);
int        vec_del(vector_t *vec, vec_idx_t idx);
vec_idx_t  vec_find(vector_t *vec, void *key, int *success);
int        vec_insert(vector_t *vec, void *value, vec_idx_t *idx);
void       vec_lock(vector_t *vec);
int        vec_make_room(vector_t *vec, vec_idx_t extra);
vec_idx_t  vec_num_entries(vector_t vec);
vec_idx_t  vec_num_entries_ptr(vector_t *vec);
void*      vec_pop(vector_t *vec, int *success);
int        vec_push(vector_t *vec, void *value);
void       vec_release(vector_t *vec, int destroy);
void*      vec_remove(vector_t *vec, vec_idx_t idx, int *success);
void*      vec_set(vector_t *vec, vec_idx_t idx, void *data, int *success);
vec_idx_t  vec_size(vector_t *vec);
void       vec_unlock(vector_t *vec);

/** Free resources associated with a value previously returned by this vector */
static inline void vec_free_value(vector_t *vec, void *value)
{
   vec->delete_fn(value);
}

/**
 * Insert a copy (created by vector class' defined copy fn) into the given vector.
 *   In this case, the user is responsible for freeing the original value.
 */
static inline int vec_insert_copy(vector_t *vec, void *value, vec_idx_t *idx)
{
   return vec_insert(vec, vec->copy_fn(value), idx);
}


void*    vecit_data(vecit_t it);

vecit_t   vecit_first(vector_t *vec);
vec_idx_t vecit_idx(vecit_t it);
int       vecit_valid(vecit_t it);
vecit_t   vecit_next(vecit_t it);


/* Helper functions for simple vector types */

int   vec_blob_add(vector_t *vec, blob_t value, vec_idx_t *idx);
int   vec_blob_init(vector_t *vec, uint8_t num);
void  vec_blob_del(void *item);
int   vec_blob_comp(void *i1, void *i2);
void* vec_blob_copy(void* item);

void  vec_simple_del(void *item);

void  vec_str_del(void *item);
int   vec_str_comp(void *i1, void *i2);
void* vec_str_copy(void* item);
int   vec_str_init(vector_t *vec, uint8_t num);

int   vec_uvast_add(vector_t *vec, uvast value, vec_idx_t *idx);
int   vec_uvast_init(vector_t *vec, uint8_t num);
int   vec_uvast_comp(void *i1, void *i2);
void* vec_uvast_copy(void* item);
int vec_uvast_find_idx(vector_t *vec, uvast value, vec_idx_t *idx);




#endif /* NM_SHARED_PRIMITIVES_VECTOR_H_ */
