/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: db.c
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for DTNMP actors to interact with
 **              SDRs to persistently store information.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  06/29/13  E. Birrane Initial Implementation (JHU/APL)
 **  08/21/16  E. Birrane     Update to AMP v02 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#include "db.h"


int  db_forget(Object *primitiveObj, Object *descObj, Object list)
{
	Sdr sdr = getIonsdr();
	Object elt;

	if((primitiveObj == NULL) || (descObj == NULL) || (list == 0))
	{
		AMP_DEBUG_ERR("db_forget","Bad Params.",NULL);
		return -1;
	}

	CHKERR(sdr_begin_xn(sdr));

	if(*primitiveObj != 0)
	{
	  sdr_free(sdr, *primitiveObj);
	}

	if(*descObj != 0)
	{
	   elt = sdr_list_first(sdr, list);
	   while(elt)
	   {
		   if(sdr_list_data(sdr, elt) == *descObj)
		   {
			   sdr_list_delete(sdr, elt, NULL, NULL);
			   sdr_free(sdr, *descObj);
			   elt = 0;
		   }
		   else
		   {
			   elt = sdr_list_next(sdr, elt);
		   }
	   }
	}

	sdr_end_xn(sdr);

	/* Forget now invalid SDR pointers. */
	*descObj = 0;
	*primitiveObj = 0;

	return 1;
}


/*
 * This function writes an item and its associated descriptor into the SDR,
 * allocating space for each, and adding the SDR descriptor pointer to a
 * given SDR list.
 *
 * item    : The serialized item to store in the SDR.
 * item_len: The size of the serialized item.
 * *itemObj: The SDR pointer to the serialized item in the SDR.
 * desc    : The item descriptor being written to the SDR.
 * desc_len: The size of the item descriptor.
 * *descObj: The SDR pointer to the item's descriptor object in the SDR.
 * list    : The SDR list holding the item descriptor (at *descrObj).
 */
int  db_persist(uint8_t  *item,
					  uint32_t  item_len,
					  Object   *itemObj,
					  void     *desc,
					  uint32_t  desc_len,
					  Object   *descObj,
					  Object    list)
{

   Sdr sdr = getIonsdr();

   CHKERR(sdr_begin_xn(sdr));


   /* Step 1: Allocate a descriptor object for this item in the SDR. */
   if((*descObj = sdr_malloc(sdr, desc_len)) == 0)
   {
	   sdr_cancel_xn(sdr);

	   AMP_DEBUG_ERR("db_persist",
			   	       "Can't allocate descriptor of size %d.",
			   	       desc_len);
	   return -1;
   }


   /* Step 2: Allocate space for the serialized rule in the SDR. */
   if((*itemObj = sdr_malloc(sdr, item_len)) == 0)
   {
	   sdr_free(sdr, *descObj);

	   sdr_cancel_xn(sdr);
	   *descObj = 0;
	   AMP_DEBUG_ERR("db_persist",
			   	   	   "Unable to allocate Item in SDR. Size %d.",
			           item_len);
	   return -1;
   }

   /* Step 3: Write the item to the SDR. */
   sdr_write(sdr, *itemObj, (char *) item, item_len);

   /* Step 4: Write the item descriptor to the SDR. */
   sdr_write(sdr, *descObj, (char *) desc, desc_len);

   /* Step 5: Save the descriptor in the AgentDB active rules list. */
   if (sdr_list_insert_last(sdr, list, *descObj) == 0)
   {
      sdr_free(sdr, *itemObj);
      sdr_free(sdr, *descObj);

      sdr_cancel_xn(sdr);

      *itemObj = 0;
      *descObj = 0;
      AMP_DEBUG_ERR("db_persist",
				        "Unable to insert item Descr. in SDR.", NULL);
      return -1;
   }

	if(sdr_end_xn(sdr))
	{
		AMP_DEBUG_ERR("db_persist", "Can't create Agent database.", NULL);
		return -1;
	}

   return 1;
}
