/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2012 The Johns Hopkins University Applied Physics Laboratory
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
 *****************************************************************************/
#ifndef NM_TYPES_H
#define NM_TYPES_H

#include <stdint.h>
#include <string.h>
#include "platform.h"

#include "debug.h"

typedef enum
{
  AMP_TYPE_EDD    = 0, /* Externally Defined Data              */
  AMP_TYPE_VAR    = 1, /* Modeled as a cd_t.                   */
  AMP_TYPE_RPT    = 2,  /* Modeled as a def_gen_t.              */
  AMP_TYPE_CTRL   = 3,  /* Modeled as a ctrl_t.                 */
  AMP_TYPE_SRL    = 4,  /* Modeled as a srl_t.                  */
  AMP_TYPE_TRL    = 5,  /* Modeled as a trl_t.                  */
  AMP_TYPE_MACRO  = 6,  /* Modeled as a Lyst of (mid_t*).       */
  AMP_TYPE_LIT    = 7,  /* Modeled as a lit_t.                  */
  AMP_TYPE_OP     = 8,  /* Modeled as a op_t.                   */
  AMP_TYPE_BYTE   = 9,  /* Modeled as uint8_t                   */
  AMP_TYPE_INT    = 10, /* Modeled as int32_t                   */
  AMP_TYPE_UINT   = 11, /* Modeled as uint32_t                  */
  AMP_TYPE_VAST   = 12, /* Modeled as vast                      */
  AMP_TYPE_UVAST  = 13, /* Modeled as uvast.                    */
  AMP_TYPE_REAL32 = 14, /* Modeled as 32-bit floating point.    */
  AMP_TYPE_REAL64 = 15, /* Modeled as 64 bit floating point.    */
  AMP_TYPE_SDNV   = 16, /* Modeled as Sdnv                      */
  AMP_TYPE_TS     = 17, /* Modeled as uint32_t                  */
  AMP_TYPE_STRING = 18, /* Modeled as (char *)                  */
  AMP_TYPE_BLOB   = 19, /* Modeled as (blob_t *)                */
  AMP_TYPE_MID    = 20, /* Modeled as (mid_t)                   */
  AMP_TYPE_MC     = 21, /* Modeled as Lyst of (mid_t *)         */
  AMP_TYPE_EXPR   = 22, /* Modeled as expr_t                    */
  AMP_TYPE_DC     = 23, /* Modeled as Lyst of (blob_t *)        */
  AMP_TYPE_TDC    = 24, /* Modeled as a tdc_t.                  */
  AMP_TYPE_TABLE  = 25, /* Modeled as a table_t.                */
  AMP_TYPE_UNK    = 26, /* Unknown.                             */
} amp_type_e;


extern const char * const amp_type_str[];
extern const char * const amp_type_fieldspec_str[];


#define AMP_MAX_EID_LEN (64)

#define UHF UVAST_HEX_FIELDSPEC

/* The DTNMP relative time cut-off, set as the first second of
 * September 9th, 2012.
 */
#define AMP_RELATIVE_TIME_EPOCH (1347148800)

typedef struct
{
    char name[AMP_MAX_EID_LEN];
} eid_t;


amp_type_e type_from_str(char *str);
amp_type_e type_from_uint(uint32_t type);

const char*  type_get_fieldspec(amp_type_e type);
size_t       type_get_size(amp_type_e type);
uint8_t      type_is_numeric(amp_type_e type);
const char*  type_to_str(amp_type_e type);


#endif /* NM_TYPES_H */
