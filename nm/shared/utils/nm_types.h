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
