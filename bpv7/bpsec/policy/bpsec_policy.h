/******************************************************************************
 **                           COPYRIGHT NOTICE
 **      (c) 2021 The Johns Hopkins University Applied Physics Laboratory
 **                         All rights reserved.
 ******************************************************************************/

/*****************************************************************************
 **
 ** File Name: bpsec_policy.h
 **
 ** Description: This file contains general utilities for initializing the
 **              BPSec policy engine and applying policy actions to blocks.
 **
 ** Notes:
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **  01/22/21  S. Heiner &    Initial implementation
 **            E. Birrane
 **
 *****************************************************************************/

#ifndef _BPSEC_POLICY_H_
#define _BPSEC_POLICY_H_

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "bpsec_asb.h"
//#include "bpsec_util.h"
#include "radix.h"
#include "smrbt.h"
#include "csi.h"

/*****************************************************************************
 *                                CONSTANTS                                  *
 *****************************************************************************/

/*  Values for Sender and Receiver BPAs. A Sender BPA may assume a policy role
 *  of security source, and a Receiver BPA may assume a policy role of
 *  security verifier or security acceptor.  */
#define BSL_SENDER							(0x1)   /*  0001  */
#define BSL_RECEIVER                        (0x2)   /*  0010  */

/*  Masks for Security Operation Event */
#define BSL_SRC_FOR_SOP						(0x0001) /* 0000 0000 0000 0001 */
#define BSL_SOP_ADDED_AT_SRC				(0x0002) /* 0000 0000 0000 0010 */
#define BSL_SOP_MISCONF_AT_SRC				(0x0004) /* 0000 0000 0000 0100 */
#define BSL_VERIFIER_FOR_SOP 				(0x0008) /* 0000 0000 0000 1000 */
#define BSL_SOP_MISCONF_AT_VERIFIER			(0x0010) /* 0000 0000 0001 0000 */
#define BSL_SOP_MISSING_AT_VERIFIER			(0x0020) /* 0000 0000 0010 0000 */
#define BSL_SOP_CORRUPT_AT_VERIFIER			(0x0040) /* 0000 0000 0100 0000 */
#define BSL_SOP_VERIFIED                    (0x0080) /* 0000 0000 1000 0000 */
#define BSL_ACCEPTOR_FOR_SOP				(0x0100) /* 0000 0001 0000 0000 */
#define BSL_SOP_MISCONF_AT_ACCEPTOR 		(0x0200) /* 0000 0010 0000 0000 */
#define BSL_SOP_MISSING_AT_ACCEPTOR			(0x0400) /* 0000 0100 0000 0000 */
#define BSL_SOP_CORRUPT_AT_ACCEPTOR			(0x0800) /* 0000 1000 0000 0000 */
#define BSL_SOP_PROCESSED					(0x1000) /* 0001 0000 0000 0000 */

typedef enum
{
	unsupported = 0,
	src_for_sop = BSL_SRC_FOR_SOP,
	sop_added_at_src = BSL_SOP_ADDED_AT_SRC,
	sop_misconf_at_src = BSL_SOP_MISCONF_AT_SRC,
	verifier_for_sop = BSL_VERIFIER_FOR_SOP,
	sop_misconf_at_verifier = BSL_SOP_MISCONF_AT_VERIFIER,
	sop_missing_at_verifier = BSL_SOP_MISSING_AT_VERIFIER,
	sop_corrupt_at_verifier = BSL_SOP_CORRUPT_AT_VERIFIER,
	sop_verified = BSL_SOP_VERIFIED,
	acceptor_for_sop = BSL_ACCEPTOR_FOR_SOP,
	sop_misconf_at_acceptor = BSL_SOP_MISCONF_AT_ACCEPTOR,
	sop_missing_at_acceptor = BSL_SOP_MISSING_AT_ACCEPTOR,
	sop_corrupt_at_acceptor = BSL_SOP_CORRUPT_AT_ACCEPTOR,
	sop_processed = BSL_SOP_PROCESSED
} BpSecEventId;


/*****************************************************************************
 *                             BPSEC POLICY DATA                             *
 *****************************************************************************/

/*
 * +--------------------------------------------------------------------------+
 * |							  DATA TYPES  								  +
 * +--------------------------------------------------------------------------+
 */

typedef struct
{
	Object   entryObj; /* The type of object. */
	uint16_t size;  /* The size of the object. */
} BpSecPolicyDbEntry;




/*****************************************************************************
 *                            FUNCTION PROTOTYPES                            *
 *****************************************************************************/

int        bsl_all_init(PsmPartition partition);
Address    bsl_bufread(void *value, char *cursor, int length, int *bytes_left);
Address    bsl_bufwrite(char *cursor, void *value, int length, int *bytes_left);
PsmAddress bsl_ed_get_ref(PsmPartition partition, char *eid);
int        bsl_sdr_bootstrap(PsmPartition wm);
int        bsl_sdr_insert(Sdr ionsdr, char *buffer, BpSecPolicyDbEntry entry, Object list);
int        bsl_vdb_init(PsmPartition partition);
void       bsl_vdb_teardown(PsmPartition partition);

/* Callbacks */
void       bsl_cb_ed_delete(PsmPartition partition, PsmAddress user_data);

/*
 * +--------------------------------------------------------------------------+
 * |	      	     SECURITY OPERATION EVENT HANDLING  	     			  +
 * +--------------------------------------------------------------------------+
 */
int        bsl_handle_sender_sop_event(Bundle *bundle, BpSecEventId sopEvent,
		     ExtensionBlock *sop, BpsecOutboundASB *asb, unsigned char tgtNum);
int        bsl_handle_receiver_sop_event(AcqWorkArea *wk, int role,
		     BpSecEventId sopEvent, LystElt sop, LystElt tgt, unsigned char tgtNum);

/*
 * +--------------------------------------------------------------------------+
 * |	      	     OPTIONAL PROCESSING ACTION CALLBACKS  	     			  +
 * +--------------------------------------------------------------------------+
 */

/* Sender Optional Processing Action Callbacks */
void       bsl_remove_sop_at_sender(Bundle *bundle, ExtensionBlock *sopBlk);
void       bsl_remove_sop_target_at_sender(Bundle *bundle, ExtensionBlock *sopBlk,
		     BpsecOutboundASB *asb, unsigned char tgtNum);
void       bsl_remove_all_target_sops_at_sender(Bundle *bundle, unsigned char tgtNum);
void       bsl_do_not_forward_at_sender(Bundle *bundle);
void       bsl_report_reason_code_at_sender(Bundle *bundle, BpSrReason reason);

/* Receiver Optional Processing Action Callbacks */
void       bsl_remove_sop_at_receiver(AcqWorkArea *wk, LystElt sopElt);
void       bsl_remove_sop_target_at_receiver(LystElt tgtElt, LystElt sopElt);
void       bsl_remove_all_target_sops_at_receiver(AcqWorkArea *wk, unsigned char tgtBlkNum);
void       bsl_do_not_forward_at_receiver(AcqWorkArea *wk);
void       bsl_report_reason_code_at_receiver(AcqWorkArea *wk, BpSrReason reason);


#endif /*_BPSEC_POLICY_H_*/
