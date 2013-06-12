/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 **
 **     This material may only be used, modified, or reproduced by or for the
 **       U.S. Government pursuant to the license rights granted under
 **          FAR clause 52.227-14 or DFARS clauses 252.227-7013/7014
 **
 **     For any other permissions, please contact the Legal Office at JHU/APL.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: adm.c
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for the identification and
 **              processing of Application Data Models (ADMs).
 **
 ** Notes:
 **       1) We need to find some more efficient way of querying ADMs by name
 **          and by MID. The current implementation uses too much stack space.
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/22/11  E. Birrane     Initial Implementation
 **  11/13/12  E. Birrane     Technical review, comment updates.
 *****************************************************************************/

#include "ion.h"
#include "platform.h"

#include "shared/utils/nm_types.h"
#include "shared/utils/utils.h"

#include "shared/adm/adm.h"
#include "shared/adm/adm_bp.h"
#include "shared/adm/adm_ltp.h"
#include "shared/adm/adm_ion.h"


int gNumAduData = 0;
adm_entry_t adus[MAX_NUM_ADUS];

int gNumAduCtrls = 0;
adm_ctrl_t ctrls[MAX_NUM_CTRLS];


int gNumAduLiterals = 0;
int gNumAduOperators = 0;

/******************************************************************************
 *
 * \par Function Name: adm_add
 *
 * \par Registers a pre-configured ADM with the local DTNMP actor.
 *
 * \param[in] name      Name of the ADM entry.
 * \param[in] mid_str   MID value, as a string.
 * \param[in] num_parms # parms needed for parameterized OIDs.
 * \param[in] collect   The data collection function.
 * \param[in] to_string The to-string function
 * \param[in] get_size  The sizing function for the ADM entry.
 *
 * \par Notes:
 *		1. When working with parameterized OIDs, the given MID string should
 *		   be all information excluding the parameterized portion of the OID.
 *		2. ADM names will be truncated after ADM_MAX_NAME bytes.
 *		3. This is the only function that should increment gNumAduData.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *****************************************************************************/

void adm_add(char *name,
		     char *mid_str,
		     int num_parms,
		     adm_data_collect_fn collect, adm_string_fn to_string,
		     adm_size_fn get_size)
{
	int id = gNumAduData;
	uint8_t *tmp = NULL;
	uint32_t size = 0;


	DTNMP_DEBUG_ENTRY("adm_add","(%#llx, %#llx, %d, %#llx, %#llx, %#llx)",
			          name, mid_str, num_parms, collect, to_string, get_size);

	/* Step 0 - Sanity Checks. */
	if((name == NULL) || (mid_str == NULL) || (collect == NULL) ||
	   (to_string == NULL) || (get_size == NULL))
	{
		DTNMP_DEBUG_ERR("adm_add","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_add","->.", NULL);
		return;
	}

	/* Step 1 - Check name length. */
	if(strlen(name) > ADM_MAX_NAME)
	{
		DTNMP_DEBUG_WARN("adm_add","Trunc. %s to %d bytes.", name, ADM_MAX_NAME)
	}

	/* Step 2 - Identify ADM slot and initialize it. */
	gNumAduData++;
	memset(&(adus[id]),0,sizeof(adm_entry_t));

	/* Step 3 - Copy the ADM information. */
	strncpy((char *)adus[id].name, name, ADM_MAX_NAME);
	tmp = utils_string_to_hex((unsigned char *)mid_str,&size);
	memcpy(adus[id].mid, tmp, size);
	MRELEASE(tmp);

	adus[id].mid_len = size;
	adus[id].num_parms = num_parms;
	adus[id].collect = collect;
	adus[id].to_string = to_string;
	adus[id].get_size = get_size;

	DTNMP_DEBUG_EXIT("adm_add","->.", NULL);
	return;
}




