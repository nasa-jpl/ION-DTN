/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
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
 **  10/22/11  E. Birrane     Initial Implementation (JHU/APL)
 **  11/13/12  E. Birrane     Technical review, comment updates. (JHU/APL)
 **  08/21/16  E. Birrane     Updated to Agent ADM v0.2 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef ADM_H_
#define ADM_H_

#include "lyst.h"

#include "../primitives/var.h"
#include "../primitives/dc.h"
#include "../primitives/def.h"
#include "../primitives/tdc.h"
#include "../primitives/mid.h"
#include "../primitives/value.h"
#include "../primitives/lit.h"
#include "../utils/nm_types.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

/* The largest size of a supported MID, in bytes. */
#define ADM_MID_ALLOC (15)

/* The longest user name of a MID, in bytes. */
#define ADM_MAX_NAME  (32)

/* Known ADMs...*/
#define ADM_ALL   0
#define ADM_AGENT 1
#define ADM_BP    2
#define ADM_LTP   3
#define ADM_ION   4
#define ADM_BPSEC 5


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
typedef value_t (*adm_data_collect_fn)(tdc_t parms);
typedef char* (*adm_string_fn)(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
typedef uint32_t (*adm_size_fn)(uint8_t* buffer, uint64_t buffer_len);

typedef value_t (*adm_op_fn)(Lyst stack);


typedef tdc_t* (*adm_ctrl_run_fn)(eid_t *def_mgr, tdc_t params, int8_t *status);


/*
 * ADM structure definitions capture the generic information that defines a
 * particular data type. This may be separate from an individual instance of
 * the item, as instantiated in time, or with specific parameters.
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
    mid_t *mid;					 /**> The MID identifying this def.        */

    amp_type_e type;           /**> The data type of this MID.           */

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
 */
typedef struct
{
    mid_t *mid;					 /**> The MID identifying this def.        */

    uint8_t num_parms;           /**> # params needed to complete this MID.*/

    adm_ctrl_run_fn run;         /**> Function implementing the control.   */

} adm_ctrl_t;


/**
 * Describes an ADM Operator in the system.
 *
 * This structure captures general information about an operator, including
 * its associated MID.
 */
typedef struct
{
    mid_t *mid;					 /**> The MID identifying this def.        */

    uint8_t num_parms;           /**> # params needed to complete this MID.*/

    adm_op_fn apply;             /**> Configured operator apply function. */
} adm_op_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  DATA DEFINITIONS  							  +
 * +--------------------------------------------------------------------------+
 */

/**
 * Global data collection of supported ADM information.
 */
extern Lyst gAdmData;
extern Lyst gAdmComputed; // Type cd_t
extern Lyst gAdmCtrls;
extern Lyst gAdmLiterals;
extern Lyst gAdmOps;
extern Lyst gAdmRpts; // Type def_gen_t
extern Lyst gAdmMacros; // Type def_gen_t


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

int         adm_add_datadef(char *mid_str, amp_type_e type, int num_parms, adm_data_collect_fn collect, adm_string_fn to_string, adm_size_fn get_size);

int		 	adm_add_computeddef(char *mid_str, amp_type_e type, expr_t *expr);


int         adm_add_ctrl(char *mid_str, adm_ctrl_run_fn run);

int			adm_add_lit(char *mid_str, value_t result, lit_val_fn calc);

int			adm_add_macro(char *mid_str, Lyst midcol);

int 		adm_add_op(char *mid_str, uint8_t num_parms, adm_op_fn run);

int 		adm_add_rpt(char *mid_str, Lyst midcol);

void        adm_build_mid_str(uint8_t flag, char *nn, int nn_len, int offset, uint8_t *mid_str);

uint8_t*     adm_copy_integer(uint8_t *value, uint8_t size, uint32_t *length);
uint8_t*     adm_copy_string(char *value, uint32_t *length);

void         adm_destroy();



/* Helper functions */
blob_t*         adm_extract_blob(tdc_t params, uint32_t idx, int8_t *success);
uint8_t          adm_extract_byte(tdc_t params, uint32_t idx, int8_t *success);
Lyst             adm_extract_dc(tdc_t params, uint32_t idx, int8_t *success);
expr_t *         adm_extract_expr(tdc_t params, uint32_t idx, int8_t *success);
int32_t          adm_extract_int(tdc_t params, uint32_t idx, int8_t *success);
Lyst             adm_extract_mc(tdc_t params, uint32_t idx, int8_t *success);
mid_t*           adm_extract_mid(tdc_t params, uint32_t idx, int8_t *success);
float            adm_extract_real32(tdc_t params, uint32_t idx, int8_t *success);
double           adm_extract_real64(tdc_t params, uint32_t idx, int8_t *success);
uvast            adm_extract_sdnv(tdc_t params, uint32_t idx, int8_t *success);
char*            adm_extract_string(tdc_t params, uint32_t idx, int8_t *success);
uint32_t         adm_extract_uint(tdc_t params, uint32_t idx, int8_t *success);
uvast            adm_extract_uvast(tdc_t params, uint32_t idx, int8_t *success);
vast             adm_extract_vast(tdc_t params, uint32_t idx, int8_t *success);

var_t* adm_find_computeddef(mid_t *mid);

adm_datadef_t* adm_find_datadef(mid_t *mid);
adm_datadef_t* adm_find_datadef_by_idx(int idx);

adm_ctrl_t*    adm_find_ctrl(mid_t *mid);
adm_ctrl_t*    adm_find_ctrl_by_idx(int idx);

lit_t*	       adm_find_lit(mid_t *mid);
adm_op_t*	   adm_find_op(mid_t *mid);

void         adm_init();

char*        adm_print_string(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char*        adm_print_string_list(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char*        adm_print_unsigned_long(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char*        adm_print_unsigned_long_list(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char*        adm_print_uvast(uint8_t* NULLbuffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);

uint32_t     adm_size_string(uint8_t* buffer, uint64_t buffer_len);
uint32_t     adm_size_string_list(uint8_t* buffer, uint64_t buffer_len);
uint32_t     adm_size_unsigned_long(uint8_t* buffer, uint64_t buffer_len);
uint32_t     adm_size_unsigned_long_list(uint8_t* buffer, uint64_t buffer_len);
uint32_t     adm_size_uvast(uint8_t* buffer, uint64_t buffer_len);

#endif /* ADM_H_*/
