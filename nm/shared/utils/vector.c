/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*
 * vector.c
 *
 *  Created on: Sep 13, 2018
 *      Author: ebirrane
 */

#include "shared/utils/utils.h"
#include "platform.h"
#include "vector.h"



/*
 * +--------------------------------------------------------------------------+
 * |					   Private Functions  								  +
 * +--------------------------------------------------------------------------+
 */



int p_vec_default_comp(void *i1, void *i2)
{
	return (i1 == i2) ? 0 : 1;
}

/*
 * +--------------------------------------------------------------------------+
 * |					   Public Functions  								  +
 * +--------------------------------------------------------------------------+
 */

int vec_append(vector_t *dest, vector_t *src)
{
	vecit_t it;

	if((dest == NULL) || (src == NULL))
	{
		return VEC_FAIL;
	}

	if(src->copy_fn == NULL)
	{
		return VEC_FAIL;
	}

	if(vec_num_entries(*src) <= 0)
	{
		return VEC_OK;
	}

	if(vec_make_room(dest, vec_num_entries(*src)) != VEC_OK)
	{
		return VEC_FAIL;
	}

	for(it = vecit_first(src); vecit_valid(it); it = vecit_next(it))
	{
		vec_push(dest, src->copy_fn(vecit_data(it)));
	}

	return VEC_OK;
}

void *vec_at(vector_t *vec, vec_idx_t idx)
{
	if(idx >= vec->total_slots)
	{
		return NULL;
	}

	return (vec->data[idx].occupied) ? vec->data[idx].value : NULL;
}


void vec_clear(vector_t *vec)
{
	vec_idx_t i;

	if((vec == NULL) || (vec->data == NULL))
	{
		return;
	}

	lockResource(&vec->lock);

	for(i = 0; i < vec->total_slots; i++)
	{
		if(vec->data[i].occupied)
		{
			vec->data[i].occupied = 0;
			if(vec->delete_fn)
			{
				vec->delete_fn(vec->data[i].value);
			}
			vec->data[i].value = NULL;
		}
	}

	vec->next_idx = 0;
    vec->num_free = vec->total_slots;
	unlockResource(&vec->lock);
}

vector_t vec_copy(vector_t *src, int *success)
{
	vector_t result;
	vec_idx_t i = 0;

	*success = AMP_FAIL;

	lockResource(&(src->lock));

	result = vec_create(src->total_slots, src->delete_fn, src->compare_fn, src->copy_fn, src->flags, success);
	if(*success != VEC_OK)
	{
		unlockResource(&(src->lock));
		return result;
	}

	for(i = 0; i < src->total_slots; i++)
	{

		result.data[i].occupied = src->data[i].occupied;
        if (result.data[i].occupied)
        {
           result.data[i].value = result.copy_fn(src->data[i].value);
        }
	}

	result.next_idx = src->next_idx;
	result.num_free = src->num_free;

	unlockResource(&(src->lock));

	*success = VEC_OK;
	return result;
}

vector_t vec_create(uint8_t num, vec_del_fn delete_fn, vec_comp_fn compare_fn, vec_copy_fn copy_fn, uint8_t flags, int *success)
{
	vector_t result;

	memset(&result,0, sizeof(vector_t));

	if(initResourceLock(&(result.lock)))
	{
		*success = VEC_SYSERR;
		return result;
	}

	result.flags = flags;

	*success = VEC_OK;
	result.delete_fn = delete_fn;
	result.copy_fn = copy_fn;
	result.compare_fn = (compare_fn) ? compare_fn : p_vec_default_comp;

	if(num > 0)
	{
		*success = vec_make_room(&result, num);
	}

	return result;
}


int vec_del(vector_t *vec, vec_idx_t idx)
{
	void *data = NULL;
	int success = VEC_FAIL;


	data = vec_remove(vec, idx, &success);

	if((success == VEC_OK) &&
	   (data != NULL) &&
	   (vec->delete_fn))
	{
		vec->delete_fn(data);
	}

	return success;
}

vec_idx_t vec_find(vector_t *vec, void *key, int *success)
{
	vec_idx_t result = 0;

	*success = VEC_FAIL;

	lockResource(&(vec->lock));
	for(result = 0; result < vec->total_slots; result++)
	{
		if(vec->data[result].occupied == 1)
		{
			if(vec->compare_fn(key, vec->data[result].value) == 0)
			{
				*success = VEC_OK;
				break;
			}
		}
	}

	unlockResource(&(vec->lock));

	return result;
}


int vec_insert(vector_t *vec, void *value, vec_idx_t *idx)
{
	int success;

	CHKERR(vec);


	lockResource(&(vec->lock));
	success = vec_make_room(vec, 1);

	if(success == VEC_OK)
	{
		vec_idx_t i;

		/* Store the data */
		vec->data[vec->next_idx].occupied = 1;
		vec->data[vec->next_idx].value = value;
        
		if(idx != NULL)
		{
			*idx = vec->next_idx;
		}

		/* Find the next open spot. */
		for(i = 0; i < vec->total_slots; i++)
		{
			if(vec->data[i].occupied == 0)
			{
				break;
			}
		}
		vec->next_idx = i;
		vec->num_free--;
	}

	unlockResource(&(vec->lock));

	return success;
}

