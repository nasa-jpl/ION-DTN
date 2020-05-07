/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: rhht.h
 **
 ** Subsystem:
 **          Primitive Types
 **
 ** Description:
 **     This file contains the functions for a Robin Hood Hash Table.
 **     It uses a "backwards-shift" removal system, which reduces the
 **     max number of probes per find and eliminates the need for
 **     tombstones. The low expected number of removals relative to
 **     the number of insertions & finds makes this a useful tradeoff.
 **
 ** Notes:
 **    - It is assumed that entries in this hash table are all the same type.
 **    - NULL values are not allowed in the hash table. If an entry has a
 **      data value of NULL it is assumed that the entry is empty in the
 **      hash table.
 **    - It is assumed that "private" functions are called from RHHT public
 **      functions that already handle resource locking.
 **
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  08/17/18  L. F. Campbell Initial Implementation (JHU/APL)
 **  09/14/18  E. Birrane     Updates for data structures, logging, reentrance,
 **                           coding standards and documentation. (JHU/APL)
 *****************************************************************************/
#include "utils.h"
#include "rhht.h"

#include "debug.h"

/*
 * +--------------------------------------------------------------------------+
 * |					   Private Functions  								  +
 * +--------------------------------------------------------------------------+
 */


static void p_rhht_bkwrd_shft(rhht_t *ht, rh_idx_t idx)
{
	rh_idx_t i;
	rh_idx_t next_idx;

	CHKVOID(ht);
	CHKVOID(idx < ht->num_bkts);

	if(ht->buckets[idx].value != NULL)
	{
		return;
	}

	/* Better to avoid recursion when you have very large hash tables. */
    for(i = 0; i < ht->num_bkts; i++)
    {
    	/* Calculate the next index. */
        next_idx = (idx + 1) % ht->num_bkts;

        /* If the next bucket is empty or perfect, we are done shifting. */
        if( (ht->buckets[next_idx].value == NULL) ||
        	(ht->buckets[next_idx].delta == 0))
        {
        	return;
        }

        /* pull the next index into the one that was just vacated. */
        ht->buckets[idx] = ht->buckets[next_idx];

        /* We just got 1 closer to our ideal index. */
        ht->buckets[idx].delta--;

        /* Set the next index to be empty. */
        ht->buckets[next_idx].value = NULL;
        ht->buckets[next_idx].key = NULL;
        ht->buckets[next_idx].delta = 0;

        /* adjust to consider the newly vacated slot and do it again... */
        idx = next_idx;
    }
}

static rh_idx_t p_rh_calc_ideal_idx(rh_idx_t size, rh_idx_t cur, rh_idx_t delta)
{
	return (cur >= delta) ? (cur - delta) : (size - delta - cur);
}


/******************************************************************************
 *
 * hash_insert_rh_elt_t
 *
 * Inserts a given rh_elt_t at index idx.
 *
 * Returns:
 *  void
 *
 * Parameters:
 *  idx		The index to insert at.
 *  rh_elt_t	The rh_elt_t to insert into the rhht_t.
 *  ht		The rhht_t to insert into.
 *
 *****************************************************************************/
static rh_idx_t p_rh_calc_placement(rhht_t *ht, rh_idx_t idx, rh_elt_t *elt)
{
	rh_idx_t iter = 0;
    rh_idx_t index = idx;

    /*
     * Walk the array until either:
     *    1. Our current spot is empty.
     *    2. Our delta is larger than the delta of the item in this spot.
     *    3. Our delta is the same but we hash lower (tie breaker).
     *    4. We have walked the list fully once.
     */

    for(iter = 0; iter < ht->num_bkts; iter++)
    {
    	if( (ht->buckets[index].value == NULL) ||         /* Condition 1 - Empty Spot. */
			(elt->delta > ht->buckets[index].delta) ||   /* Condition 2 - We are poorer. */
			((elt->delta == ht->buckets[index].delta) && (idx < ht->hash(ht, ht->buckets[index].key))) /* Condition 3. We are even. */
		  )
    	{
    		break;
    	}
    	else                                            /* Try the next spot. */
    	{
    		elt->delta++;
    		index = (idx + elt->delta) % ht->num_bkts;
    	}
    }

    return index;
}


static int p_rh_default_compare(void *key1, void *key2)
{
	return key1 == key2;
}


/******************************************************************************
 *
 * hash_key
 *
 * Calculates the hash value of the key. This cannot be type-independent.
 * Must have specific ways to handle the diverse types used in the hash table.
 * For now we're pretending that it's all MIDs, which I know is wrong, but
 * it will not be difficult to fix it, since this is the only HT function
 * that cannot be made type-independent.
 *
 * Returns:
 *  int - The hash value of the key.
 *
 * Parameters:
 *  key		The key to hash.
 *  ht		The rhht_t.
 *
 *****************************************************************************/
