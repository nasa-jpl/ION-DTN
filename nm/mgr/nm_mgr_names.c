/*****************************************************************************
 **
 ** \file nm_mgr_names.c
 **
 **
 ** Description: Helper file holding optional hard-coded human-readable
 **              names and descriptions for supported ADM entries.
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  04/26/15  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "mgr/nm_mgr_names.h"


Lyst gMgrNames;

int names_add_name(char *name, char *desc, int adm, char *mid_str)
{

	mgr_name_t *entry = NULL;
	uint32_t used = 0;

	if(gMgrNames == NULL)
	{
		return 0;
	}

	if((entry = (mgr_name_t *) STAKE(sizeof(mgr_name_t))) == NULL)
	{
		return 0;
	}

	entry->adm = adm;
	memcpy(entry->name, name, NAMES_NAME_MAX-1);
	entry->name[NAMES_NAME_MAX-1] = '\0';

	memcpy(entry->descr, desc, NAMES_DESCR_MAX-1);
	entry->descr[NAMES_DESCR_MAX-1] = '\0';

	uint32_t hex_size = 0;
	uint8_t *mid_hex = utils_string_to_hex(mid_str, &hex_size);
	if(mid_hex == NULL)
	{
		AMP_DEBUG_ERR("names_add_name","Can't made hex from %s.", mid_str);
		SRELEASE(entry);
		AMP_DEBUG_EXIT("names_add_name","-> 0.", NULL);
		return 0;
	}
	entry->mid = mid_deserialize(mid_hex, hex_size, &used);
	SRELEASE(mid_hex);

	if(entry->mid == NULL)
	{
		SRELEASE(entry);
		return 0;
	}

	lyst_insert_last(gMgrNames, entry);

	return 1;
}

mid_t *names_get_mid(int adm_type, int mid_id, int idx)
{
	LystElt elt;
	mgr_name_t *cur = NULL;
	int i = 0;

	if(gMgrNames == NULL)
	{
		return NULL;
	}

	for(elt = lyst_first(gMgrNames); elt != NULL; elt = lyst_next(elt))
	{
		cur = (mgr_name_t *) lyst_data(elt);

		if(((adm_type == cur->adm) || (adm_type == ADM_ALL)) &&
		   ((mid_id == MID_GET_FLAG_ID(cur->mid->flags)) || (mid_id == MID_ANY)))
		{
			if(i == idx)
			{
				return cur->mid;
			}

			i++;
		}
	}

	return NULL;
}


char *names_get_name(mid_t *mid)
{
	char *result = NULL;

	LystElt elt;
	mgr_name_t *cur = NULL;

	if(mid == NULL)
	{
		return NULL;
	}

	if(gMgrNames == NULL)
	{
		return NULL;
	}

	for(elt = lyst_first(gMgrNames); elt != NULL; elt = lyst_next(elt))
	{
		cur = (mgr_name_t *) lyst_data(elt);

		if(mid_compare(mid, cur->mid, 0) == 0)
		{
			if((result = (char *) STAKE(strlen(cur->name) + 1)) == NULL)
			{
				return NULL;
			}
			strcpy(result, cur->name);
			return result;
		}

	}

	/* Last chance... just print the HEX. */
	result = mid_to_string(mid);

	return result;
}


void names_init()
{

    if((gMgrNames = lyst_create()) == NULL)
    {
    	AMP_DEBUG_ERR("names_init","Failed to create known names list.",NULL);
    }

}

/*
 * Do not remove items...
 */
Lyst names_retrieve(int adm_type, int mid_id)
{
	Lyst result = NULL;
	LystElt elt;
	mgr_name_t *cur = NULL;

	if(gMgrNames == NULL)
	{
		return result;
	}

	if((result = lyst_create()) == NULL)
	{
		return result;
	}

	for(elt = lyst_first(gMgrNames); elt != NULL; elt = lyst_next(elt))
	{
		cur = (mgr_name_t *) lyst_data(elt);

		if(((adm_type == cur->adm) || (adm_type == ADM_ALL)) &&
		   ((mid_id == MID_GET_FLAG_ID(cur->mid->flags)) || (mid_id == MID_ANY)))
		{
			lyst_insert_last(result, cur);
		}
	}

	return result;
}


void names_lyst_destroy(Lyst *names)
{
	LystElt elt;
	mgr_name_t *cur;

	for(elt = lyst_first(gMgrNames); elt != NULL; elt = lyst_next(elt))
	{
		cur = (mgr_name_t *) lyst_data(elt);
		names_destroy_entry(cur);
	}

	lyst_destroy(gMgrNames);
	gMgrNames = NULL;
}

void names_destroy_entry(mgr_name_t *name)
{
	if(name != NULL)
	{
		mid_release(name->mid);
		SRELEASE(name);
	}
}
