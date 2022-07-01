/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2021 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: sciP.h
 **
 ** Namespace: bpsec_scip_
 **
 ** Description:
 **
 **     This file defines the "private" structures used to define and access
 **     instances of security contexts within ION.
 **
 **     These private structures define an "interface" for a security context
 **     implementation, such that any new security context instance to be
 **     used with ION would need to implement these interfaces to be "available"
 **     to the BPSec policy engine for use in the BPA.
 **
 ** Notes:
 **
 **     1. This interface does not, itself, perform any cryptographic functions.
 **
 **     2. The ION Open Source distribution ships with a NULL implementation of a
 **        security context. This NULL implementation performs no cryptographic
 **        functions.
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  08/05/21  E. Birrane     Initial implementation
 **
 *****************************************************************************/

#ifndef _SCIP_H_
#define _SCIP_H_

#include "bpP.h"
#include "sci_structs.h"


/*****************************************************************************
 *                           CONSTANTS DEFINITIONS                           *
 *****************************************************************************/





/*****************************************************************************
 *                                Structures                                 *
 *****************************************************************************/


/*****************************************************************************
 *                            Function Prototypes                            *
 *****************************************************************************/

int   bpsec_scvm_byIdIdxFind(sc_value_map map[], int id, sc_val_type type);
char* bpsec_scvm_byIdNameFind(sc_value_map map[], int id, sc_val_type type);
int   bpsec_scvm_byNameIdxFind(sc_value_map map[], char *name);

int      bpsec_scvm_hexCborDecode(PsmPartition wm, sc_value *val, unsigned int len, uint8_t *buffer);
uint8_t* bpsec_scvm_hexCborEncode(PsmPartition wm, sc_value *val, unsigned int *len);
int      bpsec_scvm_hexStrDecode(PsmPartition wm, sc_value *val, unsigned int len, char *value);
char*    bpsec_scvm_hexStrEncode(PsmPartition wm, sc_value *val);

int      bpsec_scvm_intCborDecode(PsmPartition wm, sc_value *val, unsigned int len, uint8_t *buffer);
uint8_t* bpsec_scvm_intCborEncode(PsmPartition wm, sc_value *val, unsigned int *len);
int      bpsec_scvm_intStrDecode(PsmPartition wm, sc_value *val, unsigned int len, char *value);
char*    bpsec_scvm_intStrEncode(PsmPartition wm, sc_value *val);

int      bpsec_scvm_strCborDecode(PsmPartition wm, sc_value *val, unsigned int len, uint8_t *buffer);
uint8_t* bpsec_scvm_strCborEncode(PsmPartition wm, sc_value *val, unsigned int *len);
int      bpsec_scvm_strStrDecode(PsmPartition wm, sc_value *val, unsigned int len, char *value);
char*    bpsec_scvm_strStrEncode(PsmPartition wm, sc_value *val);



#endif
