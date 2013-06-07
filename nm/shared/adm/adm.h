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


/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

#define ADM_MID_ALLOC (15)
#define ADM_MAX_NAME  (64)
#define MAX_NUM_ADUS (256)
#define MAX_NUM_CTRLS  (8)


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


/**
 * Functions implementing key responsibilities for each ADM item:
 * - The data collect function collects the ADM data from the local agent.
 * - The string function generates a String representation of the data value.
 * - The size function returns the size of the data, in bytes.
 */
typedef uint8_t* (*adm_data_collect_fn)(Lyst parms, uint64_t *length);
typedef char* (*adm_string_fn)(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
typedef uint32_t (*adm_size_fn)(uint8_t* buffer, uint64_t buffer_len);

typedef uint32_t (*adm_ctrl_fn)(Lyst params);

/**
 * Describes an ADM entry in the system.
 *
 * This structure capture general information for those ADM entries pre-
 * configured on the local machine (such as atomic data elements and controls).
 */
typedef struct
{
    char name[ADM_MAX_NAME];     /**> Name of this MIB item */
    
    uint8_t mid[ADM_MID_ALLOC];  /**> The MID up to, but excluding, params */
    uint8_t mid_len;             /**> Length, in bytes, of the MID. */
    uint8_t num_parms;           /**> # params needed to complete this MID. */

    adm_data_collect_fn collect; /**> Configured data collection function. */
    adm_string_fn to_string;     /**> Configured to-string function. */
    adm_size_fn get_size;        /**> Configured sizing function. */
} adm_entry_t;


typedef struct
{
	char name[ADM_MAX_NAME];
    uint8_t mid[ADM_MID_ALLOC];  /**> The MID up to, but excluding, params */
    uint8_t mid_len;             /**> Length, in bytes, of the MID. */
    uint8_t num_parms;           /**> # params needed to complete this MID. */
    adm_ctrl_fn run;         /**> Configured data collection function. */

} adm_ctrl_t;



/*
 * +--------------------------------------------------------------------------+
 * |						  DATA DEFINITIONS  							  +
 * +--------------------------------------------------------------------------+
 */

/**
 * Global data collection of ADMs.
 * \todo: Consider making this a lyst or other, better structure.
 */
extern int gNumAduData;
extern adm_entry_t adus[MAX_NUM_ADUS];

extern int gNumAduCtrls;
extern adm_ctrl_t ctrls[MAX_NUM_CTRLS];

extern int gNumAduLiterals;
extern int gNumAduOperators;



/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

void         adm_add(char *name, char *mid_str, int num_parms, adm_data_collect_fn collect, adm_string_fn to_string, adm_size_fn get_size);
void         adm_add_ctrl(char *name, char *mid_str, int num_parms, adm_ctrl_fn collect);

uint8_t*     adm_copy_integer(uint8_t *value, uint8_t size, uint64_t *length);

adm_entry_t* adm_find(mid_t *mid);
adm_ctrl_t*  adm_find_ctrl(mid_t *mid);


void         adm_init();

char*        adm_print_string(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char*        adm_print_string_list(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char*        adm_print_unsigned_long(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);
char*        adm_print_unsigned_long_list(uint8_t* buffer, uint64_t buffer_len, uint64_t data_len, uint32_t *str_len);

uint32_t     adm_size_string(uint8_t* buffer, uint64_t buffer_len);
uint32_t     adm_size_string_list(uint8_t* buffer, uint64_t buffer_len);
uint32_t     adm_size_unsigned_long(uint8_t* buffer, uint64_t buffer_len);
uint32_t     adm_size_unsigned_long_list(uint8_t* buffer, uint64_t buffer_len);

#endif /* ADM_H_*/
