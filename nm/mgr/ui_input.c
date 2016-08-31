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
 **  05/24/15  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
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

	AMP_DEBUG_ENTRY("ui_input_get_line","(%s,0x%x,%d)",prompt, *line, max_len);

	while(len == 0)
	{
		printf("\nNote: Only the first %d characters will be read.\n%s\n",
				max_len, prompt);

		if (igets(fileno(stdin), (char *)line, max_len, &len) == NULL)
		{
			if (len != 0)
			{
				AMP_DEBUG_ERR("ui_input_get_line","igets failed.", NULL);
				AMP_DEBUG_EXIT("ui_input_get_line","->0.",NULL);
				return 0;
			}
		}
	}

	AMP_DEBUG_INFO("ui_input_get_line","Read user input.", NULL);

	AMP_DEBUG_EXIT("ui_input_get_line","->1.",NULL);
	return 1;
}

blob_t *ui_input_file_contents(char *prompt)
{
	blob_t *result = NULL;
	char *filename = NULL;
	FILE *fp = NULL;
	long int file_size = 0;
	uint8_t *data = NULL;
	char *str = NULL;

	if((filename = ui_input_string("Enter filename:")) == NULL)
	{
		return NULL;
	}

	if((fp = fopen(filename, "rb")) == NULL)
	{
		AMP_DEBUG_ERR("ui_input_file_contents","Could not open %s.", filename);
		SRELEASE(filename);
		return NULL;
	}

	/* Grab the file size. */
	if((fseek(fp, 0L, SEEK_END)) != 0)
	{
		AMP_DEBUG_ERR("ui_input_file_contents","Couldn't seek to end of %s", filename);
		SRELEASE(filename);
		fclose(fp);
		return NULL;
	}

	if((file_size = ftell(fp)) == -1)
	{
		AMP_DEBUG_ERR("ui_input_file_contents","Couldn't get size of %s", filename);
		SRELEASE(filename);
		fclose(fp);
		return NULL;
	}
	rewind(fp);

	if((data = STAKE(file_size)) == NULL)
	{
		AMP_DEBUG_ERR("ui_input_file_contents","Couldn't allocate %d bytes.", file_size);
		SRELEASE(filename);
		fclose(fp);
		return NULL;
	}

	if((fread(data, 1, file_size, fp)) != file_size)
	{
		SRELEASE(data);
		AMP_DEBUG_ERR("ui_input_file_contents","Couldn't read %d bytes from %s", file_size, filename);
		SRELEASE(filename);
		fclose(fp);
		return NULL;
	}

	/* Shallow copy. */
	result = blob_create(data, file_size);

	str = utils_hex_to_string(data, file_size);
	AMP_DEBUG_ALWAYS("Read from %s: %.50s...", filename, str);
	SRELEASE(str);
	SRELEASE(filename);
	fclose(fp);

	return result;
}


blob_t* ui_input_blob(char *prompt, uint8_t no_file)
{
	blob_t *result = NULL;
	uint32_t len = 0;
	char *str = NULL;
	uint8_t *value = NULL;
	uint32_t opt = 1;


	/* Step 2: If we do not allow file input, default to text input.*/
	if(no_file == 0)
	{
		printf("\n----------------------------------------------------\n");
		printf("\t1) Typing the BLOB in hex.\n");
		printf("\t2) Select a file to read in BLOB.\n");
		printf("----------------------------------------------------\n\n");

		opt = ui_input_uint("Select One:");
	}

	switch(opt)
	{
		case 1:
			str = ui_input_string(prompt);
			value = utils_string_to_hex(str, &len);
			SRELEASE(str);
			result = blob_create(value, len);
			break;
		case 2:
			result = ui_input_file_contents(prompt);
			break;
		default:
			return NULL;
			break;
	}

	return result;
}


