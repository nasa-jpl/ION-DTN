/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: utils.h
 **
 ** Description: This file contains the definitions, prototypes, constants, and
 **              other information for general utilities in the AMP system.
 **
 ** Notes:
 **
 ** Assumptions:
 **
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  10/29/12  E. Birrane     Initial Implementation (JHU/APL)
 **  ??/??/16  E. Birrane     Added Serialize/Deserialize Functions.
 **                           Added "safe" memory functions.
 **                           Document Updates (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/

#ifndef UTILS_H_
#define UTILS_H_

#include "stdint.h"
#include "platform.h"
#include "bp.h"
#include "lyst.h"


/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

// Fatal system error. Typically resource exhausted or underlying system error
#define AMP_SYSERR (-1)
// Non-fatal failure to do something.
#define AMP_FAIL (0)
// Successfully did something.
#define AMP_OK (1)


#ifndef MIN
#define MIN(x,y) (x < y ? x : y)
#endif

#define SUCCESS 1
#define FAILURE 0


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

#define STAKE(size) utils_safe_take(size)
#define SRELEASE(ptr) utils_safe_release(ptr)


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */


int8_t utils_mem_int();
void   utils_mem_teardown();
void   utils_safe_release(void* ptr);
void*  utils_safe_take(size_t size);

//unsigned long utils_atox(char *s);
unsigned long utils_atox(char *s, int *success);


int8_t   utils_grab_byte(unsigned char *cursor, uint32_t size, uint8_t *result);
uint32_t utils_grab_sdnv(unsigned char *cursor, uint32_t size, uvast *result);
char*    utils_hex_to_string(uint8_t *buffer, uint32_t size);
void     utils_print_hex(unsigned char *s, uint32_t len);

uint8_t* utils_string_to_hex(char *value, uint32_t *size);
int      utils_time_delta(struct timeval *result, struct timeval *t1, struct timeval *t2);
vast     utils_time_cur_delta(struct timeval *t1);

int32_t  utils_deserialize_int(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used);
float    utils_deserialize_real32(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used);
double   utils_deserialize_real64(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used);
char*    utils_deserialize_string(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used);
uint32_t utils_deserialize_uint(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used);
uvast    utils_deserialize_uvast(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used);
vast     utils_deserialize_vast(uint8_t *buffer, uint32_t bytes_left, uint32_t *bytes_used);

uint8_t *utils_serialize_byte(uint8_t value, uint32_t *size);
uint8_t *utils_serialize_int(int32_t value, uint32_t *size);
uint8_t *utils_serialize_real32(float value, uint32_t *size);
uint8_t *utils_serialize_real64(double value, uint32_t *size);
uint8_t *utils_serialize_string(char *value, uint32_t *size);
uint8_t *utils_serialize_uint(uint32_t value, uint32_t *size);
uint8_t *utils_serialize_uvast(uvast value, uint32_t *size);
uint8_t *utils_serialize_vast(vast value, uint32_t *size);

#endif /* UTILS_H_ */