/******************************************************************************
 *
 * \par Function Name: adm_add_ctrl
 *
 * \par Registers a pre-configured ADM control with the local DTNMP actor.
 *
 * \param[in] name      Name of the ADM control.
 * \param[in] mid_str   MID value, as a string.
 * \param[in] num_parms # parms needed for parameterized OIDs.
 * \param[in] control   The control collection function.
 *
 * \par Notes:
 *		1. When working with parameterized OIDs, the given MID string should
 *		   be all information excluding the parameterized portion of the OID.
 *		2. ADM names will be truncated after ADM_MAX_NAME bytes.
 *		3. This is the only function that should increment gNumAduCtrls.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *****************************************************************************/

void adm_add_ctrl(char *name,
		     char *mid_str,
		     int num_parms,
		     adm_ctrl_fn run)
{
	int id = gNumAduCtrls;
	uint8_t *tmp = NULL;
	uint32_t size = 0;


	DTNMP_DEBUG_ENTRY("adm_add_ctrl","(%#llx, %#llx, %d, %#llx)",
			          name, mid_str, num_parms, run);

	/* Step 0 - Sanity Checks. */
	if((name == NULL) || (mid_str == NULL) || (run == NULL))
	{
		DTNMP_DEBUG_ERR("adm_add_ctrl","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_ctrl","->.", NULL);
		return;
	}

	/* Step 1 - Check name length. */
	if(strlen(name) > ADM_MAX_NAME)
	{
		DTNMP_DEBUG_WARN("adm_add_ctrl","Trunc. %s to %d bytes.", name, ADM_MAX_NAME)
	}

	/* Step 2 - Identify ADM slot and initialize it. */
	gNumAduCtrls++;
	memset(&(ctrls[id]),0,sizeof(adm_ctrl_t));

	/* Step 3 - Copy the ADM information. */
	strncpy((char *)ctrls[id].name, name, ADM_MAX_NAME);
	tmp = utils_string_to_hex((unsigned char *)mid_str,&size);
	memcpy(ctrls[id].mid, tmp, size);
	MRELEASE(tmp);

	ctrls[id].mid_len = size;
	ctrls[id].num_parms = num_parms;
	ctrls[id].run = run;

	DTNMP_DEBUG_EXIT("adm_add_ctrl","->.", NULL);
	return;
}


/******************************************************************************
 *
 * \par Function Name: adm_copy_integer
 *
 * \par Copies and serializes integer values of various sizes.
 *
 * \retval NULL Failure
 *         !NULL The serialized integer.
 *
 * \param[in]  value    Byte pointer to integer value.
 * \param[in]  size     Byte size of integer value.
 * \param[out] length   Size of returned integer copy.
 *
 * \par Notes:
 *		1. The serialized integer copy is allocated on the heap and must be
 *		   released when no longer needed.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *****************************************************************************/