uint8_t  ui_input_byte(char *prompt)
{
	uint8_t result = 0;
	char line[3];

	ui_input_get_line(prompt, (char**)&line, 2);

	line[2] = '\0';

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

	result = (char *) STAKE(strlen(line) + 1);

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
	blob_t *entry = NULL;

	printf("DC Input: %s", prompt);
	num = ui_input_uint("# Items:");

	if(num == 0)
	{
		entry = blob_create(NULL, 0);
		lyst_insert_last(result, entry);
		return result;
	}

	for(i = 0; i < num; i++)
	{
		char str[20];
		sprintf(str,"Item #%d", i);

		entry = ui_input_blob(str,0);
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
		mid_t *cur = ui_input_mid(prompt, ADM_ALL, MID_ANY);
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
 *  07/05/16  E. Birrane     Check for NULL result. Add File input.
 *****************************************************************************/

mid_t *ui_input_mid(char *prompt, uint8_t adm_type, uint8_t id)
{
	mid_t *result = NULL;
	tdc_t *parms;
	ui_parm_spec_t* spec = NULL;


	/* Step 1: Print the prompt. */
	printf("MID:%s\n.", prompt);

	printf("\n----------------------------------------------------\n");
	printf("Welcome to MID Builder.\n You may enter a MID by:\n");
	printf("\t1) Typing the entire MID in hex.\n");
	printf("\t2) Selecting an existing MID from a list.\n");
	printf("\t3) Building a MID in pieces.\n");
	printf("\t4) CANCEL.\n");
	printf("----------------------------------------------------\n\n");

	uint32_t opt = ui_input_uint("Select One:");

	switch(opt)
	{
		case 1:
			result = ui_input_mid_raw(1);
			break;
		case 2:
			result = ui_input_mid_list(adm_type, id);
			break;
		case 3:
			result = ui_input_mid_build();
			break;
		default:
			return NULL;
			break;
	}

	if(result == NULL)
	{
		return NULL;
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
				AMP_DEBUG_ERR("ui_input_mid", "No Parm Spec?", NULL);
				mid_release(result);

				AMP_DEBUG_EXIT("ui_input_mid","->NULL.", NULL);
				return NULL;
			}

			/* Get the parameters. */
			parms = ui_input_parms(spec);

			/* Add the parameters. */
			oid_add_params(&(result->oid), parms);

			/* Release the parameters. */
			tdc_destroy(&parms);

			mid_internal_serialize(result);

			break;
	}

	/* Step 5: Sanity check this mid. */
	if(mid_sanity_check(result) == 0)
	{
		AMP_DEBUG_ERR("ui_input_mid", "Sanity check failed.", NULL);
		AMP_DEBUG_EXIT("ui_input_mid","->NULL.", NULL);
		mid_release(result);
		return NULL;
	}

	char *mid_str = mid_to_string(result);
	printf("ui_input_mid - Defined MID: %s\n", mid_str);
	fflush(stdout);
	SRELEASE(mid_str);

	AMP_DEBUG_EXIT("ui_input_mid", "->0x%x", (unsigned long) result);
	return result;
}

mid_t* ui_input_mid_build()
{
	mid_t *result = NULL;

	/* Step 0: Allocate the resultant MID. */
	if((result = (mid_t*)STAKE(sizeof(mid_t))) == NULL)
	{
		AMP_DEBUG_ERR("ui_input_mid","Can't alloc %d bytes.", sizeof(mid_t));
		AMP_DEBUG_EXIT("ui_input_mid", "->NULL.", NULL);
		return NULL;
	}
	else
	{
		memset(result,0,sizeof(mid_t));
	}

	/* Step 2: Get and process the MID flag. */
	ui_input_mid_flag(&(result->flags));
	result->id = MID_GET_FLAG_ID(result->flags);

	/* Step 3: Grab Issuer, if necessary. */
	if(MID_GET_FLAG_ISS(result->flags))
	{
		result->issuer = ui_input_uvast("Issuer:");
	}

	/* Step 4: Read in the OID sans Parms. */
	result->oid = ui_input_oid(result->flags);
	if(result->oid.type == OID_TYPE_UNK)
	{
		AMP_DEBUG_ERR("ui_input_mid", "Bad OID.", NULL);
		mid_release(result);

		AMP_DEBUG_EXIT("ui_input_mid","->NULL.", NULL);
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


mid_t *ui_input_mid_list(uint8_t adm_type, uint8_t mid_id)
{
	mid_t *result = NULL;
	uint32_t opt = 0;
	uint8_t selected_id = mid_id;

	if(selected_id == MID_ANY)
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
				selected_id = MID_ATOMIC;
				break;
			case 2:
				selected_id = MID_COMPUTED;
				break;
			case 3:
				selected_id = MID_LITERAL;
				break;
			case 4:
				selected_id = MID_CONTROL;
				break;
			case 5:
				selected_id = MID_MACRO;
				break;
			case 6:
				selected_id = MID_REPORT;
				break;
			case 7:
				selected_id = MID_OPERATOR;
				break;
			default:
				return NULL;
		}
	}

	ui_list_gen(adm_type, selected_id);

	opt = ui_input_uint("Select One:");

	result = ui_get_mid(adm_type, selected_id, opt);

	return result;
}


mid_t* ui_input_mid_raw(uint8_t no_file)
{
	mid_t *result = NULL;
	uint32_t size = 0;
	uint32_t bytes = 0;

	blob_t *data = ui_input_blob("0x", no_file);

	result = mid_deserialize((unsigned char *)data->value, data->length, &bytes);
	blob_destroy(data, 1);

	return result;
}



oid_t ui_input_oid(uint8_t mid_flags)
{
	uint8_t *data = NULL;
	uint8_t *data2 = NULL;
	uint32_t size = 0;
	uint32_t size2 = 0;
	uint32_t bytes = 0;
	oid_t result;
	char line[MAX_INPUT_BYTES];
	Sdnv id;

	oid_init(&result);

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
			AMP_DEBUG_ERR("ui_input_oid","Unknown OID Type %d",
					    	 MID_GET_FLAG_OID(mid_flags));
			return oid_get_null();
	}

	/* Get the non-parameterized parts of the OID. */

	switch(MID_GET_FLAG_OID(mid_flags))
	{
		case OID_TYPE_FULL:
		case OID_TYPE_PARAM:
			ui_input_get_line("Enter [len][octet 1]...[octet N]: 0x", (char**)&line, MAX_INPUT_BYTES);
			data = utils_string_to_hex(line, &size);
			result = oid_deserialize_full(data, size, &bytes);
			SRELEASE(data);
			break;

		case OID_TYPE_COMP_FULL:
		case OID_TYPE_COMP_PARAM:

			id = ui_input_sdnv("Compressed OID\nEnter NN Id:");
			ui_input_get_line("Enter Root OID: [len][octet 1]...[octet N]: 0x", (char**)&line, MAX_INPUT_BYTES);
			data2 = utils_string_to_hex(line, &size2);

			size = size2 + id.length;
			data = (uint8_t *) STAKE(size);
			if (data == NULL)
			{
				putErrmsg("Failed allocating OID.", NULL);
				return oid_get_null();
			}

			memcpy(data, id.text, id.length);
			memcpy(data+id.length, data2, size2);
			SRELEASE(data2);
			result = oid_deserialize_comp(data, size, &bytes);
			SRELEASE(data);
			break;

		default:
			AMP_DEBUG_ERR("ui_input_mid","Unknown OID Type %d",
					    MID_GET_FLAG_OID(mid_flags));
			break;
	}

	return result;
}


