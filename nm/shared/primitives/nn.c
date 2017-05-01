/*****************************************************************************
 **
 ** File Name: nn.c
 **
 ** Description: This file contains the implementation of functions that
 **              operate on NickNamess (NNs).
 **
 ** Notes:
 **	     1. In the current implementation, the nickname database is not
 **	        persistent.
 **
 ** Assumptions:
 **      1. We limit the size of a nickname in the system to reduce the amount
 **         of pre-allocated memory in this embedded system. Non-embedded
 **         implementations may wish to dynamically allocate MIDs as they are
 **         received.
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **            E. Birrane     Initial Implementation. (Secure DTN - NASA: NNX14CS58P)
 **  03/11/15  E. Birrane     Pulled nicknamed out of OID into NN. (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "platform.h"

#include "../utils/utils.h"

#include "../primitives/nn.h"


Lyst nn_db;
ResourceLock nn_db_mutex;


/******************************************************************************
 *
 * \par Function Name: oid_nn_add
 *
 * \par Purpose: Adds a nickname to the database.
 *
 * \retval 0 Failure.
 *         !0 Success.
 *
 * \param[in] oid The OID whose string representation is being calculated.
 *
 * \par Notes:
 *		1. We will allocate our own entry on addition, the passed-in structure
 *		   may be deleted or changed after this call.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

int oid_nn_add(oid_nn_t *nn)
{
	oid_nn_t *new_nn = NULL;

	AMP_DEBUG_ENTRY("oid_nn_add","("ADDR_FIELDSPEC")",(uaddr)nn);

	/* Step 0: Sanity check. */
	if(nn == NULL)
	{
		AMP_DEBUG_ERR("oid_nn_add","Bad args.",NULL);
		AMP_DEBUG_EXIT("oid_nn_add","->0",NULL);
		return 0;
	}

	/* Need to lock early so our uniqueness check (step 1) doesn't change by
	 * the time we go to insert in step 4. */
	lockResource(&nn_db_mutex);

	/* Step 1: Make sure entry doesn't already exist. */
	if(oid_nn_exists(nn->id))
	{
		AMP_DEBUG_ERR("oid_nn_add","Id 0x%x already exists in db.",
				         nn->id);

		unlockResource(&nn_db_mutex);

		AMP_DEBUG_EXIT("oid_nn_add","->0",NULL);
		return 0;
	}

	/* Step 2: Allocate new entry. */
	if ((new_nn = (oid_nn_t*)STAKE(sizeof(oid_nn_t))) == NULL)
	{
		AMP_DEBUG_ERR("oid_nn_add","Can't take %d bytes for new nn.",
						sizeof(oid_nn_t));

		unlockResource(&nn_db_mutex);

		AMP_DEBUG_EXIT("oid_nn_add","->0",NULL);
		return 0;
	}

	/* Step 3: Populate new entry with passed-in data. */
	memcpy(new_nn, nn, sizeof(oid_nn_t));


	/* Step 4: Add new entry to the db. */

	lyst_insert_first(nn_db, new_nn);
    unlockResource(&nn_db_mutex);


	AMP_DEBUG_EXIT("oid_nn_add","->1",NULL);
	return 1;
}



/******************************************************************************
 *
 * \par Function Name: oid_nn_add_parm
 *
 * \par Purpose: Adds a nickname to the database from NN parameters.
 *
 * \retval 0 Failure.
 *         !0 Success.
 *
 * \param[n]  id       The unique identifier of the NN.
 * \param[in] oid      The OID whose string representation is being calculated.
 * \param[in] name     The ADM name
 * \param[in] ver      The ADM Version
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/11/15  E. Birrane     Initial implementation,
 *  08/29/15  E. Birrane     Added ADM Id.
 *****************************************************************************/
