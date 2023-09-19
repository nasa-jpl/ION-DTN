/*****************************************************************************
 **
 ** \file nm_mgr_names.h
 **
 **
 ** Description: Helper file holding optional hard-coded human-readable
 **              names and descriptions for supported ADM entries.
 **
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  04/26/15  E. Birrane     Initial Implementation (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef MGR_NAMES_H_
#define MGR_NAMES_H_

#include "nm_mgr.h"

#include "../shared/utils/nm_types.h"
#include "../shared/adm/adm.h"
#include "../shared/primitives/mid.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */
#define NAMES_NAME_MAX 32
#define NAMES_DESCR_MAX 100


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


/**
 * Describes an ADM data definition entry in the system.
 *
 * This structure captures general information for those ADM entries pre-
 * configured on the local machine.
 *
 * Note: The collect function is OPTIONAL and is only configured on DTNMP
 * Actors acting as Agents.
 */
typedef struct
{
	uint32_t adm; /**> The associated ADM.                 */

    mid_t *mid;	  /**> The MID being named.                */

    char name[NAMES_NAME_MAX];   /**> The string name of the item.        */

    char descr[NAMES_DESCR_MAX];  /**> The string description of the item. */
} mgr_name_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  DATA DEFINITIONS  							  +
 * +--------------------------------------------------------------------------+
 */

/**
 * Global data collection of supported ADM information.
 */
extern Lyst gMgrNames;


int    names_add_name(char *name, char *desc, int adm, char *mid_str);

mid_t* names_get_mid(int adm_type, int mid_id, int idx);

char*  names_get_name(mid_t *mid);

void   names_init();

Lyst   names_retrieve(int adm_type, int mid_id);

void   names_lyst_destroy(Lyst *names);

void   names_destroy_entry(mgr_name_t *name);

#endif // MGR_NAMES_H_
