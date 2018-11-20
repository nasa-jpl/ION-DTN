/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
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
 **  09/02/18  E. Birrane   Cleanup and update to latest spec. (JHU/APL)
 *****************************************************************************/
#include "nm_types.h"

const char * const amp_type_str[] = {
										 "CNST",    /* AMP_TYPE_CNST    */
										 "CTRL",    /* AMP_TYPE_CTRL    */
										 "EDD",     /* AMP_TYPE_EDD     */
										 "LIT",     /* AMP_TYPE_LIT     */
										 "MAC",     /* AMP_TYPE_MAC     */
										 "OPER",    /* AMP_TYPE_OPER    */
										 "RPT",     /* AMP_TYPE_RPT     */
										 "RPTT",    /* AMP_TYPE_RPTT    */
										 "SBR",     /* AMP_TYPE_SBR     */
										 "TBL",     /* AMP_TYPE_TBL     */
										 "TBLT",    /* AMP_TYPE_TBLT    */
										 "TBR",     /* AMP_TYPE_TBR     */
										 "VAR",     /* AMP_TYPE_VAR     */
										 "RSV1",
										 "RSV2",
										 "RSV3",

										 /* Primitive Types */
										 "BOOL",    /* AMP_TYPE_BOOL    */
										 "BYTE",    /* AMP_TYPE_BYTE    */
										 "STR",     /* AMP_TYPE_STR     */
										 "INT",     /* AMP_TYPE_INT     */
										 "UINT",    /* AMP_TYPE_UINT    */
										 "VAST",    /* AMP_TYPE_VAST    */
										 "UVAST",   /* AMP_TYPE_UVAST   */
										 "REAL32",  /* AMP_TYPE_REAL32  */
										 "REAL64",  /* AMP_TYPE_REAL64  */
										 "RSV4",
										 "RSV5",
										 "RSV6",
										 "RSV7",
										 "RSV8",
										 "RSV9",
										 "RSV10",

										 /* Compound Objects */
										 "TV",      /* AMP_TYPE_TV      */
										 "TS",      /* AMP_TYPE_TS      */
										 "TNV",     /* AMP_TYPE_TNV     */
										 "TNVC",    /* AMP_TYPE_TNVC    */
										 "ARI",     /* AMP_TYPE_ARI     */
										 "AC",      /* AMP_TYPE_AC      */
										 "EXPR",    /* AMP_TYPE_EXPR    */
										 "BYTESTR", /* AMP_TYPE_BYTESTR */
										 "UNK"
};


amp_type_e type_from_str(char *str)
{
	amp_type_e result = AMP_TYPE_UNK;
	int i = 0;

	if(str == NULL)
	{
		return result;
	}

	for(i = 0; i < AMP_TYPE_UNK; i++)
	{
		if(strcasecmp(str, amp_type_str[i]) == 0)
		{
			result = (amp_type_e) i;
			break;
		}
	}

	return result;
}


amp_type_e type_from_uint(uint32_t type)
{
	if(type >= AMP_TYPE_UNK)
	{
		return AMP_TYPE_UNK;
	}
	return (amp_type_e) type;
}




const char *type_to_str(amp_type_e type)
{
	return amp_type_str[type];
}



uint8_t type_is_numeric(amp_type_e type)
{
	uint8_t result = 0;

	switch(type)
	{
		case AMP_TYPE_BOOL:
		case AMP_TYPE_BYTE:
		case AMP_TYPE_INT:
		case AMP_TYPE_UINT:
		case AMP_TYPE_VAST:
		case AMP_TYPE_UVAST:
		case AMP_TYPE_REAL32:
		case AMP_TYPE_REAL64:
			result = 1;
			break;
		default:
			result = 0;
			break;
	}

	return result;
}

uint8_t      type_is_known(amp_type_e type)
{
	return (type < AMP_TYPE_UNK);
}