/*
 * \todo: Network Byte Order.
 * \todo: check return values.
 */
tdc_t *ui_input_parms(ui_parm_spec_t *spec)
{
	tdc_t *result = NULL;
	uint8_t *data = NULL;
	uint32_t size = 0;
	char prompt[256];
	int i;

	if(spec == NULL)
	{
		return NULL;
	}

	if((result = tdc_create(NULL, 0, 0)) == NULL)
	{
		return NULL;
	}

	for(i = 0; i < spec->num_parms; i++)
	{
		data = NULL;
		size = 0;

		switch(spec->parm_type[i])
		{
			case AMP_TYPE_BYTE:
			{
				sprintf(prompt,"(BYTE) %s", spec->parm_name[i]);
				uint8_t val = ui_input_byte(prompt);
				data = utils_serialize_byte(val, &size);
			}
				break;
			case AMP_TYPE_INT:
			{
				sprintf(prompt,"(INT) %s", spec->parm_name[i]);
				int32_t  val = ui_input_int(prompt);
				data = utils_serialize_int(val, &size);
			}
				break;
			case AMP_TYPE_UINT:
			{
				sprintf(prompt,"(UINT) %s", spec->parm_name[i]);
				uint32_t  val = ui_input_uint(prompt);
				data = utils_serialize_uint(val, &size);
			}
				break;
			case AMP_TYPE_VAST:
			{
				sprintf(prompt,"(VAST) %s", spec->parm_name[i]);
				vast  val = ui_input_vast(prompt);
				data = utils_serialize_vast(val, &size);
			}
				break;
			case AMP_TYPE_UVAST:
			{
				sprintf(prompt,"(UVAST) %s", spec->parm_name[i]);
				uvast  val = ui_input_uvast(prompt);
				data = utils_serialize_uvast(val, &size);
			}
				break;
			case AMP_TYPE_REAL32:
			{
				sprintf(prompt,"(REAL32) %s", spec->parm_name[i]);
				float  val = ui_input_float(prompt);
				data = utils_serialize_real32(val, &size);
			}
				break;
			case AMP_TYPE_REAL64:
			{
				sprintf(prompt,"(REAL64) %s", spec->parm_name[i]);
				double  val = ui_input_double(prompt);
				data = utils_serialize_real64(val, &size);
			}
				break;
			case AMP_TYPE_STRING:
			{
				sprintf(prompt,"(STRING) %s", spec->parm_name[i]);
				char *val = ui_input_string(prompt);
				data = utils_serialize_string(val, &size);
				SRELEASE(val);
			}
				break;
			case AMP_TYPE_BLOB:
			{
				sprintf(prompt,"(BLOB) %s", spec->parm_name[i]);
				blob_t *val = ui_input_blob(prompt, 0);
				data = blob_serialize(val, &size);
				SRELEASE(val);
			}
				break;
			case AMP_TYPE_SDNV:
			{
				sprintf(prompt,"(SDNV) %s", spec->parm_name[i]);
				Sdnv val = ui_input_sdnv(prompt);
				data = (uint8_t *) STAKE(val.length);
				if (data)
				{
					size = val.length;
					memcpy(data, &(val.text), val.length);
				}
				else
				{
					putErrmsg("Can't allocate data.", NULL);
				}
			}
				break;
			case AMP_TYPE_TS:
			{
				sprintf(prompt,"(TS) %s", spec->parm_name[i]);
				uint32_t  val = ui_input_uint(prompt);
				data = utils_serialize_uint(val, &size);
			}
				break;
			case AMP_TYPE_DC:
			{
				sprintf(prompt,"(DC) %s", spec->parm_name[i]);
				Lyst user_dc = ui_input_dc(prompt);
				data = dc_serialize(user_dc, &size);
				dc_destroy(&user_dc);
			}
				break;
			case AMP_TYPE_MID:
			{
				sprintf(prompt,"(MID) %s", spec->parm_name[i]);

				mid_t *mid = ui_input_mid(prompt, ADM_ALL, MID_ANY);
				data = mid_serialize(mid, &size);
				mid_release(mid);
			}
				break;
			case AMP_TYPE_MC:
			{
				sprintf(prompt,"(MID Collection) %s", spec->parm_name[i]);
				Lyst mc = ui_input_mc(prompt);
				data = midcol_serialize(mc, &size);
				midcol_destroy(&mc);
			}
				break;
			case AMP_TYPE_EXPR:
			{
				printf("(Expression) %s", spec->parm_name[i]);
				uint8_t type = ui_input_uint("EXPR Type:");
				Lyst mc = ui_input_mc("EXPR MID Collection:");
				expr_t *expr = expr_create(type, mc);
				data = expr_serialize(expr, &size);
				expr_release(expr);
			}
				break;
		}

		if(data != NULL)
		{
			tdc_insert(result, spec->parm_type[i], data, size);
			SRELEASE(data);
		}
	}

	return result;
}


