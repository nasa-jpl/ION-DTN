/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2021 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: sc_util.h
 **
 ** Namespace: bpsec_scutl_
 **
 ** Description:
 **
 **     This file contains a series of generic utility functions that can
 **     be used by any security context implementation within ION for
 **     security processing.
 **
 **     These utilities focus on the creation and handling of generic data
 **     structures comprising the SCI interface.
 **
 ** Assumptions:
 **  - Assumed that state lives as long as its security context pointer. :)
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  08/05/21  E. Birrane     Initial implementation
 **
 *****************************************************************************/

#ifndef _SC_UTIL_H_
#define _SC_UTIL_H_


#include "platform.h"
#include "ion.h"
#include "lyst.h"
#include "csi.h"
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

int   bpsec_scutl_hexNibbleGet(char c);
char* bpsec_scutl_hexStrCvt(uint8_t *data, int len);


int   bpsec_scutl_keyGet(sc_state *state, int key_id, sc_value *key_value);
int   bpsec_scutl_keyUnwrap(sc_state *state, int kek_id, csi_val_t *key_value, int wrappedId, int suite, csi_cipherparms_t *csi_parms);

void  bpsec_scutl_parmsExtract(sc_state *state, csi_cipherparms_t *parms);

char* bpsec_scutl_strFromStrsCreate(char **array, int size, int num_items);




#endif /* _SC_UTIL_H_ */
