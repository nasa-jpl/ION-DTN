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
 **  10/06/18  E. Birrane     Updated to AMP v0.5 (JHU/APL)
 *****************************************************************************/

#include "ui_input.h"
#include "metadata.h"


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

	while(len == 0)
	{
		printf("\nNote: Only the first %d characters will be read.\n%s",
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
	result = blob_create(data, file_size, file_size);

	str = utils_hex_to_string(data, file_size);
	AMP_DEBUG_ALWAYS("Read from %s: %.50s...", filename, str);
	SRELEASE(str);
	SRELEASE(filename);
	fclose(fp);

	return result;
}


uint8_t ui_input_adm_id(char *prompt)
{
	// todo: implement.
	return ADM_ALL;
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
			result = utils_string_to_hex(str);
			break;
		case 2:
			result = ui_input_file_contents(prompt);
			break;
		default:
			result = NULL;
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

double   ui_input_real64(char *prompt)
{
	double result = 0;
	char line[20];

	ui_input_get_line(prompt, (char**)&line, 20);

	sscanf(line, "%lf", &result);

	return result;
}

float    ui_input_real32(char *prompt)
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


ac_t *ui_input_ac(char *prompt)
{
	uint32_t i = 0;
	uint32_t num = 0;
	ac_t *result = ac_create();

	CHKNULL(result);

	printf("\n\n");
	printf("ARI COLLECTION (AC) Builder.\n");
	printf("----------------------------\n");
	printf("%s", prompt);


	num = ui_input_uint("# ARIs in the collection:");

	for(i = 0; i < num; i++)
	{
		char prompt[20];
		sprintf(prompt, "Build ARI %d", i);
		ari_t *cur = ui_input_ari(prompt, ADM_ALL, AMP_TYPE_UNK);
		if(vec_push(&(result->values), cur) != VEC_OK)
		{
			AMP_DEBUG_ERR("ui_input_ac","Could not input ARI %d.", i);
			ac_release(result, 1);
			result = NULL;
			break;
		}
	}

	return result;
}


/******************************************************************************
 *
 * \par Function Name: ui_input_ari
 *
 * \par Construct an ARI object completely from user input.
 *
 * \par Notes:
 *
 * \retval NULL  - Problem building the ARI.
 * 		   !NULL - The constructed ARI.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *  06/25/13  E. Birrane     Removed references to priority field. Add ISS flag.
 *  07/05/16  E. Birrane     Check for NULL result. Add File input.
 *****************************************************************************/

ari_t *ui_input_ari(char *prompt, uint8_t adm_id, amp_type_e type)
{
	ari_t *result = NULL;
	metadata_t *meta = NULL;


	/* Step 1: Print the prompt. */
	printf("\n\n");
	printf("Entering ARI for: %s\n.", prompt);

	printf("\n+--------------------------------------------------+");
	printf("\n|       Welcome to the ARI Builder. You may:       |");
	printf("\n+--------------------------------------------------+");
	printf("\n| 1) Select an existing ARI from a list.           |");
	printf("\n| 2) Build an ARI using a wizard.                  |");
	printf("\n| 3) Type the entire ARI in hex.                   |");
	printf("\n| 4) Cancel.                                       |");
	printf("\n+--------------------------------------------------+");
	printf("\n\n");

	uint32_t opt = ui_input_uint("Select an option (1-4):");

	switch(opt)
	{
		case 1:
			result = ui_input_ari_list(adm_id, type);
			break;
		case 2:
			result = ui_input_ari_build();
			break;
		case 3:
			result = ui_input_ari_raw(1);
			break;
		default:
			return NULL;
			break;
	}

	CHKNULL(result);

	if((result->type != AMP_TYPE_LIT) &&
	   (ARI_GET_FLAG_PARM(result->as_reg.flags)))
	{
		if(ui_input_parms(result) != AMP_OK)
		{
			AMP_DEBUG_ERR("ui_input_ari","Unable to get parms.", NULL);
			ari_release(result, 1);
			result = NULL;
		}
	}

	return result;
}

ari_t* ui_input_ari_build()
{
	ari_t *result = ari_create();
	uint8_t flags;
	int success;

	CHKNULL(result);

	ui_input_ari_flags(&flags);

	if(ARI_GET_FLAG_TYPE(flags) == AMP_TYPE_LIT)
	{
		result->type = AMP_TYPE_LIT;
		tnv_t *tmp = ui_input_tnv(AMP_TYPE_LIT, "");
		result->as_lit = tnv_copy(*tmp, &success);
		tnv_release(tmp, 1);

		if(result->as_lit.type == AMP_TYPE_UNK)
		{
			AMP_DEBUG_ERR("ui_input_ari_build", "Problem building ARI.", NULL);
			ari_release(result, 1);
			result = NULL;
		}
		return result;
	}

	result->as_reg.flags = flags;

	if(ARI_GET_FLAG_NN(flags))
	{
		uvast nn = ui_input_uvast("ARI Nickname:");
		if(VDB_ADD_NN(nn, &(result->as_reg.nn_idx)) != VEC_OK)
		{
			AMP_DEBUG_ERR("ui_input_ari","Unable to add nickname.", NULL);
			ari_release(result, 1);
			return NULL;
		}
	}

	if(ARI_GET_FLAG_ISS(flags))
	{
		uvast iss = ui_input_uvast("ARI Issuer:");
		if(VDB_ADD_ISS(iss, &(result->as_reg.iss_idx)) != VEC_OK)
		{
			AMP_DEBUG_ERR("ui_input_ari","Unable to add issuer.", NULL);
			ari_release(result, 1);
			return NULL;
		}
	}

	if(ARI_GET_FLAG_TAG(flags))
	{
		blob_t *tag = ui_input_blob("ARI Tag:", 0);
		if(VDB_ADD_TAG(*tag, &(result->as_reg.tag_idx)) != VEC_OK)
		{
			AMP_DEBUG_ERR("ui_input_ari","Unable to add issuer.", NULL);
			blob_release(tag, 1);
			ari_release(result, 1);
			return NULL;
		}
		blob_release(tag, 1);
	}

	if(ARI_GET_FLAG_PARM(flags))
	{
		if(ui_input_parms(result) != AMP_OK)
		{
			AMP_DEBUG_ERR("ui_input_ari","Unable to get parms.", NULL);
			ari_release(result, 1);
			result = NULL;
		}
	}

	return result;
}


/******************************************************************************
 *
 * \par Function Name: ui_input_ari_flags
 *
 * \par Construct an ARI flag byte completely from user input.
 *
 * \par Notes:
 *
 * \retval AMP Status Code
 *
 * \param[out]  flag   The resulting flags
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  01/18/13  E. Birrane     Initial Implementation
 *  06/25/13  E. Birrane     Removed references to priority field.Add ISS Flag.
 *  10/06/18  E. Birrane     Updated to AMP v0.5 (JHU/APL)
 *****************************************************************************/

int ui_input_ari_flags(uint8_t *flag)
{
	int i = 0;
	int type;

	*flag = 0;

	printf("\n\n");
	printf("+--------------------------------+\n");
	printf("|     Build an ARI Flag Byte.    |\n");
	printf("+--------------------------------+\n\n");

	/* Step 1: Figure out the AMP type. */
	type = ui_input_ari_type();

	ARI_SET_FLAG_TYPE(*flag, type);

	if(type != AMP_TYPE_LIT)
	{
		if(ui_input_int("Nickname Present? Yes (1)  No (0):") != 0)
		{
			ARI_SET_FLAG_NN(*flag);
		}

		if(ui_input_int("Issuer Field Present? Yes (1)  No (0):") != 0)
		{
			ARI_SET_FLAG_ISS(*flag);
		}

		if(ui_input_int("Tag Field Present? Yes (1)  No (0):") != 0)
		{
			ARI_SET_FLAG_TAG(*flag);
		}

		if(ui_input_int("Parameters Present? Yes (1)  No (0):") != 0)
		{
			ARI_SET_FLAG_PARM(*flag);
		}
	}

	return AMP_OK;
}


ari_t *ui_input_ari_list(uint8_t adm_id, uint8_t type)
{
	// TODO: Implement.
	AMP_DEBUG_ERR("ui_input_ari_list","Not implemented yet.", NULL);
	return NULL;
	/*
	if(type == AMP_TYPE_UNK)
	{
		type = ui_input_ari_type();
	}

	return ui_list_gen(adm_id, type);*/
}


ari_t* ui_input_ari_raw(uint8_t no_file)
{
	ari_t *result = NULL;
	int success;

	blob_t *data = ui_input_blob("0x", no_file);

	result = ari_deserialize_raw(data, &success);
	blob_release(data, 1);

	return result;
}


int ui_input_ari_type()
{
	int type;
	int i;

	for(i = 0; i <= AMP_TYPE_UNK; i++)
	{
		printf("%d) %s\t\t", i, type_to_str(i));
		if((i > 0) && ((i%5) == 0))
		{
			printf("\n");
		}
	}

	type = ui_input_int("Select ARI type (or UNK to cancel): ");

	return type;
}


int ui_input_parms(ari_t *id)
{
	metadata_t *meta = NULL;
	int num;

	uint8_t *data = NULL;
	uint32_t size = 0;
	char prompt[256];
	int i;

	CHKUSR(id, AMP_FAIL);

	if((meta = rhht_retrieve_key(&(gMgrDB.metadata), id)) == NULL)
	{
		AMP_DEBUG_ERR("ui_input_parms","ARI has parms, but can't fine metadata.", NULL);
		return AMP_FAIL;
	}

	if((num = vec_num_entries(meta->parmspec)) == 0)
	{
		return AMP_OK;
	}

	printf("\n\n");
	printf("Your Selected ARI Needs %d Parameters.\n", num);
	printf("You will now be asked to enter each parm.\n");

	for(i = 0; i < num; i++)
	{
       parm_t *parm = vec_at(&(meta->parmspec), i);

		sprintf(prompt,"Parameter %d: (%s) %s", i, type_to_str(parm->type), parm->name);
		tnv_t *val = ui_input_tnv(parm->type, prompt);

		if(vec_push(&(id->as_reg.parms.values), val) != VEC_OK)
		{
			AMP_DEBUG_ERR("ui_input_parms", "Can't add parameter.", NULL);
			tnv_release(val, 1);
			return AMP_FAIL;
		}
	}

	return AMP_OK;
}


tnv_t *ui_input_tnv(int type, char *prompt)
{
	tnv_t *result = tnv_create();
	CHKNULL(result);

	result->type = type;
	switch(result->type)
	{
		case AMP_TYPE_BOOL:
		case AMP_TYPE_BYTE:
			*result = tnv_from_byte(ui_input_byte(prompt));
			break;

		case AMP_TYPE_INT:
			*result = tnv_from_int(ui_input_int(prompt));
			break;

		case AMP_TYPE_UINT:
			*result = tnv_from_uint(ui_input_uint(prompt));
			break;

		case AMP_TYPE_VAST:
			*result = tnv_from_vast(ui_input_vast(prompt));
			break;

		case AMP_TYPE_TV:
		case AMP_TYPE_TS:
		case AMP_TYPE_UVAST:
			*result = tnv_from_uvast(ui_input_uvast(prompt));
			break;

		case AMP_TYPE_REAL32:
			*result = tnv_from_real32(ui_input_real32(prompt));
			break;

		case AMP_TYPE_REAL64:
			*result = tnv_from_real64(ui_input_real64(prompt));
			break;

		case AMP_TYPE_STR:
			*result = tnv_from_str(ui_input_string(prompt));
			break;

		case AMP_TYPE_BYTESTR:
			result->value.as_ptr = ui_input_blob(prompt, 0);
			TNV_SET_ALLOC(result->flags);
			break;

		case AMP_TYPE_CNST:
		case AMP_TYPE_EDD:
		case AMP_TYPE_LIT:
		case AMP_TYPE_ARI:
			result->value.as_ptr = ui_input_ari(prompt, ADM_ALL, AMP_TYPE_UNK);
			TNV_SET_ALLOC(result->flags);
			break;

		case AMP_TYPE_MAC:
		case AMP_TYPE_AC:
			result->value.as_ptr = ui_input_ac(prompt);
			TNV_SET_ALLOC(result->flags);
			break;
/*
		case AMP_TYPE_CTRL:
			result->value.as_ptr = ui_input_ctrl(prompt);
			TNV_SET_ALLOC(result->flags);
			break;

		case AMP_TYPE_EXPR:
			result->value.as_ptr = ui_input_expr(prompt);
			TNV_SET_ALLOC(result->flags);
			break;
		case AMP_TYPE_OPER:
			result->value.as_ptr = ui_input_oper(prompt);
			TNV_SET_ALLOC(result->flags);
			break;
		case AMP_TYPE_RPT:
			result->value.as_ptr = ui_input_rpt(prompt);
			TNV_SET_ALLOC(result->flags);
			break;
		case AMP_TYPE_RPTTPL:
			result->value.as_ptr = ui_input_rpttpl(prompt);
			TNV_SET_ALLOC(result->flags);
			break;
		case AMP_TYPE_TBR:
		case AMP_TYPE_SBR:
			result->value.as_ptr = ui_input_rule(prompt);
			TNV_SET_ALLOC(result->flags);
			break;

		case AMP_TYPE_TBL:
			result->value.as_ptr = ui_input_tbl(prompt);
			TNV_SET_ALLOC(result->flags);
			break;
		case AMP_TYPE_TBLT:
			result->value.as_ptr = ui_input_tblt(prompt);
			TNV_SET_ALLOC(result->flags);
			break;
		case AMP_TYPE_VAR:
			result->value.as_ptr = ui_input_var(prompt);
			TNV_SET_ALLOC(result->flags);
			break;
			*/
		default:
			tnv_release(result, 1);
			result = NULL;
	}

	if(result != NULL)
	{
		if(TNV_IS_ALLOC(result->flags))
		{
			if(result->value.as_ptr == NULL)
			{
				tnv_release(result, 1);
				result = NULL;
			}
		}
	}

	return result;
}
/*

ctrl_t* ui_input_ctrl(prompt)
{
	AMP_DEBUG_ERR("ui_input_ctrl", "Not implemented yet.", NULL);
	return NULL;
}

expr_t* ui_input_expr(prompt)
{
	AMP_DEBUG_ERR("ui_input_expr", "Not implemented yet.", NULL);
	return NULL;
}

op_t* ui_input_oper(prompt)
{
	AMP_DEBUG_ERR("ui_input_oper", "Not implemented yet.", NULL);
	return NULL;
}

rpt_t* ui_input_rpt(prompt)
{
	AMP_DEBUG_ERR("ui_input_rpt", "Not implemented yet.", NULL);
	return NULL;
}

rpttpl_t* ui_input_rpttpl(prompt)
{
	AMP_DEBUG_ERR("ui_input_rpttpl", "Not implemented yet.", NULL);
	return NULL;
}

rule_t *ui_input_rule(prompt)
{
	AMP_DEBUG_ERR("ui_input_rule", "Not implemented yet.", NULL);
	return NULL;
}

tbl_t* ui_input_tbl(prompt)
{
	AMP_DEBUG_ERR("ui_input_tbl", "Not implemented yet.", NULL);
	return NULL;
}

tblt_t* ui_input_tblt(prompt)
{
	AMP_DEBUG_ERR("ui_input_tblt", "Not implemented yet.", NULL);
	return NULL;
}


var_t* ui_input_var(prompt)
{
	AMP_DEBUG_ERR("ui_input_var", "Not implemented yet.", NULL);
	return NULL;
}
*/