void vec_lock(vector_t *vec)
{
	CHKVOID(vec);
	lockResource(&(vec->lock));
}



int vec_make_room(vector_t *vec, vec_idx_t extra)
{
	size_t needed = 0;
	vec_idx_t new_size = 0;
	vector_entry_t *tmp = NULL;

	CHKERR(vec);

	/* If we already have room, we are done. */
	if(extra <= vec->num_free)
	{
		return VEC_OK;
	}

	/* Calculate how many total slots are needed. */
	needed = vec->total_slots + (extra - vec->num_free);

	/* Make sure that total number is allowed. */
	CHKERR(needed <= VEC_MAX_IDX);

	/*
	 * Vector size will be allocated not by exact size, but by doubling
	 * allocations. Calculate the new vector size that will accomodate the
	 * needed size.
	 */
	new_size = (vec->total_slots <= 0) ? VEC_DEFAULT_NUM : vec->total_slots;

	while(new_size < needed)
	{
		new_size = (new_size >= VEC_HALF_IDX) ? VEC_MAX_IDX : new_size * 2;
	}

	/* Allocate the new vector and copy its data over. */
	if((tmp = STAKE(new_size * sizeof(vector_entry_t))) == NULL)
	{
		return VEC_SYSERR;
	}

	memcpy(tmp, vec->data, (vec->total_slots * sizeof(vector_entry_t)));
	SRELEASE(vec->data);
	vec->data = tmp;

	/*
	 * Do any meta-data cleanup.
	 * Note, if next_idx has been pointing to total_num_slots (meaning the
	 * vector was full) then it now points to the first open spot.
	 */
        vec->num_free += new_size - vec->total_slots;
	vec->total_slots = new_size;

	return VEC_OK;
}

vec_idx_t vec_num_entries_ptr(vector_t *vec)
{
	CHKZERO(vec);
	return vec->total_slots - vec->num_free;
}
vec_idx_t vec_num_entries(vector_t vec)
{
	return vec_num_entries_ptr(&vec);
}

void* vec_pop(vector_t *vec, int *success)
{
	return vec_remove(vec, vec->next_idx - 1, success);
}

int vec_push(vector_t *vec, void *value)
{
	return vec_insert(vec, value, NULL);
}


void vec_release(vector_t *vec, int destroy)
{
	vec_clear(vec);

    if (vec->data != NULL)
    {
       SRELEASE(vec->data);
       vec->data = NULL;
    }


	if(destroy)
	{
		killResourceLock(&(vec->lock));
		SRELEASE(vec);
	}
}

void* vec_remove(vector_t *vec, vec_idx_t idx, int *success)
{
	void *result = NULL;

	CHKNULL(success);

	*success = VEC_FAIL;
	CHKNULL(vec);
	CHKNULL(idx < vec->total_slots);

	/* If we are in stack mode, we can only pop the last thing. */
	if(vec->flags & VEC_FLAG_AS_STACK)
	{
		if(idx != (vec->next_idx - 1))
		{
			return NULL;
		}
	}

	if(vec->data[idx].occupied == 1)
	{

		vec->data[idx].occupied = 0;
		result = vec->data[idx].value;
		*success = VEC_OK;
		if(vec->next_idx > idx)
		{
			vec->next_idx = idx;
		}
		vec->num_free++;
	}

	return result;

}

/* We do not check for stack semantics here because
 * we are swapping elements explicitely, not adding or removing.
 */
void* vec_set(vector_t *vec, vec_idx_t idx, void *data, int *success)
{
	void *result = NULL;

	lockResource(&(vec->lock));


	if( (vec == NULL) ||
		(idx >= vec->total_slots)
	  )
	{
		*success = VEC_FAIL;
		unlockResource(&(vec->lock));
		return NULL;
	}

	*success = VEC_OK;

    if (vec->data[idx].occupied == 0)
    {
       vec->data[idx].occupied=1;
       vec->num_free--;
       result = NULL;
    }
    else
    {
       result = vec->data[idx].value;
    }
    
	vec->data[idx].value = data;
	unlockResource(&(vec->lock));

	return result;
}

vec_idx_t   vec_size(vector_t *vec)
{
	return vec->total_slots;
}

void vec_unlock(vector_t *vec)
{
	CHKVOID(vec);
	unlockResource(&(vec->lock));
}



void* vecit_data(vecit_t it)
{
	CHKNULL(it.vector);
	CHKNULL(vecit_valid(it));
	return it.vector->data[it.idx].value;
}


int vecit_valid(vecit_t it)
{
	return (it.idx < it.vector->total_slots);
}


