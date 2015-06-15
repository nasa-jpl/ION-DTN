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
 ** File Name: utils.h
 **
 ** Subsystem:
 **          Network Management Utilities
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information necessary for the identification and
 **              processing of DTNMP Managed Identifiers (MIDs).
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/29/12  E. Birrane     Initial Implementation
 *****************************************************************************/

#ifndef UTILS_H_
#define UTILS_H_

#include "stdint.h"

#include "bp.h"
#include "lyst.h"


/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

#ifndef MIN
#define MIN(x,y) (x < y ? x : y)
#endif



/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |						  DATA DEFINITIONS  							  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

//unsigned long utils_atox(char *s);
unsigned long utils_atox(char *s, int *success);

int8_t   utils_grab_byte(unsigned char *cursor, uint32_t size, uint8_t *result);
uint32_t utils_grab_sdnv(unsigned char *cursor, uint32_t size, uvast *result);
char*    utils_hex_to_string(uint8_t *buffer, uint32_t size);
void     utils_print_hex(unsigned char *s, uint32_t len);
uint8_t* utils_string_to_hex(char *value, uint32_t *size);
int      utils_time_delta(struct timeval *result, struct timeval *t1, struct timeval *t2);
vast     utils_time_cur_delta(struct timeval *t1);

#endif /* UTILS_H_ */
