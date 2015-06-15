/*****************************************************************************
 **
 ** \file ui_input.h
 **
 **
 ** Description: Functions to retrieve information from the user via a
 **              text-based interface.
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  05/24/15  E. Birrane     Initial Implementation
 *****************************************************************************/

#include "mgr/ui_input.h"


/******************************************************************************
 *
 * \par Function Name: ui_input_get_line
 *
 * \par Read a line of input from the user.
 *
 * \par Notes:
 *
 * \retval 0 - Could not get user input.
 * 		   1 - Got user input.
 *
 * \param[in]  prompt   The prompt to the user.
 * \param[out] line     The line of text read from the user.
 * \param [in] max_len  The maximum size of the line.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *****************************************************************************/

int ui_input_get_line(char *prompt, char **line, int max_len)
{
	int len = 0;

	DTNMP_DEBUG_ENTRY("ui_input_get_line","(%s,0x%x,%d)",prompt, *line, max_len);

	while(len == 0)
	{
		printf("\nNote: Only the first %d characters will be read.\n%s\n",
				max_len, prompt);

		if (igets(fileno(stdin), (char *)line, max_len, &len) == NULL)
		{
			if (len != 0)
			{
				DTNMP_DEBUG_ERR("ui_input_get_line","igets failed.", NULL);
				DTNMP_DEBUG_EXIT("ui_input_get_line","->0.",NULL);
				return 0;
			}
		}
	}

	DTNMP_DEBUG_INFO("ui_input_get_line","Read user input.", NULL);

	DTNMP_DEBUG_EXIT("ui_input_get_line","->1.",NULL);
	return 1;
}


uint8_t* ui_input_blob(char *prompt, uint32_t *len)
{
	uint8_t *result = NULL;
	char *str = ui_input_string(prompt);

	result = utils_string_to_hex(str, len);

	MRELEASE(str);

	return result;
}


uint8_t  ui_input_byte(char *prompt)
{
	uint8_t result = 0;
	char line[2];

	ui_input_get_line(prompt, (char**)&line, 2);

	sscanf(line, "%c", &result);

	return result;
}

double   ui_input_double(char *prompt)
{
	double result = 0;
	char line[20];

	ui_input_get_line(prompt, (char**)&line, 20);

	sscanf(line, "%lf", &result);

	return result;
}

float    ui_input_float(char *prompt)
{
	float result = 0;
	char line[20];

	ui_input_get_line(prompt, (char**)&line, 20);

	sscanf(line, "%f", &result);

	return result;
}


Sdnv     ui_input_sdnv(char *prompt)
{
	Sdnv val_sdnv;
	uvast val = ui_input_uvast(prompt);

	encodeSdnv(&val_sdnv, val);

	return val_sdnv;
}


char *   ui_input_string(char *prompt)
{
	char *result = NULL;
	char line[MAX_INPUT_BYTES];

	ui_input_get_line(prompt, (char**)&line, MAX_INPUT_BYTES);

	result = (char *) MTAKE(strlen(line));

	sscanf(line, "%s", result);

	return result;
}

int32_t     ui_input_int(char *prompt)
{
	int32_t result = 0;
	char line[20];

	ui_input_get_line(prompt, (char**)&line, 20);

	sscanf(line, "%d", &result);

	return result;
}

uint32_t     ui_input_uint(char *prompt)
{
	uint32_t result = 0;
	char line[20];

	ui_input_get_line(prompt, (char**)&line, 20);

	sscanf(line, "%u", &result);

	return result;
}

uvast     ui_input_uvast(char *prompt)
{
	uvast result = 0;
	char line[MAX_INPUT_BYTES];

	ui_input_get_line(prompt, (char**)&line, MAX_INPUT_BYTES);

	sscanf(line, UVAST_FIELDSPEC, &result);

	return result;
}

vast     ui_input_vast(char *prompt)
{
	vast result = 0;
	char line[MAX_INPUT_BYTES];

	ui_input_get_line(prompt, (char**)&line, MAX_INPUT_BYTES);

	sscanf(line, VAST_FIELDSPEC, &result);

	return result;
}