static rh_idx_t p_rh_default_hash(void *table, void *key)
{
	rhht_t *ht = (rhht_t*) table;
	CHKUSR(key, ht->num_bkts);
	return (*((rh_idx_t*)key)) % ht->num_bkts;
}




/*
 * +--------------------------------------------------------------------------+
 * |					   Public Functions  								  +
 * +--------------------------------------------------------------------------+
 */



/******************************************************************************
 *
 * \par Function Name: rhht_create
 *
 * \par Creates a new Robin Hood hash table with the given number of buckets.
 *
 * \retval  NULL - Failure.
 *         !NULL - The created hash table.
 *
 * \param[in]  buckets   The number of entries in the hast table.
 * \param[in]  compare   User-supplied compare func, or NULL to use default.
 * \param[in]  hash      User-supplied hash func, or NULL to use default.
 * \param[out] success   Whether the creation succeeded (AMP_OK) or not.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *   08/17/18  L. F. Campbell Initial Implementation (JHU/APL)
 *   09/14/18  E. Birrane     Updates for data structures, logging, reentrance,
 *                            coding standards and documentation.
 *****************************************************************************/

rhht_t rhht_create(rh_idx_t buckets, rh_comp_fn compare, rh_hash_fn hash, rh_del_fn del, int *success)
{
	rhht_t ht;

	*success = AMP_OK;

	memset(&ht, 0, sizeof(rhht_t));

	if((ht.buckets = STAKE(buckets * sizeof(rh_elt_t))) == NULL)
	{
		*success = RH_SYSERR;
	}
	else
	{
		ht.num_elts = ht.max_delta = 0;
                ht.num_bkts = buckets;

		if(initResourceLock(&(ht.lock)))
		{
	        AMP_DEBUG_ERR("rhht_create","Unable to initialize mutex, errno = %s",
	        		        strerror(errno));
	        *success = RH_SYSERR;
		}
		else
		{
			ht.compare = (compare == NULL) ? p_rh_default_compare : compare;
			ht.hash = (hash == NULL) ? p_rh_default_hash : hash;
			ht.delete = del;
		}
	}

	return ht;
}


void rhht_del_idx(rhht_t *ht, rh_idx_t idx)
{
	CHKVOID(ht);
	CHKVOID(idx < ht->num_bkts);

	lockResource(&(ht->lock));

	if((ht->buckets[idx].value != NULL) && (ht->delete != NULL))
	{
		ht->delete(&(ht->buckets[idx]));

	}
	ht->buckets[idx].key = NULL;
	ht->buckets[idx].value = NULL;
	ht->buckets[idx].delta = 0;
	ht->num_elts--;

    p_rhht_bkwrd_shft(ht, idx);

    unlockResource(&(ht->lock));
}

void rhht_del_key(rhht_t *ht, void *item)
{
	rh_idx_t idx;

	if(rhht_find(ht, item, &idx) == RH_OK)
	{
		rhht_del_idx(ht, idx);
	}
}


/******************************************************************************
 *
 * \par Function Name: rhht_find
 *
 * \par Finds and returns the index of the given value in the given hash table.
 *
 * \retval  Whether the item was found (RH_OK) or not (RH_NOT_FOUND, RH_ERROR, or RH_SYSERR)
 *
 * \param[in]  ht     The hash table being queried.
 * \param[in]  item   The item whose index is being queried.
 * \param[out] idx    The index of the found item (undefined if not found)
 *
 * Notes
 *   - If idx is NULL then this function just determines whether the item
 *     is in the hash table or not.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 **  08/17/18  L. F. Campbell Initial Implementation (JHU/APL)
 **  09/14/18  E. Birrane     Updates for data structures, logging, reentrance,
 **                           coding standards and documentation.
 *****************************************************************************/

int rhht_find(rhht_t *ht, void *key, rh_idx_t *idx)
{
	rh_idx_t i;
	rh_idx_t tmp;

	CHKZERO(key);

    if (ht->num_elts == 0)
    {
       // HT is empty. Nothing to be found (and not an error)
       return RH_NOT_FOUND;
    }
    
	CHKZERO(idx);

	/* Step 1: Hash the item. */
	tmp = ht->hash(ht, key);

	lockResource(&(ht->lock));

	/* Step 2: If nothing is there, it.. isn't there. */
	if(ht->buckets[tmp].value == NULL)
	{
		unlockResource(&(ht->lock));
		return RH_NOT_FOUND;
	}

	for(i = 0; i < ht->num_bkts; i++)
	{
		if(ht->compare(key, ht->buckets[tmp].key) == 0)
		{
			if(idx != NULL)
			{
				*idx = tmp;
			}

			unlockResource(&(ht->lock));
			return RH_OK;
		}

		tmp = (tmp + 1) % ht->num_bkts;

		/* If we run into someone who is in their ideal slot, we
		 * can stop looking as we are at the end of our virtual bucket.
		 */
		if(ht->buckets[tmp].delta == 0)
		{
			break;
		}
	}

	tmp = ht->num_bkts;

	if(idx != NULL)
	{
		*idx = tmp;
	}

	unlockResource(&(ht->lock));

	return RH_NOT_FOUND;
}



