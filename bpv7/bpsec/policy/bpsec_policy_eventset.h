/*****************************************************************************
 **
 ** File Name: bpsec_policy_eventset.h
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

#ifndef BPSEC_POLICY_EVENTSET_H_
#define BPSEC_POLICY_EVENTSET_H_

/*****************************************************************************
 *                              FILE INCLUSIONS                              *
 *****************************************************************************/

#include "bpsec_asb.h"
#include "bpsec_util.h"

#include "bpsec_policy.h"
#include "bpsec_policy_event.h"

/*
 * +--------------------------------------------------------------------------+
 * |							  CONSTANTS  								  +
 * +--------------------------------------------------------------------------+
 */

// MUST stay less than 1 byte or else serialization rules need to change.
#define MAX_EVENT_SET_NAME_LEN			(12)
#define MAX_EVENT_SET_DESC_LEN			(32)

/*****************************************************************************
 *                           Eventset Structures                             *
 *****************************************************************************/

typedef struct BpSecEventSet
{
	//uint8_t 	id;							  /**< Event set ID			    */
	char 		name[MAX_EVENT_SET_NAME_LEN]; /**< Unique event set name    */
	char		desc[MAX_EVENT_SET_DESC_LEN]; /**< Event set description	*/
	uint16_t 	mask;						  /**< Configured events 	    */
	PsmAddress 	events; 					  /**< sm_list of BPsecPolEvent	*/
	uint8_t		ruleCount;			          /**< # rules using the set.   */
} BpSecEventSet;

/*****************************************************************************
 *                        Eventset Function Prototypes                       *
 *****************************************************************************/

int            bsles_add(PsmPartition wm, char *name, char *desc);
int            bsles_add_event(PsmPartition wm, BpSecEventSet *esPtr, PsmAddress eventAddr, BpSecEventId eventId);
BpSecEventSet* bsles_create(PsmPartition wm, char *name, char *desc, uint8_t ruleCnt, PsmAddress *addr);
int            bsles_clear_event(PsmPartition wm, BpSecEventSet *esPtr, BpSecEventId event);
int            bsles_delete(PsmPartition wm, char *name);
int            bsles_destroy(PsmPartition wm, PsmAddress esAddr, BpSecEventSet *esPtr);
PsmAddress     bsles_get_addr(PsmPartition wm, char *name);
Lyst           bsles_get_all(PsmPartition wm);
BpSecEvent*    bsles_get_event(PsmPartition wm, BpSecEventSet *esPtr, BpSecEventId eventId);
BpSecEventSet* bsles_get_ptr(PsmPartition wm, char *name);
int            bsles_match(BpSecEventSet *es1, char *name);
int            bsles_set_event(BpSecEventSet *esPtr, BpSecEventId event);

/* SDR-related Functions */
int bsles_sdr_deserialize(PsmPartition wm, PsmAddress *eventSetAddr, char *buffer, int *bytes_left);
int bsles_sdr_forget(PsmPartition wm, char *name);
int bsles_sdr_persist(PsmPartition wm, PsmAddress eventSetAddr);
int bsles_sdr_restore(PsmPartition vm, BpSecPolicyDbEntry entry);
int bsles_sdr_serialize_buffer(PsmPartition wm, BpSecEventSet *eventSetPtr, char *buffer, int *bytes_left);
int bsles_sdr_size(PsmPartition wm, PsmAddress eventSetAddr);

/* Eventset Red-Black Tree Functions */
int  bsles_cb_rbt_key_comp(PsmPartition wm, PsmAddress refData, void *dataBuffer);
void bsles_cb_rbt_key_del(PsmPartition wm, PsmAddress refData, void *arg);
void bsles_cb_smlist_del(PsmPartition wm, PsmAddress elt, void *arg);


#endif /* BPSEC_POLICY_EVENTSET_H_ */