int oid_nn_add_parm(uvast id, char *oid, char *name, char *version)
{
	int result = 0;
	oid_nn_t tmp;
	uint8_t *data = NULL;
	uint32_t datasize = 0;

	AMP_DEBUG_ENTRY("oid_nn_add_parm","("ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC","ADDR_FIELDSPEC")",
		(uaddr) id, (uaddr)oid, (uaddr)name, (uaddr)version);

	/* Step 0: Sanity check. */
	if(oid == NULL)
	{
		AMP_DEBUG_ERR("oid_nn_add_parm","Bad args.",NULL);
		AMP_DEBUG_EXIT("oid_nn_add_parm","->0",NULL);
		return 0;
	}

	if((data = utils_string_to_hex(oid, &datasize)) == NULL)
	{
		AMP_DEBUG_ERR("oid_nn_add_parm","Can't cnv to hex.",NULL);
		AMP_DEBUG_EXIT("oid_nn_add_parm","->0",NULL);
		return 0;
	}

	if(datasize > MAX_NN_SIZE)
	{
		AMP_DEBUG_ERR("oid_nn_add_parm","OID size %d > %d.", datasize, MAX_NN_SIZE);
		SRELEASE(data);
		AMP_DEBUG_EXIT("oid_nn_add_parm","->0",NULL);
		return 0;
	}

	/* Step 1: Populate temp structure. */
	tmp.id = id;

	memcpy(&(tmp.raw), data, datasize);
	tmp.raw_size = datasize;

	istrcpy(tmp.adm_name, name, 16);
	istrcpy(tmp.adm_ver, version, 16);

	/* Step 2: Add the NN entry. */
	result = oid_nn_add(&tmp);

	SRELEASE(data);

	AMP_DEBUG_EXIT("oid_nn_add_parm","->%d",result);

	return result;
}


/******************************************************************************
 *
 * \par Function Name: oid_nn_cleanup
 *
 * \par Purpose: Breaks down the nickname database.
 *
 * \retval void
 *
 * \par Notes:
 *		1.  We assume there are no other people who will use the nn_db after
 *		    this!
 *		2.  We also kill the mutex around the database.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

void oid_nn_cleanup()
{
    LystElt elt;
    oid_nn_t *entry = NULL;

    AMP_DEBUG_ENTRY("oid_nn_cleanup","()",NULL);

    /* Step 0: Sanity Check. */
    if(nn_db == NULL)
    {
    	AMP_DEBUG_WARN("oid_nn_cleanup","NN database already cleaned.",NULL);
    	return;
    }


    /* Step 1: Wait for folks to be done with the database. */
    lockResource(&nn_db_mutex);

    /* Step 2: Release each entry. */
    for (elt = lyst_first(nn_db); elt; elt = lyst_next(elt))
    {
    	entry = (oid_nn_t*) lyst_data(elt);
    	if (entry != NULL)
    	{
    		SRELEASE(entry);
    	}
    	else
    	{
    		AMP_DEBUG_WARN("oid_nn_cleanup","Found NULL entry in nickname db.",
    				         NULL);
    	}
    }
    lyst_destroy(nn_db);

    /* Step 3: Clean up locking mechanisms too. */
    unlockResource(&nn_db_mutex);
    killResourceLock(&nn_db_mutex);
}



