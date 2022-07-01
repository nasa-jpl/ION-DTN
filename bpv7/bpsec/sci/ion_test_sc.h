/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2021 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: ion_test_sc.h
 **
 ** Namespace:
 **
 ** bpsec_itsci_   - Functions that implement the SC interface
 ** bpsec_itscu_   - Utility functions specific to this SC
 ** bpsec_itscbib_ - Functions used for BIB processing.
 ** bpsec_itscbcb_ - Functions used for BCB processing
 **
 ** Description:
 **
 **     This file implements a testing-only security context suitable for basic
 **     testing of ION security block packaging.
 **
 ** Notes:
 **
 **     1. This context is not standardized, and uses the identifier -1.
 **        SC Ids < 0 are reserved by RFC9172 for local/site-specific uses.
 **
 ** **     2. The ION Open Source distribution ships with an ION Testing Security
 **        Context (ITSC). The ITSC does not implement any standardized security
 **        context and it used only for local testing of security block amongst
 **        ION agents supporting the ITSC.
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  08/07/21  E. Birrane     Initial implementation
 **
 *****************************************************************************/

#ifndef _NSC_H_

#define _NSC_H_


#include "sci.h"
#include "sc_util.h"
#include "bpsec_util.h"
#include "sci_valmap.h"

//TODO: Clean up this file - put functions in consistent naming format.

#define BPSEC_ITSC_SC_NAME "ION Test Contexts"
#define BPSEC_ITSC_SC_ID 0

#define BPSEC_ITSC_BIB_SUITE CSTYPE_HMAC_SHA256
#define BPSEC_ITSC_BCB_SUITE CSTYPE_AES256_GCM

#define BPSEC_ITSC_BCB_FILENAME "bcb_tmpfile"
#define BPSEC_ITSC_MAX_TEMP_FILES_PER_SECOND   5

#define BPSEC_ITSC_XMIT_RATE 125000

#define BPSEC_ITSC_MIN_FILE_BUFFER (BPSEC_ITSC_XMIT_RATE / BPSEC_ITSC_MAX_TEMP_FILES_PER_SECOND)

typedef sc_value_map* (*bpsec_sc_valMapGet)();

/*****************************************************************************
 *                               BCB Functions                               *
 *****************************************************************************/

int      bpsec_itscbcb_decrypt(sc_state *state, AcqWorkArea *wk, BpsecInboundASB *asb, BpsecInboundTargetResult *tgtResult);
int      bpsec_itscbcb_parmsGet(sc_state *state, csi_cipherparms_t *parms, csi_val_t *sessionKey, csi_val_t *encryptedSessionKey);
int      bpsec_itscbcb_compute(Object *dataObj, uint32_t chunkSize, uint32_t suite, csi_val_t sessionKey, csi_cipherparms_t *parms, uint8_t function);
int      bpsec_itscbcb_encrypt(sc_state *state, Lyst extraParms, Bundle *bundle, BpsecOutboundASB *asb, BpsecOutboundTargetResult *tgtResult);


/*****************************************************************************
 *                               BIB Functions                               *
 *****************************************************************************/

uint8_t* bpsec_itscbib_compute(Object dataObj, sc_value *key_value, csi_svcid_t svc);
int      bpsec_itscbib_sign(sc_state *state, Lyst extraParms, Bundle *bundle, BpsecOutboundASB *asb, BpsecOutboundTargetResult *tgtResult);
int      bpsec_itscbib_verify(sc_state *state, AcqWorkArea *wk, BpsecInboundASB *asb, BpsecInboundTargetResult *tgtResult);



/*****************************************************************************
 *                           SC Interface Functions                          *
 *****************************************************************************/

int           bpsec_itsci_initAsbFn(void *def, Bundle *bundle, BpsecOutboundASB *asb, Sdr sdr, PsmPartition wm, PsmAddress parms);
int           bpsec_itsci_procInBlk(sc_state *state, AcqWorkArea *wk, BpsecInboundASB *asb, LystElt tgtBlkElt, BpsecInboundTargetResult *tgtResult);
int           bpsec_itsci_procOutBlk(sc_state *state, Lyst extraParms, Bundle *bundle, BpsecOutboundASB *asb, BpsecOutboundTargetResult *tgtResult);
sc_value_map* bpsec_itsci_valMapGet();







int32_t bpsec_itsc_updatePayloadFromFile(uint32_t suite, csi_cipherparms_t *csi_parms,
        uint8_t *context, csi_blocksize_t *blocksize, Object dataObj,
        ZcoReader *dataReader, uvast cipherBufLen, Object *cipherZco,
        uint8_t function);



#endif