uint8_t *adm_copy_integer(uint8_t *value, uint8_t size, uint64_t *length)
{
	uint8_t *result = NULL;

	DTNMP_DEBUG_ENTRY("adm_copy_integer","(%#llx, %d, %#llx)", value, size, length);

	/* Step 0 - Sanity Check. */
	if((value == NULL) || (size <= 0) || (length == NULL))
	{
		DTNMP_DEBUG_ERR("adm_copy_integer","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_copy_integer","->NULL.", NULL);
		return NULL;
	}

	/* Step 1 - Alloc new space. */
	if((result = (uint8_t *) MTAKE(size)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_copy_integer","Can't alloc %d bytes.", size);
		DTNMP_DEBUG_EXIT("adm_copy_integer","->NULL.", NULL);
		return NULL;
	}

	/* Step 2 - Copy data in. */
	*length = size;
	memcpy(result, value, size);

	/* Step 3 - Return. */
	DTNMP_DEBUG_EXIT("adm_copy_integer","->%#llx", result);
	return (uint8_t*)result;
}



/******************************************************************************
 *
 * \par Function Name: adm_find
 *
 * \par Find an ADM entry that corresponds to a received MID.
 *
 * \retval NULL Failure
 *         !NULL The found ADM entry
 *
 * \param[in]  mid  The MID whose ADM-match is being queried.
 *
 * \par Notes:
 *		1. The returned entry is a direct pointer to the official ADM entry,
 *		   it must be treated as read-only.
 *		2. When the input MID is parameterized, the ADM find function only
 *		   matches the non-parameterized portion.
 *		3. This function is not complete, compare must be made to work when
 *		   tag values are in play
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *****************************************************************************/

adm_entry_t *adm_find(mid_t *mid)
{
	int i;

	DTNMP_DEBUG_ENTRY("adm_find","(%#llx)", mid);

	/* Step 0 - Sanity Check. */
	if(mid == NULL)
	{
		DTNMP_DEBUG_ERR("adm_find", "Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_find", "->NULL.", NULL);
		return NULL;
	}

	/* Step 1 - Go lookin'. */
	for(i = 0; i < gNumAduData; i++)
	{
		/* Step 1.1 - Determine if we need to account for parameters. */
		if(adus[i].num_parms == 0)
		{
			/* Step 1.1.1 - If no params, straight compare */
			if((mid->raw_size == adus[i].mid_len) &&
				(memcmp(mid->raw, adus[i].mid, mid->raw_size) == 0))
			{
				break;
			}
		}
		else
		{
			uvast tmp;
			unsigned char *cursor = (unsigned char*) &(adus[i].mid[1]);
			/* Grab size less paramaters. Which is SDNV at [1]. */
			/* \todo: We need a more refined compare here.  For example, the
			 *        code below will not work if tag values are used.
			 */
			unsigned long bytes = decodeSdnv(&tmp, cursor);
			if(memcmp(mid->raw, adus[i].mid, tmp + bytes + 1) == 0)
			{
				break;
			}

		}
	}

	/* Step 2 - See if we found it. */
	if(i >= gNumAduData)
	{
		DTNMP_DEBUG_EXIT("adm_find", "->NULL.", NULL);
		return NULL;
	}

	DTNMP_DEBUG_EXIT("adm_find", "->%#llx.", &(adus[i]));

	return &(adus[i]);
}



/******************************************************************************
 *
 * \par Function Name: adm_find_ctrl
 *
 * \par Find an ADM control that corresponds to a received MID.
 *
 * \retval NULL Failure
 *         !NULL The found ADM control
 *
 * \param[in]  mid  The MID whose ADM-match is being queried.
 *
 * \par Notes:
 *		1. The returned entry is a direct pointer to the official ADM control,
 *		   it must be treated as read-only.
 *		2. When the input MID is parameterized, the ADM find function only
 *		   matches the non-parameterized portion.
 *		3. This function is not complete, compare must be made to work when
 *		   tag values are in play
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/22/13  E. Birrane     Initial implementation.
 *****************************************************************************/

adm_ctrl_t*  adm_find_ctrl(mid_t *mid)
{
	int i;

	DTNMP_DEBUG_ENTRY("adm_find_ctrl","(%#llx)", mid);

	/* Step 0 - Sanity Check. */
	if(mid == NULL)
	{
		DTNMP_DEBUG_ERR("adm_find_ctrl", "Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_find_ctrl", "->NULL.", NULL);
		return NULL;
	}

	/* Step 1 - Go lookin'. */
	for(i = 0; i < gNumAduCtrls; i++)
	{
		/* Step 1.1 - Determine if we need to account for parameters. */
		if(ctrls[i].num_parms == 0)
		{
			/* Step 1.1.1 - If no params, straight compare */
			if((mid->raw_size == ctrls[i].mid_len) &&
				(memcmp(mid->raw, ctrls[i].mid, mid->raw_size) == 0))
			{
				break;
			}
		}
		else
		{
			uvast tmp;
			unsigned char *cursor = (unsigned char*) &(ctrls[i].mid[1]);
			/* Grab size less paramaters. Which is SDNV at [1]. */
			/* \todo: We need a more refined compare here.  For example, the
			 *        code below will not work if tag values are used.
			 */
			unsigned long bytes = decodeSdnv(&tmp, cursor);
			if(memcmp(mid->raw, ctrls[i].mid, tmp + bytes + 1) == 0)
			{
				break;
			}

		}
	}

	/* Step 2 - See if we found it. */
	if(i >= gNumAduCtrls)
	{
		DTNMP_DEBUG_EXIT("adm_find_ctrl", "->NULL.", NULL);
		return NULL;
	}

	DTNMP_DEBUG_EXIT("adm_find_ctrl", "->%#llx.", &(ctrls[i]));

	return &(ctrls[i]);
}


/******************************************************************************
 *
 * \par Function Name: adm_init
 *
 * \par Initialize pre-configured ADMs.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *****************************************************************************/

void adm_init()
{
	DTNMP_DEBUG_ENTRY("adm_init","()", NULL);
	initBpAdm();
	initLtpAdm();
	initIonAdm();
	DTNMP_DEBUG_EXIT("adm_init","->.", NULL);
}



/******************************************************************************
 *
 * \par Function Name: adm_print_string
 *
 * \par Performs the somewhat straightforward function of building a string
 *      representation of a string. This is a generic to-string function for
 *      ADM entries whose values are strings.
 *
 * \retval NULL Failure
 *         !NULL The string representation of the ADM entry value.
 *
 * \param[in]  buffer      The start of the ADM entry value.
 * \param[in]  buffer_len  Length of the given buffer.
 * \param[in]  data_len    Length of data item at head of the buffer.
 * \param[out] str_len     Length of returned string from print function.
 *
 * \par Notes:
 *		1. The string representation is allocated on the heap and must be
 *		   freed when no longer necessary.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *****************************************************************************/
char *adm_print_string(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len)
{
	char *result = NULL;
	uint32_t len = 0;

	DTNMP_DEBUG_ENTRY("adm_print_string","(%#llx, %ull, %ull, %#llx)", buffer, buffer_len, data_len, str_len);

	/* Step 0 - Sanity Checks. */
	if((buffer == NULL) || (str_len == NULL))
	{
		DTNMP_DEBUG_ERR("adm_print_string", "Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_print_string", "->NULL.", NULL);
		return NULL;
	}

	/* Step 1 - Data at head of buffer should be a string. Grab len & check. */
	len = strlen((char*) buffer);

	if((len < 0) || (len > buffer_len) || (len != data_len))
	{
		DTNMP_DEBUG_ERR("adm_print_string", "Bad len %d. Expected %d.",
				        len, data_len);
		DTNMP_DEBUG_EXIT("adm_print_string", "->NULL.", NULL);
		return NULL;
	}

	/* Step 2 - Allocate size for string rep. of the string value. */
	*str_len = len + 1;
	if((result = (char *) MTAKE(*str_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_print_string", "Can't alloc %d bytes",
				        *str_len);
		DTNMP_DEBUG_EXIT("adm_print_string", "->NULL.", NULL);
		return NULL;
	}

	/* Step 3 - Copy over. */
	sprintf(result,"%s", (char*) buffer);

	DTNMP_DEBUG_EXIT("adm_print_string", "->%s.", result);

	return result;
}



/******************************************************************************
 *
 * \par Function Name: adm_print_string_list
 *
 * \par Generates a single string representation of a list of strings.
 *
 * \retval NULL Failure
 *         !NULL The string representation of the ADM entry value.
 *
 * \param[in]  buffer      The start of the ADM entry value.
 * \param[in]  buffer_len  Length of the given buffer.
 * \param[in]  data_len    Length of data item at head of the buffer.
 * \param[out] str_len     Length of returned string from print function.
 *
 * \par Notes:
 *		1. The string representation is allocated on the heap and must be
 *		   freed when no longer necessary.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *****************************************************************************/

char *adm_print_string_list(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len)
{
	char *result = NULL;
	char *cursor = NULL;
	uint8_t *buf_ptr = NULL;
	uvast num = 0;
	int len = 0;
	int i;

	DTNMP_DEBUG_ENTRY("adm_print_string_list", "(%#llx, %ull, %ull, %#llx)", buffer, buffer_len, data_len, str_len);

	/* Step 0 - Sanity Checks. */
	if((buffer == NULL) || (str_len == NULL))
	{
		DTNMP_DEBUG_ERR("adm_print_string_list", "Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_print_string_list", "->NULL.", NULL);
		return NULL;
	}

	/* Step 1 - Figure out size of resulting string. */
	buf_ptr = buffer;
	len = decodeSdnv(&num, buf_ptr);
	buf_ptr += len;

	*str_len = data_len + /* String length   */
		   9 +            /* Header info.    */
		   (2 * len) +    /* ", " per string */
		   1;             /* Trailer.        */

	/* Step 2 - Allocate the result. */
	if((result = (char *) MTAKE(*str_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_print_string_list","Can't alloc %d bytes", *str_len);

		*str_len = 0;
		DTNMP_DEBUG_EXIT("adm_print_string_list", "->NULL.", NULL);
		return NULL;
	}

	/* Step 3 - Accumulate the result. */
	cursor = result;

	cursor += sprintf(cursor,"(UVAST_FIELDSPEC): ",num);

	/* Add stirngs to result. */
	for(i = 0; i < num; i++)
	{
		cursor += sprintf(cursor, "%s, ",buf_ptr);
		buf_ptr += strlen((char*)buf_ptr) + 1;
	}

	DTNMP_DEBUG_EXIT("adm_print_string_list", "->%#llx.", result);
	return result;
}




/******************************************************************************
 *
 * \par Function Name: adm_print_unsigned_long
 *
 * \par Generates a single string representation of an unsigned long.
 *
 * \retval NULL Failure
 *         !NULL The string representation of the ADM entry value.
 *
 * \param[in]  buffer      The start of the ADM entry value.
 * \param[in]  buffer_len  Length of the given buffer.
 * \param[in]  data_len    Length of data item at head of the buffer.
 * \param[out] str_len     Length of returned string from print function.
 *
 * \par Notes:
 *		1. The string representation is allocated on the heap and must be
 *		   freed when no longer necessary.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *****************************************************************************/
char *adm_print_unsigned_long(uint8_t* buffer, uint64_t buffer_len,
		                      uint64_t data_len, uint32_t *str_len)
{
  char *result;
  uint64_t temp = 0;

  DTNMP_DEBUG_ENTRY("adm_print_unsigned_long", "(%#llx, %ull, %ull, %#llx)", buffer, buffer_len, data_len, str_len);

  /* Step 0 - Sanity Checks. */
  if((buffer == NULL) || (str_len == NULL))
  {
	  DTNMP_DEBUG_ERR("adm_print_unsigned_long", "Bad Args.", NULL);
	  DTNMP_DEBUG_EXIT("adm_print_unsigned_long", "->NULL.", NULL);
	  return NULL;
  }

  /* Step 1 - Make sure we have buffer space. */
  if(data_len > buffer_len)
  {
	 DTNMP_DEBUG_ERR("adm_print_unsigned_long","Data Len %d > buf len %d.",
			         data_len, buffer_len);
	 *str_len = 0;

	 DTNMP_DEBUG_EXIT("adm_print_unsigned_long", "->NULL.", NULL);
	 return NULL;
  }

  /* Step 2 - Size the string and allocate it.
   * \todo: A better estimate should go here. */
  *str_len = 22;

  if((result = (char *) MTAKE(*str_len)) == NULL)
  {
		 DTNMP_DEBUG_ERR("adm_print_unsigned_long","Can't alloc %d bytes.",
				         *str_len);
		 *str_len = 0;

		 DTNMP_DEBUG_EXIT("adm_print_unsigned_long", "->NULL.", NULL);
		 return NULL;
  }

  /* Step 3 - Copy data and return. */
  memcpy(&temp, buffer, data_len);
  isprintf(result,*str_len,"%ld", temp);

  DTNMP_DEBUG_EXIT("adm_print_unsigned_long", "->%#llx.", result);
  return result;
}



/******************************************************************************
 *
 * \par Function Name: adm_print_unsigned_long_list
 *
 * \par Generates a single string representation of a list of unsigned longs.
 *
 * \retval NULL Failure
 *         !NULL The string representation of the ADM entry value.
 *
 * \param[in]  buffer      The start of the ADM entry value.
 * \param[in]  buffer_len  Length of the given buffer.
 * \param[in]  data_len    Length of data item at head of the buffer.
 * \param[out] str_len     Length of returned string from print function.
 *
 * \par Notes:
 *		1. The string representation is allocated on the heap and must be
 *		   freed when no longer necessary.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *****************************************************************************/
char *adm_print_unsigned_long_list(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len)
{
	char *result = NULL;
	char *cursor = NULL;
	uint8_t *buf_ptr = NULL;
	uvast num = 0;
	unsigned long val = 0;
	int len = 0;
	int i;

	DTNMP_DEBUG_ENTRY("adm_print_unsigned_long_list", "(%#llx, %ull, %ull, %#llx)", buffer, buffer_len, data_len, str_len);

	/* Step 0 - Sanity Checks. */
	if((buffer == NULL) || (str_len == NULL))
	{
		DTNMP_DEBUG_ERR("adm_print_unsigned_long_list", "Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_print_unsigned_long_list", "->NULL.", NULL);
		return NULL;
	}

	/* Step 1 - Figure out how many unsigned longs we need to print out. */
	buf_ptr = buffer;
	len = decodeSdnv(&num, buf_ptr);
	buf_ptr += len;

	/* Step 2 - Size & allocate the string. */
	*str_len = data_len + /* Data length   */
		   9 +            /* Header info.    */
		   (2 * len) +    /* ", " per number */
		   1;             /* Trailer.        */

	if((result = (char *) MTAKE(*str_len)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_print_unsigned_long_list", "Can't alloc %d bytes.",
				        *str_len);
		*str_len = 0;
		DTNMP_DEBUG_EXIT("adm_print_unsigned_long_list", "->NULL.", NULL);
		return NULL;
	}

	/* Step 3 - Accumulate string result. */
	cursor = result;

	cursor += sprintf(cursor,"(UVAST_FIELDSPEC): ",num);

	for(i = 0; i < num; i++)
	{
		memcpy(&val, buf_ptr, sizeof(val));
		buf_ptr += sizeof(val);
		cursor += sprintf(cursor, "%ld, ",val);
	}

	DTNMP_DEBUG_EXIT("adm_print_unsigned_long_list", "->%#llx.", result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: adm_size_string
 *
 * \par Calculates size of a string, as an ADM sizing callback.
 *
 * \retval Size of the ADM value entry.
 *
 * \param[in]  buffer      The start of the ADM entry value.
 * \param[in]  buffer_len  Length of the given buffer.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *****************************************************************************/

uint32_t adm_size_string(uint8_t* buffer, uint64_t buffer_len)
{
	uint32_t len = 0;

	DTNMP_DEBUG_ENTRY("adm_size_string","(%#llx, %ull)", buffer, buffer_len);

	/* Step 0 - Sanity Check. */
	if(buffer == NULL)
	{
		DTNMP_DEBUG_ERR("adm_size_string","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_size_string","->0.", NULL);
		return 0;
	}

	len = strlen((char*) buffer);
	if(len > buffer_len)
	{
		DTNMP_DEBUG_ERR("adm_size_string","Bad len: %ul > %ull.", len, buffer_len);
		DTNMP_DEBUG_EXIT("adm_size_string","->0.", NULL);
		return 0;
	}

	DTNMP_DEBUG_EXIT("adm_size_string","->%ul", len);
	return len;
}



/******************************************************************************
 *
 * \par Function Name: adm_size_string_list
 *
 * \par Calculates size of a string list, as an ADM sizing callback.
 *
 * \retval Size of the ADM value entry.
 *
 * \param[in]  buffer      The start of the ADM entry value.
 * \param[in]  buffer_len  Length of the given buffer.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *****************************************************************************/

uint32_t adm_size_string_list(uint8_t* buffer, uint64_t buffer_len)
{
	uint32_t result = 0;
	uvast num = 0;
	uint8_t *cursor = NULL;
	int tmp = 0;
	int i;

	DTNMP_DEBUG_ENTRY("adm_size_string_list","(%#llx, %ull)", buffer, buffer_len);

	/* Step 0 - Sanity Check. */
	if(buffer == NULL)
	{
		DTNMP_DEBUG_ERR("adm_size_string_list","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_size_string_list","->0.", NULL);
		return 0;
	}

	/* Step 1 - Figure out # strings. */
	result = decodeSdnv(&num, buffer);
	cursor = buffer + result;

	/* Add up the strings to calculate length. */
	for(i = 0; i < num; i++)
	{
		tmp = strlen((char *)cursor) + 1;
		result += tmp;
		cursor += tmp;
	}

	DTNMP_DEBUG_EXIT("adm_size_string_list", "->%ul", result);
	return result;
}



/******************************************************************************
 *
 * \par Function Name: adm_size_unsigned_long
 *
 * \par Calculates size of an unsigned long, as an ADM sizing callback.
 *
 * \retval Size of the ADM value entry.
 *
 * \param[in]  buffer      The start of the ADM entry value.
 * \param[in]  buffer_len  Length of the given buffer.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *****************************************************************************/
uint32_t adm_size_unsigned_long(uint8_t* buffer, uint64_t buffer_len)
{
	uint32_t len = 0;

	DTNMP_DEBUG_ENTRY("adm_size_unsigned_long","(%#llx, %ull)", buffer, buffer_len);

	/* Step 0 - Sanity Check. */
	if(buffer == NULL)
	{
		DTNMP_DEBUG_ERR("adm_size_unsigned_long","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_size_unsigned_long","->0.", NULL);
		return 0;
	}

	DTNMP_DEBUG_EXIT("adm_size_string","->%ul", sizeof(unsigned long));
	return sizeof(unsigned long);
}



/******************************************************************************
 *
 * \par Function Name: adm_size_unsigned_long_list
 *
 * \par Calculates size of a list of unsigned long, as an ADM sizing callback.
 *
 * \retval Size of the ADM value entry.
 *
 * \param[in]  buffer      The start of the ADM entry value.
 * \param[in]  buffer_len  Length of the given buffer.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *****************************************************************************/
uint32_t adm_size_unsigned_long_list(uint8_t* buffer, uint64_t buffer_len)
{
	uint32_t result = 0;
	uvast num = 0;

	DTNMP_DEBUG_ENTRY("adm_size_unsigned_long","(%#llx, %ull)", buffer, buffer_len);

	/* Step 0 - Sanity Check. */
	if(buffer == NULL)
	{
		DTNMP_DEBUG_ERR("adm_size_unsigned_long","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_size_unsigned_long","->0.", NULL);
		return 0;
	}

	/* Step 1 - Calculate size. This is size of the SDNV, plus size of the
	 *          "num" of unsigned longs after the SDNV.
	 */
	result = decodeSdnv(&num, buffer);
	result += (num * sizeof(unsigned long));

	DTNMP_DEBUG_EXIT("adm_size_string","->%ul", result);
	return result;
}