vecit_t vecit_first(vector_t *vec)
{
	vecit_t result;
	vec_idx_t i;

	result.vector = vec;
	result.idx = 0;
	CHKUSR(vec, result);

	for(i = 0; i < vec->total_slots; i++)
	{
		if(vec->data[i].occupied == 1)
		{
			break;
		}
	}
	result.idx = i;

	return result;
}

vec_idx_t vecit_idx(vecit_t it)
{
	return it.idx;
}

vecit_t vecit_next(vecit_t it)
{
	CHKUSR(it.vector, it);

	it.idx++;
	while(it.idx < it.vector->total_slots)
	{
		if(it.vector->data[it.idx].occupied == 1)
		{
			break;
		}
		it.idx++;
	}

	return it;
}



int vec_blob_add(vector_t *vec, blob_t value, vec_idx_t *idx)
{
	int success = VEC_OK;
	blob_t *new_entry;
	vecit_t it;
	vec_idx_t tmp_idx;
	
	// Check for uniqueness
    tmp_idx = vec_find(vec, &value, &success);
	if (success == VEC_OK)
	{
		if (idx != NULL)
		{
			*idx = tmp_idx;
		}
		return success;
	}
	
	// Create a copy to insert
	if((new_entry = blob_create(value.value, value.length, value.alloc)) == NULL)
	{
		return VEC_SYSERR;
	}

	if((success = vec_insert(vec, new_entry, idx)) != VEC_OK)
	{
		blob_release(new_entry, 1);
	}

	return success;
}


int vec_blob_init(vector_t *vec, uint8_t num)
{
	int success;
	CHKERR(vec);

	*vec = vec_create(num, vec_blob_del, vec_blob_comp, vec_blob_copy, 0, &success);
	return success;
}

void vec_blob_del(void *item)
{
	blob_t *blob = (blob_t *) item;
	blob_release(blob, 1);
}

int vec_blob_comp(void *i1, void *i2)
{
	return blob_compare((blob_t *) i1, (blob_t *) i2);
}

void* vec_blob_copy(void* item)
{
	return blob_copy_ptr((blob_t*)item);
}

void vec_simple_del(void *item)
{
	SRELEASE(item);
}


int   vec_str_comp(void *i1, void *i2)
{
	return strcmp((char*)i1, (char*)i2);
}

void* vec_str_copy(void* item)
{
	char *new_item = NULL;
	CHKNULL(item);

	if((new_item = STAKE(strlen((char*)item)+1)) != NULL)
	{
		strcpy(new_item, item);
	}

	return new_item;
}

int   vec_str_init(vector_t *vec, uint8_t num)
{
	int success;
	CHKERR(vec);

	*vec = vec_create(num, vec_simple_del, vec_str_comp, vec_str_copy, 0, &success);
	return success;
}



int vec_uvast_add(vector_t *vec, uvast value, vec_idx_t *idx)
{
	int success = VEC_OK;
	vecit_t it;
	uvast *new_entry;

	/* First, make sure we don't already have an entry. */
	for(it = vecit_first(vec); vecit_valid(it); it = vecit_next(it))
	{
		new_entry = (uvast *) vecit_data(it);
		if(*new_entry == value)
		{
			*idx = vecit_idx(it);
			return VEC_OK;
		}
	}

	if((new_entry = STAKE(sizeof(uvast))) == NULL)
	{
		return VEC_SYSERR;
	}

	*new_entry = value;

	if((success = vec_insert(vec, new_entry, idx)) != VEC_OK)
	{
		SRELEASE(new_entry);
	}

	return success;
}

int vec_uvast_init(vector_t *vec, uint8_t num)
{
	int success;
	CHKERR(vec);

	*vec = vec_create(num, vec_simple_del, vec_uvast_comp, vec_uvast_copy, 0, &success);
	return success;
}



int vec_uvast_comp(void *i1, void *i2)
{
	uvast *v1 = (uvast *) i1;
	uvast *v2 = (uvast *) i2;

	CHKERR(v1);
	CHKERR(v2);

	return (*v1 == *v2) ? 0 : 1;
}

void* vec_uvast_copy(void* item)
{
	void *new_item = NULL;
	CHKNULL(item);

	if((new_item = STAKE(sizeof(uvast))) == NULL)
	{
		return NULL;
	}
	memcpy(new_item, item, sizeof(uvast));
	return new_item;
}


int vec_uvast_find_idx(vector_t *vec, uvast value, vec_idx_t *idx)
{
	vecit_t it;

	if((idx == NULL) || (vec == NULL))
	{
		return VEC_FAIL;
	}

	for(it = vecit_first(vec); vecit_valid(it); it=vecit_next(it))
	{
		uvast *data = (uvast *) vecit_data(it);
		if(data != NULL)
		{
			if(*data == value)
			{
				*idx = vecit_idx(it);
				return VEC_OK;
			}
		}
	}
	return VEC_FAIL;
}

