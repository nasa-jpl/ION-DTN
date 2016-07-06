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
 ** \file nm_types.h
 **
 ** Description: AMP Structures and Data Types
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR       DESCRIPTION
 **  --------  ------------ ---------------------------------------------
 **  06/27/15  E. Birrane   Initial Implementation. (Secure DTN - NASA: NNX14CS58P)
 **  06/30/16  E. Birrane   Update to AMP v0.3 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/
#include "nm_types.h"

const char * const dtnmp_type_str[] = {
										"AD",    /* DTNMP_TYPE_AD     */
										"CD",    /* DTNMP_TYPE_CD     */
										"RPT",   /* DTNMP_TYPE_RPT    */
										"CTRL",  /* DTNMP_TYPE_CTRL   */
										"SRL",   /* DTNMP_TYPE_SRL    */
										"TRL",   /* DTNMP_TYPE_TRL    */
										"MACRO", /* DTNMP_TYPE_MACRO  */
										"LIT",   /* DTNMP_TYPE_LIT    */
										"OP",    /* DTNMP_TYPE_OP     */
										"BYTE",  /* DTNMP_TYPE_BYTE   */
		                                "INT",    /* DTNMP_TYPE_INT    */
		                                "UINT",   /* DTNMP_TYPE_UINT   */
		                                "VAST",   /* DTNMP_TYPE_VAST   */
		                                "UVAST",  /* DTNMP_TYPE_UVAST  */
		                                "REAL32", /* DTNMP_TYPE_REAL32 */
		                                "REAL64", /* DTNMP_TYPE_REAL64 */
		                                "SDNV",   /* DTNMP_TYPE_SDNV   */
		                                "TS",     /* DTNMP_TYPE_TS     */
		                                "STRING", /* DTNMP_TYPE_STRING */
		                                "BLOB",   /* DTNMP_TYPE_BLOB   */
		                                "MID",    /* DTNMP_TYPE_MID    */
		                                "MC",     /* DTNMP_TYPE_MC     */
		                                "EXPR",   /* DTNMP_TYPE_EXPR   */
		                                "DC",     /* DTNMP_TYPE_DC     */
		                                "TDC",    /* DTNMP_TYPE_TDC    */
		                                "TABLE",  /* DTNMP_TYPE_TABLE  */
		                                "UNK"     /* DTNMP_TYPE_UNK    */
                                       };


const char * const dtnmp_type_fieldspec_str[] = {
		UHF,                 /* DTNMP_TYPE_AD     */
		UHF,                 /* DTNMP_TYPE_CD     */
		UHF,                 /* DTNMP_TYPE_RPT    */
		UHF,                 /* DTNMP_TYPE_CTRL   */
		UHF,                 /* DTNMP_TYPE_SRL    */
		UHF,                 /* DTNMP_TYPE_TRL    */
		UHF,                 /* DTNMP_TYPE_MACRO  */
		UHF,                 /* DTNMP_TYPE_LIT    */
	    UHF,                 /* DTNMP_TYPE_OP     */
		"%c",                /* DTNMP_TYPE_BYTE   */
        "%d",                /* DTNMP_TYPE_INT    */
        "%u",                /* DTNMP_TYPE_UINT   */
        VAST_FIELDSPEC,      /* DTNMP_TYPE_VAST   */
        UVAST_FIELDSPEC,     /* DTNMP_TYPE_UVAST  */
        "%f",                /* DTNMP_TYPE_REAL32 */
        "%f",                /* DTNMP_TYPE_REAL64 */
        UVAST_FIELDSPEC,     /* DTNMP_TYPE_SDNV   */
        UVAST_FIELDSPEC,     /* DTNMP_TYPE_TS     */
        "%s",                /* DTNMP_TYPE_STRING */
        UHF,                 /* DTNMP_TYPE_BLOB   */
        UHF,                 /* DTNMP_TYPE_MID    */
        UHF,                 /* DTNMP_TYPE_MC     */
        UHF,                 /* DTNMP_TYPE_EXPR   */
        UHF,                 /* DTNMP_TYPE_DC     */
        UHF,                 /* DTNMP_TYPE_TDC    */
        UHF,                 /* DTNMP_TYPE_TABLE  */
	    UHF                  /* DTNMP_TYPE_UNK    */
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
 *  03/13/15  J.P Mayer      Initial implementation (DLR)
 *  06/28/15  E. Birrane     Migrated to utils from datalist as part of TDC dev.
 *                           (Secure DTN - NASA: NNX14CS58P)
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
		default:                result = 0; break;
	}

	return result;
}



const char *type_to_str(dtnmp_type_e type)
{
	return dtnmp_type_str[type];
}



uint8_t type_is_numeric(dtnmp_type_e type)
{
	uint8_t result = 0;

	switch(type)
	{
		case DTNMP_TYPE_BYTE:
		case DTNMP_TYPE_INT:
		case DTNMP_TYPE_UINT:
		case DTNMP_TYPE_VAST:
		case DTNMP_TYPE_UVAST:
		case DTNMP_TYPE_REAL32:
		case DTNMP_TYPE_REAL64:
			result = 1;
			break;
		default:
			result = 0;
			break;
	}

	return result;
}

