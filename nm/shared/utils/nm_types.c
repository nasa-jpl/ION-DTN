/*
 * nm_types.c
 *
 *  Created on: Jun 27, 2015
 *      Author: ebirrane
 */

#include "nm_types.h"

const char * const dtnmp_type_str[] = {"BYTE",   /* DTNMP_TYPE_BYTE   */
		                               "INT",    /* DTNMP_TYPE_INT    */
		                               "UINT",   /* DTNMP_TYPE_UINT   */
		                               "VAST",   /* DTNMP_TYPE_VAST   */
		                               "UVAST",  /* DTNMP_TYPE_UVAST  */
		                               "FLOAT",  /* DTNMP_TYPE_FLOAT  */
		                               "DOUBLE", /* DTNMP_TYPE_DOUBLE */
		                               "STRING", /* DTNMP_TYPE_STRING */
		                               "BLOB",   /* DTNMP_TYPE_BLOB   */
		                               "SDNV",   /* DTNMP_TYPE_SDNV   */
		                               "TS",     /* DTNMP_TYPE_TS     */
		                               "DC",     /* DTNMP_TYPE_DC     */
		                               "MID",    /* DTNMP_TYPE_MID    */
		                               "MC",     /* DTNMP_TYPE_MC     */
		                               "EXPR",   /* DTNMP_TYPE_EXPR   */
		                               "DEF",    /* DTNMP_TYPE_DEF    */
		                               "TRL",    /* DTNMP_TYPE_TRL    */
		                               "SRL",    /* DTNMP_TYPE_SRL    */
		                               "UNK"     /* DTNMP_TYPE_UNK    */
                                      };

const char * const dtnmp_type_fieldspec_str[] = {
  "%c",            /* DTNMP_TYPE_BYTE   */
  "%d",            /* DTNMP_TYPE_INT    */
  "%d",            /* DTNMP_TYPE_UINT   */
  VAST_FIELDSPEC,  /* DTNMP_TYPE_VAST   */
  UVAST_FIELDSPEC, /* DTNMP_TYPE_UVAST  */
  "%f",            /* DTNMP_TYPE_FLOAT  */
  "%f",            /* DTNMP_TYPE_DOUBLE */
  "%s",            /* DTNMP_TYPE_STRING */
  UVAST_FIELDSPEC, /* DTNMP_TYPE_BLOB   */
  UVAST_FIELDSPEC, /* DTNMP_TYPE_SDNV   */
  "%d",            /* DTNMP_TYPE_TS     */
  UVAST_FIELDSPEC, /* DTNMP_TYPE_DC     */
  UVAST_FIELDSPEC, /* DTNMP_TYPE_MID    */
  UVAST_FIELDSPEC, /* DTNMP_TYPE_MC     */
  UVAST_FIELDSPEC, /* DTNMP_TYPE_EXPR   */
  UVAST_FIELDSPEC, /* DTNMP_TYPE_DEF    */
  UVAST_FIELDSPEC, /* DTNMP_TYPE_TRL    */
  UVAST_FIELDSPEC, /* DTNMP_TYPE_SRL    */
  UVAST_FIELDSPEC  /* DTNMP_TYPE_UNK    */
};



dtnmp_type_e type_from_str(char *str)
{
	dtnmp_type_e result = DTNMP_TYPE_UNK;
	int i = 0;

	if(str == NULL)
	{
		return result;
	}

	for(i = 0; i < DTNMP_TYPE_UNK; i++)
	{
		if(strcasecmp(str, dtnmp_type_str[i]) == 0)
		{
			result = (dtnmp_type_e) i;
			break;
		}
	}

	return result;
}


const char *   type_get_fieldspec(dtnmp_type_e type)
{
	return dtnmp_type_fieldspec_str[type];
}

/******************************************************************************
 *
 * \par Function Name: type_get_size
 *
 * \par Purpose: This is basically sizeof
 *
 * \return The size, or 0 if A) something went wrong, or
 *         B) the variable is variable length
 *
 * \param[in]   type		The requested type
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/13/15  J.P Mayer      Initial implementation
 *  06/28/15  E. Birrane     Migrated to utils from datalist as part of TDC dev.
 *****************************************************************************/

size_t   type_get_size(dtnmp_type_e type)
{
	size_t result = 0;

	switch(type)
	{
		case DTNMP_TYPE_BYTE:   result = sizeof(uint8_t);  break;
		case DTNMP_TYPE_INT:    result = sizeof(int32_t);  break;
		case DTNMP_TYPE_UINT:   result = sizeof(uint32_t); break;
		case DTNMP_TYPE_VAST:   result = sizeof(vast);     break;
		case DTNMP_TYPE_UVAST:  result = sizeof(uvast);    break;
		case DTNMP_TYPE_REAL32:  result = sizeof(float);    break;
		case DTNMP_TYPE_REAL64: result = sizeof(double);   break;
		case DTNMP_TYPE_TS:     result = sizeof(uint32_t); break;
		case DTNMP_TYPE_DC:
		case DTNMP_TYPE_STRING:
		case DTNMP_TYPE_BLOB:
		case DTNMP_TYPE_SDNV:
		case DTNMP_TYPE_MID:
		case DTNMP_TYPE_MC:
		case DTNMP_TYPE_EXPR:
		case DTNMP_TYPE_DEF:
		case DTNMP_TYPE_TRL:
		case DTNMP_TYPE_SRL:
		case DTNMP_TYPE_UNK:
		default:                result = 0; break;
	}

	return result;
}

const char *   type_to_str(dtnmp_type_e type)
{
	return dtnmp_type_str[type];
}