void rhht_foreach(rhht_t *ht, rh_foreach_fn for_fn, void *tag)
{
	rh_idx_t i;

	CHKVOID(ht);
	CHKVOID(for_fn);

	lockResource(&(ht->lock));
	for(i = 0; i < ht->num_bkts; i++)
	{
		if(ht->buckets[i].value != NULL)
		{
			for_fn(&(ht->buckets[i]), tag);
		}
	}
	unlockResource(&(ht->lock));
}


/******************************************************************************
 *
 * \par Function Name: rhht_insert
 *
 * \par Adds a new element into the hash table.
 *
 * \retval  RH Success Code
 *
 * \param[in|out]  ht    The hash table getting the new element.
 * \param[in]      item  The item being inserted.
 * \param[out]     idx   The index of the inserted item (can be NULL).
 *
 * Note:
 *   - The item is shallow-copied into the hash table.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *   08/17/18  L. F. Campbell Initial Implementation (JHU/APL)
 *   09/14/18  E. Birrane     Updates for data structures, logging, reentrance,
 *                            coding standards and documentation.
 *****************************************************************************/
int rhht_insert(rhht_t *ht, void *key, void *value, rh_idx_t *idx)
{
	rh_idx_t actual_idx = 0;
	rh_idx_t ideal_idx;
	rh_idx_t iter = 0;
	rh_elt_t elt;

	if(rhht_find(ht, key, &actual_idx) == RH_OK)
	{
		return RH_DUPLICATE;
	}

	if(ht->num_elts == ht->num_bkts)
	{
		return RH_FULL;
	}

	elt.key = key;
	elt.value = value;
	elt.delta = 0;
	ideal_idx = ht->hash(ht, key);

	lockResource(&(ht->lock));

	for(iter = 0; (iter < ht->num_bkts) && (elt.value != NULL); iter++)
	{
		/* Get the place where the item should live. */
		actual_idx = p_rh_calc_placement(ht, ideal_idx, &elt);

		if((iter == 0) && (idx != NULL))
		{
			if(idx != NULL)
			{
				*idx = actual_idx;
			}
		}

		/*
		 * Store the new item at its new index. Remember what was there,
		 * if anything, because we may have to propagate...
		 */
		rh_elt_t temp = ht->buckets[actual_idx];
		ht->buckets[actual_idx] = elt;

		/*
		 * If there was an item at the actual_idx, we have bumped it.
		 * Save the bumped item as we will need to go back in and
		 * rehash it. We do this by:
		 * - saving its data pointer.
		 * - calculating it's ideal index.
		 * - setting its delta to 0. We will re-calculate its delta
		 *   as part of finding its new home.
		 */
		elt.key = temp.key;
		elt.value = temp.value;
		elt.delta = 0;

		/*
		 * We don't have to re-hash the bumped item. We can calculate its ideal
		 * index as a function of where it had been living (actual_idx) and
		 * what it's delta was (temp.delta).
		 */
		ideal_idx = p_rh_calc_ideal_idx(ht->num_bkts, actual_idx, temp.delta);
	}

	ht->num_elts++;

	unlockResource(&(ht->lock));

    return RH_OK;
}



/******************************************************************************
 *
 * delete_rhht_t
 *
 * Deconstructor for the rhht_t.
 *
 * Returns:
 *  int >0 - Success.
 *      <0 - Failure.
 *
 * Parameters:
 *  ht		The rhht_t to delete.
 *
 ******************************************************************************/
void rhht_release(rhht_t *ht, int destroy)
{
    rh_idx_t i;
    rh_elt_t elt;

    for (i = 0; i < ht->num_bkts; i++)
    {
    	if(ht->delete && ht->buckets[i].value != NULL)
    	{
    		elt = ht->buckets[i];
    		ht->delete(&elt);
    	}
    }

    SRELEASE(ht->buckets);

    if(destroy)
    {
    	killResourceLock(&(ht->lock));
    	SRELEASE(ht);
    }
}


void* rhht_retrieve_idx(rhht_t *ht, rh_idx_t idx)
{
	CHKNULL(ht);
	CHKNULL(idx <= ht->num_bkts);

	return ht->buckets[idx].value;
}

void* rhht_retrieve_key(rhht_t *ht, void *key)
{
	rh_idx_t idx;
	if(rhht_find(ht, key, &idx) != RH_OK)
	{
		return NULL;
	}

	return ht->buckets[idx].value;
}