/******************************************************************************
 *
 * \par Function Name: oid_nn_delete
 *
 * \par Purpose: Removes a nickname from the database.
 *
 * \retval 0 - Entry not found (or other error)
 * 		   !0 - Success.
 *
 * \param[in] nn_id The ID of the entry to remove.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/
int oid_nn_delete(uvast nn_id)
{
	oid_nn_t *cur_nn = NULL;
	LystElt tmp_elt;
	int result = 0;

	AMP_DEBUG_ENTRY("oid_nn_delete","(%#llx)",nn_id);

	/* Step 1: Need to lock to preserve validity of the lookup result. . */
	lockResource(&nn_db_mutex);

	/* Step 2: If you find it, delete it. */
	if((tmp_elt = oid_nn_exists(nn_id)) != NULL)
	{
    	cur_nn = (oid_nn_t*) lyst_data(tmp_elt);
		lyst_delete(tmp_elt);
		SRELEASE(cur_nn);
		result = 1;
	}

    unlockResource(&nn_db_mutex);

	AMP_DEBUG_EXIT("oid_nn_delete","->%d",result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: oid_nn_exists
 *
 * \par Purpose: Determines if an ID is in the nickname db.
 *
 * \retval NULL - Not found.
 * 		   !NULL - ELT pointing to the found element.
 *
 * \param[in] nn_id The ID of the nickname whose existence is in question.
 *
 * \todo There is probably a smarter way to do this with a lyst find function
 * 	     and a search callback.
 *
 * \par Notes:
 *		1. LystElt is a pointer. Handle this handle with care.
 *		2. We break abstraction here and return a Lyst structure because this
 *		   lookup function is often called in the context of lyst maintenance,
 *		   but if we were to change the underlying nickname database storage
 *		   approach, this function would, clearly, need to change. Those who
 *		   do not like this approach are referred to the much less deprecable
 *		   function: oid_find.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/
LystElt oid_nn_exists(uvast nn_id)
{
	oid_nn_t *cur_nn = NULL;
	LystElt tmp_elt = NULL;
	LystElt result = NULL;

	AMP_DEBUG_ENTRY("oid_nn_exists","(%#llx)",nn_id);

	/* Step 0: Make sure no +/-'s while we search. */
	lockResource(&nn_db_mutex);

    for(tmp_elt = lyst_first(nn_db); tmp_elt; tmp_elt = lyst_next(tmp_elt))
    {
    	cur_nn = (oid_nn_t*) lyst_data(tmp_elt);
    	if(cur_nn != NULL)
    	{
    		if(cur_nn->id == nn_id)
    		{
    			result = tmp_elt;
    			break;
    		}
    	}
    	else
    	{
    		AMP_DEBUG_WARN("oid_nn_exists","Encountered NULL nn?",NULL);
    	}
    }

    unlockResource(&nn_db_mutex);

	AMP_DEBUG_EXIT("oid_nn_delete","->%x",result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: oid_nn_find
 *
 * \par Purpose: Convenience function to grab a nickname entry from its ID.
 *
 * \retval NULL - Item not found.
 *         !NULL - Handle to the found item.
 *
 * \param[in] nn_id  The ID of the nickname to find.
 *
 * \todo There is probably a smarter way to do this with a lyst find function
 * 	     and a search callback.
 *
 * \par Notes:
 *		1. The returned pointer should NOT be released. It points directly into
 *		   the nickname list.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

oid_nn_t* oid_nn_find(uvast nn_id)
{
	LystElt tmpElt = NULL;
	oid_nn_t *result = NULL;

	AMP_DEBUG_ENTRY("oid_nn_find","(%#llx)",nn_id);

	/* Step 0: Call exists function (exists should block mutex, so we don't. */
	if((tmpElt = oid_nn_exists(nn_id)) != NULL)
	{
		result = (oid_nn_t*) lyst_data(tmpElt);
	}

	AMP_DEBUG_EXIT("oid_nn_find","->%#llx",result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: oid_nn_init
 *
 * \par Purpose: Initialize the nickname database for OIDs.
 *
 * \retval <0 - Failure.
 * 		    0 - Success.
 *
 * \param[in] nn_id  The ID of the nickname to find.
 *
 * \todo Add functions here to read the nickname database from persistent
 *       storage, such as an SDR.
 *
 * \par Notes:
 *		1. nn_db must only hold items that have been dynamically allocated
 *		   from the	memory pool.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  10/14/12  E. Birrane     Initial implementation,
 *****************************************************************************/

int oid_nn_init()
{
	AMP_DEBUG_ENTRY("oid_init_nn_db","()",NULL);

	/* Step 0: Sanity Check. */
	if(nn_db != NULL)
	{
		AMP_DEBUG_WARN("oid_nn_init","Trying to init existing NN db.",NULL);
		return 0;
	}

	if((nn_db = lyst_create()) == NULL)
	{
		AMP_DEBUG_ERR("oid_nn_init","Can't allocate NN DB!", NULL);
		AMP_DEBUG_EXIT("oid_nn_init","->-1.",NULL);
		return -1;
	}

	if(initResourceLock(&nn_db_mutex))
	{
        AMP_DEBUG_ERR("oid_init_nn_db","Unable to initialize mutex, errno = %s",
        		        strerror(errno));
        AMP_DEBUG_EXIT("oid_init_nn_db","->-1.",NULL);
        return -1;
	}

    AMP_DEBUG_EXIT("oid_init_nn_db","->0.",NULL);
	return 0;
}