Lyst ui_input_dc(char *prompt)
{
	uint32_t num = 0;
	uint32_t i = 0;
	Lyst result = lyst_create();
	datacol_entry_t *entry = NULL;

	printf("DC Input: %s", prompt);
	num = ui_input_uint("# Items:");

	for(i = 0; i < num; i++)
	{
		char str[20];
		sprintf(str,"Item #%d", i);

		entry = (datacol_entry_t *) MTAKE(sizeof(datacol_entry_t));
		entry->value = ui_input_blob(str, &(entry->length));

		lyst_insert_last(result, entry);
	}

	return result;
}


Lyst ui_input_mc(char *prompt)
{
	uint32_t i = 0;
	uint32_t num = 0;
	Lyst result = lyst_create();

	printf("MC: %s", prompt);

	num = ui_input_uint("# MIDS:");

	for(i = 0; i < num; i++)
	{
		char prompt[20];
		sprintf(prompt, "Enter MID %d", i);
		mid_t *cur = ui_input_mid(prompt, ADM_ALL, MID_TYPE_ANY, MID_CAT_ANY);
		lyst_insert_last(result, cur);
	}

	return result;
}


/******************************************************************************
 *
 * \par Function Name: ui_input_mid
 *
 * \par Construct a MID object completely from user input.
 *
 * \par Notes:
 *
 * \retval NULL  - Problem building the MID.
 * 		   !NULL - The constructed MID.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *  06/25/13  E. Birrane     Removed references to priority field. Add ISS flag.
 *****************************************************************************/

mid_t *ui_input_mid(char *prompt, uint8_t adm_type, uint8_t type, uint8_t cat)
{
	mid_t *result = NULL;
	Lyst parms;
	ui_parm_spec_t* spec = NULL;


	/* Step 1: Print the prompt. */
	printf("MID:%s\n.", prompt);

	printf("\n----------------------------------------------------\n");
	printf("Welcome to MID Builder.\n You may enter a MID by:\n");
	printf("\t1) Typing the entire MID in hex.\n");
	printf("\t2) Selecting an existing MID from a list.\n");
	printf("\t3) Building a MID in pieces.\n");
	printf("----------------------------------------------------\n\n");

	uint32_t opt = ui_input_uint("Select One:");

	switch(opt)
	{
		case 1:
			result = ui_input_mid_raw();
			break;
		case 2:
			result = ui_input_mid_list(adm_type, type, cat);
			break;
		case 3:
			result = ui_input_mid_build();
			break;
		default:
			return NULL;
			break;
	}


	/* Step 7: If the MID is parameterized, find and
	 * input the parameters. Re-create the MID.
	 */
	switch(MID_GET_FLAG_OID(result->flags))
	{
		case OID_TYPE_PARAM:
		case OID_TYPE_COMP_PARAM:

			/* Get the parameter spec for this MID. */
			if((spec = ui_get_parmspec(result)) == NULL)
			{
				DTNMP_DEBUG_ERR("ui_input_mid", "No Parm Spec?", NULL);
				mid_release(result);

				DTNMP_DEBUG_EXIT("ui_input_mid","->NULL.", NULL);
				return NULL;
			}

			/* Get the parameters. */
			parms = ui_input_parms(spec);

			/* Add the parameters. */
			oid_add_params(result->oid, parms);

			/* Release the parameters. */
			dc_destroy(&parms);

			mid_internal_serialize(result);
			break;
	}

	/* Step 5: Sanity check this mid. */
	if(mid_sanity_check(result) == 0)
	{
		DTNMP_DEBUG_ERR("ui_input_mid", "Sanity check failed.", NULL);
		DTNMP_DEBUG_EXIT("ui_input_mid","->NULL.", NULL);
		mid_release(result);
		return NULL;
	}

	char *mid_str = mid_to_string(result);
	DTNMP_DEBUG_ALWAYS("ui_input_mid", "Defined MID: %s", mid_str);
	MRELEASE(mid_str);

	DTNMP_DEBUG_EXIT("ui_input_mid", "->0x%x", (unsigned long) result);
	return result;
}

