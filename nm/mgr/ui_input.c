/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2018 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/
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

#include "../shared/adm/adm.h"

extern int *global_nm_running;


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

	while(*global_nm_running && len == 0)
	{
		printf("%s\n", prompt);

		if (igets(STDIN_FILENO, (char *)line, max_len, &len) == NULL)
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
	SRELEASE(data);

	str = utils_hex_to_string(data, file_size);
	AMP_DEBUG_ALWAYS("ui_input_file_contents", "Read from %s: %.50s...", filename, str);
	SRELEASE(str);
	SRELEASE(filename);
	fclose(fp);

	return result;
}


uint8_t ui_input_adm_id()
{
	vecit_t it;
	int i = 0;
	int max = 0;

	adm_info_t *info = NULL;

	ui_printf("\n");
	for(it = vecit_first(&g_adm_info); vecit_valid(it); it = vecit_next(it))
	{
		info = (adm_info_t*) vecit_data(it);
		ui_printf("\n%d. %s", i, info->name);
		i++;
	}
	max = i;

	i = ui_input_uint("\nSelect ADM:");

	if(i >= max)
	{
		i = 0;
	}

	info = (adm_info_t*) vec_at(&g_adm_info, i);

	return (info == NULL) ? 0 : info->id;
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
       opt = ui_prompt("Select Blob Input Method", "Abort", "Enter in HEX", "Select File");
	}

	switch(opt)
	{
		case 1:
			str = ui_input_string(prompt);
			result = utils_string_to_hex(str);
			SRELEASE(str);
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
	memset(line,0,3);
	ui_input_get_line(prompt, (char**)&line, 2);

	blob_t *blob = utils_string_to_hex(line);
	if(blob == NULL)
	{
		AMP_DEBUG_ERR("ui_input_byte","Problem reading value. Returning 0.", NULL);
		return 0;
	}
	if(blob->length > 1)
	{
		ui_printf("Read %d bytes. Only selecting first.", blob->length);
	}
	result = blob->value[0];
	blob_release(blob, 1);
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

#ifdef USE_NCURSES
char *   ui_input_string(char *prompt)
{
   char line[MAX_INPUT_BYTES] = "";
   char *result;
   form_fields_t fields[] = {
      {"String", line, MAX_INPUT_BYTES, 0, 0}
   };
   if (ui_form(prompt, NULL, fields, 1) == 1)
   {
      // Success
      result = (char *)STAKE(strlen(line) + 1);
      strcpy(result, line);
      
      return result;
   }
   else
   {
      return NULL;
   }   
}
#else
char *   ui_input_string(char *prompt)
{
	char *result = NULL;
	char line[MAX_INPUT_BYTES];
	memset(line, 0, MAX_INPUT_BYTES);
	ui_input_get_line(prompt, (char**)&line, MAX_INPUT_BYTES-1);

	result = (char *) STAKE(strlen(line) + 1);

	sscanf(line, "%s", result);

	return result;
}
#endif

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
	printf("%s\n", prompt);


	num = ui_input_uint("# ARIs in the collection:");
	if (num > VEC_MAX_IDX)
	{
		// Limited by the maximum vector size
		AMP_DEBUG_ERR("ui_input_ac", "Invalid number (%d) entered", num);
		ac_release(result, 1);
		return NULL;
	}

	for(i = 0; i < num; i++)
	{
		char ari_prompt[24];
		snprintf(ari_prompt, 24, "Build ARI %d", i);
		ari_t *cur = ui_input_ari(ari_prompt, ADM_ENUM_ALL, TYPE_MASK_ALL);
		if(cur == NULL || vec_push(&(result->values), cur) != VEC_OK)
		{
			AMP_DEBUG_ERR("ui_input_ac","Could not input ARI %d.", i);
			ac_release(result, 1);
			if (cur != NULL)
			{
				ari_release(cur, 1);
			}
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

ari_t *ui_input_ari(char *prompt, uint8_t adm_id, uvast mask)
{
	ari_t *result = NULL;
	metadata_t *meta = NULL;


	/* Step 1: Print the prompt. */
    char *builder_menu[] = {
       "Cancel",
       "Select an existing ARI from a list.",
       "Build an ARI using a wizard.",
       "Type the entire ARI in hex.",
    };
    char title[64];
    sprintf(title, "Entering ARI for: %s", prompt);
    uint32_t opt = ui_menu(title, builder_menu, NULL, ARRAY_SIZE(builder_menu), NULL);
    
	switch(opt)
	{
		case 1:
			result = ui_input_ari_list(adm_id, mask);
			break;
		case 2:
			result = ui_input_ari_build(mask);
			break;
		case 3:
			result = ui_input_ari_raw(1);
			break;
		default:
			return NULL;
			break;
	}

	if(result == NULL)
	{
		AMP_DEBUG_INFO("ui_input_ari","User did not select ARI.", NULL);
		return NULL;
	}

	if((result->type != AMP_TYPE_LIT) &&
		(result->type != AMP_TYPE_OPER) &&
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

ari_t* ui_input_ari_build(uvast mask)
{
	ari_t *result = NULL;
	uint8_t flags;
	int success;


	ui_input_ari_flags(&flags);


	result = ari_create(ARI_GET_FLAG_TYPE(flags));
	CHKNULL(result);

	if(TYPE_MATCHES_MASK(result->type, mask) == 0)
	{
		AMP_DEBUG_ERR("ui_input_ari_build", "Invalid type of %s. Aborting...", type_to_str(result->type));
		ari_release(result, 1);
		return NULL;
	}

	if(result->type == AMP_TYPE_LIT)
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
#if AMP_VERSION < 7
		uvast iss = ui_input_uvast("ARI Issuer:");
		if(VDB_ADD_ISS(iss, &(result->as_reg.iss_idx)) != VEC_OK)
		{
			AMP_DEBUG_ERR("ui_input_ari","Unable to add issuer.", NULL);
			ari_release(result, 1);
			return NULL;
		}
#else
		blob_t *issuer = ui_input_blob("ARI Issuer:", 0);
        if (issuer == NULL)
        {
			AMP_DEBUG_ERR("ui_input_ari","Invalid issuer input.", NULL);
			ari_release(result, 1);
			return NULL;
        }
		else if(VDB_ADD_ISS(*issuer, &(result->as_reg.iss_idx)) != VEC_OK)
		{
			AMP_DEBUG_ERR("ui_input_ari","Unable to add issuer.", NULL);
			blob_release(issuer, 1);
			ari_release(result, 1);
			return NULL;
		}
		blob_release(issuer, 1);

#endif
	}

	if(ARI_GET_FLAG_TAG(flags))
	{
		blob_t *tag = ui_input_blob("ARI Tag:", 0);
        if (tag == NULL)
        {
			AMP_DEBUG_ERR("ui_input_ari","Invalid tag input.", NULL);
			ari_release(result, 1);
			return NULL;
        }
		else if(VDB_ADD_TAG(*tag, &(result->as_reg.tag_idx)) != VEC_OK)
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

	blob_t *blob = ui_input_blob("ARI Name", 1);
	if(blob != NULL)
	{
		blob_copy(*blob, &(result->as_reg.name));
		blob_release(blob, 1);
	}
	else
	{
		AMP_DEBUG_ERR("ui_input_ari","Unable to get NAME.", NULL);
		ari_release(result, 1);
		result = NULL;
	}

	if(result != NULL)
	{
		if((blob = ari_serialize_wrapper(result)) != NULL)
		{
			char *ari_str = utils_hex_to_string(blob->value, blob->length);
			ui_printf("Constructed ARI: %s\n", ari_str);
			SRELEASE(ari_str);
			blob_release(blob, 1);
		}
		else
		{
			AMP_DEBUG_ERR("ui_input_ari","Error checking built ARI.", NULL);
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

#ifdef USE_NCURSES
	/* Step 1: Figure out the AMP type. */
    type = ui_input_ari_type(TYPE_MASK_ALL);
	ARI_SET_FLAG_TYPE(*flag, type);

    if(type != AMP_TYPE_LIT)
    {
       char nn[4]="0", iss[4]="0", tag[4]="0", parm[4]="0";
       form_fields_t fields[] = {
          {"Nicename Present?", (char*)&nn, sizeof(int), 0, TYPE_CHECK_INT},
          {"Issuer Field Present?", (char*)&iss, sizeof(int), 0, TYPE_CHECK_INT},
          {"Tag Field Present?", (char*)&tag, sizeof(int), 0, TYPE_CHECK_INT},
          {"Parameters Present?", (char*)&parm, sizeof(int), 0, TYPE_CHECK_INT}
       };
       // Current compiler settings do not support static initialization of above union
       fields[0].args.num.padding=0;fields[0].args.num.vmin=0;fields[0].args.num.vmax=1;
       fields[1].args.num.padding=0;fields[1].args.num.vmin=0;fields[1].args.num.vmax=1;
       fields[2].args.num.padding=0;fields[2].args.num.vmin=0;fields[2].args.num.vmax=1;
       fields[3].args.num.padding=0;fields[3].args.num.vmin=0;fields[3].args.num.vmax=1;
       
       ui_form("Build an ARI Flag Byte",
               "Enter 1 for yes, 0 for no.  Enter or arrow keys to advance fields",
               fields,
               ARRAY_SIZE(fields)
       );
       if (atoi(nn) != 0)
       {
          ARI_SET_FLAG_NN(*flag);
       }
       if (atoi(iss) != 0)
       {
          ARI_SET_FLAG_ISS(*flag);
       }
       if (atoi(tag) != 0)
       {
          ARI_SET_FLAG_TAG(*flag);
       }
       if (atoi(parm) != 0)
       {
          ARI_SET_FLAG_PARM(*flag);
       }
    }
#else
	printf("\n\n");
	printf("+--------------------------------+\n");
	printf("|     Build an ARI Flag Byte.    |\n");
	printf("+--------------------------------+\n\n");

	/* Step 1: Figure out the AMP type. */
	type = ui_input_ari_type(TYPE_MASK_ALL);

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
#endif

	return AMP_OK;
}

ari_t *ui_input_ari_list(uint8_t adm_id, uvast mask)
{
	ari_t *result = NULL;
	int idx = 0;
	meta_col_t *col = NULL;
	metadata_t *meta = NULL;

	int enum_id = ui_input_adm_id();
	ui_list_objs(enum_id, mask, &result);
	return result;
}

ari_t*  ui_input_ari_lit(char *prompt)
{
	ari_t *result = NULL;
	uvast mask = 0;
	amp_type_e type;

	if(prompt != NULL)
	{
		ui_printf("%s", prompt);
	}
	else
	{
		ui_printf("Enter LITERAL value.\n");
	}


	ui_printf("Select type for the literal value:\n");
	mask = TYPE_AS_MASK(AMP_TYPE_INT)  | TYPE_AS_MASK(AMP_TYPE_INT) |
		   TYPE_AS_MASK(AMP_TYPE_UINT) | TYPE_AS_MASK(AMP_TYPE_VAST) | TYPE_AS_MASK(AMP_TYPE_UVAST) |
		   TYPE_AS_MASK(AMP_TYPE_REAL32) | TYPE_AS_MASK(AMP_TYPE_REAL64);

	if((type = ui_input_ari_type(mask)) == AMP_TYPE_UNK)
	{
		ui_printf("Aborting...\n");
		return NULL;
	}

	result = ari_create(AMP_TYPE_LIT);
	CHKNULL(result);

	result->as_lit.flags = 0;
	result->as_lit.type = type;

	switch(type)
	{
		case AMP_TYPE_BOOL:
		case AMP_TYPE_BYTE:
			result->as_lit.value.as_byte = ui_input_byte("Enter Literal Value as BYTE");
			break;
		case AMP_TYPE_STR:
			result->as_lit.value.as_ptr = ui_input_string("Enter Literal Value as STR");
			TNV_SET_ALLOC(result->as_lit.flags);
			break;
		case AMP_TYPE_INT:
			result->as_lit.value.as_int = ui_input_int("Enter Literal Value as INT");
			break;
		case AMP_TYPE_UINT:
			result->as_lit.value.as_uint = ui_input_uint("Enter Literal Value as UINT");
			break;
		case AMP_TYPE_VAST:
			result->as_lit.value.as_vast = ui_input_vast("Enter Literal Value as VAST");
			break;
		case AMP_TYPE_UVAST:
			result->as_lit.value.as_uvast = ui_input_uvast("Enter Literal Value as UVAST");
			break;
		case AMP_TYPE_REAL32:
			result->as_lit.value.as_real32 = ui_input_real32("Enter Literal Value as REAL32");
			break;
		case AMP_TYPE_REAL64:
			result->as_lit.value.as_real64 = ui_input_real64("Enter Literal Value as REAL64");
			break;
		default:
			ari_release(result, 1);
			result = NULL;
			break;
	}

	return result;
}


ari_t* ui_input_ari_raw(uint8_t no_file)
{
	ari_t *result = NULL;
	int success;

	blob_t *data = ui_input_blob("0x", no_file);

    if (data != NULL)
    {
       result = ari_deserialize_raw(data, &success);   
       blob_release(data, 1);
    }

	return result;
}


int ui_input_ari_type(uvast mask)
{
	int i= 0;
	int idx = 0;
	int select = 0;
	amp_type_e types[AMP_TYPE_UNK + 1];

#ifdef USE_NCURSES
    select = ui_menu_select("Select AMP object (ari) type", amp_type_str, NULL, AMP_TYPE_UNK, NULL, 4);

    if (select < 0)
    {
       select = AMP_TYPE_UNK;
    }
    return select;
#else
	for(i = 0; i <= AMP_TYPE_UNK; i++)
	{
		if(TYPE_MATCHES_MASK(i, mask) || (i == AMP_TYPE_UNK))
		{
			types[idx++] = i;
		}
	}

	/* If there was only 1 choice other than UNK, auto-select it. */
	if(idx == 2)
	{
		select = 0;
	}
	else
	{
		for(i = 0; i < idx; i++)
		{
			ui_printf("%2d) %-10s ", i, type_to_str(types[i]));
			if((i > 0) && ((i % 7) == 0))
			{
				ui_printf("\n");
			}
		}
		select = ui_input_int("\nSelect ARI type (or UNK to cancel): ");
	}
    
	return types[select];
#endif

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
		AMP_DEBUG_ERR("ui_input_parms","ARI has parms, but can't find metadata.", NULL);
		return AMP_FAIL;
	}

	if((num = vec_num_entries(meta->parmspec)) == 0)
	{
		return AMP_OK;
	}

	ui_printf("\n\n");
	ui_printf("Your Selected ARI Needs %d Parameters.\n", num);
	ui_printf("You will now be asked to enter each parm.\n");

	for(i = 0; i < num; i++)
	{
       meta_fp_t *parm = vec_at(&(meta->parmspec), i);

		sprintf(prompt,"Parameter %d: (%s) %s", i, type_to_str(parm->type), parm->name);
		tnv_t *val = ui_input_tnv(parm->type, prompt);

        if (val == NULL)
        {
			AMP_DEBUG_ERR("ui_input_parms", "User failed to input a valid tnv, aborting", NULL);
			return AMP_FAIL;
        }
        
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
	tnv_t *result = NULL;

	switch(type)
	{
		case AMP_TYPE_BOOL:
		case AMP_TYPE_BYTE:  result = tnv_from_byte(ui_input_byte(prompt));     break;
		case AMP_TYPE_INT:   result = tnv_from_int(ui_input_int(prompt));       break;
		case AMP_TYPE_UINT:  result = tnv_from_uint(ui_input_uint(prompt));     break;
		case AMP_TYPE_VAST:  result = tnv_from_vast(ui_input_vast(prompt));     break;
		case AMP_TYPE_TV:
		case AMP_TYPE_TS:
		case AMP_TYPE_UVAST:  result = tnv_from_uvast(ui_input_uvast(prompt));   break;
		case AMP_TYPE_REAL32: result = tnv_from_real32(ui_input_real32(prompt)); break;
		case AMP_TYPE_REAL64: result = tnv_from_real64(ui_input_real64(prompt)); break;
		case AMP_TYPE_STR:    result = tnv_from_obj(type, ui_input_string(prompt)); break;
		case AMP_TYPE_BYTESTR:result = tnv_from_obj(type, ui_input_blob(prompt, 0)); break;
		case AMP_TYPE_CNST:
		case AMP_TYPE_EDD:
		case AMP_TYPE_ARI:    result = tnv_from_obj(type, ui_input_ari(prompt, ADM_ENUM_ALL, TYPE_MASK_ALL)); break;
		case AMP_TYPE_MAC:    result = tnv_from_obj(type, ui_input_mac(prompt));    break;
		case AMP_TYPE_AC:     result = tnv_from_obj(type, ui_input_ac(prompt));     break;
		case AMP_TYPE_TNVC:   result = tnv_from_obj(type, ui_input_tnvc(prompt));   break;
		case AMP_TYPE_CTRL:   result = tnv_from_obj(type, ui_input_ctrl(prompt));   break;
		case AMP_TYPE_EXPR:   result = tnv_from_obj(type, ui_input_expr(prompt));   break;
		case AMP_TYPE_OPER:   result = tnv_from_obj(type, ui_input_oper(prompt));   break;
		case AMP_TYPE_RPTTPL: result = tnv_from_obj(type, ui_input_rpttpl(prompt)); break;
		case AMP_TYPE_TBR:
		case AMP_TYPE_SBR:    result = tnv_from_obj(type, ui_input_rule(prompt));   break;
		case AMP_TYPE_TBLT:   result = tnv_from_obj(type, ui_input_tblt(prompt));   break;
		case AMP_TYPE_VAR:    result = tnv_from_obj(type, ui_input_var(prompt));    break;
		case AMP_TYPE_LIT:    result = tnv_from_obj(type, ui_input_ari_lit(prompt));break;
		default:
			break;
	}

	return result;
}


tnvc_t* ui_input_tnvc(char *prompt)
{
	tnvc_t *result = NULL;
	int num;
	int i;
	char tnv_prompt[32];

	num = ui_input_int("Number of elements of the TNVC");
	result = tnvc_create(num);

	for(i = 0; i < num; i++)
	{
		int type = ui_input_ari_type(TYPE_MASK_ALL);
		snprintf(tnv_prompt,32, "TNV for Item %d", i);
		tnv_t *cur = ui_input_tnv(type, tnv_prompt);
        if (cur == NULL || tnvc_insert(result, cur) != AMP_OK)
        {
           AMP_DEBUG_ERR("ui_input_tnvc", "Could not input TNV %d.", i);
           tnvc_release(result, 1);
           result = NULL;
           break;
        }
	}

	return result;
}



ctrl_t* ui_input_ctrl(char * prompt)
{

   
	AMP_DEBUG_ERR("ui_input_ctrl", "Not implemented yet.", NULL);
	return NULL;
}

expr_t* ui_input_expr(char* prompt)
{
   expr_t* expr = NULL;
   ari_t *val = NULL;
   amp_type_e type = AMP_TYPE_UNK;
   uvast mask;

   ui_printf("\n\n");
   ui_printf("Expression Builder\n");
   ui_printf("----------------------------------------\n");

   ui_printf("Enter Expression type.\n");
   ui_printf("----------------------------------------\n");

   mask = TYPE_AS_MASK(AMP_TYPE_INT)  | TYPE_AS_MASK(AMP_TYPE_INT) |
		  TYPE_AS_MASK(AMP_TYPE_UINT) | TYPE_AS_MASK(AMP_TYPE_VAST) | TYPE_AS_MASK(AMP_TYPE_UVAST) |
		  TYPE_AS_MASK(AMP_TYPE_REAL32) | TYPE_AS_MASK(AMP_TYPE_REAL64);

   if((type = ui_input_ari_type(mask)) == AMP_TYPE_UNK)
   {
	   ui_printf("Canceling Expression...\n");
	   return NULL;
   }

   ui_printf("Enter Expression as AC list.\n");
   ui_printf("Each ARI must be numeric or of type OPER, CNST, EDD, or VAR.\n");
   ui_printf("Enter type UNK to end ARI entry for this expression.\n");

   if((expr = expr_create(type)) == NULL)
   {
	   ui_printf("Error allocating expression. Aborting...");
	   return NULL;
   }

   mask = TYPE_AS_MASK(AMP_TYPE_OPER) | TYPE_AS_MASK(AMP_TYPE_CNST) | TYPE_AS_MASK(AMP_TYPE_EDD) |
		  TYPE_AS_MASK(AMP_TYPE_LIT) | TYPE_AS_MASK(AMP_TYPE_VAR);

   while( (val = ui_input_ari("Expression ARI Value Input", ADM_ENUM_ALL, mask)) != NULL )
   {
	   expr_add_item(expr, val);
   }

   return expr;
}

op_t* ui_input_oper(char* prompt)
{
	AMP_DEBUG_ERR("ui_input_oper", "Not implemented yet.", NULL);
	return NULL;
}

rpt_t* ui_input_rpt(char* prompt)
{
	AMP_DEBUG_ERR("ui_input_rpt", "Not implemented yet.", NULL);
	return NULL;
}

rpttpl_t* ui_input_rpttpl(char* prompt)
{
	AMP_DEBUG_ERR("ui_input_rpttpl", "Not implemented yet.", NULL);
	return NULL;
}

rule_t *ui_input_rule(char* prompt)
{
	AMP_DEBUG_ERR("ui_input_rule", "Not implemented yet.", NULL);
	return NULL;
}

tbl_t* ui_input_tbl(char* prompt)
{
	AMP_DEBUG_ERR("ui_input_tbl", "Not implemented yet.", NULL);
	return NULL;
}

tblt_t* ui_input_tblt(char* prompt)
{
	AMP_DEBUG_ERR("ui_input_tblt", "Not implemented yet.", NULL);
	return NULL;
}


var_t* ui_input_var(char* prompt)
{
	AMP_DEBUG_ERR("ui_input_var", "Not implemented yet.", NULL);
	return NULL;
}

macdef_t *ui_input_mac(char *prompt)
{
	AMP_DEBUG_ERR("ui_input_var", "Not implemented yet.", NULL);
	return NULL;
}


