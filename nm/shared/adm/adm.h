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
 ** File Name: adm.h
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

#ifndef ADM_H_
#define ADM_H_

#include "lyst.h"

#include "shared/primitives/mid.h"
#include "shared/utils/expr.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

/* The largest size of a supported MID, in bytes. */
#define ADM_MID_ALLOC (15)

/* The longest user name of a MID, in bytes. */
#define ADM_MAX_NAME  (32)


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


/**
 * Functions implementing key responsibilities for each ADM item:
 * - The data collect function collects data from the local agent.
 * - The string function generates a String representation of the data value.
 * - The size function returns the size of the data, in bytes.
 * - The ctrl function executes a control associated with this MID.
 */
typedef expr_result_t (*adm_data_collect_fn)(Lyst parms);
typedef char* (*adm_string_fn)(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
typedef uint32_t (*adm_size_fn)(uint8_t* buffer, uint64_t buffer_len);
typedef uint32_t (*adm_ctrl_fn)(Lyst params);


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
    char name[ADM_MAX_NAME];     /**> Name of this MIB item                */

    mid_t *mid;					 /**> The MID identifying this def.        */

    //uint8_t mid[ADM_MID_ALLOC];  /**> The MID up to, but excluding, params */
    //uint8_t mid_len;             /**> Length, in bytes, of the MID.        */
    uint8_t num_parms;           /**> # params needed to complete this MID.*/

    adm_size_fn get_size;        /**> Configured sizing function.          */

    adm_data_collect_fn collect; /**> Configured data collection function. */

    adm_string_fn to_string;     /**> Configured to-string function.       */

} adm_datadef_t;


/**
 * Describes an ADM Control in the system.
 *
 * This structure captures general information about a control, including
 * its name an associated MID.
 *
 * Note: The run function is OPTIONAL and is only configured on DTNMP Actors
 * that are authorized to perform the control.
 */
typedef struct
{
	char name[ADM_MAX_NAME];

    mid_t *mid;					 /**> The MID identifying this def.        */

//    uint8_t mid[ADM_MID_ALLOC];  /**> The MID up to, but excluding, params */
//    uint8_t mid_len;             /**> Length, in bytes, of the MID.        */
    uint8_t num_parms;           /**> # params needed to complete this MID.*/

    adm_ctrl_fn run;             /**> Configured data collection function. */
} adm_ctrl_t;



/*
 * +--------------------------------------------------------------------------+
 * |						  DATA DEFINITIONS  							  +
 * +--------------------------------------------------------------------------+
 */

/**
 * Global data collection of supported ADM information.
 */
extern Lyst gAdmData;
extern Lyst gAdmCtrls;
extern Lyst gAdmLiterals;
extern Lyst gAdmOps;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

void         adm_add_datadef(char *name, uint8_t *mid_str, int num_parms, adm_string_fn to_string, adm_size_fn get_size);
void         adm_add_datadef_collect(uint8_t *mid_str, adm_data_collect_fn collect);

void         adm_add_ctrl(char *name, uint8_t *mid_str, int num_parms);
void         adm_add_ctrl_run(uint8_t *mid_str, adm_ctrl_fn run);


void         adm_build_mid_str(uint8_t flag, char *nn, int nn_len, int offset, uint8_t *mid_str);

uint8_t*     adm_copy_integer(uint8_t *value, uint8_t size, uint64_t *length);
uint8_t*     adm_copy_string(char *value, uint64_t *length);

// \todo: void 		 adm_erase_datadef(adm_datadef_t *entry);
// \todo: void 		 adm_erase_ctrl(adm_ctrl_t *entry);

void         adm_destroy();

adm_datadef_t* adm_find_datadef(mid_t *mid);
adm_datadef_t* adm_find_datadef_by_name(char *name);
adm_datadef_t* adm_find_datadef_by_idx(int idx);

adm_ctrl_t*    adm_find_ctrl(mid_t *mid);
adm_ctrl_t*    adm_find_ctrl_by_name(char *name);
adm_ctrl_t*    adm_find_ctrl_by_idx(int idx);

void         adm_init();

char*        adm_print_string(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char*        adm_print_string_list(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char*        adm_print_unsigned_long(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char*        adm_print_unsigned_long_list(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char*        adm_print_uvast(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);

uint32_t     adm_size_string(uint8_t* buffer, uint64_t buffer_len);
uint32_t     adm_size_string_list(uint8_t* buffer, uint64_t buffer_len);
uint32_t     adm_size_unsigned_long(uint8_t* buffer, uint64_t buffer_len);
uint32_t     adm_size_unsigned_long_list(uint8_t* buffer, uint64_t buffer_len);
uint32_t     adm_size_uvast(uint8_t* buffer, uint64_t buffer_len);


#endif /* ADM_H_*/
