//
//  nm_types.h
//  DTN Network Management Agent Structures and Data Types
//
//  Author: V. Ramachandran, 8/31/2011
//  Copyright 2011 Johns Hopkins University APL. All rights reserved.
//
#ifndef NM_TYPES_H
#define NM_TYPES_H

#include <stdint.h>
#include <string.h>
#include "platform.h"

#include "debug.h"
// \todo add a report type?
typedef enum
{
  DTNMP_TYPE_BYTE   = 0,  /* Modeled as uint8_t                   */
  DTNMP_TYPE_INT    = 1,  /* Modeled as int32_t                   */
  DTNMP_TYPE_UINT   = 2,  /* Modeled as uint32_t                  */
  DTNMP_TYPE_VAST   = 3,  /* Modeled as vast                      */
  DTNMP_TYPE_UVAST  = 4,  /* Modeled as uvast.                    */
  DTNMP_TYPE_REAL32 = 5,  /* Modeled as 32-bit floating point.    */
  DTNMP_TYPE_REAL64 = 6,  /* Modeled as 64 bit floating point.    */
  DTNMP_TYPE_STRING = 7,  /* Modeled as (char *)                  */
  DTNMP_TYPE_BLOB   = 8,  /* Modeled as (uint8_t *)               */
  DTNMP_TYPE_SDNV   = 9,  /* Modeled as Sdnv                      */
  DTNMP_TYPE_TS     = 10, /* Modeled as uint32_t                  */
  DTNMP_TYPE_DC     = 11, /* Modeled as Lyst of (datacol_entry *) */
  DTNMP_TYPE_MID    = 12, /* Modeled as (mid_t)                   */
  DTNMP_TYPE_MC     = 13, /* Modeled as Lyst of (mid_t *)         */
  DTNMP_TYPE_EXPR   = 14, /* Modeled as Lyst of (mid_t *)         */
  DTNMP_TYPE_DEF    = 15, /* Modeled as a def_gen_t               */
  DTNMP_TYPE_TRL    = 16, /* Modeled as a trl_t.                  */
  DTNMP_TYPE_SRL    = 17, /* Modeled as a srl_t.                  */
  DTNMP_TYPE_UNK    = 18, /* Unknown.                             */
} dtnmp_type_e;

extern const char * const dtnmp_type_str[];
extern const char * const dtnmp_type_fieldspec_str[];


#define MAX_EID_LEN (256)
#define SUCCESS 1
#define FAILURE 0

// The beginning of the J2000 epoch (01 Jan 2000, 11:58:55.816 UTC) in
// seconds of the Posix (1970) epoch.
#define BEGIN_J2000 (946727936)

/* The DTNMP relative time cut-off, set as the first second of
 * September 9th, 2012.
 */
#define DTNMP_RELATIVE_TIME_EPOCH (1348025776)

typedef struct
{
    char name[MAX_EID_LEN];
} eid_t;

typedef unsigned char* buffer_t;


dtnmp_type_e type_from_str(char *str);

const char *   type_get_fieldspec(dtnmp_type_e type);
size_t   type_get_size(dtnmp_type_e type);
const char *   type_to_str(dtnmp_type_e type);

#endif /* NM_TYPES_H */
