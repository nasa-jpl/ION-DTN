/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2011 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/


/*****************************************************************************
 **
 ** \file nm_types.h
 **
 ** Description: AMP Structures and Data Types
 **
 ** Notes:
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  08/31/11  V. Ramachandran Initial Implementation. (JHU/APL)
 **  06/30/16  E. Birrane      Update to AMP v0.3 (Secure DTN - NASA: NNX14CS58P)
 **  07/31/16  E. Birrane      Rename DTNMP items to AMP_(Secure DTN - NASA: NNX14CS58P)
 **  09/02/18  E. Birrane      Cleanup and update to latest spec. (JHU/APL)
 *****************************************************************************/
#ifndef NM_TYPES_H
#define NM_TYPES_H

#include <stdint.h>
#include <string.h>
#include "platform.h"

#include "debug.h"


/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */


typedef enum
{

  /* AMM Objects */
  AMP_TYPE_CNST    =  0, /* Hex: 0x00. Struct: ari_t    */
  AMP_TYPE_CTRL    =  1, /* Hex: 0x01. Struct: ctrl_t   */
  AMP_TYPE_EDD     =  2, /* Hex: 0x02. Struct: edd_t    */
  AMP_TYPE_LIT     =  3, /* Hex: 0x03. Struct: ari_t    */
  AMP_TYPE_MAC     =  4, /* Hex: 0x04. Struct: mac_t    */
  AMP_TYPE_OPER    =  5, /* Hex: 0x05. Struct: op_t     */
  AMP_TYPE_RPT     =  6, /* Hex: 0x06. Struct: rpt_t    */
  AMP_TYPE_RPTTPL  =  7, /* Hex: 0x07. Struct: rpttpl_t */
  AMP_TYPE_SBR     =  8, /* Hex: 0x08. Struct: sbr_t    */
  AMP_TYPE_TBL     =  9, /* Hex: 0x09. Struct: tbl_t    */
  AMP_TYPE_TBLT    = 10, /* Hex: 0x0A. Struct: tblt_t   */
  AMP_TYPE_TBR     = 11, /* Hex: 0x0B. Struct: tbr_t    */
  AMP_TYPE_VAR     = 12, /* Hex: 0x0C. Struct: var_t    */

  /* Primitive Types */
  AMP_TYPE_BOOL    = 16, /* Hex: 0x10.  Type: uint8_t   */
  AMP_TYPE_BYTE    = 17, /* Hex: 0x11.  Type: uint8_t   */
  AMP_TYPE_STR     = 18, /* Hex: 0x12.  Type: char      */
  AMP_TYPE_INT     = 19, /* Hex: 0x13.  Type: int32_t   */
  AMP_TYPE_UINT    = 20, /* Hex: 0x14.  Type: uint32_t  */
  AMP_TYPE_VAST    = 21, /* Hex: 0x15.  Type: vast_t    */
  AMP_TYPE_UVAST   = 22, /* Hex: 0x16.  Type: uvast_t   */
  AMP_TYPE_REAL32  = 23, /* Hex: 0x17.  Type: float     */
  AMP_TYPE_REAL64  = 24, /* Hex: 0x18.  Type: double    */

  /* Compound Objects */
  AMP_TYPE_TV      = 32, /* Hex: 0x20.   Type: uvast_t  */
  AMP_TYPE_TS      = 33, /* Hex: 0x21.   Type: uvast_t  */
  AMP_TYPE_TNV     = 34, /* Hex: 0x22. Struct: tnv_t    */
  AMP_TYPE_TNVC    = 35, /* Hex: 0x23. Struct: tnvc_t   */
  AMP_TYPE_ARI     = 36, /* Hex: 0x24. Struct: ari_t    */
  AMP_TYPE_AC      = 37, /* Hex: 0x25. Struct: ac_t     */
  AMP_TYPE_EXPR    = 38, /* Hex: 0x26. Struct: expr_t   */
  AMP_TYPE_BYTESTR = 39, /* Hex: 0x27. Struct: blob_t   */

  AMP_TYPE_UNK     = 40
} amp_type_e;




#define AMP_MAX_EID_LEN (16)


/* The AMP relative time cut-off, set as the first second of
 * September 9th, 2012.
 */
#define AMP_RELATIVE_TIME_EPOCH (1347148800)

/*
 * +--------------------------------------------------------------------------+
 * |							  	MACROS  								  +
 * +--------------------------------------------------------------------------+
 */


/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */

extern const char * const amp_type_str[];


/**
 * AMP represents an EID as simply a string name.
 */
typedef struct
{
    char name[AMP_MAX_EID_LEN];
} eid_t;


/*
 * +--------------------------------------------------------------------------+
 * |						  FUNCTION PROTOTYPES  							  +
 * +--------------------------------------------------------------------------+
 */

amp_type_e   type_from_str(char *str);
amp_type_e   type_from_uint(uint32_t type);

uint8_t      type_is_numeric(amp_type_e type);
uint8_t      type_is_known(amp_type_e type);

const char*  type_to_str(amp_type_e type);


#endif /* NM_TYPES_H */
