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

#include "shared/primitives/def.h"
#include "shared/primitives/lit.h"

#include "shared/utils/nm_types.h"
#include "shared/utils/utils.h"

#include "shared/adm/adm.h"
#include "shared/adm/adm_bp.h"
#include "shared/adm/adm_ltp.h"
#include "shared/adm/adm_ion.h"
#include "shared/adm/adm_agent.h"

Lyst gAdmData;
Lyst gAdmComputed;
Lyst gAdmCtrls;
Lyst gAdmLiterals;
Lyst gAdmOps;
Lyst gAdmRpts;
Lyst gAdmMacros; // Type def_gen_t

/******************************************************************************
 *
 * \par Function Name: adm_add_datadef
 *
 * \par Registers a pre-configured ADM data definition with the local DTNMP actor.
 *
 * \retval 0  Failure - The datadef was not added.
 *         !0 Success - The datadef was added.
 *
 * \param[in] mid_str   Serialized MID value
 * \param[in] type      The type of the data definition.
 * \param[in] num_parms # parms needed for parameterized OIDs.
 * \param[in] collect   The data collection function.
 * \param[in] to_string The to-string function
 * \param[in] get_size  The sizing function for the ADM entry.
 *
 * \par Notes:
 *		1. When working with parameterized OIDs, the given MID should
 *		   be all information excluding the parameterized portion of the OID.
 *		2. ADM names will be truncated after ADM_MAX_NAME bytes.
 *		3. If a NULL to_string is given, we assume it is unsigned long.
 *		4. If a NULL get_size is given, we assume is it unsigned long.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/04/15  E. Birrane     Added type information.
 *****************************************************************************/

