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


#define DTNMP_TYPE_BYTE   0
#define DTNMP_TYPE_INT    1
#define DTNMP_TYPE_UINT   2
#define DTNMP_TYPE_VAST   3
#define DTNMP_TYPE_UVAST  4
#define DTNMP_TYPE_FLOAT  5
#define DTNMP_TYPE_DOUBLE 6
#define DTNMP_TYPE_STRING 7
#define DTNMP_TYPE_BLOB   8
#define DTNMP_TYPE_SDNV   9
#define DTNMP_TYPE_TS    10
#define DTNMP_TYPE_DC    11
#define DTNMP_TYPE_MID   12
#define DTNMP_TYPE_MC    13
#define DTNMP_TYPE_EXPR  14

#define MAX_EID_LEN (256)

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

#endif /* NM_TYPES_H */
