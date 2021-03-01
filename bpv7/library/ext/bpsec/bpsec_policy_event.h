/*****************************************************************************
 **
 ** File Name: bpsec_policy_event.h
 **
 ** Description:
 **
 ** Notes:
 **
 **
 ** Assumptions:
 **
 ** Modification History:
 **  MM/DD/YY  AUTHOR         DESCRIPTION
 **  --------  ------------   ---------------------------------------------
 **                           Initial implementation
 **
 *****************************************************************************/

#ifndef BPSEC_POLICY_EVENT_H_
#define BPSEC_POLICY_EVENT_H_

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "bpsec_policy.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

#define BSLACT_MAX_PARM (3)

/*	Optional Processing Actions	 */
#define BSLACT_REMOVE_SOP                     (0x01) /*	0000 0001 */
#define BSLACT_REMOVE_SOP_TARGET              (0x02) /*	0000 0010 */
#define BSLACT_REMOVE_ALL_TARGET_SOPS         (0x04) /*	0000 0100 */
#define BSLACT_DO_NOT_FORWARD                 (0x08) /* 0000 1000 */
#define BSLACT_REQUEST_STORAGE                (0x10) /*	0001 0000 */
#define BSLACT_REPORT_REASON_CODE             (0x20) /*	0010 0000 */
#define BSLACT_OVERRIDE_TARGET_BPCF           (0x40) /*	0100 0000 */
#define BSLACT_OVERRIDE_SOP_BPCF              (0x80) /*	1000 0000 */
#define BSLACT_NOT_IMPLEMENTED                (0xFF) /* 0000 0000 */

/*  Masks for optional processing actions at each SOP event */
#define BSLEVT_SRC_FOR_SOP_MASK               (0x00) /* 0000 0000 */
#define BSLEVT_SOP_ADDED_AT_SRC_MASK          (0x80) /*	1000 0000 */
#define BSLEVT_SOP_MISCONF_AT_SRC_MASK        (0x3F) /* 0011 1111 */
#define BSLEVT_VERIFIER_FOR_SOP_MASK          (0x00) /* 0000 0000 */
#define BSLEVT_SOP_MISCONF_AT_VERIFIER_MASK   (0xFB) /* 1111 1011 */
#define BSLEVT_SOP_MISSING_AT_VERIFIER_MASK   (0x7A) /* 0111 1010 */
#define BSLEVT_SOP_CORRUPT_AT_VERIFIER_MASK   (0xFF) /* 1111 1111 */
#define BSLEVT_SOP_VERIFIED_MASK              (0x00) /* 0000 0000 */
#define BSLEVT_ACCEPTOR_FOR_SOP_MASK          (0x00) /* 0000 0000 */
#define BSLEVT_SOP_MISCONF_AT_ACCEPTOR_MASK   (0xFA) /* 1111 1010 */
#define BSLEVT_SOP_MISSING_AT_ACCEPTOR_MASK   (0x7A) /* 0111 1010 */
#define BSLEVT_SOP_CORRUPT_AT_ACCEPTOR_MASK   (0xFE) /* 1111 1110 */
#define BSLEVT_SOP_PROCESSED_MASK             (0x00) /* 0000 0000 */

#define MAX_EVENT_LEN					(30)
#define MAX_ACTION_LEN					(25)


#define BSLEVT_IS_SET(mask, id) (mask & id)


/*****************************************************************************
 *                            Event Structures                               *
 *****************************************************************************/


/**
 * Parameter for the Report with Reason Code optional processing action.
 * reasonCode: Reason code to include in status report.
 */
/**
 * Parameters for the Override Block Processing Control Flags processing
 * actions. Can be used when overriding the security operation's BPCF or
 * security target's BPCF.
 * mask: BPCF values to maintain and use as-is.
 * val: BPCF values to override (force to 1 or 0).
 */

typedef union BpSecPolActionParms
{
  struct {
  	int reasonCode;		/**< Reason code parameter   			     */
  } asReason;			/**< Use with report_reason_code action      */

  struct {
  	uint64_t mask;		/**< BPCF values to use as-is 				 */
  	uint64_t val;		/**< BPCF values to override   				 */
  } asOverride;			/**< Use with override_bpcf actions    		 */

} BpSecEvtActionParms;

typedef struct
{
	uint8_t 			action_mask; 	/**< Configured optional actions */
	BpSecEvtActionParms	action_parms[BSLACT_MAX_PARM];
	BpSecEventId		id;
} BpSecEvent;

/*****************************************************************************
 *                            Function Prototypes                            *
 *****************************************************************************/

int          bslevt_add(PsmPartition wm, char *esName, BpSecEventId eventId,
						uint8_t actions, BpSecEvtActionParms *actionParms);
int			 bslevt_create(PsmPartition wm, BpSecEventId eventId, uint8_t actions,
						BpSecEvtActionParms *actionParms, PsmAddress *addr);
int          bslevt_delete(PsmPartition wm, char *esName, BpSecEventId eventId);
BpSecEventId bslevt_get_id(char *name);
char*        bslevt_get_name(BpSecEventId eventId);
Address      bslevt_sdr_persist(char *cursor, BpSecEvent *event, int *bytes_left);
Address      bslevt_sdr_restore(BpSecEvent *event, char *cursor, int *bytes_left);
int          bslevt_validate_actions(BpSecEventId eventId, uint8_t *actions);

#endif /* BPSEC_POLICY_EVENT_H_ */
