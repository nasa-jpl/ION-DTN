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
 **  06/30/16  E. Birrane     Update to AMP v0.3 (Secure DTN - NASA: NNX14CS58P)
 *****************************************************************************/
#ifndef NM_TYPES_H
#define NM_TYPES_H

#include <stdint.h>
#include <string.h>
#include "platform.h"

#include "debug.h"

typedef enum
{
  DTNMP_TYPE_AD     = 0,  /* Atomic Data */
  DTNMP_TYPE_CD     = 1,  /* Modeled as a cd_t.                   */
  DTNMP_TYPE_RPT    = 2,  /* Modeled as a def_gen_t.              */
  DTNMP_TYPE_CTRL   = 3,  /* Modeled as a ctrl_t.                 */
  DTNMP_TYPE_SRL    = 4,  /* Modeled as a srl_t.                  */
  DTNMP_TYPE_TRL    = 5,  /* Modeled as a trl_t.                  */
  DTNMP_TYPE_MACRO  = 6,  /* Modeled as a Lyst of (mid_t*).       */
  DTNMP_TYPE_LIT    = 7,  /* Modeled as a lit_t.                  */
  DTNMP_TYPE_OP     = 8,  /* Modeled as a op_t.                   */
  DTNMP_TYPE_BYTE   = 9,  /* Modeled as uint8_t                   */
  DTNMP_TYPE_INT    = 10, /* Modeled as int32_t                   */
  DTNMP_TYPE_UINT   = 11, /* Modeled as uint32_t                  */
  DTNMP_TYPE_VAST   = 12, /* Modeled as vast                      */
  DTNMP_TYPE_UVAST  = 13, /* Modeled as uvast.                    */
  DTNMP_TYPE_REAL32 = 14, /* Modeled as 32-bit floating point.    */
  DTNMP_TYPE_REAL64 = 15, /* Modeled as 64 bit floating point.    */
  DTNMP_TYPE_SDNV   = 16, /* Modeled as Sdnv                      */
  DTNMP_TYPE_TS     = 17, /* Modeled as uint32_t                  */
  DTNMP_TYPE_STRING = 18, /* Modeled as (char *)                  */
  DTNMP_TYPE_BLOB   = 19, /* Modeled as (blob_t *)                */
  DTNMP_TYPE_MID    = 20, /* Modeled as (mid_t)                   */
  DTNMP_TYPE_MC     = 21, /* Modeled as Lyst of (mid_t *)         */
  DTNMP_TYPE_EXPR   = 22, /* Modeled as expr_t                    */
  DTNMP_TYPE_DC     = 23, /* Modeled as Lyst of (blob_t *)        */
  DTNMP_TYPE_TDC    = 24, /* Modeled as a tdc_t.                  */
  DTNMP_TYPE_TABLE  = 25, /* Modeled as a table_t.                */
  DTNMP_TYPE_UNK    = 26, /* Unknown.                             */
} dtnmp_type_e;


extern const char * const dtnmp_type_str[];
extern const char * const dtnmp_type_fieldspec_str[];


#define AMP_MAX_EID_LEN (64)

#define UHF UVAST_HEX_FIELDSPEC

/* The DTNMP relative time cut-off, set as the first second of
 * September 9th, 2012.
 */
#define DTNMP_RELATIVE_TIME_EPOCH (1347148800)

typedef struct
{
    char name[AMP_MAX_EID_LEN];
} eid_t;


dtnmp_type_e type_from_str(char *str);
const char*  type_get_fieldspec(dtnmp_type_e type);
size_t       type_get_size(dtnmp_type_e type);
uint8_t      type_is_numeric(dtnmp_type_e type);
const char*  type_to_str(dtnmp_type_e type);


#endif /* NM_TYPES_H */