int adm_add_datadef(char *mid_str,
		            dtnmp_type_e type,
		     	 	int num_parms,
  		     	 	adm_data_collect_fn collect,
		     	 	adm_string_fn to_string,
		     	 	adm_size_fn get_size)
{
	uint32_t size = 0;
	uint32_t used = 0;
	adm_datadef_t *new_entry = NULL;

	DTNMP_DEBUG_ENTRY("adm_add_datadef","(%llx, %d, %llx, %llx)",
			          mid_str, num_parms, to_string, get_size);

	/* Step 0 - Sanity Checks. */
	if(mid_str == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_datadef","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_datadef","-> 0.", NULL);
		return 0;
	}
	if(gAdmData == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_datadef","Global data list not initialized.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_datadef","-> 0.", NULL);
		return 0;
	}

	/* Step 2 - Allocate a Data Definition. */
	if((new_entry = (adm_datadef_t *) MTAKE(sizeof(adm_datadef_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_datadef","Can't allocate new entry of size %d.",
				        sizeof(adm_datadef_t));
		DTNMP_DEBUG_EXIT("adm_add_datadef","-> 0.", NULL);
		return 0;
	}

	new_entry->mid = mid_from_string(mid_str);

	new_entry->type = type;
	new_entry->num_parms = num_parms;
	new_entry->collect = collect;
	new_entry->to_string = (to_string == NULL) ? adm_print_uvast : to_string;
	new_entry->get_size = (get_size == NULL) ? adm_size_uvast : get_size;

	/* Step 4 - Add the new entry. */
	lyst_insert_last(gAdmData, new_entry);

	DTNMP_DEBUG_EXIT("adm_add_datadef","-> 1.", NULL);
	return 1;
}


/******************************************************************************
 *
 * \par Function Name: adm_add_datadef_collect
 *
 * \par Registers a collection function to a data definition.
 *
 * \param[in] mid_hex   serialized MID value
 * \param[in] collect   The data collection function.
 *
 * \par Notes:
 *		1. When working with parameterized OIDs, the given MID should
 *		   be all information excluding the parameterized portion of the OID.
 *		2. ADM names will be truncated after ADM_MAX_NAME bytes.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  11/25/12  E. Birrane     Initial implementation.
 *  07/27/13  E. BIrrane     Updated ADM to use Lysts.
 *****************************************************************************/
void adm_add_datadef_collect(uint8_t *mid_hex, adm_data_collect_fn collect)
{
	uint32_t used = 0;
	mid_t *mid = NULL;
	adm_datadef_t *entry = NULL;

	DTNMP_DEBUG_ENTRY("adm_add_datadef_collect","(%lld, %lld)", mid_hex, collect);

	if((mid_hex == NULL) || (collect == NULL))
	{
		DTNMP_DEBUG_ERR("adm_add_datadef_collect","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_datadef_collect","->.", NULL);
		return;
	}

	if((mid = mid_deserialize(mid_hex, ADM_MID_ALLOC, &used)) == NULL)
	{
		char *tmp = utils_hex_to_string(mid_hex, ADM_MID_ALLOC);
		DTNMP_DEBUG_ERR("adm_add_datadef_collect","Can't deserialize MID str %s.",tmp);
		MRELEASE(tmp);

		DTNMP_DEBUG_EXIT("adm_add_datadef_collect","->.", NULL);
		return;
	}

	if((entry = adm_find_datadef(mid)) == NULL)
	{
		char *tmp = mid_to_string(mid);
		DTNMP_DEBUG_ERR("adm_add_datadef_collect","Can't find data for MID %s.", tmp);
		MRELEASE(tmp);
	}
	else
	{
		entry->collect = collect;
	}

	mid_release(mid);

	DTNMP_DEBUG_EXIT("adm_add_datadef_collect","->.", NULL);
}




/******************************************************************************
 *
 * \par Function Name: adm_add_computeddef
 *
 * \par Registers a pre-configured ADM computed data definition with the local
 *      DTNMP actor.
 *
 * \retval 0  Failure - The computeddef was not added.
 *         !0 Success - The computeddef was added.
 *
 * \param[in] mid_str   serialized MID value
 * \param[in] type      The type of the computed data definition result.
 * \param[in] def       The MID Collection defining the computed value.
 *
 * \par Notes:
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  03/16/15  E. Birrane     Initial implementation.
 *  07/04/15  E. Birrane     Added type information.
 *  08/10/15  E. Birrane     Updated to def_gen_t.
  *****************************************************************************/

int adm_add_computeddef(char *mid_str,
						dtnmp_type_e type,
		                Lyst def)
{
	uint32_t size = 0;
	uint32_t used = 0;
	def_gen_t *new_entry = NULL;
	mid_t *mid = NULL;

	DTNMP_DEBUG_ENTRY("adm_add_computeddef","("UVAST_FIELDSPEC", %d,"UVAST_FIELDSPEC")",
			          (uvast) mid_str, type, (uvast) def);

	/* Step 0 - Sanity Checks. */
	if((mid_str == NULL) || def == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_computeddef","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_computeddef","-> 0.", NULL);
		return 0;
	}

	if(gAdmComputed == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_computeddef","Global data list not initialized.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_computeddef","-> 0.", NULL);
		return 0;
	}

	/* Step 2 - Make the MID. */
	if((mid = mid_from_string(mid_str)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_computeddef", "Cannot allocate mid.", NULL);
		return 0;
	}

	/* Step 3: Make the data definition. */
	if((new_entry = def_create_gen(mid, type, def)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_computeddef", "Canot allocate def.", NULL);
		mid_release(mid);
		return 0;
	}

	/* Step 4 - Add the new entry. */
	lyst_insert_last(gAdmComputed, new_entry);

	DTNMP_DEBUG_EXIT("adm_add_computeddef","-> 1.", NULL);
	return 1;
}




/******************************************************************************
 *
 * \par Function Name: adm_add_ctrl
 *
 * \par Registers a pre-configured ADM control with the local DTNMP actor.
 *
 * \retval 0  Failure - The control was not added.
 *         !0 Success - The control was added.
 *
 * \param[in] mid_str   MID value, as a string.
 * \param[in] parm      The parameter conversion function
 * \param[in] run       The control function.
 *
 * \par Notes:
 *		1. When working with parameterized OIDs, the given MID string should
 *		   be all information excluding the parameterized portion of the OID.
 *		2. ADM names will be truncated after ADM_MAX_NAME bytes.
 *
 *****************************************************************************/

int adm_add_ctrl(char *mid_str,
				 adm_ctrl_run_fn run)
{
	uint8_t *tmp = NULL;
	uint32_t size = 0;
	uint32_t used = 0;
	adm_ctrl_t *new_entry = NULL;

	DTNMP_DEBUG_ENTRY("adm_add_ctrl","(%#llx, %#llx)",
			          mid_str, run);

	/* Step 0 - Sanity Checks. */
	if(mid_str == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_ctrl","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_ctrl","-> 0.", NULL);
		return 0;
	}

	if(gAdmCtrls == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_ctrl","Global Controls list not initialized.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_ctrl","-> 0.", NULL);
		return 0;
	}

	/* Step 2 - Allocate a Data Definition. */
	if((new_entry = (adm_ctrl_t *) MTAKE(sizeof(adm_ctrl_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_ctrl","Can't allocate new entry of size %d.",
				        sizeof(adm_ctrl_t));
		DTNMP_DEBUG_EXIT("adm_add_ctrl","-> 0.", NULL);
		return 0;
	}

	new_entry->mid = mid_from_string(mid_str);

	if(new_entry->mid == NULL)
	{
		MRELEASE(new_entry);
		DTNMP_DEBUG_ERR("adm_add_ctrl","Can't get mid from %s.", mid_str);
		DTNMP_DEBUG_EXIT("adm_add_ctrl","-> 0.", NULL);
		return 0;
	}

	new_entry->num_parms = mid_get_num_parms(new_entry->mid);
	new_entry->run = run;

	/* Step 4 - Add the new entry. */
	lyst_insert_last(gAdmCtrls, new_entry);

	DTNMP_DEBUG_EXIT("adm_add_ctrl","-> 1.", NULL);
	return 1;
}


/******************************************************************************
 *
 * \par Function Name: adm_add_ctrl_run
 *
 * \par Registers a control function with a Control MID.
 *
 * \param[in] mid_str   MID value, as a string.
 * \param[in] control   The control function.
 *
 * \par Notes:
 *		1. When working with parameterized OIDs, the given MID string should
 *		   be all information excluding the parameterized portion of the OID.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/28/13  E. Birrane     Initial implementation.
 *****************************************************************************/

void adm_add_ctrl_run(uint8_t *mid_str, adm_ctrl_run_fn run)
{
	uint32_t used = 0;
	mid_t *mid = NULL;
	adm_ctrl_t *entry = NULL;

	DTNMP_DEBUG_ENTRY("adm_add_ctrl_run","(%lld, %lld)", mid_str, run);

	if((mid_str == NULL) || (run == NULL))
	{
		DTNMP_DEBUG_ERR("adm_add_ctrl_run","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_ctrl_run","->.",NULL);
		return;
	}

	if((mid = mid_deserialize(mid_str, ADM_MID_ALLOC, &used)) == NULL)
	{
		char *tmp = utils_hex_to_string(mid_str, ADM_MID_ALLOC);
		DTNMP_DEBUG_ERR("adm_add_ctrl_run","Can't deserialized MID %s", tmp);
		MRELEASE(tmp);
		DTNMP_DEBUG_EXIT("adm_add_ctrl_run","->.",NULL);
		return;
	}

	if((entry = adm_find_ctrl(mid)) == NULL)
	{
		char *tmp = mid_to_string(mid);
		DTNMP_DEBUG_ERR("adm_add_ctrl_run","Can't find control for MID %s", tmp);
		MRELEASE(tmp);
	}
	else
	{
		entry->run = run;
	}

	mid_release(mid);

	DTNMP_DEBUG_EXIT("adm_add_ctrl_run","->.",NULL);
}

int adm_add_lit(char *mid_str, value_t result, lit_val_fn calc)
{
	uint32_t used = 0;
	lit_t *new_entry = NULL;

	DTNMP_DEBUG_ENTRY("adm_add_lit","(0x%lx, (%d,%d,%d), 0x%lx)",
			          (unsigned long) mid_str, result.type, result.value.as_int, result.length,
			          (unsigned long) calc);

	/* Step 0 - Sanity Checks. */
	if(mid_str == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_lit","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_lit","-> 0.", NULL);
		return 0;
	}

	if(gAdmLiterals == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_lit","Global Literals list not initialized.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_lit","-> 0.", NULL);
		return 0;
	}

	/* Step 1 - Build the literal definition. */
	mid_t *id = mid_from_string(mid_str);

	if(id == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_lit","Can't make mid from %s.", mid_str);
		DTNMP_DEBUG_EXIT("adm_add_lit","-> 0.", NULL);
		return 0;
	}

	new_entry = lit_create(id, result, calc);

	if(new_entry == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_lit","Can't allocate new entry of size %d.",
				        sizeof(lit_t));
		DTNMP_DEBUG_EXIT("adm_add_lit","-> 0.", NULL);
		return 0;
	}

	/* Step 2 - Add the new entry. */
	lyst_insert_last(gAdmLiterals, new_entry);

	DTNMP_DEBUG_EXIT("adm_add_lit","-> 1.", NULL);
	return 1;
}

int adm_add_macro(char *mid_str, Lyst midcol)
{
	int success = 0;
	uint32_t used = 0;
	mid_t *mid = NULL;
	def_gen_t *new_entry = NULL;

	DTNMP_DEBUG_ENTRY("adm_add_macro","(%s, 0x%lx)",
			          mid_str, (unsigned long) midcol);

	/* Step 0 - Sanity Checks. */
	if((mid_str == NULL) || (midcol == NULL))
	{
		DTNMP_DEBUG_ERR("adm_add_macro","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_macro","-> 0.", NULL);
		return success;
	}

	if(gAdmMacros == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_macro","Global Macros list not initialized.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_macro","-> 0.", NULL);
		return success;
	}

	/* Step 1 - Build the MID. */
	mid = mid_from_string(mid_str);

	if(mid == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_macro","Can't make mid from %s.", mid_str);
		DTNMP_DEBUG_EXIT("adm_add_macro","-> 0.", NULL);
		return success;
	}

	Lyst newdefs = midcol_copy(midcol);

	/* Step 2 - Build the definition to hold the macro. */
	if((new_entry = def_create_gen(mid, DTNMP_TYPE_MC, newdefs)) == NULL)
	{
		MRELEASE(mid);
		midcol_destroy(&newdefs);

		DTNMP_DEBUG_ERR("adm_add_macro","Can't allocate new macro entry.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_macro","-> 0.", NULL);
		return success;
	}

	/* Step 3 - Add the new entry. */
	lyst_insert_last(gAdmMacros, new_entry);
	success = 1;

	DTNMP_DEBUG_EXIT("adm_add_macro","-> 1.", NULL);
	return success;
}



/******************************************************************************
 *
 * \par Function Name: adm_add_op
 *
 * \par Registers a pre-configured ADM operator with the local DTNMP actor.
 *
 * \retval 0 - Failure - The op was not added.
 *        !0 - Success - The op was added.
 *
 * \param[in] mid_str   MID value, as a string.
 * \param[in] num_parms # parms needed for parameterized OIDs.
 * \param[in] apply     Function for applying the operator
 *
 *****************************************************************************/

int adm_add_op(char *mid_str, uint8_t num_parms, adm_op_fn apply)
{
	uint32_t used = 0;
	adm_op_t *new_entry = NULL;

	DTNMP_DEBUG_ENTRY("adm_add_op","(%#llx, %d, %#llx)",
			          mid_str, num_parms, apply);

	/* Step 0 - Sanity Checks. */
	if(mid_str == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_op","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_op","-> 0.", NULL);
		return 0;
	}

#ifdef AGENT_ROLE
	if(apply == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_op","Bad Apply Function.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_op","-> 0.", NULL);
		return 0;
	}
#endif

	if(gAdmOps == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_op","Global Operators list not initialized.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_op","-> 0.", NULL);
		return 0;
	}

	/* Step 1 - Allocate an Operator Definition. */
	if((new_entry = (adm_op_t *) MTAKE(sizeof(adm_op_t))) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_op","Can't allocate new entry of size %d.",
				        sizeof(adm_op_t));
		DTNMP_DEBUG_EXIT("adm_add_op","-> 0.", NULL);
		return 0;
	}

	new_entry->mid = mid_from_string(mid_str);

	/* Step 2 - Copy the ADM information. */
	if(new_entry->mid == NULL)
	{
		MRELEASE(new_entry);
		DTNMP_DEBUG_ERR("adm_add_op","Can't make MID from %s.", mid_str);
		DTNMP_DEBUG_EXIT("adm_add_op","-> 0.", NULL);
		return 0;
	}


	new_entry->num_parms = num_parms;
	new_entry->apply = apply;

	/* Step 4 - Add the new entry. */
	lyst_insert_last(gAdmOps, new_entry);

	DTNMP_DEBUG_EXIT("adm_add_op","-> 1.", NULL);
	return 1;
}


int adm_add_rpt(char *mid_str, Lyst midcol)
{
	int success = 0;
	uint32_t used = 0;
	mid_t *mid = NULL;
	def_gen_t *new_entry = NULL;

	DTNMP_DEBUG_ENTRY("adm_add_rpt","(%s, 0x%lx)",
			          mid_str, (unsigned long) midcol);

	/* Step 0 - Sanity Checks. */
	if((mid_str == NULL) || (midcol == NULL))
	{
		DTNMP_DEBUG_ERR("adm_add_rpt","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_rpt","-> 0.", NULL);
		return success;
	}

	if(gAdmRpts == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_rpt","Global Reports list not initialized.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_rpt","-> 0.", NULL);
		return success;
	}

	/* Step 1 - Build the MID. */
	mid = mid_from_string(mid_str);

	if(mid == NULL)
	{
		DTNMP_DEBUG_ERR("adm_add_rpt","Can't make mid from %s.", mid_str);
		DTNMP_DEBUG_EXIT("adm_add_rpt","-> 0.", NULL);
		return success;
	}

	Lyst newdefs = midcol_copy(midcol);

	/* Step 2 - Build the definition to hold the macro. */
	if((new_entry = def_create_gen(mid, DTNMP_TYPE_MC, newdefs)) == NULL)
	{
		MRELEASE(mid);
		midcol_destroy(&newdefs);

		DTNMP_DEBUG_ERR("adm_add_rpt","Can't allocate new macro entry.", NULL);
		DTNMP_DEBUG_EXIT("adm_add_rpt","-> 0.", NULL);
		return success;
	}

	/* Step 3 - Add the new entry. */
	lyst_insert_last(gAdmRpts, new_entry);
	success = 1;

	DTNMP_DEBUG_EXIT("adm_add_rpt","-> 1.", NULL);
	return success;
}


/******************************************************************************
 *
 * \par Function Name: adm_build_mid_str
 *
 * \par Constructs a MID string from a nickname and a single additional offset.
 *
 * \param[in] flag      The MID FLAG byte.
 * \param[in] nn        The nickname string.
 * \param[in] nn_len    The number of SDNVs in the nickname.
 * \param[in] offset    Integer acting as the next (and last) SDNV.
 * \param[out] mid_str  The constructed string.
 *
 * \par Notes:
 * 	1. The output string MUST be pre-allocated. This function does not create it.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/27/13  E. Birrane     Initial implementation.
 *****************************************************************************/

void adm_build_mid_str(uint8_t flag, char *nn, int nn_len, int offset, uint8_t *mid_str)
{
	uint8_t *cursor = NULL;
	Sdnv len;
	Sdnv off;
	uint32_t nn_size;
	uint8_t *tmp = NULL;
	int size = 0;

	DTNMP_DEBUG_ENTRY("adm_build_mid_str", "(%d, %s, %d, %d)",
			          flag, nn, nn_len, offset);


	encodeSdnv(&len, nn_len + 1);
	encodeSdnv(&off, offset);
	tmp = utils_string_to_hex(nn, &nn_size);

	size = 1 + nn_size + len.length + off.length + 1;

	if(size > ADM_MID_ALLOC)
	{
		DTNMP_DEBUG_ERR("adm_build_mid_str",
						"Size %d bigger than max MID size of %d.",
						size,
						ADM_MID_ALLOC);
		DTNMP_DEBUG_EXIT("adm_build_mid_str","->.", NULL);
		MRELEASE(tmp);
		return;
	}

	cursor = mid_str;

	memcpy(cursor, &flag, 1);
	cursor += 1;

	memcpy(cursor, len.text, len.length);
	cursor += len.length;

	memcpy(cursor, tmp, nn_size);
	cursor += nn_size;

	memcpy(cursor, off.text, off.length);
	cursor += off.length;

	memset(cursor, 0, 1); // NULL terminator.

	DTNMP_DEBUG_EXIT("adm_build_mid_str","->%s", mid_str);
	MRELEASE(tmp);
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
 *  07/27/13  E. Birrane     Hold data defs in a Lyst.
 *
 *****************************************************************************/

uint8_t *adm_copy_integer(uint8_t *value, uint8_t size, uint32_t *length)
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

uint8_t* adm_copy_string(char *value, uint32_t *length)
{
	uint8_t *result = NULL;
	uint32_t size = 0;

	DTNMP_DEBUG_ENTRY("adm_copy_string","(%#llx, %d, %#llx)", value, size, length);

	/* Step 0 - Sanity Check. */
	if((value == NULL) || (length == NULL))
	{
		DTNMP_DEBUG_ERR("adm_copy_string","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_copy_string","->NULL.", NULL);
		return NULL;
	}

	size = strlen(value) + 1;
	/* Step 1 - Alloc new space. */
	if((result = (uint8_t *) MTAKE(size)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_copy_string","Can't alloc %d bytes.", size);
		DTNMP_DEBUG_EXIT("adm_copy_string","->NULL.", NULL);
		return NULL;
	}

	/* Step 2 - Copy data in. */
	*length = size;
	memcpy(result, value, size);

	/* Step 3 - Return. */
	DTNMP_DEBUG_EXIT("adm_copy_string","->%s", (char *)result);
	return (uint8_t*)result;
}



void adm_destroy()
{
   LystElt elt = 0;

   for (elt = lyst_first(gAdmData); elt; elt = lyst_next(elt))
   {
	   adm_datadef_t *cur = (adm_datadef_t *) lyst_data(elt);
	   mid_release(cur->mid);
	   MRELEASE(cur);
   }
   lyst_destroy(gAdmData);
   gAdmData = NULL;

   for (elt = lyst_first(gAdmComputed); elt; elt = lyst_next(elt))
   {
	   def_gen_t *cur = (def_gen_t *) lyst_data(elt);
	   def_release_gen(cur);
   }
   lyst_destroy(gAdmComputed);
   gAdmComputed = NULL;


   for (elt = lyst_first(gAdmCtrls); elt; elt = lyst_next(elt))
   {
	   adm_ctrl_t *cur = (adm_ctrl_t *) lyst_data(elt);
	   mid_release(cur->mid);
	   MRELEASE(cur);
   }
   lyst_destroy(gAdmCtrls);
   gAdmCtrls = NULL;

   for (elt = lyst_first(gAdmRpts); elt; elt = lyst_next(elt))
   {
	   def_gen_t *cur = (def_gen_t *) lyst_data(elt);
	   def_release_gen(cur);
   }
   lyst_destroy(gAdmRpts);
   gAdmRpts = NULL;

   lyst_destroy(gAdmLiterals);
   gAdmLiterals = NULL;

   lyst_destroy(gAdmOps);
   gAdmOps = NULL;

}



uint8_t* adm_extract_blob(Lyst params, uint32_t idx, uint32_t *size, uint8_t *success)
{
	datacol_entry_t *entry = NULL;
	uint8_t *result = NULL;
	*success = 0;

	if((params == NULL) || (size == NULL))
	{
		DTNMP_DEBUG_ERR("adm_extract_blob","Bad Args.", NULL);
		return NULL;
	}

	if((entry = adm_extract_dc_entry(params, idx, success)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_blob","Can't get item %d", idx);
		return NULL;
	}

	*size = entry->length;
	if((result = (uint8_t *) MTAKE(*size)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_blob","Can't allocate result of size %d", *size);
		return NULL;
	}

	memcpy(result, entry->value, *size);
	*success = 1;
	return result;
}

uint8_t adm_extract_byte(Lyst params, uint32_t idx, uint8_t *success)
{
	datacol_entry_t *entry = NULL;
	uint8_t result = 0;

	*success = 0;

	if((entry = adm_extract_dc_entry(params, idx, success)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_byte","Can't get item %d", idx);
		return 0;
	}

	if(entry->length != sizeof(result))
	{
		DTNMP_DEBUG_ERR("adm_extract_byte","Expected length %d and got %d", sizeof(result),entry->length);
		return 0;
	}

	result = (uint8_t) entry->value[0];
	*success = 1;
	return result;
}


Lyst adm_extract_dc(Lyst params, uint32_t idx, uint8_t *success)
{
	datacol_entry_t *entry = NULL;
	uint32_t bytes = 0;
	Lyst result = NULL;

	*success = 0;
	if((entry = adm_extract_dc_entry(params, idx, success)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_dc","Can't get item %d", idx);
		return NULL;
	}

	if((result = dc_deserialize(entry->value, entry->length, &bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_dc","Can't get DC.", NULL);
		return NULL;
	}

	if(bytes != entry->length)
	{
		DTNMP_DEBUG_ERR("adm_extract_dc","mismatched deserialize (%d != %d)", bytes, entry->length);
		dc_destroy(&result);
		return NULL;
	}

	*success = 1;
	return result;
}

datacol_entry_t* adm_extract_dc_entry(Lyst params, uint32_t idx, uint8_t *success)
{
	LystElt elt = NULL;
	uint32_t i = 1;

	*success = 0;
	if(params == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_dc_entry","Bad Args.", NULL);
		return NULL;
	}


	for(elt = lyst_first(params); elt; elt = lyst_next(elt))
	{
		if(i == idx)
		{
			*success = 1;
			return (datacol_entry_t *) lyst_data(elt);
		}
		i++;
	}

	return NULL;
}


def_gen_t* adm_extract_def(Lyst params, uint32_t idx, uint8_t *success)
{
	datacol_entry_t *entry = NULL;
	uint32_t bytes = 0;
	def_gen_t *result = NULL;

	*success = 0;

	if((entry = adm_extract_dc_entry(params, idx, success)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_def","Can't get item %d", idx);
		return NULL;
	}

	if((result = def_deserialize_gen(entry->value, entry->length, &bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_def","Can't get def.", NULL);
		return NULL;
	}

	if(bytes != entry->length)
	{
		DTNMP_DEBUG_ERR("adm_extract_def","mismatched deserialize (%d != %d)", bytes, entry->length);
		def_release_gen(result);
		return NULL;
	}

	*success = 1;
	return result;
}


int32_t adm_extract_int(Lyst params, uint32_t idx, uint8_t *success)
{
	datacol_entry_t *entry = NULL;
	uint32_t bytes = 0;
	int32_t result = 0;

	*success = 0;

	if((entry = adm_extract_dc_entry(params, idx, success)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_int","Can't get item %d", idx);
		return 0;
	}

	result = val_deserialize_int(entry->value, entry->length, &bytes);

	if(bytes != entry->length)
	{
		DTNMP_DEBUG_ERR("adm_extract_int","mismatched deserialize (%d != %d)", bytes, entry->length);
		return 0;
	}

	*success = 1;
	return result;
}

Lyst             adm_extract_mc(Lyst params, uint32_t idx, uint8_t* success)
{
	datacol_entry_t *entry = NULL;
	uint32_t bytes = 0;
	Lyst result = NULL;

	*success = 0;

	if((entry = adm_extract_dc_entry(params, idx, success)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_mc","Can't get item %d", idx);
		return NULL;
	}

	if((result = midcol_deserialize(entry->value, entry->length, &bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_mc","Can't get def.", NULL);
		return NULL;
	}

	if(bytes != entry->length)
	{
		DTNMP_DEBUG_ERR("adm_extract_mc","mismatched deserialize (%d != %d)", bytes, entry->length);
		midcol_destroy(&result);
		return NULL;
	}

	*success = 1;
	return result;
}

mid_t* adm_extract_mid(Lyst params, uint32_t idx, uint8_t* success)
{
	datacol_entry_t *entry = NULL;
	uint32_t bytes = 0;
	mid_t *result = NULL;

	*success = 0;

	if((entry = adm_extract_dc_entry(params, idx, success)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_mid","Can't get item %d", idx);
		return NULL;
	}

	if((result = mid_deserialize(entry->value, entry->length, &bytes)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_mid","Can't get def.", NULL);
		return NULL;
	}

	if(bytes != entry->length)
	{
		DTNMP_DEBUG_ERR("adm_extract_mid","mismatched deserialize (%d != %d)", bytes, entry->length);
		mid_release(result);
		return NULL;
	}

	*success = 1;
	return result;

}


float adm_extract_real32(Lyst params, uint32_t idx, uint8_t* success)
{
	datacol_entry_t *entry = NULL;
	uint32_t bytes = 0;
	float result = 0;

	*success = 0;

	if((entry = adm_extract_dc_entry(params, idx, success)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_real32","Can't get item %d", idx);
		return 0;
	}

	result = val_deserialize_real32(entry->value, entry->length, &bytes);

	if(bytes != entry->length)
	{
		DTNMP_DEBUG_ERR("adm_extract_real32","mismatched deserialize (%d != %d)", bytes, entry->length);
		return 0;
	}

	*success = 1;
	return result;
}


double adm_extract_real64(Lyst params, uint32_t idx, uint8_t* success)
{
	datacol_entry_t *entry = NULL;
	uint32_t bytes = 0;
	double result = 0;

	*success = 0;

	if((entry = adm_extract_dc_entry(params, idx, success)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_real64","Can't get item %d", idx);
		return 0;
	}

	result = val_deserialize_real64(entry->value, entry->length, &bytes);

	if(bytes != entry->length)
	{
		DTNMP_DEBUG_ERR("adm_extract_real64","mismatched deserialize (%d != %d)", bytes, entry->length);
		return 0;
	}

	*success = 1;
	return result;
}


uvast adm_extract_sdnv(Lyst params, uint32_t idx, uint8_t* success)
{
	datacol_entry_t *entry = NULL;
	uint32_t bytes = 0;
	uvast result = 0;

	*success = 0;

	if((entry = adm_extract_dc_entry(params, idx, success)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_sdnv","Can't get item %d", idx);
		return 0;
	}


	bytes = utils_grab_sdnv((unsigned char *) entry->value, entry->length, &result);

	if(bytes != entry->length)
	{
		DTNMP_DEBUG_ERR("adm_extract_sdnv","mismatched deserialize (%d != %d)", bytes, entry->length);
		return 0;
	}

	*success = 1;
	return result;
}

char* adm_extract_string(Lyst params, uint32_t idx, uint8_t* success)
{
	datacol_entry_t *entry = NULL;
	uint32_t bytes = 0;
	char *result = NULL;

	*success = 0;

	if((entry = adm_extract_dc_entry(params, idx, success)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_string","Can't get item %d", idx);
		return 0;
	}

	if((result = (char *) MTAKE(entry->length + 1)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_string","Can'tallocate %d bytes.", entry->length + 1);
		return NULL;
	}

	memcpy(result, entry->value, entry->length);
	result[entry->length] = '\0';

	*success = 1;
	return result;
}


uint32_t adm_extract_uint(Lyst params, uint32_t idx, uint8_t* success)
{
	return (uint32_t) adm_extract_int(params, idx, success);
}


uvast adm_extract_uvast(Lyst params, uint32_t idx, uint8_t* success)
{
	return (uvast) adm_extract_vast(params, idx, success);
}

vast adm_extract_vast(Lyst params, uint32_t idx, uint8_t* success)
{
	datacol_entry_t *entry = NULL;
	uint32_t bytes = 0;
	vast result = 0;

	*success = 0;

	if((entry = adm_extract_dc_entry(params, idx, success)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_extract_vast","Can't get item %d", idx);
		return 0;
	}

	result = val_deserialize_vast(entry->value, entry->length, &bytes);

	if(bytes != entry->length)
	{
		DTNMP_DEBUG_ERR("adm_extract_vast","mismatched deserialize (%d != %d)", bytes, entry->length);
		return 0;
	}

	*success = 1;
	return result;
}


def_gen_t* adm_find_computeddef(mid_t *mid)
{
	LystElt elt = 0;
	def_gen_t *cur = NULL;

	DTNMP_DEBUG_ENTRY("adm_find_computeddef","(%llx)", mid);

	/* Step 0 - Sanity Check. */
	if(mid == NULL)
	{
		DTNMP_DEBUG_ERR("adm_find_computeddef", "Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_find_computeddef", "->NULL.", NULL);
		return NULL;
	}

	/* Step 1 - Go lookin'. */
	for(elt = lyst_first(gAdmComputed); elt; elt = lyst_next(elt))
	{
		cur = (def_gen_t*) lyst_data(elt);

		/* Step 1.1 - Determine if we need to account for parameters. */
		if (mid_compare(mid, cur->id, 0) == 0)
		{
			break;
		}
		cur = NULL;
	}

	/* Step 2 - Return what we found, or NULL. */

	DTNMP_DEBUG_EXIT("adm_find_computeddef", "->%llx.", cur);
	return cur;
}


/******************************************************************************
 *
 * \par Function Name: adm_find_datadef
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
 *  07/27/13  E. Birrane     Hold data defs in a Lyst.
 *****************************************************************************/

adm_datadef_t *adm_find_datadef(mid_t *mid)
{
	LystElt elt = 0;
	adm_datadef_t *cur = NULL;

	DTNMP_DEBUG_ENTRY("adm_find_datadef","(%llx)", mid);

	/* Step 0 - Sanity Check. */
	if(mid == NULL)
	{
		DTNMP_DEBUG_ERR("adm_find_datadef", "Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_find_datadef", "->NULL.", NULL);
		return NULL;
	}

	/* Step 1 - Go lookin'. */
	for(elt = lyst_first(gAdmData); elt; elt = lyst_next(elt))
	{
		cur = (adm_datadef_t*) lyst_data(elt);

		/* Step 1.1 - Determine if we need to account for parameters. */
		if (mid_compare(mid, cur->mid, 0) == 0)
		{
			break;
		}

		cur = NULL;
	}

	/* Step 2 - Return what we found, or NULL. */

	DTNMP_DEBUG_EXIT("adm_find_datadef", "->%llx.", cur);
	return cur;
}



/******************************************************************************
 *
 * \par Function Name: adm_find_datadef_by_name
 *
 * \par Find an ADM entry that corresponds to a user-readable name.
 *
 * \retval NULL Failure
 *         !NULL The found ADM entry
 *
 * \param[in]  name  The name whose ADM-match is being queried.
 *
 * \par Notes:
 *		1. The returned entry is a direct pointer to the official ADM entry,
 *		   it must be treated as read-only.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/27/13  E. Birrane     Initial implementation.
 ***************************************************************************** /

adm_datadef_t* adm_find_datadef_by_name(char *name)
{
	LystElt elt = 0;
	adm_datadef_t *cur = NULL;

	DTNMP_DEBUG_ENTRY("adm_find_datadef_by_name","(%s)", name);

	/ * Step 0 - Sanity Check. * /
	if(name == NULL)
	{
		DTNMP_DEBUG_ERR("adm_find_datadef_by_name", "Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_find_datadef_by_name", "->NULL.", NULL);
		return NULL;
	}

	/ * Step 1 - Go lookin'. * /
	for(elt = lyst_first(gAdmData); elt; elt = lyst_next(elt))
	{
		cur = (adm_datadef_t*) lyst_data(elt);

		if(memcmp(name, cur->name, strlen(cur->name)) == 0)
		{
			break;
		}

		cur = NULL;
	}

	/ * Step 2 - Return what we found, or NULL. * /

	DTNMP_DEBUG_EXIT("adm_find_datadef_by_name", "->%llx.", cur);
	return cur;
}
************/

adm_datadef_t* adm_find_datadef_by_idx(int idx)
{
	LystElt elt = 0;
	int i = 0;
	adm_datadef_t *cur = NULL;

	DTNMP_DEBUG_ENTRY("adm_find_datadef_by_name","(%d)", idx);

	/* Step 1 - Go lookin'. */
	for(elt = lyst_first(gAdmData); elt; elt = lyst_next(elt))
	{
		cur = (adm_datadef_t*) lyst_data(elt);
		if(i == idx)
		{
			break;
		}
		i++;
		cur = NULL;
	}

	/* Step 2 - Return what we found, or NULL. */

	DTNMP_DEBUG_EXIT("adm_find_datadef_by_name", "->%llx.", cur);
	return cur;
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
 *  07/27/13  E. Birrane     Hold data defs in a Lyst.
 *****************************************************************************/

adm_ctrl_t*  adm_find_ctrl(mid_t *mid)
{
	LystElt elt = 0;
	adm_ctrl_t *cur = NULL;

	DTNMP_DEBUG_ENTRY("adm_find_ctrl","(%#llx)", mid);

	/* Step 0 - Sanity Check. */
	if(mid == NULL)
	{
		DTNMP_DEBUG_ERR("adm_find_ctrl", "Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_find_ctrl", "->NULL.", NULL);
		return NULL;
	}

	for(elt = lyst_first(gAdmCtrls); elt; elt = lyst_next(elt))
	{
		cur = (adm_ctrl_t *) lyst_data(elt);

		char *tmp1 = mid_to_string(mid);
		char *tmp2 = mid_to_string(cur->mid);
		MRELEASE(tmp1);
		MRELEASE(tmp2);

		if (mid_compare(mid, cur->mid, 0) == 0)
		{
			break;
		}
/**
		/ * Step 1.1 - Determine if we need to account for parameters. * /
		if(cur->num_parms == 0)
		{
			/ * Step 1.1.1 - If no params, straight compare * /
			if((mid->raw_size == cur->mid_len) &&
				(memcmp(mid->raw, cur->mid, mid->raw_size) == 0))
			{
				break;
			}
		}
		else
		{
			uvast tmp;
			unsigned char *cursor = (unsigned char*) &(cur->mid[1]);
			/ * Grab size less paramaters. Which is SDNV at [1]. * /
			/ * \todo: We need a more refined compare here.  For example, the
			 *        code below will not work if tag values are used.
			 * /
			unsigned long bytes = decodeSdnv(&tmp, cursor);
			if(memcmp(mid->raw, cur->mid, tmp + bytes + 1) == 0)
			{
				break;
			}
		}
	*/

		cur = NULL;
	}

	/* Step 2 - Return what we found, or NULL. */

	DTNMP_DEBUG_EXIT("adm_find_ctrl", "->%llx.", cur);

	return cur;
}



/******************************************************************************
 *
 * \par Function Name: adm_find_ctrl_by_name
 *
 * \par Find an ADM control that corresponds to a user-readable name.
 *
 * \retval NULL Failure
 *         !NULL The found ADM entry
 *
 * \param[in]  name  The name whose ADM-match is being queried.
 *
 * \par Notes:
 *		1. The returned entry is a direct pointer to the official ADM entry,
 *		   it must be treated as read-only.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  07/27/13  E. Birrane     Initial implementation.
 ***************************************************************************** /

adm_ctrl_t* adm_find_ctrl_by_name(char *name)
{
	LystElt elt = 0;
	adm_ctrl_t *cur = NULL;

	DTNMP_DEBUG_ENTRY("adm_find_ctrl_by_name","(%s)", name);

	/ * Step 0 - Sanity Check. * /
	if(name == NULL)
	{
		DTNMP_DEBUG_ERR("adm_find_ctrl_by_name", "Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_find_ctrl_by_name", "->NULL.", NULL);
		return NULL;
	}

	/ * Step 1 - Go lookin'. * /
	for(elt = lyst_first(gAdmData); elt; elt = lyst_next(elt))
	{
		cur = (adm_ctrl_t*) lyst_data(elt);

		if(memcmp(name, cur->name, strlen(cur->name)) == 0)
		{
			break;
		}

		cur = NULL;
	}

	/ * Step 2 - Return what we found, or NULL. * /
	DTNMP_DEBUG_EXIT("adm_find_ctrl_by_name", "->%llx.", cur);
	return cur;
}
*****/


adm_ctrl_t* adm_find_ctrl_by_idx(int idx)
{
	LystElt elt = 0;
	int i = 0;
	adm_ctrl_t *cur = NULL;

	DTNMP_DEBUG_ENTRY("adm_find_ctrl_by_idx","(%d)", idx);

	/* Step 1 - Go lookin'. */
	for(elt = lyst_first(gAdmCtrls); elt; elt = lyst_next(elt))
	{
		cur = (adm_ctrl_t*) lyst_data(elt);
		if(i == idx)
		{
			break;
		}
		i++;
		cur = NULL;
	}

	/* Step 2 - Return what we found, or NULL. */

	DTNMP_DEBUG_EXIT("adm_find_ctrl_by_idx", "->%llx.", cur);
	return cur;
}



/******************************************************************************
 *
 * \par Function Name: adm_find_lit
 *
 * \par Find an ADM literal that corresponds to a received MID.
 *
 * \retval NULL Failure
 *         !NULL The found ADM literal
 *
 * \param[in]  mid  The MID whose ADM-match is being queried.
 *
 * \par Notes:
 *		1. The returned entry is a direct pointer to the official ADM lit,
 *		   it must be treated as read-only.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/25/15  E. Birrane     Initial implementation.
 *****************************************************************************/

lit_t*	   adm_find_lit(mid_t *mid)
{
	LystElt elt = 0;
	lit_t *cur = NULL;

	DTNMP_DEBUG_ENTRY("adm_find_lit","(%#llx)", (unsigned long) mid);

	/* Step 0 - Sanity Check. */
	if(mid == NULL)
	{
		DTNMP_DEBUG_ERR("adm_find_lit", "Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_find_lit", "->NULL.", NULL);
		return NULL;
	}

	for(elt = lyst_first(gAdmLiterals); elt; elt = lyst_next(elt))
	{
		cur = (lit_t *) lyst_data(elt);

		if (mid_compare(mid, cur->id, 0) == 0)
		{
			break;
		}
	}

	/* Step 2 - Return what we found, or NULL. */

	DTNMP_DEBUG_EXIT("adm_find_lit", "->%llx.", (unsigned long) cur);

	return cur;
}


/******************************************************************************
 *
 * \par Function Name: adm_find_op
 *
 * \par Find an ADM operator that corresponds to a received MID.
 *
 * \retval NULL Failure
 *         !NULL The found ADM operator
 *
 * \param[in]  mid  The MID whose ADM-match is being queried.
 *
 * \par Notes:
 *		1. The returned entry is a direct pointer to the official ADM op,
 *		   it must be treated as read-only.
 *
 * Modification History:
 *  MM/DD/YY  AUTHOR         DESCRIPTION
 *  --------  ------------   ---------------------------------------------
 *  04/25/15  E. Birrane     Initial implementation.
 *****************************************************************************/

adm_op_t*  adm_find_op(mid_t *mid)
{
	LystElt elt = 0;
	adm_op_t *cur = NULL;

	DTNMP_DEBUG_ENTRY("adm_find_op","(%#llx)", (unsigned long) mid);

	/* Step 0 - Sanity Check. */
	if(mid == NULL)
	{
		DTNMP_DEBUG_ERR("adm_find_op", "Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_find_op", "->NULL.", NULL);
		return NULL;
	}

	for(elt = lyst_first(gAdmOps); elt; elt = lyst_next(elt))
	{
		cur = (adm_op_t *) lyst_data(elt);

		if (mid_compare(mid, cur->mid, 0) == 0)
		{
			break;
		}
	}

	/* Step 2 - Return what we found, or NULL. */

	DTNMP_DEBUG_EXIT("adm_find_op", "->%llx.", (unsigned long) cur);

	return cur;
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

	gAdmData = lyst_create();
	gAdmComputed = lyst_create();
	gAdmCtrls = lyst_create();
	gAdmLiterals = lyst_create();
	gAdmOps = lyst_create();
	gAdmRpts = lyst_create();
	gAdmMacros = lyst_create();

	adm_bp_init();

#ifdef _HAVE_LTP_ADM_
	adm_ltp_init();
#endif /* _HAVE_LTP_ADM_ */

#ifdef _HAVE_ION_ADM_
	adm_ion_init();
#endif /* _HAVE_ION_ADM_ */

	adm_agent_init();

	//initBpAdm();
	//initLtpAdm();
	//initIonAdm();


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

	if((len > buffer_len) || (len != data_len))
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

	cursor += sprintf(cursor,"("UVAST_FIELDSPEC"): ",num);

	/* Add stirngs to result. */
	int i;
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

	cursor += sprintf(cursor,"("UVAST_FIELDSPEC"): ",num);

	int i;
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
 * \par Function Name: adm_print_uvast
 *
 * \par Generates a single string representation of a uvast.
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
 *  08/16/13  E. Birrane     Initial implementation.
 *****************************************************************************/
char *adm_print_uvast(uint8_t* buffer, uint64_t buffer_len,
		              uint64_t data_len, uint32_t *str_len)
{
  char *result;
  uint64_t temp = 0;

  DTNMP_DEBUG_ENTRY("adm_print_uvast", "(%#llx, %ull, %ull, %#llx)", buffer, buffer_len, data_len, str_len);

  /* Step 0 - Sanity Checks. */
  if((buffer == NULL) || (str_len == NULL))
  {
	  DTNMP_DEBUG_ERR("adm_print_uvast", "Bad Args.", NULL);
	  DTNMP_DEBUG_EXIT("adm_print_uvast", "->NULL.", NULL);
	  return NULL;
  }

  /* Step 1 - Make sure we have buffer space. */
  if(data_len > buffer_len)
  {
	 DTNMP_DEBUG_ERR("adm_print_uvast","Data Len %d > buf len %d.",
			         data_len, buffer_len);
	 *str_len = 0;

	 DTNMP_DEBUG_EXIT("adm_print_uvast", "->NULL.", NULL);
	 return NULL;
  }

  /* Step 2 - Size the string and allocate it.
   * \todo: A better estimate should go here. */
  *str_len = 32;

  if((result = (char *) MTAKE(*str_len)) == NULL)
  {
		 DTNMP_DEBUG_ERR("adm_print_uvast","Can't alloc %d bytes.",
				         *str_len);
		 *str_len = 0;

		 DTNMP_DEBUG_EXIT("adm_print_uvast", "->NULL.", NULL);
		 return NULL;
  }

  /* Step 3 - Copy data and return. */
  memcpy(&temp, buffer, data_len);
  isprintf(result,*str_len,UVAST_FIELDSPEC, temp);

  DTNMP_DEBUG_EXIT("adm_print_uvast", "->%#llx.", result);
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
	int i;
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



/******************************************************************************
 *
 * \par Function Name: adm_size_uvast
 *
 * \par Calculates size of a uvast, as an ADM sizing callback.
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
 *  08/16/13  E. Birrane     Initial implementation.
 *****************************************************************************/
uint32_t adm_size_uvast(uint8_t* buffer, uint64_t buffer_len)
{
	uint32_t len = 0;

	DTNMP_DEBUG_ENTRY("adm_size_uvast","(%#llx, %ull)", buffer, buffer_len);

	/* Step 0 - Sanity Check. */
	if(buffer == NULL)
	{
		DTNMP_DEBUG_ERR("adm_size_uvast","Bad Args.", NULL);
		DTNMP_DEBUG_EXIT("adm_size_uvast","->0.", NULL);
		return 0;
	}

	DTNMP_DEBUG_EXIT("adm_size_uvast","->%ul", sizeof(uvast));
	return sizeof(uvast);
}



/*
 * Take parameters in the MID and turn them into a MC lyst.
 */
Lyst adm_parm_mc(mid_t *mid)
{
	Lyst result = NULL;
	uint32_t bytes = 0;
	datacol_entry_t *dc = NULL;

	if(mid == NULL)
	{
		return NULL;
	}

	if(mid_get_num_parms(mid) != 1)
	{
		DTNMP_DEBUG_ERR("adm_parm_mc","Expected 1 MC parm, not %d.",
				        mid_get_num_parms(mid));
		return NULL;
	}

	if((dc = mid_get_param(mid, 0)) == NULL)
	{
		DTNMP_DEBUG_ERR("adm_parm_mc","Can't get parameter.",NULL);
		return NULL;
	}

	result = midcol_deserialize(dc->value, dc->length, &bytes);

	MRELEASE(dc->value);
	MRELEASE(dc);

	return result;
}