mid_t* ui_input_mid_build()
{
	mid_t *result = NULL;

	/* Step 0: Allocate the resultant MID. */
	if((result = (mid_t*)MTAKE(sizeof(mid_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("ui_input_mid","Can't alloc %d bytes.", sizeof(mid_t));
		DTNMP_DEBUG_EXIT("ui_input_mid", "->NULL.", NULL);
		return NULL;
	}
	else
	{
		memset(result,0,sizeof(mid_t));
	}

	/* Step 2: Get and process the MID flag. */
	ui_input_mid_flag(&(result->flags));
	result->type = MID_GET_FLAG_TYPE(result->flags);
	result->category = MID_GET_FLAG_CAT(result->flags);

	/* Step 3: Grab Issuer, if necessary. */
	if(MID_GET_FLAG_ISS(result->flags))
	{
		result->issuer = ui_input_uvast("Issuer:");
	}

	/* Step 4: Read in the OID sans Parms. */
	if((result->oid = ui_input_oid(result->flags)) == NULL)
	{
		DTNMP_DEBUG_ERR("ui_input_mid", "Bad OID.", NULL);
		mid_release(result);

		DTNMP_DEBUG_EXIT("ui_input_mid","->NULL.", NULL);
		return NULL;
	}

	/* Step 5: Grab a tag, if necessary. */
	if(MID_GET_FLAG_TAG(result->flags))
	{
		result->tag = ui_input_uvast("Tag:");
	}

	/* Step 6: Create a version of the MID. */
	mid_internal_serialize(result);

	return result;
}


/******************************************************************************
 *
 * \par Function Name: ui_input_mid_flag
 *
 * \par Construct a MID flag byte completely from user input.
 *
 * \par Notes:
 *
 * \retval 0 - Can't construct flag byte
 * 		   1 - Flag byte constructed
 *
 * \param[out]  flag   The resulting flags
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *  06/25/13  E. Birrane     Removed references to priority field.Add ISS Flag.
 *****************************************************************************/

int ui_input_mid_flag(uint8_t *flag)
{
	int result = 0;
	char line[256];
	int tmp;

	*flag = 0;

	ui_input_get_line("Type: Data (0), Ctrl (1), Literal (2), Op (3):",
			          (char**)&line, 256);
	sscanf(line,"%d",&tmp);
	*flag = (tmp & 0x3);

	ui_input_get_line("Cat: Atomic (0), Computed (1), Collection (2):",
			          (char**)&line, 256);
	sscanf(line,"%d",&tmp);
	*flag |= (tmp & 0x3) << 2;

	ui_input_get_line("Issuer Field Present? Yes (1)  No (0):",
			          (char**)&line, 256);
	sscanf(line,"%d",&tmp);
	*flag |= (tmp & 0x1) << 4;

	ui_input_get_line("Tag Field Present? Yes (1)  No (0):",
			          (char**)&line, 256);
	sscanf(line,"%d",&tmp);
	*flag |= (tmp & 0x1) << 5;

	ui_input_get_line("OID Type: Full (0), Parm (1), Comp (2), Parm+Comp(3):",
			          (char**)&line, 256);
	sscanf(line,"%d",&tmp);
	*flag |= (tmp & 0x3) << 6;

	printf("Constructed Flag Byte: 0x%x\n", *flag);

	return 1;
}


mid_t *ui_input_mid_list(uint8_t adm_type, uint8_t type, uint8_t cat)
{
	mid_t *result = NULL;
//	int type, cat;
	uint32_t opt = 0;

	if((type == MID_TYPE_ANY) && (cat == MID_CAT_ANY))
	{

		printf("\n--------------------------------\n");
		printf("What kind of MID?\n");
		printf("\t1. Atomic Data   2. Computed Data\n");
		printf("\t3. Literal       4. Control\n");
		printf("\t5. Macro         6. Report\n");
		printf("\t7. Operator\n");
		printf("\n--------------------------------\n");

		opt = ui_input_uint("Select One:");

		switch(opt)
		{
			case 1:
				type = MID_TYPE_DATA;
				cat = MID_CAT_ATOMIC;
				break;
			case 2:
				type = MID_TYPE_DATA;
				cat = MID_CAT_COMPUTED;
				break;
			case 3:
				type = MID_TYPE_LITERAL;
				cat = MID_CAT_ATOMIC;
				break;
			case 4:
				type = MID_TYPE_CONTROL;
				cat = MID_CAT_ATOMIC;
				break;
			case 5:
				type = MID_TYPE_CONTROL;
				cat = MID_CAT_COLLECTION;
				break;
			case 6:
				type = MID_TYPE_DATA;
				cat = MID_CAT_COLLECTION;
				break;
			case 7:
				type = MID_TYPE_OPERATOR;
				cat = MID_CAT_ATOMIC;
				break;
			default:
				return NULL;
		}
	}

	ui_list_gen(adm_type, type, cat);

	opt = ui_input_uint("Select One:");

	result = ui_get_mid(adm_type, type, cat, opt);

	return result;
}


mid_t*             ui_input_mid_raw()
{
	mid_t *result = NULL;
	uint32_t size = 0;
	uint32_t bytes = 0;
	uint8_t *data = ui_input_blob("0x", &size);

	result = mid_deserialize((unsigned char *)data, size, &bytes);
	MRELEASE(data);

	return result;
}



oid_t* ui_input_oid(uint8_t mid_flags)
{
	uint8_t *data = NULL;
	uint8_t *data2 = NULL;
	uint32_t size = 0;
	uint32_t size2 = 0;
	uint32_t bytes = 0;
	oid_t *result = NULL;
	char line[MAX_INPUT_BYTES];
	Sdnv id;


	switch(MID_GET_FLAG_OID(mid_flags))
	{
		case OID_TYPE_FULL:
			printf("Entering a Full OID.\n");
			printf("+------+\n");
			printf("| OID  |\n");
			printf("| [DC] |\n");
			printf("+------+\n\n");
			break;

		case OID_TYPE_PARAM:
			printf("Entering a FULL Parameterized OID.\n");
			printf("+------+---------+--------+-----+--------+\n");
			printf("| OID  | # Parms | Parm 1 | ... | Parm N |\n");
			printf("| [DC] |  [SDNV] |  [DC]  |     |  [DC}  |\n");
			printf("+------+---------+--------+-----+--------+\n\n");
			break;

		case OID_TYPE_COMP_FULL:
			printf("Entering a Compressed Full OID.\n");
			printf("+--------+------+\n");
			printf("| NN ID  | OID  |\n");
			printf("| [SDNV] | [DC] |\n");
			printf("+--------+------+\n\n");
			break;


		case OID_TYPE_COMP_PARAM:
			printf("Entering a Compressed Parameterized OID.\n");
			printf("+--------+------+---------+--------+-----+--------+\n");
			printf("| NN ID  | OID  | # Parms | Parm 1 | ... | Parm N |\n");
			printf("| [SDNV] | [DC] |  [SDNV] |  [DC]  |     |  [DC}  |\n");
			printf("+--------+------+---------+--------+-----+--------+\n\n");
			break;

		default:
			DTNMP_DEBUG_ERR("ui_input_oid","Unknown OID Type %d",
					    	 MID_GET_FLAG_OID(mid_flags));
			return NULL;
			break;
	}

	/* Get the non-parameterized parts of the OID. */

	switch(MID_GET_FLAG_OID(mid_flags))
	{
		case OID_TYPE_FULL:
		case OID_TYPE_PARAM:
			ui_input_get_line("Enter [len][octet 1]...[octet N]: 0x", (char**)&line, MAX_INPUT_BYTES);
			data = utils_string_to_hex(line, &size);
			result = oid_deserialize_full(data, size, &bytes);
			MRELEASE(data);
			break;

		case OID_TYPE_COMP_FULL:
		case OID_TYPE_COMP_PARAM:

			id = ui_input_sdnv("Compressed OID\nEnter NN Id:");
			ui_input_get_line("Enter Root OID: [len][octet 1]...[octet N]: 0x", (char**)&line, MAX_INPUT_BYTES);
			data2 = utils_string_to_hex(line, &size2);

			size = size2 + id.length;
			data = (uint8_t *) MTAKE(size);
			memcpy(data, id.text, id.length);
			memcpy(data+id.length, data2, size2);
			MRELEASE(data2);
			result = oid_deserialize_comp(data, size, &bytes);
			MRELEASE(data);
			break;

		default:
			DTNMP_DEBUG_ERR("ui_input_mid","Unknown OID Type %d",
					    MID_GET_FLAG_OID(mid_flags));
			break;
	}

	return result;
}


/*
 * Return a DC in lyst format.
 * \todo: Network Byte Order.
 */
Lyst               ui_input_parms(ui_parm_spec_t *spec)
{
	Lyst result;
	datacol_entry_t *entry = NULL;
	int i;

	if(spec == NULL)
	{
		return NULL;
	}

	if((result = lyst_create()) == NULL)
	{
		return NULL;
	}

	for(i = 0; i < spec->num_parms; i++)
	{
		entry = (datacol_entry_t*) MTAKE(sizeof(datacol_entry_t));

		switch(spec->parm_type[i])
		{
			case DTNMP_TYPE_BYTE:
			{
				uint8_t val = ui_input_byte("Enter BYTE:");
				dc_add(result, &val, 1);
			}
				break;
			case DTNMP_TYPE_INT:
			{
				int32_t  val = ui_input_int("Enter Signed Integer:");
				dc_add(result, (uint8_t*)&val, sizeof(val));
			}
				break;
			case DTNMP_TYPE_UINT:
			{
				uint32_t  val = ui_input_uint("Enter Unsigned Integer:");
				dc_add(result, (uint8_t*)&val, sizeof(val));
			}
				break;
			case DTNMP_TYPE_VAST:
			{
				vast  val = ui_input_vast("Enter Vast:");
				dc_add(result, (uint8_t*)&val, sizeof(val));
			}
				break;
			case DTNMP_TYPE_UVAST:
			{
				uvast  val = ui_input_uvast("Enter Uvast:");
				dc_add(result, (uint8_t*)&val, sizeof(val));
			}
				break;
			case DTNMP_TYPE_FLOAT:
			{
				float  val = ui_input_float("Enter Float:");
				dc_add(result, (uint8_t*)&val, sizeof(val));
			}
				break;
			case DTNMP_TYPE_DOUBLE:
			{
				double  val = ui_input_double("Enter Double:");
				dc_add(result, (uint8_t*)&val, sizeof(val));
			}
				break;
			case DTNMP_TYPE_STRING:
			{
				char *val = ui_input_string("Enter String:");
				if(val != NULL)
				{
					dc_add(result, (uint8_t*)val, strlen(val));
					MRELEASE(val);
				}
			}
				break;
			case DTNMP_TYPE_BLOB:
			{
				uint32_t len;
				uint8_t *val = ui_input_blob("Enter BLOB:", &len);
				if(val != NULL)
				{
					dc_add(result, val, len);
					MRELEASE(val);
				}
			}
				break;
			case DTNMP_TYPE_SDNV:
			{
				Sdnv val = ui_input_sdnv("Enter SDNV (as uint):");
				dc_add(result, (uint8_t*)&(val.text), val.length);
			}
				break;
			case DTNMP_TYPE_TS:
			{
				uint32_t  val = ui_input_uint("Enter Timestamp:");
				dc_add(result, (uint8_t*)&val, sizeof(val));
			}
				break;
			case DTNMP_TYPE_DC:
			{
				Lyst user_dc = ui_input_dc("Enter DC:");
				uint32_t size = 0;
				uint8_t *val = dc_serialize(user_dc, &size);
				dc_destroy(&user_dc);
				dc_add(result, val, size);
				MRELEASE(val);
			}
				break;
			case DTNMP_TYPE_MID:
			{
				mid_t *mid = ui_input_mid("Enter MID:", ADM_ALL, MID_TYPE_ANY, MID_CAT_ANY);
				uint32_t size = 0;
				uint8_t *val = mid_serialize(mid, &size);
				mid_release(mid);
				dc_add(result, val, size);
				MRELEASE(val);
			}
				break;
			case DTNMP_TYPE_MC:
			{
				Lyst mc = ui_input_mc("Enter MID Collection:");
				uint32_t size = 0;
				uint8_t *val = midcol_serialize(mc, &size);
				midcol_destroy(&mc);
				dc_add(result, val, size);
				MRELEASE(val);
			}
				break;
			case DTNMP_TYPE_EXPR:
			{
				Lyst mc = ui_input_mc("Enter Expression as MID Collection:");
				uint32_t size = 0;
				uint8_t *val = midcol_serialize(mc, &size);
				midcol_destroy(&mc);
				dc_add(result, val, size);
				MRELEASE(val);
			}
				break;
		}
	}

	return result;
}


