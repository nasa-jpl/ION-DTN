/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2022 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/


/*****************************************************************************
 **
 ** File Name: rfc9173_utils.h
 **
 ** Namespace:
 **    bpsec_rfc9173utl_  Utility functions
 **
 ** Description:
 **
 **     This file implements common utilities/functions used by the security
 **     contexts standardized by RFC9173.
 **
 ** Notes:
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  03/07/22  E. Birrane     Initial implementation
 *****************************************************************************/

#ifndef _RFC9173_UTIL_H_
#define _RFC9173_UTIL_H_


#include "sci.h"
#include "sc_util.h"
#include "bpsec_util.h"
#include "sci_valmap.h"


/*
 * RFC9173 defines the BIB-HMAC-SHA2 IPPT scope flags and the
 * BCB-AES-GCM AAD flags to have the same values and same
 * construction behavior:
 *
 * https://www.rfc-editor.org/rfc/rfc9173.html#name-integrity-scope-flags
 * https://www.rfc-editor.org/rfc/rfc9173.html#name-aad-scope-flags
 *
 * So a single set of flag values is defined here and used when
 * calculating the IPPT and AAD values.
 */

#define BPSEC_RFC9173_UTIL_SCOPE_PRIMARY (0x1)
#define BPSEC_RFC9173_UTIL_SCOPE_TGT_HDR (0x2)
#define BPSEC_RFC9173_UTIL_SCOPE_SEC_HDR (0x4)

#define BPSEC_RFC9173_UTIL_SCOPE_DEFAULT (0x7)
#define BPSEC_RFC9173_UTIL_SCOPE_MASK    (0x7)


/* The maximum size of an encoded block header is calculated as
 * - The Block Type (0-255) encodes in up to 3 CBOR bytes.
 * - The block number (8 byte) encodes in up to 9 CBOR bytes
 * - The block processing flags (8 byte) encodes in up to 9 CBOR bytes
 */

#define BPSEC_RFC9173_UTIL_BLK_HDR_MAX_SIZE (3 + 9 + 9)


/*****************************************************************************
 *                           SC Interface Functions                          *
 *****************************************************************************/


/* Incoming Bundle Functions */
int      bpsec_rfc9173utl_inBlkHdrSerialize(AcqWorkArea *wk, uint8_t blkNbr, uint8_t *buffer);
uint8_t *bpsec_rfc9173utl_inExtBlkDataGet(AcqWorkArea *wk, uint8_t blkNbr, int *len);


/* Outgoing Bundle Functions */

int      bpsec_rfc9173utl_outBlkHdrSerialize(Bundle *bundle, uint8_t blkNbr, uint8_t *buffer);
uint8_t *bpsec_rfc9173utl_outExtBlkDataGet(Bundle *bundle, uint8_t blkNbr, int *len);
int      bpsec_rfc9173utl_sesKeyGet(sc_state *state, int kek_id, int wrap_id, int csi_suite, csi_val_t *sesKey, sc_value *encSesKey);




/* Generic Utilities */
BpsecSerializeData bpsec_rfc9173utl_authDataBuild(sc_state *state, int parm_id, int tgtBlk, int addData, Bundle *bundle, AcqWorkArea *wk);
uint16_t           bpsec_rfc9173utl_intParmGet(sc_state *state, int id, uint16_t defVal);



#endif
